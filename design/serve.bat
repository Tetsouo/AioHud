@echo off
REM Lance le serveur design AioHUD (auto-save zones.json) puis ouvre la page.
cd /d "%~dp0"
start "" http://localhost:8777/plugins/_aiohud_re/design/aiohud_mockup.html
node serve.mjs %*
