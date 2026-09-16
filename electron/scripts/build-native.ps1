param(
    [switch]$ConfigureOnly
)

$ErrorActionPreference = "Stop"
$cmake = $null

$cmakeCmd = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCmd) {
    $cmake = $cmakeCmd.Source
}

if (-not $cmake) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vswhere) {
        $vsPath = & $vswhere -latest -products * -property installationPath
        if ($vsPath) {
            $candidate = Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
            if (Test-Path -LiteralPath $candidate) {
                $cmake = $candidate
            }
        }
    }
}

if (-not $cmake) {
    $candidates = @(
        "D:\Tools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\CMake\bin\cmake.exe"
    )
    foreach ($cand in $candidates) {
        if (Test-Path -LiteralPath $cand) {
            $cmake = $cand
            break
        }
    }
}

$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$build = Join-Path $root "out\build\electron"
$buildX86 = Join-Path $root "out\build\electron-x86"
$cache = Join-Path $build "CMakeCache.txt"
$project = Join-Path $build "GottvergessenNative.vcxproj"
$cacheX86 = Join-Path $buildX86 "CMakeCache.txt"
$projectX86 = Join-Path $buildX86 "GottvergessenNative.vcxproj"

# g3log invokes the Windows find.exe while extracting its Git version. Put
# System32 before Git's Unix tools so `find` cannot accidentally scan C:\.
$env:PATH = "$env:SystemRoot\System32;C:\Program Files\Git\cmd;$env:PATH"

if (-not $cmake -or -not (Test-Path -LiteralPath $cmake)) {
    throw "CMake was not found. Please ensure CMake or Visual Studio with C++ CMake tools is installed."
}

if ($ConfigureOnly -or -not (Test-Path -LiteralPath $cache) -or -not (Test-Path -LiteralPath $project)) {
    & $cmake -S $root -B $build -G "Visual Studio 18 2026" -A x64 -DBUILD_IMGUI_FRONTEND=OFF
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if ($ConfigureOnly -or -not (Test-Path -LiteralPath $cacheX86) -or -not (Test-Path -LiteralPath $projectX86)) {
    $deps = Join-Path $build "_deps"
    $cmakeArgs = @(
        "-S", $root,
        "-B", $buildX86,
        "-G", "Visual Studio 18 2026",
        "-A", "Win32",
        "-DBUILD_IMGUI_FRONTEND=OFF",
        "-DFETCHCONTENT_SOURCE_DIR_JSON=$(Join-Path $deps 'json-src')",
        "-DFETCHCONTENT_SOURCE_DIR_G3LOG=$(Join-Path $deps 'g3log-src')",
        "-DFETCHCONTENT_SOURCE_DIR_CPR=$(Join-Path $deps 'cpr-src')",
        "-DFETCHCONTENT_SOURCE_DIR_CURL=$(Join-Path $deps 'curl-src')",
        "-DFETCHCONTENT_SOURCE_DIR_ZLIB=$(Join-Path $deps 'zlib-src')"
    )
    & $cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if (-not $ConfigureOnly) {
    & $cmake --build $build --config Release --target GottvergessenNative GottvergessenUpdater
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    & $cmake --build $buildX86 --config Release --target GottvergessenNative
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    $x86Dll = Join-Path $buildX86 "bin\Release\native-core-x86.dll"
    $outputDir = Join-Path $build "bin\Release"
    if (-not (Test-Path -LiteralPath $x86Dll)) {
        throw "The x86 native DLL was not produced: $x86Dll"
    }
    Copy-Item -LiteralPath $x86Dll -Destination (Join-Path $outputDir "native-core-x86.dll") -Force
}
