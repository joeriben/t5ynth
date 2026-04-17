## Installation

### macOS
1. Download **`T5ynth-macOS-Installer.pkg`**
2. Double-click the `.pkg` -- macOS will block it because T5ynth is not signed with an Apple Developer certificate.
3. Open **System Settings > Privacy & Security**, scroll down, and click **"Open Anyway"** next to the T5ynth message.
4. Enter your admin password when prompted.
5. The installer runs and places **T5ynth.app** in `/Applications/` and creates the preset/model folders under `/Library/Application Support/T5ynth/`.
6. Launch T5ynth from `/Applications/`

> **Note:** On older macOS versions (before Ventura), you can also right-click the `.pkg` and choose "Open" instead of steps 3-4.

Current macOS releases ship only `T5ynth.app`, delivered through the installer.

### Windows
1. Download **`T5ynth-Windows-Setup.exe`**
2. Run the installer (may require "Run as administrator")
3. Launch T5ynth from the Start Menu

The installer places the T5ynth app, VST3 plugin, and factory presets in their standard locations.

<details>
<summary>Manual install (without Setup.exe)</summary>

1. Extract `T5ynth-Windows-Standalone.tar.xz` to a folder of your choice
2. For VST3: extract `T5ynth-Windows-VST3.tar.xz` to `C:\Program Files\Common Files\VST3\`
3. Restart your DAW.

</details>

### Linux
Download **`T5ynth-Linux-x86_64-Standalone.tar.xz`** for the app. These Linux builds are currently **x86_64 only**. You may need to `chmod +x T5ynth` and `chmod +x backend/pipe_inference` if your archive tool does not preserve permissions.

---
