param(
    [switch]$ConfigureOnly
)

$ErrorActionPreference = "Stop"
$cmake = "C:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$build = Join-Path $root "out\build\electron"
$cache = Join-Path $build "CMakeCache.txt"

# g3log invokes the Windows find.exe while extracting its Git version. Put
# System32 before Git's Unix tools so `find` cannot accidentally scan C:\.
$env:PATH = "$env:SystemRoot\System32;C:\Program Files\Git\cmd;$env:PATH"

if (-not (Test-Path -LiteralPath $cmake)) {
    throw "CMake was not found at $cmake"
}

if ($ConfigureOnly -or -not (Test-Path -LiteralPath $cache)) {
    & $cmake -S $root -B $build -G "Visual Studio 18 2026" -A x64 -DBUILD_IMGUI_FRONTEND=OFF
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if (-not $ConfigureOnly) {
    & $cmake --build $build --config Release --target GottvergessenNative
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
