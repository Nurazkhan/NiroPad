@echo off
cd /d "%~dp0"
title NiroPad Companion Daemon

echo ========================================================
echo           NiroPad PC Telemetry Companion
echo ========================================================
echo.

where python >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    where py >nul 2>nul
    if %ERRORLEVEL% NEQ 0 (
        echo ERROR: Python was not found in your system PATH.
        echo Please install Python 3 and check "Add Python to PATH".
        echo.
        pause
        exit /b 1
    )
    set PY_CMD=py -3
) else (
    set PY_CMD=python
)

echo Using Python: %PY_CMD%
%PY_CMD% -c "import psutil, hid, winsdk, pycaw" >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Installing missing Python dependencies...
    %PY_CMD% -m pip install psutil hidapi pycaw winsdk
)

echo Starting telemetry daemon...
echo Keep this window open or minimized while using NiroPad.
echo.
%PY_CMD% niropad_stats.py
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Daemon exited with code: %ERRORLEVEL%
)
pause
