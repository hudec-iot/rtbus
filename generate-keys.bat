@echo off
setlocal
if not "%~2"=="" (
    echo Usage: %~nx0 [output-directory]
    exit /b 2
)
set "RTBUS_KEYGEN_SCRIPT=%~f0"
set "RTBUS_KEYGEN_ROOT=%~dp0"
set "RTBUS_KEYGEN_OUTPUT=%~1"
powershell.exe -NoProfile -NonInteractive -Command "$source = [IO.File]::ReadAllText($env:RTBUS_KEYGEN_SCRIPT); $marker = '# POWERSHELL-BEGIN'; & ([scriptblock]::Create($source.Substring($source.LastIndexOf($marker) + $marker.Length)))"
exit /b %errorlevel%

# POWERSHELL-BEGIN
# SPDX-License-Identifier: MPL-2.0
$ErrorActionPreference = 'Stop'
$workDir = $null
try {
    $sdkRoot = $env:RTBUS_KEYGEN_ROOT
    $outputDir = $env:RTBUS_KEYGEN_OUTPUT
    if ([string]::IsNullOrWhiteSpace($outputDir)) { $outputDir = $sdkRoot }
    $outputDir = [IO.Path]::GetFullPath($outputDir)

    $compiler = $null
    if ($env:TCC) {
        $compiler = (Get-Command $env:TCC -ErrorAction Stop).Source
    } else {
        $command = Get-Command tcc.exe -ErrorAction SilentlyContinue
        if ($command) { $compiler = $command.Source }
        if (-not $compiler) {
            $locations = @((Join-Path $sdkRoot '../../../tools/tcc/*/tcc.exe'))
            if ($env:LOCALAPPDATA) {
                $locations += Join-Path $env:LOCALAPPDATA 'Arduino15/packages/rtbus/tools/tcc/*/tcc.exe'
            }
            foreach ($location in $locations) {
                $found = Get-ChildItem -Path $location -File -ErrorAction SilentlyContinue |
                    Sort-Object FullName -Descending | Select-Object -First 1
                if ($found) { $compiler = $found.FullName; break }
            }
        }
    }
    if (-not $compiler) { throw 'Set TCC to the full path of the SDK tcc.exe.' }

    $workDir = Join-Path ([IO.Path]::GetTempPath()) ('rtbus-keygen-' + [guid]::NewGuid().ToString('N'))
    [void][IO.Directory]::CreateDirectory($workDir)
    $packer = Join-Path $workDir 'application_pack.exe'
    & $compiler -x c -std=c99 -Wall -Wextra -Werror -O2 -o $packer (Join-Path $sdkRoot 'system/header.c')
    if ($LASTEXITCODE -ne 0) { throw 'TCC failed to compile the key tool.' }

    $privateTemp = Join-Path $workDir 'private.key'
    $publicTemp = Join-Path $workDir 'public.pub'
    $random = New-Object byte[] 32
    $rng = [Security.Cryptography.RandomNumberGenerator]::Create()
    try { $rng.GetBytes($random) } finally { $rng.Dispose() }
    $scalar = [BitConverter]::ToString($random).Replace('-', '').ToLowerInvariant()
    [IO.File]::WriteAllText($privateTemp, $scalar + [Environment]::NewLine, [Text.Encoding]::ASCII)
    & $packer --public-key $privateTemp $publicTemp
    if ($LASTEXITCODE -ne 0) { throw 'Private scalar validation or public-key derivation failed; run again.' }

    [void][IO.Directory]::CreateDirectory($outputDir)
    $privateKey = Join-Path $outputDir 'application-p256-private.key'
    $publicKey = Join-Path $outputDir 'application-p256-public.pub'
    if ((Test-Path -LiteralPath $privateKey) -or (Test-Path -LiteralPath $publicKey)) {
        $backupDir = Join-Path $outputDir ('.key-backup-' + [guid]::NewGuid().ToString('N'))
        [void][IO.Directory]::CreateDirectory($backupDir)
        foreach ($keyFile in @($privateKey, $publicKey)) {
            if (Test-Path -LiteralPath $keyFile) { Copy-Item -LiteralPath $keyFile -Destination $backupDir }
        }
        Write-Host ('Previous keys saved in: ' + $backupDir)
    }
    Move-Item -LiteralPath $privateTemp -Destination $privateKey -Force
    Move-Item -LiteralPath $publicTemp -Destination $publicKey -Force
    Write-Host ('Generated private key: ' + $privateKey)
    Write-Host ('Generated public key: ' + $publicKey)
    Write-Host 'Rebuild the application, then use Burn Bootloader with J-Link to install runtime and the new public key.'
} catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
} finally {
    if ($workDir -and (Test-Path -LiteralPath $workDir)) {
        Remove-Item -LiteralPath $workDir -Recurse -Force
    }
}
