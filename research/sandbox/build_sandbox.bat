@echo off
REM Build the SANDBOX (experimentation lab) -> build\AioTest.dll  (32-bit, MSVC).
REM The sandbox is the scratch space for prototyping effects before they are
REM promoted into a real widget under src\. NOTE: it outputs the SAME AioTest.dll,
REM so build the sandbox OR the clean base (build.bat), not both at once.
setlocal
set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
set "ROOT=%~dp0"
call "%VCVARS%" x86 >nul
if errorlevel 1 ( echo [build] vcvars failed & exit /b 1 )

cl /nologo /LD /O2 /MT /EHsc- /I"%ROOT%include" "%ROOT%sandbox\sandbox.cpp" ^
   /Fo"%ROOT%build\sandbox.obj" /Fe"%ROOT%build\AioTest.dll" ^
   /link /DEF:"%ROOT%sandbox\sandbox.def" user32.lib kernel32.lib /OUT:"%ROOT%build\AioTest.dll"

if exist "%ROOT%build\AioTest.dll" ( echo [build] OK -^> %ROOT%build\AioTest.dll ) else ( echo [build] FAILED & exit /b 1 )
