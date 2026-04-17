#!/bin/bash
set -euo pipefail

# ── T5ynth macOS .pkg Installer Builder ──────────────────────────────
# Usage: build_pkg.sh --app <path> --presets <dir>
#                     --version <ver> --output <pkg>
#                     [--sign-app-identity <identity>]
#                     [--sign-pkg-identity <identity>]
#                     [--notary-keychain-profile <profile>]
#                     [--notary-apple-id <apple-id> --notary-password <password> --notary-team-id <team-id>]
#                     [--notary-api-key-path <p8> --notary-api-key-id <id> --notary-api-issuer <issuer>]

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# ── Parse arguments ──────────────────────────────────────────────────
APP="" PRESETS="" VERSION="0.3.0" OUTPUT="T5ynth-macOS-Installer.pkg"
APP_SIGN_IDENTITY="${MACOS_APP_SIGN_IDENTITY:-}"
PKG_SIGN_IDENTITY="${MACOS_PKG_SIGN_IDENTITY:-${MACOS_INSTALLER_SIGN_IDENTITY:-}}"
NOTARY_KEYCHAIN_PROFILE="${MACOS_NOTARY_KEYCHAIN_PROFILE:-}"
NOTARY_APPLE_ID="${MACOS_NOTARY_APPLE_ID:-}"
NOTARY_PASSWORD="${MACOS_NOTARY_PASSWORD:-}"
NOTARY_TEAM_ID="${MACOS_NOTARY_TEAM_ID:-}"
NOTARY_API_KEY_PATH="${MACOS_NOTARY_API_KEY_PATH:-}"
NOTARY_API_KEY_ID="${MACOS_NOTARY_API_KEY_ID:-}"
NOTARY_API_ISSUER="${MACOS_NOTARY_API_ISSUER:-}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --app)     APP="$2";     shift 2 ;;
        --presets) PRESETS="$2";  shift 2 ;;
        --version) VERSION="$2"; shift 2 ;;
        --output)  OUTPUT="$2";  shift 2 ;;
        --sign-app-identity) APP_SIGN_IDENTITY="$2"; shift 2 ;;
        --sign-pkg-identity) PKG_SIGN_IDENTITY="$2"; shift 2 ;;
        --notary-keychain-profile) NOTARY_KEYCHAIN_PROFILE="$2"; shift 2 ;;
        --notary-apple-id) NOTARY_APPLE_ID="$2"; shift 2 ;;
        --notary-password) NOTARY_PASSWORD="$2"; shift 2 ;;
        --notary-team-id) NOTARY_TEAM_ID="$2"; shift 2 ;;
        --notary-api-key-path) NOTARY_API_KEY_PATH="$2"; shift 2 ;;
        --notary-api-key-id) NOTARY_API_KEY_ID="$2"; shift 2 ;;
        --notary-api-issuer) NOTARY_API_ISSUER="$2"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

die() {
    echo "Error: $*" >&2
    exit 1
}

NOTARY_ARGS=()

build_notary_args() {
    NOTARY_ARGS=()

    if [[ -n "$NOTARY_KEYCHAIN_PROFILE" ]]; then
        NOTARY_ARGS=(--keychain-profile "$NOTARY_KEYCHAIN_PROFILE")
        return 0
    fi

    if [[ -n "$NOTARY_APPLE_ID" || -n "$NOTARY_PASSWORD" || -n "$NOTARY_TEAM_ID" ]]; then
        [[ -n "$NOTARY_APPLE_ID" ]] || die "notary Apple ID auth requires --notary-apple-id"
        [[ -n "$NOTARY_PASSWORD" ]] || die "notary Apple ID auth requires --notary-password"
        [[ -n "$NOTARY_TEAM_ID" ]] || die "notary Apple ID auth requires --notary-team-id"
        NOTARY_ARGS=(--apple-id "$NOTARY_APPLE_ID" --password "$NOTARY_PASSWORD" --team-id "$NOTARY_TEAM_ID")
        return 0
    fi

    if [[ -n "$NOTARY_API_KEY_PATH" || -n "$NOTARY_API_KEY_ID" || -n "$NOTARY_API_ISSUER" ]]; then
        [[ -n "$NOTARY_API_KEY_PATH" ]] || die "notary API auth requires --notary-api-key-path"
        [[ -f "$NOTARY_API_KEY_PATH" ]] || die "notary API key file not found: $NOTARY_API_KEY_PATH"
        [[ -n "$NOTARY_API_KEY_ID" ]] || die "notary API auth requires --notary-api-key-id"
        [[ -n "$NOTARY_API_ISSUER" ]] || die "notary API auth requires --notary-api-issuer"
        NOTARY_ARGS=(--key "$NOTARY_API_KEY_PATH" --key-id "$NOTARY_API_KEY_ID" --issuer "$NOTARY_API_ISSUER")
        return 0
    fi

    return 1
}

