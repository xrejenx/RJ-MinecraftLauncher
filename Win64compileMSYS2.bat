@echo off
setlocal enabledelayedexpansion

set QT_PATH=C:\Qt\6.11.0\mingw_64
set BUILD_DIR=ninja
set PORTABLE_DIR=%BUILD_DIR%\Windows-Portable

echo =====================================================
echo RJ Launcher - Portable Production Build ^& Signing
echo =====================================================

if exist %BUILD_DIR% (
    rmdir /s /q %BUILD_DIR%
)

echo [1/4] Configuring Project...
cmake -B "%BUILD_DIR%" -DCMAKE_PREFIX_PATH="%QT_PATH%" -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 goto :error

echo [2/4] Building Core Binaries (Launcher ^& Tools)...
cmake --build "%BUILD_DIR%" --target WDLauncher MadeChanges
if %ERRORLEVEL% neq 0 goto :error

echo [3/4] Silently Installing PFX Certificate...
if exist RezenDeveloper.pfx (
    certutil -f -p password -importpfx RezenDeveloper.pfx
) else (
    echo ERROR: RezenDeveloper.pfx not found in root. Skipping cert import.
)

echo [4/4] Signing generated executables...
set SIGNTOOL_PATH=
for /r "C:\Program Files (x86)\Windows Kits" %%i in (signtool.exe) do (
    if "%%~ni"=="signtool" set SIGNTOOL_PATH=%%i
)

if defined SIGNTOOL_PATH (
    for /r "%PORTABLE_DIR%" %%f in (*.exe) do (
        echo Signing: %%f
        "%SIGNTOOL_PATH%" sign /a /n "Rezen Developer" /fd sha256 /tr http://timestamp.digicert.com /td sha256 "%%f"
        if !ERRORLEVEL! equ 0 (
            echo   - SUCCESS
        ) else (
            echo   - FAILED / SKIPPED
        )
    )
) else (
    echo ERROR: signtool.exe was not found on this system.
)

:success
echo.
echo =====================================================
echo BUILD COMPLETE
echo Launcher Folder: %PORTABLE_DIR%
echo =====================================================
pause
exit /b 0

:error
echo.
echo !!! A CRITICAL BUILD ERROR OCCURRED !!!
pause
exit /b 1
