param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('ENROLL_MCU_F103', 'ENROLL_MCU_F407', 'ENROLL_MCU_G3507')]
    [string]$Target
)

$ErrorActionPreference = 'Stop'

$file = Join-Path $PSScriptRoot '..\Enroll\Enroll.h'
$file = [System.IO.Path]::GetFullPath($file)

if (-not (Test-Path $file)) {
    throw "Enroll.h not found: $file"
}

$text = Get-Content -Raw $file
$pattern = '(?m)^(#define\s+ENROLL_MCU_TARGET\s+).*$'

if ($text -notmatch $pattern) {
    throw 'ENROLL_MCU_TARGET define not found in Enroll.h'
}

$newText = [regex]::Replace($text, $pattern, ('$1' + $Target))
Set-Content -Path $file -Value $newText -Encoding utf8

Write-Host ("ENROLL_MCU_TARGET -> " + $Target)
