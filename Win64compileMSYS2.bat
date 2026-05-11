@echo off
setlocal enabledelayedexpansion

set QT_PATH=C:\Qt\6.11.0\mingw_64
set BUILD_DIR=ninja
set PORTABLE_DIR=%BUILD_DIR%\Windows-Portable

echo =====================================================
echo RJ Launcher - Portable Production Build
echo =====================================================
echo To being the build process, ensure you run this in msys2-mingw terminal with the QT installed in your c: drive
echo To Continue Install Dependencies Press Any Key...
pause
pacman -S --needed base-devel mingw-w64-x86_64-openssl mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-declarative mingw-w64-x86_64-qt6-webview mingw-w64-x86_64-qt6-tools mingw-w64-x86_64-qt6-svg
if exist %BUILD_DIR% (
    rmdir /s /q %BUILD_DIR%
)

echo [1/2] Configuring Project...
cmake -B "%BUILD_DIR%" -DCMAKE_PREFIX_PATH="%QT_PATH%" -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 goto :error

echo [2/2] Building Core Binaries (Launcher ^& Tools)...
cmake --build "%BUILD_DIR%" --target WDLauncher MadeChanges
if %ERRORLEVEL% neq 0 goto :error

:success
echo.
echo =====================================================
echo BUILD COMPLETE
echo Launcher Folder: %PORTABLE_DIR%
echo =====================================================
pause
./Win64compileMSYS2.bat

:error
echo.
echo !!! A CRITICAL BUILD ERROR OCCURRED !!!
pause
./Win64compileMSYS2.bat
