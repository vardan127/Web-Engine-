@echo off
echo Building FukFukGo...

gcc src\main.c src\http.c src\parser.c src\cJSON.c -Iinclude -lcurl -Wall -O2 -o fukfukgo.exe

if %ERRORLEVEL% EQU 0 (
    echo Build successful! Run with: fukfukgo.exe
) else (
    echo Build failed.
)
