@echo off
setlocal
REM SFB Fleet Bridge - tactical display (Board / Bridge / Ships / Comms).
REM
REM   double-click                        -> Kzinti (AI) vs Lyran (you)
REM   Start SFB AI.bat Klingon Federation -> AI side first, your side second
REM   Start SFB AI.bat Kzinti Lyran --no-voice
REM
REM On start it ASKS which save to follow, listing each with its ship count,
REM races and turn, then stays on that one. It no longer picks the newest file
REM by itself - that quietly followed whatever the client last wrote, so a
REM scratch test scenario could displace a live battle mid-game.
REM
REM NEWEST WINS: relaunching always opens a fresh bridge and closes any prior one
REM (so a crashed/hung bridge can never stop the next launch from opening).
REM
REM   ... --save "<path>"    follow this save, skip the question
REM   ... --auto-save-pick   restore the old newest-file-wins behaviour
REM
REM If this window flashes an error, run "Start SFB AI (debug).bat" instead -
REM that one keeps the console open and shows what went wrong.

REM %~dp0 = this file's own folder, so the launcher works from anywhere
REM (Explorer, a shortcut, a pinned taskbar item) rather than a hardcoded path.
cd /d "%~dp0"

set "AI=%~1"
set "ADVISE=%~2"
if "%AI%"=="" set "AI=Kzinti"
if "%ADVISE%"=="" set "ADVISE=Lyran"

REM Resolve pythonw explicitly. A bare "pythonw" can hit the Microsoft Store
REM stub in WindowsApps, which silently opens the Store instead of running.
set "PYW=C:\Python314\pythonw.exe"
if not exist "%PYW%" set "PYW=pythonw.exe"

if not exist "%~dp0sfb_bridge.py" (
  echo Cannot find sfb_bridge.py next to this launcher:
  echo   %~dp0
  pause
  exit /b 1
)

REM A real window title, full exe path, and a fully qualified script path -
REM START parses "start "" prog args" inconsistently when the title is empty
REM and the path contains spaces.
start "SFB Fleet Bridge" "%PYW%" "%~dp0sfb_bridge.py" --ai "%AI%" --advise "%ADVISE%" %3 %4 %5
