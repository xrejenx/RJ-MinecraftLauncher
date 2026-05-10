@echo off
setlocal enabledelayedexpansion
set "CERT_NAME=Rezen Developer"
set "CERT_PATH=%~dp0Installer\cert\RezenDeveloper.pfx"
set "INSTALLER_DIR=%~dp0Installer"
echo Checking for certificate: !CERT_NAME!...
certutil -verifystore Root "!CERT_NAME!" >nul 2>&1
if !ERRORLEVEL! equ 0 (
    echo Certificate is already installed.
    if exist "%~dp0Installer\cert" (
        echo Cleaning up certificate source folder...
        rd /s /q "%~dp0Installer\cert"
    )
) else (
    if exist "!CERT_PATH!" (
        echo Importing Rezen Developer Certificate...
        certutil -f -p password -importpfx My "!CERT_PATH!"
        certutil -f -p password -importpfx Root "!CERT_PATH!"
        if !ERRORLEVEL! equ 0 (
            echo Installation successful. Removing cert folder...
            rd /s /q "%~dp0Installer\cert"
        ) else (
            echo ERROR: Certificate installation failed.
            pause
            exit /b 1
        )
    ) else (
        echo WARNING: Certificate file not found and not installed.
    )
)
set "EXE_TO_RUN="
for %%f in ("!INSTALLER_DIR!\*.exe") do (
    set "EXE_TO_RUN=%%f"
)
if defined EXE_TO_RUN (
    echo Launching !EXE_TO_RUN! as Administrator...
    powershell -Command "Start-Process '!EXE_TO_RUN!' -Verb RunAs"
) else (
    echo ERROR: No executable found in !INSTALLER_DIR!
    pause
)
