cmake_minimum_required(VERSION 3.20)
# MADE IN THAILAND
project(RJML VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(WIN32)
    # On Windows, we avoid the massive WebEngine-based QtWebView module
    find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui QuickWidgets Network)
else()
    find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui WebView QuickWidgets Network)
endif()

set(APP_VERSION ${PROJECT_VERSION})

# App Meta Details
set(APP_NAME "RJMinecraftLauncher")
set(APP_SHORT_NAME "RJML")
set(APP_AUTHOR "xrejenx")

# Get Icon version directory from iconset.txt (e.g. 0.3 or 0.4)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/iconset.txt")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/iconset.txt" ICON_VER_DIR_RAW)
    string(STRIP "${ICON_VER_DIR_RAW}" ICON_VER_DIR)
else()
    set(ICON_VER_DIR "0.4")
endif()

include_directories(
    .
    Account
    Account/OFFLINE
    CmakeLauncherIcon
    CMakeSplashScreen
    CMakeSplashScreen/Fetch
    CMakeSplashScreen/Downloader
    LTheme
    Profile
    Profile_java
    Profile_java/INSTANCES_MANAGER
    Windows
    LauncherUpdater/Launcher
    LauncherUpdater/Launcher/LauncherUrls
    LauncherUpdater/LauncherDefender
    LauncherUpdater/LauncherFetch
    LauncherUpdater/LauncherSource
    LauncherUpdater/LauncherUpdateCrashHandle
    LauncherUpdater/LauncherDownload
    LauncherUpdater/LauncherzipExtractor
)

# Made App
add_executable(WDLauncher WIN32
    Core.cpp
    ConsoleOutput.cpp
    Settings.cpp
    Account/AddAccount.cpp
    Account/OFFLINE/offline_auth.cpp
    Account/OFFLINE/offline_handle.cpp
    Instances/instra_list.cpp
    LauncherPatch/version.cpp
    CmakeLauncherIcon/icon.cpp
    CMakeSplashScreen/splash.cpp
    CMakeSplashScreen/splashbar.cpp
    CMakeSplashScreen/splashscreenlogic.cpp
    CMakeSplashScreen/assets/splashmanager.cpp
    CMakeSplashScreen/assets/splashcolor.cpp
    CMakeSplashScreen/Fetch/fetchhelper.cpp
    CMakeSplashScreen/Downloader/splashdownloadmanager.cpp
    LTheme/theme.cpp
    LTheme/desktoptheme.cpp
    LTheme/Themeloader.cpp
    Profile/InstanceSettings.cpp
    Profile/Java_download.cpp
    Profile/Java_settings.cpp
    Profile/Profile.cpp
    Profile_java/JavaProfile.cpp
    Profile_java/lwjglhandle.cpp
    Profile_java/lwjgl-lib.cpp
    Profile_java/INSTANCES_MANAGER/insta_handle.cpp
    Profile_java/INSTANCES_MANAGER/instance-javaRequirement.cpp
    Profile_java/INSTANCES_MANAGER/launchargs_creator.cpp
    Profile_java/INSTANCES_MANAGER/vanilla.cpp
    Profile_java/INSTANCES_MANAGER/vanilla_installer.cpp
    Profile_java/INSTANCES_MANAGER/vanilla_mc_downloader.cpp
    Profile_java/INSTANCES_MANAGER/vanilla_mc_handle.cpp
    LauncherUpdater/Launcher/LauncherUrls/urldetector.cpp
    LauncherUpdater/Launcher/updater.cpp
    LauncherUpdater/Launcher/updatecore.cpp
    LauncherUpdater/LauncherDefender/shacheckv1.cpp
    LauncherUpdater/LauncherDefender/shacheckv2.cpp
    LauncherUpdater/LauncherDefender/shachecklocal.cpp
    LauncherUpdater/LauncherFetch/fetch.cpp
    LauncherUpdater/LauncherFetch/fetchurl.cpp
    LauncherUpdater/LauncherFetch/fetchdetail.cpp
    LauncherUpdater/LauncherFetch/fetchos.cpp
    LauncherUpdater/LauncherSource/zipdetector.cpp
    LauncherUpdater/LauncherSource/rollback.cpp
    LauncherUpdater/LauncherUpdateCrashHandle/crash-handle.cpp
    LauncherUpdater/LauncherUpdateCrashHandle/crash.cpp
    LauncherUpdater/LauncherDownload/download.cpp
    LauncherUpdater/LauncherDownload/downloadzip.cpp
    LauncherUpdater/LauncherDownload/downloadpackage.cpp
    LauncherUpdater/LauncherzipExtractor/downloadlib.cpp
)

