@echo off
REM URUS Test Runner for Windows
REM Usage: run_tests.bat [path\to\urusc.exe]

setlocal enabledelayedexpansion

set URUSC=%1
if "%URUSC%"=="" set URUSC=..\compiler\urusc.exe

set PASS=0
set FAIL=0
set SKIP=0

echo === URUS Test Suite ===
echo.

echo --- Valid programs (should compile) ---
for %%f in (valid\*.urus) do (
    %URUSC% --emit-c "%%f" > nul 2>&1
    if !errorlevel! == 0 (
        echo   PASS %%~nf
        set /a PASS+=1
    ) else (
        echo   FAIL %%~nf (compilation failed)
        set /a FAIL+=1
    )
)

echo.
echo --- Invalid programs (should reject) ---
for %%f in (invalid\*.urus) do (
    %URUSC% --emit-c "%%f" > nul 2>&1
    if !errorlevel! == 0 (
        echo   FAIL %%~nf (should have been rejected)
        set /a FAIL+=1
    ) else (
        echo   PASS %%~nf
        set /a PASS+=1
    )
)

echo.
echo --- Run tests (compile, run, check output) ---
for %%f in (run\*.urus) do (
    if exist "run\%%~nf.expected" (
        %URUSC% "%%f" -o "run\%%~nf.exe" > nul 2>&1
        if !errorlevel! == 0 (
            "run\%%~nf.exe" > "run\%%~nf.actual" 2>&1
            fc /b "run\%%~nf.expected" "run\%%~nf.actual" > nul 2>&1
            if !errorlevel! == 0 (
                echo   PASS %%~nf
                set /a PASS+=1
            ) else (
                echo   FAIL %%~nf (output mismatch)
                set /a FAIL+=1
            )
            del /q "run\%%~nf.exe" "run\%%~nf.actual" 2>nul
        ) else (
            echo   FAIL %%~nf (compilation failed)
            set /a FAIL+=1
        )
    ) else (
        echo   SKIP %%~nf (no .expected file)
        set /a SKIP+=1
    )
)

echo.
echo === Results: %PASS% passed, %FAIL% failed, %SKIP% skipped ===

if %FAIL% gtr 0 exit /b 1
