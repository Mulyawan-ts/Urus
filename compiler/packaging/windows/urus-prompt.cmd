@echo off
rem ----------------------------------------------------------------------
rem Urus Compiler Prompt — wraps cmd.exe with Urus bin/ already on PATH.
rem
rem Installed by the NSIS bundle and surfaced via the Start Menu shortcut
rem the installer creates. Lets users run `urusc --version` /
rem `urusc hello.urus -o app` straight from a friendly console without
rem having to know or remember the install directory.
rem
rem We resolve the install root from this script's own directory rather
rem than hard-coding C:\Program Files\Urus — the installer lets users
rem pick a custom location and that custom path needs to keep working.
rem ----------------------------------------------------------------------

setlocal

rem %~dp0 = directory of THIS script, with trailing backslash.
rem Layout shipped by the installer is:
rem   <prefix>\bin\urusc.exe
rem   <prefix>\bin\urus-prompt.cmd       <-- here
rem   <prefix>\lib\urusc\*.urus
rem so the install prefix is one directory above %~dp0.

set "URUS_BIN=%~dp0"
for %%I in ("%URUS_BIN%..") do set "URUS_PREFIX=%%~fI"
set "URUSCPATH=%URUS_PREFIX%\lib\urusc"
set "PATH=%URUS_BIN%;%PATH%"

title Urus Compiler Prompt

echo.
echo  Urus Compiler Prompt
echo  --------------------
echo  Install root : %URUS_PREFIX%
echo  Stdlib path  : %URUSCPATH%
echo.
echo  Try:  urusc --version
echo        urusc hello.urus -o hello
echo.

rem /K = run the rest then drop into an interactive shell.
cmd /K
