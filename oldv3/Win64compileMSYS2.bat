@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo =   RJ Launcher Builder (Qt 6.11)        =
echo ==========================================
echo [Initializing MSYS2 Subsystem Deployment...]

:: 1. Force explicit paths variables to prevent syntax corruption
set "QT_PATH=C:\Qt\6.11.0\mingw_64"
set "BUILD_DIR=ninja_build"
set "PORTABLE_DIR=%BUILD_DIR%\Windows-Portable"

:: 2. Transform the Windows path style to match UNIX style for CMake
set "UNIX_QT_PATH=%QT_PATH:\=/%"
set "UNIX_QT_PATH=/c%UNIX_QT_PATH:C:=%"

:: 3. Launch MSYS2 directly using regular arguments. No tricky formatting strings.
call "C:\msys64\msys2_shell.cmd" -mingw64 -here -no-start -defterm -c "pacman -S --needed --noconfirm base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-openssl mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja && rm -rf %BUILD_DIR% && cmake -G 'Ninja' -B %BUILD_DIR% -DCMAKE_PREFIX_PATH='%UNIX_QT_PATH%' -DCMAKE_BUILD_TYPE=Release && cmake --build %BUILD_DIR% --target WDLauncher MadeChanges -j %NUMBER_OF_PROCESSORS%"

:: 4. Verify MSYS2 execution status
if %ERRORLEVEL% neq 0 goto :error

:success
echo.
echo =====================================================
echo   BUILD SUCCESSFUL
echo   Location: %PORTABLE_DIR%
echo =====================================================
pause
exit /b 0

:error
echo.
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo   BUILD FAILED: Check toolchain flags or pathing logs
echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
pause
exit /b 1
