@echo off
REM URUS Compiler Build Script for Windows
REM Requires: GCC (MinGW-w64 or MSYS2)

echo Building URUS Compiler v1.0.0...

gcc -Wall -Wextra -std=c11 -g -Iinclude -o urusc.exe src\main.c src\lexer.c src\ast.c src\parser.c src\util.c src\sema.c src\codegen.c -lm

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    echo.
    echo Make sure GCC is installed and in your PATH.
    echo Install options:
    echo   1. MSYS2: https://www.msys2.org/
    echo      Then run: pacman -S mingw-w64-x86_64-gcc
    echo   2. MinGW-w64: https://www.mingw-w64.org/
    echo   3. WinLibs: https://winlibs.com/
    exit /b 1
)

echo Build successful! Output: urusc.exe
