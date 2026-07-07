param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Debug",

    [string]$Target = "FinderX",

    [switch]$RunTests
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $repoRoot "build"

function Find-Tool {
    param(
        [string]$Name,
        [string[]]$Candidates
    )

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    foreach ($candidate in $Candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    throw "Could not find $Name. Install Visual Studio C++ tools or add $Name to PATH."
}

$vsCmakeRoot = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
$cmake = Find-Tool "cmake" @(
    (Join-Path $vsCmakeRoot "cmake.exe"),
    "C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
)
$ctest = Find-Tool "ctest" @(
    (Join-Path $vsCmakeRoot "ctest.exe"),
    "C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe"
)

Push-Location $repoRoot
try {
    $cacheFile = Join-Path $buildDir "CMakeCache.txt"
    if (Test-Path $cacheFile) {
        & $cmake -S . -B $buildDir
    } else {
        & $cmake -S . -B $buildDir -G "Visual Studio 18 2026" -A x64
    }
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    $buildTarget = $Target
    if ($RunTests) {
        $buildTarget = "ALL_BUILD"
    }

    & $cmake --build $buildDir --config $Config --target $buildTarget
    if ($LASTEXITCODE -ne 0) {
        Write-Host ""
        Write-Host "Build failed. If the linker cannot write FinderX.exe, close the running FinderX app and run this script again." -ForegroundColor Yellow
        exit $LASTEXITCODE
    }

    if ($RunTests) {
        & $ctest --test-dir $buildDir -C $Config --output-on-failure
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }
} finally {
    Pop-Location
}
