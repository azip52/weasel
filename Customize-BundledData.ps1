param(
  [string]$DataDirectory = (Join-Path $PSScriptRoot 'output\data')
)

$ErrorActionPreference = 'Stop'
$schema = Join-Path $PSScriptRoot 'data\luna_pinyin_tw.schema.yaml'
if (-not (Test-Path -LiteralPath (Join-Path $DataDirectory 'luna_pinyin.schema.yaml'))) {
  throw 'The bundled luna-pinyin dependency is missing.'
}
Copy-Item -LiteralPath $schema -Destination (Join-Path $DataDirectory 'luna_pinyin_tw.schema.yaml') -Force

# Make the supplied Taiwan variant the first choice on a clean user profile.
# Existing user default.custom.yaml overrides continue to take precedence.
$defaultPath = Join-Path $DataDirectory 'default.yaml'
$text = [IO.File]::ReadAllText($defaultPath).Replace("`r`n", "`n")
$text = [regex]::Replace($text, '(?m)^  - schema: luna_pinyin_tw[ \t]*\n?', '')
$header = [regex]::new('(?m)^schema_list:[ \t]*$')
if ($header.Matches($text).Count -ne 1) {
  throw 'Expected exactly one default schema_list.'
}
$text = $header.Replace($text, "schema_list:`n  - schema: luna_pinyin_tw", 1)
[IO.File]::WriteAllText($defaultPath, $text, [Text.UTF8Encoding]::new($false))
Write-Output 'Bundled the supplied Taiwan schema and selected it as the first default scheme.'
