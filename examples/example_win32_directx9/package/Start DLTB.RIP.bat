@echo off
setlocal
title DLTB.RIP Launcher
cd /d "%~dp0"

:: DLTB.RIP - way 1: starts Dying Light: The Beast through Steam (if it is not
:: running yet), waits for the game, then starts the trainer from this folder.
:: Way 2 needs no launcher: just run DLTB.RIP.exe while the game is running.

set "GAME_EXE=DyingLightGame_TheBeast_x64_rwdi.exe"
set "TRAINER=%~dp0DLTB.RIP.exe"

if not exist "%TRAINER%" (
    echo [ERROR] DLTB.RIP.exe was not found next to this file.
    echo Extract the whole archive into one folder and try again.
    pause
    exit /b 1
)

:: CSV output: the default table cuts image names to 25 characters, so the
:: long game exe name would never be found.
tasklist /FI "IMAGENAME eq %GAME_EXE%" /FO CSV /NH 2>nul | find /I "%GAME_EXE%" >nul
if not errorlevel 1 goto :start_trainer

echo Starting Dying Light: The Beast through Steam...
start "" "steam://rungameid/3008130"

echo Waiting for the game (up to 3 minutes)...
for /L %%i in (1,1,90) do (
    timeout /t 2 /nobreak >nul
    tasklist /FI "IMAGENAME eq %GAME_EXE%" /FO CSV /NH 2>nul | find /I "%GAME_EXE%" >nul && goto :start_trainer
)
echo [ERROR] The game did not start. Launch it from Steam, then run DLTB.RIP.exe.
pause
exit /b 1

:start_trainer
:: The trainer asks for administrator rights itself and waits until the game has
:: finished loading. If an older copy is running, it closes it cleanly.
echo Starting DLTB.RIP... (press INSERT in game to open the menu)
start "" "%TRAINER%"
exit /b 0
