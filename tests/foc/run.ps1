param([string]$Compiler = 'gcc')
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$testExe = Join-Path ([IO.Path]::GetTempPath()) ('foc-test-' + [guid]::NewGuid().ToString('N') + '.exe')
try {
    $includes = @('lib/FOC','lib/Controller','lib/Sensors/Encorder','Config','Platform/PWM') |
        ForEach-Object { '-I' + (Join-Path $projectRoot $_) }
    & $Compiler -std=c11 -Wall -Wextra -Werror -O2 ('-I' + $PSScriptRoot) @includes `
        (Join-Path $projectRoot 'lib/FOC/foc.c') `
        (Join-Path $projectRoot 'lib/Controller/pid.c') `
        (Join-Path $PSScriptRoot 'test_foc.c') -lm -o $testExe
    if ($LASTEXITCODE -ne 0) { throw 'FOC test compilation failed.' }
    & $testExe
    if ($LASTEXITCODE -ne 0) { throw 'FOC tests failed.' }
}
finally {
    if (Test-Path -LiteralPath $testExe) { Remove-Item -LiteralPath $testExe }
}
