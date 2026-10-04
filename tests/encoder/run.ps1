param([string]$Compiler = 'gcc')

$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$testExe = Join-Path ([System.IO.Path]::GetTempPath()) ('encoder-test-' + [guid]::NewGuid().ToString('N') + '.exe')

try {
    & $Compiler -std=c11 -Wall -Wextra -Werror -O2 `
        ('-I' + $PSScriptRoot) `
        ('-I' + (Join-Path $projectRoot 'lib/Sensors/Encorder')) `
        ('-I' + (Join-Path $projectRoot 'Config')) `
        (Join-Path $projectRoot 'lib/Sensors/Encorder/encoder_mt6826.c') `
        (Join-Path $PSScriptRoot 'test_encoder.c') -lm -o $testExe
    if ($LASTEXITCODE -ne 0) { throw 'Encoder host-test compilation failed.' }
    & $testExe
    if ($LASTEXITCODE -ne 0) { throw 'Encoder host tests failed.' }
}
finally {
    if (Test-Path -LiteralPath $testExe) { Remove-Item -LiteralPath $testExe }
}