# PatchHistory
# Use CONFIGURE_DEPENDS so adding new files triggers a rebuild
file(GLOB PATCH_FILES CONFIGURE_DEPENDS "LauncherPatch/PatchHistory/*.txt")
if(PATCH_FILES)
    qt_add_resources(WDLauncher "patches"
        PREFIX "/patches"
        BASE "LauncherPatch/PatchHistory"
        FILES ${PATCH_FILES}
    )
endif()

# Launcher Icons based on Build Legit Prefixes
qt_add_resources(WDLauncher "launcher_icons"
    PREFIX "/launcher/icons/CmakeLauncherIcon"
    BASE "CmakeLauncherIcon/${ICON_VER_DIR}"
    FILES
        "CmakeLauncherIcon/${ICON_VER_DIR}/undev/icon.png"
        "CmakeLauncherIcon/${ICON_VER_DIR}/beta/icon.png"
        "CmakeLauncherIcon/${ICON_VER_DIR}/release/icon.png"
)

# Splash Icons based on Build Legit Prefixes
qt_add_resources(WDLauncher "splash_icons"
    PREFIX "/launcher/splash/CmakeLauncherIcon"
    BASE "CmakeLauncherIcon/${ICON_VER_DIR}"
    FILES
        "CmakeLauncherIcon/${ICON_VER_DIR}/undev/icon.png"
        "CmakeLauncherIcon/${ICON_VER_DIR}/beta/icon.png"
        "CmakeLauncherIcon/${ICON_VER_DIR}/release/icon.png"
)

# API Configuration
qt_add_resources(WDLauncher "api_config"
    PREFIX "/config"
    FILES
        "CMakeSplashScreen/api.txt"
)

# App name change from Codename To Production
set_target_properties(WDLauncher PROPERTIES
    OUTPUT_NAME "${APP_SHORT_NAME}"
    PROJECT_LABEL "${APP_NAME}"
)

