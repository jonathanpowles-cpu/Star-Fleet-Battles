@echo off
setlocal
REM Same launcher, but with a VISIBLE console that stays open.
REM Use this when the normal launcher fails - Python tracebacks and any
REM startup error appear here instead of vanishing with the window.

cd /d "%~dp0"

set "AI=%~1"
set "ADVISE=%~2"
if "%AI%"=="" set "AI=Kzinti"
if "%ADVISE%"=="" set "ADVISE=Lyran"

set "PY=C:\Python314\python.exe"
if not exist "%PY%" set "PY=python.exe"

echo folder : %~dp0
echo python : %PY%
echo sides  : AI=%AI%  you=%ADVISE%
echo.
"%PY%" -c "import sys;print('python',sys.version)"
echo.
echo --- launching (close this window to stop the app) ---
"%PY%" "%~dp0sfb_bridge.py" --ai "%AI%" --advise "%ADVISE%" %3 %4 %5
echo.
echo --- exited with code %ERRORLEVEL% ---
pause