# Strip leading 'v' from version tag (e.g. v1.1.0 -> 1.1.0)
VERSION="${VERSION#v}"

# pkgbuild/productbuild expect a dotted numeric version, and macOS receipts
# compare that version for reinstall / upgrade decisions. Keep the user-facing
# tag semantics, but translate prereleases into a monotonically increasing
# fourth numeric component:
#   0.3.0-alpha.2 -> 0.3.0.102
#   0.3.0-beta.1  -> 0.3.0.201
#   0.3.0-rc.3    -> 0.3.0.303
#   0.3.0         -> 0.3.0.400
CORE_VERSION="${VERSION%%-*}"
PACKAGE_VERSION=""

if [[ "$VERSION" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
    PACKAGE_VERSION="${BASH_REMATCH[1]}.${BASH_REMATCH[2]}.${BASH_REMATCH[3]}.400"
elif [[ "$VERSION" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)-(alpha|beta|rc)\.([0-9]+)$ ]]; then
    case "${BASH_REMATCH[4]}" in
        alpha) PACKAGE_STAGE_BASE=100 ;;
        beta)  PACKAGE_STAGE_BASE=200 ;;
        rc)    PACKAGE_STAGE_BASE=300 ;;
    esac

    PACKAGE_STAGE_NUMBER=$((PACKAGE_STAGE_BASE + BASH_REMATCH[5]))
    PACKAGE_VERSION="${BASH_REMATCH[1]}.${BASH_REMATCH[2]}.${BASH_REMATCH[3]}.${PACKAGE_STAGE_NUMBER}"
else
    echo "Error: unsupported version format '$VERSION'"
    echo "Expected X.Y.Z or X.Y.Z-alpha.N / -beta.N / -rc.N"
    exit 1
fi

for var in APP PRESETS; do
    if [[ -z "${!var}" ]]; then
        echo "Error: --$(echo $var | tr '[:upper:]' '[:lower:]') is required"
        exit 1
    fi
done

[[ -d "$APP" ]] || die "app bundle not found: $APP"
[[ -d "$PRESETS" ]] || die "presets directory not found: $PRESETS"

# ── Temp workspace ───────────────────────────────────────────────────
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

echo "==> Building T5ynth installer ${VERSION} (pkg version ${PACKAGE_VERSION})"

# ── Stage: Standalone ────────────────────────────────────────────────
echo "  Staging Standalone..."
STAGE_APP="$WORK/stage-standalone"
mkdir -p "$STAGE_APP"
cp -R "$APP" "$STAGE_APP/"
STAGED_APP="$STAGE_APP/$(basename "$APP")"

if [[ -n "$APP_SIGN_IDENTITY" ]]; then
    echo "  Signing app bundle with Developer ID..."
    codesign \
        --force \
        --deep \
        --timestamp \
        --options runtime \
        --sign "$APP_SIGN_IDENTITY" \
        "$STAGED_APP"
    codesign --verify --deep --strict "$STAGED_APP"
fi

# Prevent Installer from "following" an existing T5ynth.app with the same
# bundle identifier into a dev/build path. We always want the packaged app to
# land at /Applications on the selected volume.
COMPONENT_PLIST="$WORK/standalone-components.plist"
pkgbuild --analyze --root "$STAGE_APP" "$COMPONENT_PLIST" >/dev/null
/usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$COMPONENT_PLIST"
/usr/libexec/PlistBuddy -c "Set :0:BundleIsVersionChecked false" "$COMPONENT_PLIST"

