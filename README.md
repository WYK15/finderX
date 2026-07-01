# FinderX

FinderX is a Windows file manager prototype built with C++20, Win32, Direct2D, and DirectWrite.

## Requirements

- Windows 10 or later
- Visual Studio 2026 with the Desktop development with C++ workload
- CMake
- PowerShell

This repository has been built with the CMake bundled in Visual Studio:

```powershell
"D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
```

If `cmake` is already available in `PATH`, you can use `cmake` directly in the commands below.

## Configure

From the repository root:

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
```

If you are using the Visual Studio bundled CMake explicitly:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -G "Visual Studio 18 2026" -A x64
```

## Build

Build the Release executable:

```powershell
cmake --build build --config Release --target FinderX
```

The executable is generated at:

```text
build\Release\FinderX.exe
```

If the build fails with `LNK1104` because `FinderX.exe` cannot be opened, close the running FinderX app first. The linker cannot overwrite an executable that is currently running.

## Run Tests

Build everything and run the test suite:

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

With the Visual Studio bundled tools:

```powershell
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
& "D:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe" --test-dir build -C Release --output-on-failure
```

## Package a Portable ZIP

The current app can run as a portable executable. After building Release:

```powershell
New-Item -ItemType Directory -Force dist\FinderX | Out-Null
Copy-Item build\Release\FinderX.exe dist\FinderX\FinderX.exe -Force
Compress-Archive -Path dist\FinderX\* -DestinationPath dist\FinderX-portable.zip -Force
```

The ZIP is generated at:

```text
dist\FinderX-portable.zip
```

## Package an MSI Locally

MSI packaging should use CPack with the WiX generator. The recommended toolchain is:

- CPack from CMake
- WiX Toolset CLI

Install WiX:

```powershell
dotnet tool install --global wix --version 4.0.6
```

Then configure, build, test, and package:

```powershell
cmake -S . -B build -G "Visual Studio 18 2026" -A x64 "-DFINDERX_VERSION=0.1.0"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cpack --config build\CPackConfig.cmake -C Release -G WIX
```

The MSI is generated with a name based on the package metadata, for example:

```text
FinderX-0.1.0-win64.msi
```

`CPACK_WIX_UPGRADE_GUID` is fixed in `CMakeLists.txt`. Do not change it after publishing installers, otherwise future MSI builds will not upgrade older installations cleanly.

## GitHub Actions

The repository includes two workflows:

- `.github/workflows/build.yml`: runs on push and pull requests, builds Release, runs tests, and uploads `FinderX.exe` as an artifact.
- `.github/workflows/release.yml`: runs when pushing a `v*` tag or when manually triggered, builds Release, runs tests, packages an MSI and a portable ZIP, uploads both as artifacts, and attaches both to a GitHub Release.

To publish a release from your machine:

```powershell
git tag v0.1.0
git push origin v0.1.0
```

GitHub Actions will create or update the release for that tag and attach:

```text
FinderX-0.1.0-x64.msi
FinderX-0.1.0-portable.zip
```
