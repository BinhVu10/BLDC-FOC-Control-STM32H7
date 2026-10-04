param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$testExe = Join-Path ([IO.Path]::GetTempPath()) ('calib-test-' + [guid]::NewGuid().ToString('N') + '.exe')
try {
    $includes = @('tests/encoder','tests/foc','lib/Calib','lib/FOC','lib/Controller',
                  'lib/Sensors/Encorder','Config','Platform/PWM') |
        ForEach-Object { '-I' + (Join-Path $projectRoot $_) }
    & $Compiler -std=c11 -Wall -Wextra -Werror -O2 @includes `
        (Join-Path $projectRoot 'lib/Calib/Calib.c') `
        (Join-Path $projectRoot 'lib/Sensors/Encorder/encoder_mt6826.c') `
        (Join-Path $PSScriptRoot 'test_calib.c') -lm -o $testExe
    if ($LASTEXITCODE -ne 0) { throw 'Calibration test compilation failed.' }
    & $testExe
    if ($LASTEXITCODE -ne 0) { throw 'Calibration tests failed.' }
}
finally {
    if (Test-Path -LiteralPath $testExe) { Remove-Item -LiteralPath $testExe }
}
