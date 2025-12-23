$workspace   = "c:\Users\niazye\Documents\inECNU\compiler-in-ECNU\lab1"
$testDir     = Join-Path $workspace "test"
$sources     = @("C_LexAnalysis_mainProcess.cpp")
$compiler    = "C:\Program Files\mingw64\bin\g++.exe"
$outputExe   = "lex_analysis.exe"
$exePath     = Join-Path $workspace $outputExe

Set-Location $workspace

if (-not (Test-Path $compiler)) {
    Write-Error "Compiler not found at $compiler"
    exit 1
}

$buildTime = Measure-Command {
    & $compiler -g $sources -o $outputExe
    if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }
}

Write-Host ("Build time : {0:N3} ms" -f $buildTime.TotalMilliseconds)

$inputs = Get-ChildItem -Path $testDir -Filter "input*.txt" | Sort-Object Name
if (-not $inputs) {
    Write-Warning "No input*.txt found in $testDir"
    exit 0
}

$passCount = 0
$failCount = 0

foreach ($caseFile in $inputs) {
    $idx        = ($caseFile.BaseName -replace '\D+', '')
    $outPath    = Join-Path $testDir ("out{0}.txt" -f $idx)
    $answerPath = Join-Path $testDir ("ans{0}.txt" -f $idx)

    $runTime = Measure-Command {
        $cmd = '"{0}" < "{1}" > "{2}"' -f $exePath, $caseFile.FullName, $outPath
        cmd /c $cmd
        if ($LASTEXITCODE -ne 0) { throw "Execution failed with exit code $LASTEXITCODE on input $($caseFile.Name)" }
    }

    $status = "NO_ANS"
    $diff   = $null
    if ((Test-Path $answerPath) -and (Test-Path $outPath)) {
        $ansLines  = Get-Content $answerPath
        $outLines  = Get-Content $outPath
        if ($ansLines -eq $null) { $ansLines = @() }
        if ($outLines -eq $null) { $outLines = @() }
        $diff = Compare-Object $ansLines $outLines -SyncWindow 0
        $status = if ($diff) { "FAIL" } else { "PASS" }
    }

    if ($status -eq "PASS") { $passCount++ } elseif ($status -eq "FAIL") { $failCount++ }

    Write-Host ("Test {0}: {1,-4} Run time: {2:N3} ms" -f $idx, $status, $runTime.TotalMilliseconds)
}

Write-Host ("Summary -> PASS: {0}, FAIL: {1}, NO_ANS: {2}" -f $passCount, $failCount, ($inputs.Count - $passCount - $failCount))