# --- Metadata Detection from Patch History (All Platforms) ---
file(GLOB DETECT_PATCH_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/LauncherPatch/PatchHistory/*.txt")
set(LATEST_BUILD_NUM -1)
set(DETECTED_BUILD_STATUS "release")
set(RC_VERSION_STR "1.0.0")
set(RC_BUILD_TYPE_SHORT "r")
set(RC_PRODUCT_NAME "${APP_NAME}")
set(RC_COMPANY "Rezen Developer")
set(RC_DESCRIPTION "RJ Launcher Installer")

foreach(PATCH_FILE ${DETECT_PATCH_FILES})
    file(READ "${PATCH_FILE}" PATCH_CONTENT)
    if(PATCH_CONTENT MATCHES "Build_number *= *([0-9]+)")
        set(CURRENT_FILE_BUILD ${CMAKE_MATCH_1})
        if(CURRENT_FILE_BUILD GREATER LATEST_BUILD_NUM)
            set(LATEST_BUILD_NUM ${CURRENT_FILE_BUILD})
            set(CURRENT_PREFIX "release")
            if(PATCH_CONTENT MATCHES "Build_type_prefix *= *([^ \n\r\t]+)")
                set(CURRENT_PREFIX ${CMAKE_MATCH_1})
            endif()
            if(PATCH_CONTENT MATCHES "App_ver *= *([^ \n\r\t]+)")
                set(RC_VERSION_STR ${CMAKE_MATCH_1})
            endif()
            if(PATCH_CONTENT MATCHES "Build_type_prefix_short *= *([^ \n\r\t]+)")
                set(RC_BUILD_TYPE_SHORT ${CMAKE_MATCH_1})
            endif()
            if(PATCH_CONTENT MATCHES "App_name *= *([^\n\r]+)")
                set(RC_PRODUCT_NAME ${CMAKE_MATCH_1})
            endif()
            if(PATCH_CONTENT MATCHES "CompanyName *= *([^\n\r]+)")
                set(RC_COMPANY ${CMAKE_MATCH_1})
            endif()
            if(PATCH_CONTENT MATCHES "FileDescription *= *([^\n\r]+)")
                set(RC_DESCRIPTION ${CMAKE_MATCH_1})
            endif()
            if(PATCH_CONTENT MATCHES "InternalName *= *([^ \n\r\t]+)")
                set(RC_INTERNAL_NAME ${CMAKE_MATCH_1})
            else()
                set(RC_INTERNAL_NAME "RJML")
            endif()
            if(PATCH_CONTENT MATCHES "Cmakeiconconfig *= *([^ \n\r\t]+)")
                set(CONFIG_VAL ${CMAKE_MATCH_1})
                string(REPLACE "<Build_type_prefix>" "${CURRENT_PREFIX}" CONFIG_VAL "${CONFIG_VAL}")
                set(DETECTED_BUILD_STATUS "${CONFIG_VAL}")
            else()
                set(DETECTED_BUILD_STATUS "${CURRENT_PREFIX}")
            endif()
        endif()
    endif()
endforeach()

set(BUILD_STATUS "${DETECTED_BUILD_STATUS}")

# Embed Metadata inside the executable binary via compile definitions
target_compile_definitions(WDLauncher PRIVATE
    LAUNCHER_APP_NAME="${APP_NAME}"
    LAUNCHER_VERSION="${APP_VERSION}"
    LAUNCHER_AUTHOR="${APP_AUTHOR}"
    LAUNCHER_SHORT_NAME="${APP_SHORT_NAME}"
    LAUNCHER_BUILD_TYPE_SHORT="${RC_BUILD_TYPE_SHORT}"
)

# --- Windows Resource Compilation (Icon and Metadata) ---
if(WIN32)
    # Prepare the comma-separated version (0.3.8 -> 0,3,8,0)
    string(REPLACE "." ";" VER_LIST ${RC_VERSION_STR})
    list(LENGTH VER_LIST VER_LEN)
    set(RC_VERSION_COMMA "0,0,0,0")
    if(VER_LEN GREATER_EQUAL 3)
        list(GET VER_LIST 0 V1)
        list(GET VER_LIST 1 V2)
        list(GET VER_LIST 2 V3)
        set(RC_VERSION_COMMA "${V1},${V2},${V3},0")
    endif()

    # Prepare extra RC strings
    set(RC_ORIGINAL_FILENAME "RJML.exe")
    set(RC_PRODUCT_VERSION_STR "${RC_VERSION_STR}i${RC_BUILD_TYPE_SHORT}")

    message(STATUS "Auto-detected BUILD_STATUS: ${BUILD_STATUS}")
    message(STATUS "Generating Resource Details for Version: ${RC_VERSION_STR}")
    # Generate the final .rc file
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Windows/details.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/details_generated.rc")

    set(APP_ICON_SRC "${CMAKE_CURRENT_SOURCE_DIR}/CmakeLauncherIcon/${ICON_VER_DIR}/${BUILD_STATUS}/icon.ico")
    set(APP_ICON_DST "${CMAKE_CURRENT_SOURCE_DIR}/Windows/launcher_icon.ico")
    set(INST_ICON_SRC "${CMAKE_CURRENT_SOURCE_DIR}/CmakeLauncherIcon/${ICON_VER_DIR}/release/icon.ico")
    set(INST_ICON_DST "${CMAKE_CURRENT_SOURCE_DIR}/Windows/RJINSTICON.ico")

    # Copy the selected icon to the Windows directory so the .rc file can find it as 'launcher_icon.ico'
    if(EXISTS "${APP_ICON_SRC}")
        configure_file("${APP_ICON_SRC}" "${APP_ICON_DST}" COPYONLY)
    endif()
    if(EXISTS "${INST_ICON_SRC}")
        configure_file("${INST_ICON_SRC}" "${INST_ICON_DST}" COPYONLY)
    endif()

    if(EXISTS "${APP_ICON_DST}")
        # IMPORTANT: Remove any VERSIONINFO from Windows/windows_resources.rc manually!
        target_sources(WDLauncher PRIVATE 
            Windows/windows_resources.rc 
            "${CMAKE_CURRENT_BINARY_DIR}/details_generated.rc"
        )
    else()
        message(STATUS "WARNING: icon.ico not found for status '${BUILD_STATUS}'. Skipping resource compilation.")
    endif()
endif()

if(WIN32)
    # --- Main Launcher Configuration ---
    target_link_libraries(WDLauncher PRIVATE
        Qt6::Widgets Qt6::Core Qt6::Gui Qt6::QuickWidgets Qt6::Network
        ole32 # Required for COM initialization (WebView2)
    )

    # Set specific output directory for the portable version (with all DLLs)
    set(PORTABLE_DIR "${CMAKE_BINARY_DIR}/Windows-Portable")
    set_target_properties(WDLauncher PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PORTABLE_DIR}")

    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

    # Automatically collect DLLs into the Portable folder
    add_custom_command(TARGET WDLauncher POST_BUILD
        COMMAND "${_qt_bin_dir}/windeployqt" --no-translations --compiler-runtime "$<TARGET_FILE:WDLauncher>"
        COMMENT "Deploying DLL dependencies to ${PORTABLE_DIR}..."
    )

    # --- RJL Installer Configuration ---
    add_executable(RJLInstaller WIN32
        Windows/WinInstaller/installer.cpp
        LauncherUpdater/LauncherFetch/fetch.cpp
        LauncherUpdater/Launcher/LauncherUrls/urldetector.cpp
        LauncherPatch/version.cpp
        LauncherUpdater/LauncherDownload/download.cpp
        LauncherUpdater/LauncherDownload/downloadzip.cpp
        ConsoleOutput.cpp
    )
    
    # --- MadeChanges (Rollback Tool) Configuration ---
    add_executable(MadeChanges WIN32
        Windows/WinInstaller/MadeChanges.cpp
        LauncherUpdater/LauncherDownload/download.cpp
        LauncherUpdater/LauncherDownload/downloadzip.cpp
        ConsoleOutput.cpp
    )
    
    # We put MadeChanges in a Tools subfolder of the portable directory
    set_target_properties(MadeChanges PROPERTIES
        OUTPUT_NAME "MadeChanges"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Windows-Portable/Tools"
    )
    target_link_libraries(MadeChanges PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network)

    # Output the installer to the root build folder for easy access
    set_target_properties(RJLInstaller PROPERTIES 
        OUTPUT_NAME "RJLInstaller"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

    # Linking and Resources
    target_link_libraries(RJLInstaller PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network)
    
    # Attach generated metadata resource
    if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/details_generated.rc")
        target_sources(RJLInstaller PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/details_generated.rc")
    endif()

    # Windows Specific Resource for Installer Icon
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Windows/RJINSTICON.ico")
        target_sources(RJLInstaller PRIVATE Windows/installer_resources.rc)
    endif()

    # Generate Install.bat alongside RJLInstaller.exe in the build root
    file(WRITE "${CMAKE_BINARY_DIR}/Install.bat"
        "@echo off\r\n"
        "setlocal enabledelayedexpansion\r\n"
        "set \"CERT_NAME=Rezen Developer\"\r\n"
        "set \"CERT_PATH=%~dp0Installer\\cert\\RezenDeveloper.pfx\"\r\n"
        "set \"INSTALLER_DIR=%~dp0Installer\"\r\n"
        "echo Checking for certificate: !CERT_NAME!...\r\n"
        "certutil -verifystore Root \"!CERT_NAME!\" >nul 2>&1\r\n"
        "if !ERRORLEVEL! equ 0 (\r\n"
        "    echo Certificate is already installed.\r\n"
        "    if exist \"%~dp0Installer\\cert\" (\r\n"
        "        echo Cleaning up certificate source folder...\r\n"
        "        rd /s /q \"%~dp0Installer\\cert\"\r\n"
        "    )\r\n"
        ") else (\r\n"
        "    if exist \"!CERT_PATH!\" (\r\n"
        "        echo Importing Rezen Developer Certificate...\r\n"
        "        certutil -f -p password -importpfx My \"!CERT_PATH!\"\r\n"
        "        certutil -f -p password -importpfx Root \"!CERT_PATH!\"\r\n"
        "        if !ERRORLEVEL! equ 0 (\r\n"
        "            echo Installation successful. Removing cert folder...\r\n"
        "            rd /s /q \"%~dp0Installer\\cert\"\r\n"
        "        ) else (\r\n"
        "            echo ERROR: Certificate installation failed.\r\n"
        "            pause\r\n"
        "            exit /b 1\r\n"
        "        )\r\n"
        "    ) else (\r\n"
        "        echo WARNING: Certificate file not found and not installed.\r\n"
        "    )\r\n"
        ")\r\n"
        "set \"EXE_TO_RUN=\"\r\n"
        "for %%f in (\"!INSTALLER_DIR!\\*.exe\") do (\r\n"
        "    set \"EXE_TO_RUN=%%f\"\r\n"
        ")\r\n"
        "if defined EXE_TO_RUN (\r\n"
        "    echo Launching !EXE_TO_RUN! as Administrator...\r\n"
        "    powershell -Command \"Start-Process '!EXE_TO_RUN!' -Verb RunAs\"\r\n"
        ") else (\r\n"
        "    echo ERROR: No executable found in !INSTALLER_DIR!\r\n"
        "    pause\r\n"
        ")\r\n"
    )

    target_compile_definitions(RJLInstaller PRIVATE
        LAUNCHER_APP_NAME="${APP_NAME}"
        LAUNCHER_VERSION="${APP_VERSION}"
        LAUNCHER_BUILD_TYPE_SHORT="${RC_BUILD_TYPE_SHORT}"
    )
    
    # --- DLL Deployment for Installer itself ---
    set(INSTALLER_BUNDLE_DIR "${CMAKE_BINARY_DIR}/Installer")
    add_custom_command(TARGET RJLInstaller POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${INSTALLER_BUNDLE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:RJLInstaller>" "${INSTALLER_BUNDLE_DIR}/"
        COMMAND "${_qt_bin_dir}/windeployqt" --no-translations --compiler-runtime "${INSTALLER_BUNDLE_DIR}/RJLInstaller.exe"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/Windows/Cert"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${INSTALLER_BUNDLE_DIR}/cert"
        COMMAND powershell -NoProfile -Command "if (!(Test-Path '${CMAKE_CURRENT_SOURCE_DIR}/RezenDeveloper.pfx')) { \$cert = New-SelfSignedCertificate -Type CodeSigningCert -Subject 'CN=Rezen Developer' -CertStoreLocation Cert:\\CurrentUser\\My; Export-PfxCertificate -Cert \$cert -FilePath '${CMAKE_CURRENT_SOURCE_DIR}/RezenDeveloper.pfx' -Password (ConvertTo-SecureString -String 'password' -Force -AsPlainText) }"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/RezenDeveloper.pfx" "${CMAKE_CURRENT_SOURCE_DIR}/Windows/Cert/RezenDeveloper.pfx"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/RezenDeveloper.pfx" "${INSTALLER_BUNDLE_DIR}/cert/RezenDeveloper.pfx"
        COMMENT "Creating standalone installer bundle and managing certificates in ${INSTALLER_BUNDLE_DIR}..."
        VERBATIM
    )

    # --- DLL Deployment for Rollback Tool ---
    add_custom_command(TARGET MadeChanges POST_BUILD
        COMMAND "${_qt_bin_dir}/windeployqt" --no-translations --compiler-runtime "$<TARGET_FILE:MadeChanges>"
        COMMENT "Deploying DLL dependencies for Rollback Tool into Tools folder..."
        VERBATIM
    )

    # Icons
    qt_add_resources(RJLInstaller "installer_icons"
        PREFIX "/CmakeLauncherIcon"
        BASE "CmakeLauncherIcon/${ICON_VER_DIR}"
        FILES "CmakeLauncherIcon/${ICON_VER_DIR}/release/icon.png"
    )

    # Payload
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Windows/WinInstaller/Payload/app_data.zip")
        qt_add_resources(RJLInstaller "installer_payload"
            PREFIX "/Windows/WinInstaller/Payload"
            BASE "Windows/WinInstaller/Payload"
            FILES "Windows/WinInstaller/Payload/app_data.zip"
        )
        target_compile_definitions(RJLInstaller PRIVATE HAS_PAYLOAD)
    endif()

    # Ensure Ninja/MSYS2 builds the launcher and DLLs BEFORE the installer
    add_dependencies(RJLInstaller WDLauncher MadeChanges)

else()
    target_link_libraries(WDLauncher PRIVATE
        Qt6::Widgets Qt6::Core Qt6::Gui Qt6::WebView Qt6::QuickWidgets Qt6::Network
    )
endif()

# --- Mac Portable ---
if(APPLE)
    set_target_properties(WDLauncher PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.xrejenx.rjml"
        MACOSX_BUNDLE_BUNDLE_NAME "${APP_NAME}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_LONG_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2024 ${APP_AUTHOR}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Mac-Portable"
    )
endif()
