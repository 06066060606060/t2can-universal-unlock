[CmdletBinding()]
param(
    [string]$ProjectPath = '',
    [string]$ArduinoCli = 'arduino-cli',
    [string]$Python = 'python',
    [string]$OutputRoot = '',
    [ValidateSet('t2can', 'waveshare-s3-can-2ch')]
    [string]$Board = 't2can',
    [switch]$DryRun,
    [switch]$PrepareOnly
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($ProjectPath)) {
    if (@(Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.ino' -File).Count -eq 1) {
        $ProjectPath = $PSScriptRoot
    } else {
        $candidates = @(Get-ChildItem -LiteralPath $PSScriptRoot -Directory |
            Where-Object { $_.Name -like 'T2CAN*' -and
                @(Get-ChildItem -LiteralPath $_.FullName -Filter '*.ino' -File).Count -eq 1 })
        if ($candidates.Count -ne 1) {
            throw 'Specify -ProjectPath with the T2CAN project folder (one root .ino required).'
        }
        $ProjectPath = $candidates[0].FullName
    }
}
$sketch = (Resolve-Path -LiteralPath $ProjectPath).ProviderPath
$sketchFiles = @(Get-ChildItem -LiteralPath $sketch -Filter '*.ino' -File)
if ($sketchFiles.Count -ne 1) { throw 'Expected exactly one root .ino file.' }
foreach ($required in @('dashboard_source.html', 'zh-CN/dashboard_source_zh-CN.html', 'tools/build_dashboard.py')) {
    if (!(Test-Path -LiteralPath (Join-Path $sketch $required) -PathType Leaf)) {
        throw "Missing project file: $required"
    }
}
if ([string]::IsNullOrWhiteSpace($OutputRoot)) { $OutputRoot = Join-Path $sketch 'build_variants' }
$output = [System.IO.Path]::GetFullPath($OutputRoot).TrimEnd('\', '/')
$projectFull = [System.IO.Path]::GetFullPath($sketch).TrimEnd('\', '/')
$finalBuild = Join-Path $sketch 'Final-Build'
if ($projectFull -ieq $output -or $projectFull.StartsWith($output + '\', [System.StringComparison]::OrdinalIgnoreCase) -or
    $output -ieq $finalBuild -or $output.StartsWith($finalBuild + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputRoot must not be the project, an ancestor of it, or Final-Build.'
}
$pythonExe = (Get-Command $Python -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$cli = if (!$PrepareOnly) { (Get-Command $ArduinoCli -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source } else { $ArduinoCli }
$sketchName = $sketchFiles[0].Name
$firmwareName = $sketchFiles[0].BaseName

# Exact menu settings from the ESP32S3 Dev Module screenshot. Unspecified
# options (CPU, flash mode and PSRAM) retain the installed board defaults.
$options = 'FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,USBMode=hwcdc,CDCOnBoot=cdc,DFUOnBoot=default,MSCOnBoot=default,DebugLevel=none,EraseFlash=none,EventsCore=1,LoopCore=1,JTAGAdapter=default,UploadMode=default'
$extraFlags = ''
$boardSuffix = ''
if ($Board -eq 'waveshare-s3-can-2ch') {
    # Waveshare ESP32-S3-CAN-2CH: same S3 + 16 MB flash layout; CAN_1 = TWAI
    # (GPIO 15/16), CAN_2 = XL2515 (MCP2515-compatible, 16 MHz) on SPI. The
    # board has 8 MB octal PSRAM (ESP32-S3R8), so select OPI explicitly.
    $options += ',PSRAM=opi'
    $extraFlags = '-DBOARD_WAVESHARE_S3_CAN_2CH'
    $boardSuffix = '.waveshare-s3-can-2ch'
}
$fqbn = "esp32:esp32:esp32s3:$options"
$runRoot = Join-Path $output ((Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + $Board + '-' + [guid]::NewGuid().ToString('N').Substring(0,8))
$failed = @()

# Python receives paths as arguments, never as executable source substitutions.
# Both languages are staged so the original dashboard/header stay untouched.
$prepareCode = @'
from pathlib import Path
import shutil, subprocess, sys
project, destination, output = map(lambda x: Path(x).resolve(), sys.argv[1:4])
language = sys.argv[4]
sketches = list(project.glob('*.ino'))
if len(sketches) != 1:
    raise SystemExit('Expected exactly one root .ino file')
excluded = {output, (project / 'Final-Build').resolve()}
skip = {'.git', 'build', 'build_variants', 'Final-Build', '__pycache__', '.pio'}
def ignore(directory, names):
    return [n for n in names if n in skip or (Path(directory) / n).is_symlink()
            or (Path(directory) / n).resolve() in excluded]
staged = destination / sketches[0].stem
shutil.copytree(project, staged, ignore=ignore)
if language == 'zh-CN':
    shutil.copyfile(project / 'zh-CN/dashboard_source_zh-CN.html', staged / 'dashboard_source.html')
subprocess.run([sys.executable, str(staged / 'tools/build_dashboard.py')], check=True)
print(staged)
'@

Write-Host "Project: $sketch"
Write-Host "Board: $Board"
Write-Host "FQBN: $fqbn"
foreach ($language in @('en', 'zh-CN')) {
    $destination = Join-Path $runRoot $language
    $buildPath = Join-Path $destination 'build'
    $artifacts = Join-Path $destination 'firmware'
    $log = Join-Path $destination 'compile.log'
    $compileSketch = Join-Path (Join-Path $destination 'work') $firmwareName
    $suffix = if ($language -eq 'zh-CN') { '.zh-CN' } else { '' }
    $releaseName = "$firmwareName$boardSuffix$suffix"
    $cliArgs = @('compile', '--fqbn', $fqbn, '--build-path', $buildPath, '--output-dir', $artifacts)
    if ($extraFlags) {
        $cliArgs += @('--build-property', "compiler.cpp.extra_flags=$extraFlags")
    }
    $cliArgs += $compileSketch
    Write-Host "`n=== $language ==="
    Write-Host "Final OTA: $(Join-Path $finalBuild "$releaseName.OTA.bin")"
    Write-Host "Final fullflash: $(Join-Path $finalBuild "$releaseName.FULLFLASH.merged.bin")"
    if ($DryRun) {
        Write-Host 'Plan: isolated sketch copy, regenerate index_html.h for this language, then compile.'
        Write-Host ('Command: "{0}" {1}' -f $cli, (($cliArgs | ForEach-Object { '"' + $_ + '"' }) -join ' '))
        continue
    }
    try {
        New-Item -ItemType Directory -Path $destination, $buildPath, $artifacts -Force | Out-Null
        $prepared = & $pythonExe -c $prepareCode $sketch (Join-Path $destination 'work') $output $language
        if ($LASTEXITCODE -ne 0) { throw "Dashboard preparation failed (exit $LASTEXITCODE)." }
        $prepared | ForEach-Object { Write-Host $_ }
        if ($PrepareOnly) { continue }
        $previousPreference = $ErrorActionPreference
        try {
            $ErrorActionPreference = 'Continue'
            & $cli @cliArgs 2>&1 | Tee-Object -FilePath $log
            $result = $LASTEXITCODE
        } finally { $ErrorActionPreference = $previousPreference }
        if ($result -ne 0) { throw "Compiler exit $result. Log: $log" }
        $publishFiles = @(
            @{ Source = "$sketchName.bin"; Target = "$releaseName.OTA.bin" },
            @{ Source = "$sketchName.merged.bin"; Target = "$releaseName.FULLFLASH.merged.bin" }
        )
        foreach ($file in $publishFiles) {
            $original = Join-Path $artifacts $file.Source
            if (!(Test-Path -LiteralPath $original -PathType Leaf) -or (Get-Item -LiteralPath $original).Length -eq 0) {
                throw "Missing or empty compiler output: $original"
            }
        }
        New-Item -ItemType Directory -Path $finalBuild -Force | Out-Null
        foreach ($file in $publishFiles) {
            Copy-Item -LiteralPath (Join-Path $artifacts $file.Source) -Destination (Join-Path $finalBuild $file.Target) -Force
        }
        Write-Host "SUCCESS: $language. OTA and fullflash copied to $finalBuild" -ForegroundColor Green
    } catch {
        $failed += $language
        Write-Host "FAILED: $language. $_" -ForegroundColor Red
    }
}
if ($failed.Count -gt 0) { Write-Host ('Failed: ' + ($failed -join ', ')) -ForegroundColor Red; exit 1 }
if ($DryRun) { Write-Host "`nDry run complete. No files changed and no compilation started." }
elseif ($PrepareOnly) { Write-Host "`nBoth language sketches prepared without compiling: $runRoot" }
else { Write-Host "`nBoth variants compiled. Final binaries: $finalBuild. Logs and work files: $runRoot" }
exit 0
