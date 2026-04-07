## Installation

### macOS
Extract the `.tar.xz` archive, then **remove the quarantine flag** before first launch:
```bash
xattr -cr T5ynth.app
```
Alternatively: right-click the app → **Open** → confirm in the dialog.

This is required because T5ynth is not signed with an Apple Developer certificate.

### Linux
Extract and run. You may need to `chmod +x T5ynth` and `chmod +x backend/pipe_inference` if your archive tool does not preserve permissions.

---
