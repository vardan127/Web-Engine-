@echo off
echo ======================================================
echo          Building Vardan's Browser
echo ======================================================

set BIN_DIR=bin
if not exist %BIN_DIR% mkdir %BIN_DIR%

echo [1/3] Copying runtime binaries to %BIN_DIR%...
xcopy /E /I /Y /Q cef\Release\* %BIN_DIR%\ >nul
xcopy /E /I /Y /Q cef\Resources\* %BIN_DIR%\ >nul

echo [2/3] Compiling Vardan's Browser in C...
gcc src\browser.c -Icef -Lcef\Release -lcef -lgdi32 -luser32 -lcomctl32 -mwindows -O2 -o %BIN_DIR%\vardan_browser.exe

if %ERRORLEVEL% EQU 0 (
    echo [3/3] Build Successful!
    echo.
    echo Launch Vardan's Browser with:
    echo   .\bin\vardan_browser.exe
) else (
    echo [Error] Build failed! Check compiler logs above.
)
