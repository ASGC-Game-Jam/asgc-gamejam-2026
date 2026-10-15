# GameAnalytics Mac library

`libGameAnalytics.a` contains GameAnalytics C++ SDK 5.1.0, rebuilt for macOS
14.0 with both `arm64` and `x86_64` architectures. The previously bundled
library targeted macOS 15.0 and produced linker warnings with Unreal 5.7's
standard macOS 14.0 target. No Unreal installation changes are required.

Source: https://github.com/GameAnalytics/gameanalytics-sdk-cpp/tree/v5.1.0

Source commit: `18dcf7b961400875e1d48e554a0ddeb4f0bb49b9`.

## Rebuilding

Requires Git, Python 3, CMake, and Xcode. Build in a temporary directory:

```sh
git clone --depth 1 --branch v5.1.0 https://github.com/GameAnalytics/gameanalytics-sdk-cpp.git ga-sdk
```

The release's CMake configuration unconditionally adds unit tests and requires
the Google Test submodule, even when only building the SDK. Remove that test
section from the temporary checkout; SDK sources stay unchanged:

```sh
python3 - <<'PY'
from pathlib import Path
path = Path('ga-sdk/CMakeLists.txt')
text = path.read_text()
marker = '# --------------------------- Google Test Setup'
assert marker in text
path.write_text(text.split(marker, 1)[0])
PY
cmake -S ga-sdk -B ga-build \
  -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
  -DPLATFORM=osx \
  -DGA_BUILD_SAMPLE=OFF \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build ga-build --target GameAnalytics --parallel 10
```

The result is `ga-build/Release/libGameAnalytics.a`. This is a static archive;
the Unreal plugin continues to supply its existing OpenSSL, libcurl, and
framework dependencies when linking. Follow repository binary-lock guidelines
before replacing the committed archive. SDK compilation can emit warnings in
upstream source; those sources are not compiled during ordinary game builds.
