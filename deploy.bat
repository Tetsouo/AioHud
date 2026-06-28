@echo off
REM Copy the built plugin into Windower's plugins folder.
REM NOTE: //unload AioTest in game FIRST -- Windower keeps the DLL file-locked while loaded.
setlocal
set "ROOT=%~dp0"
set "DST=D:\Windower Tetsouo\plugins\AioTest.dll"
copy /Y "%ROOT%build\AioTest.dll" "%DST%" >nul
if errorlevel 1 ( echo [deploy] FAILED -- is AioTest still loaded? do //unload AioTest first & exit /b 1 )
echo [deploy] OK -^> %DST%   (now //load AioTest in game)
