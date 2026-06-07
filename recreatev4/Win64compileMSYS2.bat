@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo =   RJ Launcher Builder (Qt 6.11)        =
echo ==========================================
echo.

:: -------------------------
:: Configuration (edit if needed)
:: -------------------------
set "QT_PATH=C:\Qt\6.11.0\mingw_64"
set "MSYS2_PATH=C:\msys64"
set "BUILD_DIR=ninja_build"
set "PORTABLE_DIR=%BUILD_DIR%\Windows-Portable"
set "GENERATOR=Ninja"
set "CMAKE_BUILD_TYPE=Release"

:: Number of parallel jobs (default to number of processors)
if "%NUMBER_OF_PROCESSORS%"=="" (
    set "JOBS=4"
) else (
    set "JOBS=%NUMBER_OF_PROCESSORS%"
)

:: Convert Windows path to MSYS style for CMake prefix path
set "UNIX_QT_PATH=%QT_PATH:\=/%"
set "UNIX_QT_PATH=/c%UNIX_QT_PATH:C:=%"

echo QT_PATH = %QT_PATH%
echo MSYS2_PATH = %MSYS2_PATH%
echo BUILD_DIR = %BUILD_DIR%
echo PORTABLE_DIR = %PORTABLE_DIR%
echo.

:: -------------------------
:: Ensure MSYS2 exists
:: -------------------------
if not exist "%MSYS2_PATH%\msys2_shell.cmd" (
    echo ERROR: MSYS2 not found at %MSYS2_PATH%
    pause
    exit /b 1
)

:: -------------------------
:: Run MSYS2 shell and execute build commands
:: We call pacman to ensure required packages are installed,
:: then run cmake configure and build with Ninja.
:: -------------------------
set "MSYS_CMD=%MSYS2_PATH%\msys2_shell.cmd"
set "MSYS_ARGS=-mingw64 -here -no-start -defterm -c"

:: Build command to run inside MSYS2 (use single-quoted UNIX style paths)
set "INNER_CMD=pacman -S --needed --noconfirm base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-openssl mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja || true; \
rm -rf %BUILD_DIR% || true; \
cmake -G \"%GENERATOR%\" -B %BUILD_DIR% -S . -DCMAKE_PREFIX_PATH='%UNIX_QT_PATH%' -DCMAKE_BUILD_TYPE=%CMAKE_BUILD_TYPE%; \
cmake --build %BUILD_DIR% --target RJML0444060626 MadeChanges -j %JOBS%; \
if [ $? -ne 0 ]; then exit 1; fi; \
echo Build finished;"

echo Running MSYS2 build...
call "%MSYS_CMD%" %MSYS_ARGS% "%INNER_CMD%"

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
