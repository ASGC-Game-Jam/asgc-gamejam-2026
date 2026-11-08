# macOS Development Setup

Install the Unreal Engine version required by your checkout, a full Xcode
installation, and Rider if you plan to use it. The following configuration successfully built a checkout
associated with Unreal Engine 5.7:

- macOS 26.7 on Apple silicon
- Xcode 26.6 (build 17F113), including macOS SDK 26.5
- Rider

This is a known working configuration for that 5.7 checkout, not a guarantee
for every engine version or branch. For the current 5.8 checkout, the
[release guide](RELEASES.md#build-for-macos) also records a successful packaged
build with Unreal Engine 5.8.4 and Xcode 27.2.

Download Xcode 26.6 from [Apple Developer Downloads](https://developer.apple.com/download/all/)
by searching for its version. The Mac App Store may offer a newer version
instead. Sign in with an Apple Account and, if prompted, register for free and
accept the Apple Developer Agreement; paid Apple Developer Program membership
is not required for the download. Extract the downloaded archive, move Xcode
to `/Applications`, and open it once to complete any requested setup. Keep
enough free disk space for Xcode, Unreal, build files, and shaders.

Select the full Xcode installation as the active developer directory. Adjust the
path if the app has a different name:

```bash
sudo xcode-select --switch /Applications/Xcode.app/Contents/Developer

xcode-select -p
xcodebuild -version
xcrun --sdk macosx --show-sdk-version
clang --version
```

For the configuration above, these commands should show the Xcode developer
directory, Xcode 26.6, and SDK 26.5. If `xcode-select -p` instead shows
`/Library/Developer/CommandLineTools`, select the full Xcode app as above;
the standalone Command Line Tools do not include `xcodebuild`. Selecting Xcode
with `sudo xcode-select` changes the system-wide default developer directory.

Open `ProjectAtlantis.uproject` in Rider, make sure it uses the engine version
required by the checkout, and build **ProjectAtlantisEditor / Development /
Mac**. If you change Xcode or engine versions, restart Rider and regenerate
project files if needed. An UnrealBuildTool message that Mac was skipped due
to an invalid SDK means the selected SDK is outside that engine's accepted
range; check the reported range and the selected Xcode version. For example,
the observed 5.7 installation accepted SDKs through 26.9 but rejected SDK
27.2. See [Build for macOS](RELEASES.md#build-for-macos) for packaging.

## Running from Xcode

Generate Xcode files from `ProjectAtlantis.uproject` and open the generated Mac
workspace. In Xcode's scheme selector, choose **ProjectAtlantisEditor**, select
**My Mac**, and run the **Development** configuration. This opens the project
in Unreal Editor, where you can use Play in Editor.

If the debugger stops with `SIGHUP` during startup, configure the editor's
scheme to use the project-specific LLDB settings: choose **Product > Scheme >
Edit Scheme**, select **Run > Info**, and set **LLDB Init File** to the absolute
path of [`Scripts/xcode-editor.lldbinit`](../Scripts/xcode-editor.lldbinit).
This prevents LLDB from stopping on or passing SIGHUP to the editor during
that scheme's debug runs; other signals are unchanged. Unreal may regenerate
the scheme, so check this setting again after regenerating Xcode files.

The **ProjectAtlantis** scheme is a standalone game target. Building it does
not cook and stage the game's content. Running it directly without the required
staged data can stop in Xcode with `EXC_BREAKPOINT`, including an ICU data
assertion or a failed project descriptor message. Use the editor scheme for
normal development, or cook and package the standalone game as described in
the [release guide](RELEASES.md#build-for-macos).

