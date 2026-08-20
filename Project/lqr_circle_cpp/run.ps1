$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot
$Executable = Join-Path $ProjectRoot "build/lqr_circle.exe"
$CsvOutput = Join-Path $ProjectRoot "results/trajectory.csv"
$SvgOutput = Join-Path $ProjectRoot "results/trajectory.svg"

if (-not (Test-Path $Executable)) {
    & (Join-Path $ProjectRoot "build.ps1")
}

& $Executable --output $CsvOutput @args
if ($LASTEXITCODE -ne 0) { throw "Simulation failed" }

$PythonCommand = Get-Command python -ErrorAction SilentlyContinue
if ($PythonCommand) {
    & python (Join-Path $ProjectRoot "tools/plot_results.py") $CsvOutput $SvgOutput
    if ($LASTEXITCODE -ne 0) { throw "Plot generation failed" }
    Write-Host "Plot written to: $SvgOutput"
} else {
    Write-Warning "Python not found; skipped SVG plot generation."
}
