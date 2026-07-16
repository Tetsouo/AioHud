# AioHud updater helper — driven by aioupdate.lua.
#   -Mode prepare : compare the latest GitHub release to -Current ; if newer, download the payload zip to -Zip.
#                   prints  "UPTODATE <ver>" | "READY <ver>" | "ERROR <msg>"
#   -Mode apply   : extract -Zip over -PluginsDir (must be run AFTER //unload AioHud so the DLL is unlocked).
#                   prints  "OK" | "ERROR <msg>"
param(
  [Parameter(Mandatory)][string]$Mode,
  [string]$Repo = 'Tetsouo/AioHud',
  [string]$Current = '0',
  [string]$PluginsDir,
  [string]$Zip
)
$ErrorActionPreference = 'Stop'
try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 } catch {}
$ua = @{ 'User-Agent' = 'AioUpdate' }
try {
  if ($Mode -eq 'prepare') {
    $r = Invoke-RestMethod "https://api.github.com/repos/$Repo/releases/latest" -Headers $ua
    $tag = ($r.tag_name -replace '^v', '')
    if ($tag -eq $Current) { "UPTODATE $tag"; return }
    $a = $r.assets | Where-Object { $_.name -like 'AioHud-*.zip' } | Select-Object -First 1
    if (-not $a) { "ERROR no-zip-in-latest-release"; return }
    New-Item -ItemType Directory -Force -Path (Split-Path $Zip) | Out-Null
    Invoke-WebRequest $a.browser_download_url -OutFile $Zip -Headers $ua
    "READY $tag"
  }
  elseif ($Mode -eq 'apply') {
    # Extract the payload (AioHud.dll + AioHud\assets + design) over plugins\ ; the user's AioHud\data\ is NOT in the
    # zip, so config/profiles are preserved. -Force overwrites the DLL (now unlocked) + refreshed assets.
    Expand-Archive -Path $Zip -DestinationPath $PluginsDir -Force
    Remove-Item $Zip -Force -ErrorAction SilentlyContinue
    "OK"
  }
  else { "ERROR unknown-mode" }
}
catch { "ERROR $($_.Exception.Message)" }
