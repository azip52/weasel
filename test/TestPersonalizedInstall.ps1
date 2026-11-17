param([Parameter(Mandatory = $true)][string]$Installer)

$ErrorActionPreference = 'Stop'
if ($env:GITHUB_ACTIONS -ne 'true') {
  throw 'Run this installation test only on a disposable GitHub Actions runner.'
}

function Assert-Test($Condition, [string]$Message) {
  if (-not $Condition) { throw $Message }
}
function Invoke-Setup([string]$Path, [string]$Arguments) {
  $process = [Diagnostics.Process]::Start($Path, $Arguments)
  if (-not $process.WaitForExit(180000)) {
    $process.Kill()
    throw 'Setup timed out.'
  }
  Assert-Test ($process.ExitCode -eq 0) ('Setup exit code: ' + $process.ExitCode)
}

$languages = Get-WinUserLanguageList
$display = (Get-WinUILanguageOverride).Name
$inputTip = (Get-WinDefaultInputMethodOverride).InputMethodTip
$userKey = 'HKCU:\Software\Rime\Weasel'
$userDirectory = Join-Path $env:RUNNER_TEMP ('personalized-rime-' + [guid]::NewGuid())
$setup = $null
try {
  # The runner is disposable and the previous upgrade suite has uninstalled.
  if (Test-Path -LiteralPath $userKey) { Remove-Item -LiteralPath $userKey -Recurse -Force }
  New-Item -Path $userKey -Force | Out-Null
  New-ItemProperty -LiteralPath $userKey -Name RimeUserDir -Value $userDirectory -PropertyType String | Out-Null
  Set-WinUserLanguageList (New-WinUserLanguageList 'en-US') -Force
  $english = ((Get-WinUserLanguageList)[0].InputMethodTips -join ',')
  Invoke-Setup $Installer '/S'
  $root = (Get-ItemProperty 'HKLM:\SOFTWARE\WOW6432Node\Rime\Weasel').WeaselRoot
  $setup = Join-Path $root 'WeaselSetup.exe'
  $settings = Get-ItemProperty -LiteralPath $userKey
  Assert-Test ($settings.Profile -eq 'hant' -and $settings.Hant -eq 1) 'A clean install did not select Taiwan registration.'
  Assert-Test ($null -eq $settings.WordTextLanguage -and $null -eq $settings.WordFollowStyleOnEnter) 'The test requires absent Word settings to exercise compiled defaults.'
  $actual = Get-WinUserLanguageList
  Assert-Test ($actual[0].LanguageTag -eq 'en-US' -and ($actual[0].InputMethodTips -join ',') -eq $english) 'Existing English input methods changed.'
  $taiwan = @($actual | Where-Object LanguageTag -eq 'zh-TW')
  $tip = '0404:{A3F4CDED-B1E9-41EE-9CA6-7B4D0DE6CB0A}{3D02CAB6-2B8E-4781-BA20-1C9267529467}'
  Assert-Test ($taiwan.Count -eq 1 -and $taiwan[0].InputMethodTips -contains $tip) 'The Taiwan input language or Weasel TIP is missing.'
  Assert-Test ((Get-WinUILanguageOverride).Name -eq $display) 'Display language changed.'
  Assert-Test ((Get-WinDefaultInputMethodOverride).InputMethodTip -eq $inputTip) 'Default input method override changed.'
  $source = Join-Path $PSScriptRoot '..\data\luna_pinyin_tw.schema.yaml'
  $installed = Join-Path $root 'data\luna_pinyin_tw.schema.yaml'
  Assert-Test ((Get-FileHash $source).Hash -eq (Get-FileHash $installed).Hash) 'The installed Taiwan schema differs from the supplied file.'
  $default = Get-Content -LiteralPath (Join-Path $root 'data\default.yaml') -Raw
  Assert-Test ($default -match 'schema_list:\s*\n\s*- schema: luna_pinyin_tw\s*\n') 'Taiwan Pinyin is not the first default scheme.'
  $prism = Join-Path $userDirectory 'build\luna_pinyin_tw.prism.bin'
  Assert-Test (Test-Path -LiteralPath $prism) 'The Taiwan Pinyin scheme was not deployed on the fresh profile.'
  Assert-Test ((Get-Item -LiteralPath $prism).Length -gt 0) 'The deployed Taiwan Pinyin prism is empty.'
  Write-Output 'Fresh Taiwan install, supplied schema, deployment and absent-setting defaults passed.'
} finally {
  if ($setup -and (Test-Path -LiteralPath $setup)) { Invoke-Setup $setup '/u' }
  Set-WinUserLanguageList $languages -Force
  Set-WinUILanguageOverride -Language $display
  if ($inputTip) { Set-WinDefaultInputMethodOverride -InputTip $inputTip }
  else { Set-WinDefaultInputMethodOverride }
}
