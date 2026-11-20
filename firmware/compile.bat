@echo off
set MSYSTEM=MINGW64
echo Compiling NiroPad Firmware with VIA and OLED Telemetry...
"C:\QMK_MSYS\usr\bin\bash.exe" -lc "qmk compile -kb nurazkhan/mymacropad -km default -e VERSION_H_FLAGS=--skip-git"
if %ERRORLEVEL% EQU 0 (
    copy /Y "C:\Users\nuraz\qmk_firmware\nurazkhan_mymacropad_default.uf2" "%~dp0nurazkhan_mymacropad_default.uf2"
    echo.
    echo ========================================================
    echo Build SUCCESS! New UF2 copied to:
    echo %~dp0nurazkhan_mymacropad_default.uf2
    echo ========================================================
) else (
    echo Build failed with error code %ERRORLEVEL%
)
pause
