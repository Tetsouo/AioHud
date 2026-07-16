@echo off
REM Copy the built plugin + runtime assets into Windower. The plugin loads its data from plugins\AioHud\ ;
REM since the dev repo now lives OUTSIDE Windower (G:\01_Development\Game_Project\aiohud), that runtime folder is a
REM SEPARATE clean copy -> we sync the assets here too, not just the DLL.
REM NOTE: //unload AioHud in game FIRST -- Windower keeps the DLL file-locked while loaded.
setlocal
set "ROOT=%~dp0"
set "WP=D:\Windower Tetsouo\plugins"
set "DATA=%WP%\AioHud"

REM 0) MIGRATE the data folder name (once) : _aiohud_re -> AioHud. MUST run before the asset sync below, else robocopy
REM    would create a fresh empty AioHud\ and orphan the old config/profiles still sitting in _aiohud_re\.
if not exist "%DATA%\" if exist "%WP%\_aiohud_re\" ren "%WP%\_aiohud_re" "AioHud"

REM 1) the DLL
copy /Y "%ROOT%build\AioHud.dll" "%WP%\AioHud.dll" >nul
if errorlevel 1 ( echo [deploy] FAILED -- is AioHud still loaded? do //unload AioHud first & exit /b 1 )

REM 2) runtime assets + default layout -> the plugin's data folder.
REM    robocopy is INCREMENTAL (skips unchanged -> near-instant when only code changed) ; *_src\ regen sources excluded.
robocopy "%ROOT%assets" "%DATA%\assets" /E /XD *_src /NFL /NDL /NJH /NJS /NP >nul
mkdir "%DATA%\design\exports" 2>nul
copy /Y "%ROOT%design\exports\layout.json" "%DATA%\design\exports\layout.json" >nul

echo [deploy] OK -^> %WP%\AioHud.dll  (+ assets synced to AioHud\)   (now //load AioHud in game)
