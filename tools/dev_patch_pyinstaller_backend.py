#!/usr/bin/env python3
"""Patch a local macOS T5ynth.app backend for fast development testing.

This is not a release tool. It keeps the existing PyInstaller bootloader and
bundled runtime, but replaces the frozen entry script with a tiny loader that
runs the repo's current backend/pipe_inference.py. This avoids a full
PyInstaller rebuild while testing backend source changes locally.
"""

from __future__ import annotations

import argparse
import marshal
import os
from pathlib import Path
import shutil
import stat
import struct
import subprocess
import sys
import tempfile
import zlib

try:
    from PyInstaller.archive.readers import CArchiveReader
    from PyInstaller.archive.writers import CArchiveWriter
except ImportError as exc:  # pragma: no cover - developer environment check
    raise SystemExit("PyInstaller is required: python3 -m pip install pyinstaller") from exc


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_APP = (
    REPO_ROOT
    / "build_ci_local_app"
    / "T5ynth_artefacts"
    / "Release"
    / "Standalone"
    / "T5ynth.app"
)
DEFAULT_SOURCE = REPO_ROOT / "backend" / "pipe_inference.py"
BACKUP_SUFFIX = ".release-backup"


def _backend_executable(app: Path) -> Path:
    return app / "Contents" / "Resources" / "backend" / "pipe_inference"


def _read_cookie(reader: CArchiveReader, executable: Path):
    cookie_len = CArchiveWriter._COOKIE_LENGTH
    with executable.open("rb") as handle:
        handle.seek(reader._end_offset - cookie_len)
        cookie = handle.read(cookie_len)

    magic, archive_length, toc_offset, toc_length, pyvers, pylib_name_raw = struct.unpack(
        CArchiveWriter._COOKIE_FORMAT, cookie
    )
    if magic != CArchiveWriter._COOKIE_MAGIC_PATTERN:
        raise RuntimeError("Not a PyInstaller CArchive executable")

    pylib_name = pylib_name_raw.split(b"\0", 1)[0].decode("ascii")
    return pyvers, pylib_name


def _write_blob(handle, blob: bytes, compress: bool):
    data_offset = handle.tell()
    data_length = len(blob)
    if compress:
        blob = zlib.compress(blob, level=9)
    handle.write(blob)
    return data_offset, len(blob), data_length, int(compress)


def _bootstrap_blob(source: Path) -> bytes:
    source_literal = str(source.resolve())
    code = f"""\
import os
import runpy
import sys
source = {source_literal!r}
backend_dir = os.path.dirname(source)
if backend_dir not in sys.path:
    sys.path.insert(0, backend_dir)
sys.argv[0] = source
runpy.run_path(source, run_name="__main__")
"""
    return marshal.dumps(compile(code, "pipe_inference.py", "exec"))


def patch_backend(executable: Path, source: Path) -> None:
    executable = executable.resolve()
    source = source.resolve()
    if not executable.exists():
        raise FileNotFoundError(executable)
    if not source.exists():
        raise FileNotFoundError(source)

    backup = executable.with_name(executable.name + BACKUP_SUFFIX)
    if not backup.exists():
        shutil.copy2(executable, backup)

    template = backup
    reader = CArchiveReader(str(template))
    pyvers, pylib_name = _read_cookie(reader, template)

    with template.open("rb") as handle:
        prefix = handle.read(reader._start_offset)
        handle.seek(reader._end_offset)
        trailing_signature = handle.read()

    bootstrap = _bootstrap_blob(source)
    archive_capacity = reader._end_offset - reader._start_offset

    with tempfile.TemporaryDirectory() as tmp:
        tmp_dir = Path(tmp)
        archive_path = tmp_dir / "archive.pkg"
        output_path = tmp_dir / executable.name
        toc_entries = []

        with archive_path.open("wb") as archive:
            for option in reader.options:
                data_offset, clen, ulen, cflag = _write_blob(archive, b"", False)
                toc_entries.append((data_offset, clen, ulen, cflag, "o", option))

            for name, entry in reader.toc.items():
                _offset, _clen, _ulen, compress, typecode = entry
                if name == "pipe_inference":
                    blob = bootstrap
                    typecode = "s"
                else:
                    blob = reader.extract(name)

                data_offset, clen, ulen, cflag = _write_blob(archive, blob, bool(compress))
                toc_entries.append((data_offset, clen, ulen, cflag, typecode, name))

            toc_offset = archive.tell()
            toc_data = CArchiveWriter._serialize_toc(toc_entries)
            archive.write(toc_data)
            archive_length = toc_offset + len(toc_data) + CArchiveWriter._COOKIE_LENGTH
            archive.write(
                struct.pack(
                    CArchiveWriter._COOKIE_FORMAT,
                    CArchiveWriter._COOKIE_MAGIC_PATTERN,
                    archive_length,
                    toc_offset,
                    len(toc_data),
                    pyvers,
                    pylib_name.encode("ascii"),
                )
            )

        archive_data = archive_path.read_bytes()
        if len(archive_data) > archive_capacity:
            raise RuntimeError(
                f"Patched archive is too large ({len(archive_data)} > {archive_capacity})"
            )

        padding = b"\0" * (archive_capacity - len(archive_data))
        with output_path.open("wb") as output:
            output.write(prefix)
            output.write(archive_data)
            output.write(padding)
            output.write(trailing_signature)

        output_path.chmod(executable.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
        shutil.move(str(output_path), executable)

    if sys.platform == "darwin":
        subprocess.run(["codesign", "--force", "--sign", "-", str(executable)], check=True)


def restore_backend(executable: Path) -> None:
    backup = executable.with_name(executable.name + BACKUP_SUFFIX)
    if not backup.exists():
        raise FileNotFoundError(f"No backup found: {backup}")
    shutil.copy2(backup, executable)
    if sys.platform == "darwin":
        subprocess.run(["codesign", "--force", "--sign", "-", str(executable)], check=True)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    target = parser.add_mutually_exclusive_group()
    target.add_argument("--app", type=Path, default=DEFAULT_APP, help="Path to T5ynth.app")
    target.add_argument("--backend-exe", type=Path, help="Path to backend/pipe_inference")
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE, help="Source pipe_inference.py")
    parser.add_argument("--restore", action="store_true", help="Restore the original frozen backend")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    executable = args.backend_exe if args.backend_exe else _backend_executable(args.app)

    if args.restore:
        restore_backend(executable)
        print(f"Restored frozen backend: {executable}")
    else:
        patch_backend(executable, args.source)
        print(f"Patched backend executable: {executable}")
        print(f"Live backend source: {args.source.resolve()}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
