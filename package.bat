@echo off
REM package.bat -- build, then assemble ONLY the game-necessary files into dist\ (a clean, shareable payload).
REM
REM   dist\AioHud.dll        ->  copy to  <windower>\plugins\AioHud.dll
REM   dist\AioHud\        ->  copy to  <windower>\plugins\AioHud\   (the DLL loads its data from this sibling)
REM
REM Ships ONLY what the plugin actually loads at runtime : the DLL + assets\ (minus the *_src regeneration sources)
REM + the default layout.json. Everything else in the project (src\, docs\, scripts\, re\ dumps, build\ objs,
REM screenshots, .git, ...) is dev-only and is NOT copied. Runtime-written files (aio_config.txt, profiles,
REM aiohud_*.bin) are created fresh in-game, so they are not shipped either.
setlocal
set "ROOT=%~dp0"
set "DIST=%ROOT%dist"
set "DATA=%DIST%\AioHud"

REM 1) build first -- a bad build aborts the package (never ship a stale DLL).
call "%ROOT%build.bat"
if errorlevel 1 ( echo [package] build failed -- aborting & exit /b 1 )

REM 2) clean dist\ from scratch so removed assets never linger.
if exist "%DIST%" rmdir /S /Q "%DIST%"
mkdir "%DATA%"

REM 3) the DLL.
copy /Y "%ROOT%build\AioHud.dll" "%DIST%\AioHud.dll" >nul

REM 4) runtime assets : the whole assets\ tree EXCEPT any *_src\ (job/window/icon regeneration sources).
REM    robocopy exit codes 0-7 = success ; 8+ = real error (don't treat <8 as failure).
robocopy "%ROOT%assets" "%DATA%\assets" /E /XD *_src /NFL /NDL /NJH /NJS /NP >nul
if errorlevel 8 ( echo [package] asset copy failed & exit /b 1 )

REM 5) the default box layout (positions/toggles) the plugin reads at startup.
mkdir "%DATA%\design\exports" 2>nul
copy /Y "%ROOT%design\exports\layout.json" "%DATA%\design\exports\layout.json" >nul

echo.
echo [package] OK -^> %DIST%
echo   dist\AioHud.dll     (-^> ^<windower^>\plugins\AioHud.dll)
echo   dist\AioHud\     (-^> ^<windower^>\plugins\AioHud\)
powershell -NoProfile -Command "'  total payload : {0:N1} MB' -f ((Get-ChildItem -Recurse -File '%DIST%' | Measure-Object -Sum Length).Sum/1MB)"
