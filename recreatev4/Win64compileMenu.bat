@echo off
:: Save the starting directory
set "STARTDIR=%~dp0"

:menu
cls
echo ============================
echo    COMPILE AND RUN MENU
echo ============================
echo [C] / Enter = Compile
echo [R] / Del   = Run RJML.exe
echo [X] / Esc   = Exit
echo ============================
set "choice="
set /p choice="Select option: "

:: Handle choices
if /i "%choice%"=="C" goto compile
if /i "%choice%"=="" goto compile
if /i "%choice%"=="R" goto runRJML
if /i "%choice%"=="X" goto exitScript

goto menu

:compile
echo Starting Win64compileMSYS2.bat...
cd /d "%STARTDIR%"
call Win64compileMSYS2.bat
goto menu

:runRJML
echo Launching RJML.exe...
cd /d "%STARTDIR%ninja_build\Windows-Portable"
start "" RJML.exe
goto menu

:exitScript
echo Exiting...
exit