pkgbuild \
    --root "$STAGE_APP" \
    --component-plist "$COMPONENT_PLIST" \
    --identifier org.ai4artsed.t5ynth.standalone \
    --version "$PACKAGE_VERSION" \
    --install-location /Applications \
    --scripts "$SCRIPT_DIR/scripts-standalone" \
    "$WORK/standalone.pkg"

# ── Stage: Support data (factory presets + empty models dir) ─────────
echo "  Staging support data..."
STAGE_SUPPORT="$WORK/stage-support"
mkdir -p "$STAGE_SUPPORT/presets"
mkdir -p "$STAGE_SUPPORT/models"

# Copy factory presets
if [[ -d "$PRESETS" ]]; then
    cp "$PRESETS"/*.t5p "$STAGE_SUPPORT/presets/" 2>/dev/null || true
fi

# Copy license
if [[ -f "$SCRIPT_DIR/../../LICENSE.txt" ]]; then
    cp "$SCRIPT_DIR/../../LICENSE.txt" "$STAGE_SUPPORT/"
fi

pkgbuild \
    --root "$STAGE_SUPPORT" \
    --identifier org.ai4artsed.t5ynth.support-data \
    --version "$PACKAGE_VERSION" \
    --install-location "/Library/Application Support/T5ynth" \
    --scripts "$SCRIPT_DIR/scripts" \
    "$WORK/support-data.pkg"

# ── Copy resources for the product installer ─────────────────────────
RESOURCES="$WORK/resources"
mkdir -p "$RESOURCES"
cp "$SCRIPT_DIR/resources/welcome.html" "$RESOURCES/"
if [[ -f "$SCRIPT_DIR/../../LICENSE.txt" ]]; then
    cp "$SCRIPT_DIR/../../LICENSE.txt" "$RESOURCES/"
fi

# ── Build product archive ────────────────────────────────────────────
echo "  Building product installer..."

# Inject version into distribution.xml pkg-ref elements
sed "s/version=\"0\"/version=\"${PACKAGE_VERSION}\"/g" \
    "$SCRIPT_DIR/distribution.xml" > "$WORK/distribution.xml"

mkdir -p "$(dirname "$OUTPUT")"
UNSIGNED_PRODUCT="$WORK/T5ynth-macOS-Installer-unsigned.pkg"
productbuild \
    --distribution "$WORK/distribution.xml" \
    --package-path "$WORK" \
    --resources "$RESOURCES" \
    "$UNSIGNED_PRODUCT"

FINAL_PRODUCT="$UNSIGNED_PRODUCT"

if [[ -n "$PKG_SIGN_IDENTITY" ]]; then
    echo "  Signing product installer with Developer ID Installer..."
    SIGNED_PRODUCT="$WORK/T5ynth-macOS-Installer-signed.pkg"
    productsign \
        --sign "$PKG_SIGN_IDENTITY" \
        "$UNSIGNED_PRODUCT" \
        "$SIGNED_PRODUCT"
    pkgutil --check-signature "$SIGNED_PRODUCT" >/dev/null
    FINAL_PRODUCT="$SIGNED_PRODUCT"
fi

if build_notary_args; then
    [[ -n "$PKG_SIGN_IDENTITY" ]] || die "notarization requires a signed installer (--sign-pkg-identity)"
    echo "  Submitting installer to Apple notary service..."
    xcrun notarytool submit "$FINAL_PRODUCT" "${NOTARY_ARGS[@]}" --wait
    echo "  Stapling notarization ticket..."
    xcrun stapler staple "$FINAL_PRODUCT"
    xcrun stapler validate "$FINAL_PRODUCT"
fi

rm -f "$OUTPUT"
cp "$FINAL_PRODUCT" "$OUTPUT"

echo "==> Done: $OUTPUT ($(du -h "$OUTPUT" | cut -f1))"
