$ErrorActionPreference = "Stop"
$ProjectRoot = $PSScriptRoot
$Executable = "build/lqr_circle.exe"
$CsvOutput = "results/trajectory.csv"
$SvgOutput = "results/trajectory.svg"

Push-Location $ProjectRoot
try {
    New-Item -ItemType Directory -Force -Path "results" | Out-Null

    if (-not (Test-Path $Executable)) {
        & (Join-Path $ProjectRoot "build.ps1")
    }

    & ".\$Executable" --output $CsvOutput @args
    if ($LASTEXITCODE -ne 0) { throw "Simulation failed" }

    $PythonCommand = Get-Command python -ErrorAction SilentlyContinue
    if ($PythonCommand) {
        & python (Join-Path $ProjectRoot "tools/plot_results.py") $CsvOutput $SvgOutput
        if ($LASTEXITCODE -ne 0) { throw "Plot generation failed" }
        Write-Host "Plot written to: $(Join-Path $ProjectRoot $SvgOutput)"
    } else {
        Write-Warning "Python not found; skipped SVG plot generation."
    }
} finally {
    Pop-Location
}
