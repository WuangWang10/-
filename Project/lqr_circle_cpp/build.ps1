param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot
$BuildDirectory = Join-Path $ProjectRoot "build"
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null

$Optimization = if ($Configuration -eq "Release") { "-O2" } else { "-O0" }
$DebugInfo = if ($Configuration -eq "Debug") { "-g" } else { "" }
$CommonFlags = @(
    "-std=c++17", $Optimization, "-Wall", "-Wextra", "-Wpedantic",
    "-I$ProjectRoot/include"
)
if ($DebugInfo) { $CommonFlags += $DebugInfo }

$LibrarySources = @(
    "$ProjectRoot/src/circle_trajectory.cpp",
    "$ProjectRoot/src/lqr_controller.cpp",
    "$ProjectRoot/src/point_mass.cpp",
    "$ProjectRoot/src/simulation.cpp"
)

Write-Host "Building simulator ($Configuration)..."
& g++ @CommonFlags @LibrarySources "$ProjectRoot/src/main.cpp" -o "$BuildDirectory/lqr_circle.exe"
if ($LASTEXITCODE -ne 0) { throw "Simulator compilation failed" }

Write-Host "Building tests..."
& g++ @CommonFlags @LibrarySources "$ProjectRoot/tests/test_main.cpp" -o "$BuildDirectory/lqr_tests.exe"
if ($LASTEXITCODE -ne 0) { throw "Test compilation failed" }

Write-Host "Running tests..."
& "$BuildDirectory/lqr_tests.exe"
if ($LASTEXITCODE -ne 0) { throw "Tests failed" }

Write-Host "Build completed: $BuildDirectory/lqr_circle.exe"
