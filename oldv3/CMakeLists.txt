cmake_minimum_required(VERSION 3.20)
# MADE IN THAILAND
project(RJML VERSION 0.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
# Enable path prefixes for AUTOMOC to prevent collisions between files with the same name (e.g., Settings.cpp)
set(CMAKE_AUTOMOC_PATH_PREFIX ON)
set(CMAKE_POLICY_DEFAULT_CMP0054 NEW)

# Find common Qt components
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui QuickWidgets Network)
if(NOT WIN32) # Conditionally find WebView for non-Windows platforms
    find_package(Qt6 COMPONENTS WebView QUIET)
endif()

set(APP_VERSION ${PROJECT_VERSION})
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

# Consolidate include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Account
    ${CMAKE_CURRENT_SOURCE_DIR}/CmakeLauncherIcon
    ${CMAKE_CURRENT_SOURCE_DIR}/CMakeSplashScreen
    ${CMAKE_CURRENT_SOURCE_DIR}/LTheme
    ${CMAKE_CURRENT_SOURCE_DIR}/Profile_java
    ${CMAKE_CURRENT_SOURCE_DIR}/JVM
    ${CMAKE_CURRENT_SOURCE_DIR}/Profile_java/INSTANCES_MANAGER
    ${CMAKE_CURRENT_SOURCE_DIR}/Windows
    ${CMAKE_CURRENT_SOURCE_DIR}/LauncherUpdater
    ${CMAKE_CURRENT_SOURCE_DIR}/minecraft
    ${CMAKE_CURRENT_SOURCE_DIR}/Instances
    ${CMAKE_CURRENT_SOURCE_DIR}/LauncherPatch
)

# 2. Consolidate Source collection & Overrides
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.c" 
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

# Exclude files that belong to separate executables, internal CMake checks, or are duplicates
# This regex is updated to better handle environments where the root folder might have spaces or parentheses
list(FILTER SRC_FILES EXCLUDE REGEX "Windows/WinInstaller/(installer\\.cpp|MadeChanges\\.cpp)$|/installer\\.cpp$|Profile_java/INSTANCES_MANAGER/Themeloader\\.cpp|Profile_java/INSTANCES_MANAGER/Settings\\.cpp|CMakeSplashScreen/Fetch/splashfetchupdate\\.cpp|/[Bb]uild.*/|/CMakeFiles/|/ninja_build/|/.*_autogen/|modapi_autogen\\.h")

file(GLOB DETECT_PATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/LauncherPatch/PatchHistory/*.txt")

set(LATEST_BUILD_NUM -1)
set(DETECTED_BUILD_STATUS "release")
set(RC_VERSION_STR "1.0.0")
set(RC_BUILD_TYPE_SHORT "u")
set(RC_PRODUCT_NAME "RJ Launcher")
set(RC_COMPANY "Rezen Developer")
set(RC_DESCRIPTION "Minecraft Launcher")

foreach(PATCH_FILE ${DETECT_PATCH_FILES})
    file(READ "${PATCH_FILE}" PATCH_CONTENT)
    if(PATCH_CONTENT MATCHES "Build_number *= *([0-9]+)")
        set(CURRENT_FILE_BUILD ${CMAKE_MATCH_1})
        if(CURRENT_FILE_BUILD GREATER LATEST_BUILD_NUM)
            set(LATEST_BUILD_NUM ${CURRENT_FILE_BUILD})
            if(PATCH_CONTENT MATCHES "Build_type_prefix *= *([^ \n\r\t]+)")
                set(CURRENT_PREFIX ${CMAKE_MATCH_1})
            else()
                set(CURRENT_PREFIX "release")
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

# Simplify platform blocks
# 5. Simplified Platform Blocks

# --- THE TARGETS ---
# Exclude duplicate files and generate main target
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/updater.cpp")
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/LTheme/ThemeMaker/ThemeMakerCore.cpp")

add_executable(WDLauncher ${SRC_FILES})

# --- CONFIGURATION AND RESOURCES (Applied after target creation) ---

# App name change from Codename To Production
set_target_properties(WDLauncher PROPERTIES
    OUTPUT_NAME "${APP_SHORT_NAME}"
    PROJECT_LABEL "${APP_NAME}"
)

# Embed Metadata inside the executable binary via compile definitions
target_compile_definitions(WDLauncher PRIVATE
    LAUNCHER_APP_NAME="${APP_NAME}"
    LAUNCHER_VERSION="${RC_VERSION_STR}"
    LAUNCHER_AUTHOR="${APP_AUTHOR}"
    LAUNCHER_SHORT_NAME="${APP_SHORT_NAME}"
    LAUNCHER_BUILD_TYPE_SHORT="${RC_BUILD_TYPE_SHORT}"
    LAUNCHER_BUILD_NUMBER=${LATEST_BUILD_NUM}
)

# Resources
file(GLOB PATCH_FILES "${CMAKE_CURRENT_SOURCE_DIR}/LauncherPatch/PatchHistory/*.txt")
if(PATCH_FILES)
    qt_add_resources(WDLauncher patches PREFIX "/patches" BASE "LauncherPatch/PatchHistory" FILES ${PATCH_FILES})
endif()

set(COMMON_ICON_FILES
    "CmakeLauncherIcon/${ICON_VER_DIR}/undev/icon.png"
    "CmakeLauncherIcon/${ICON_VER_DIR}/beta/icon.png"
    "CmakeLauncherIcon/${ICON_VER_DIR}/release/icon.png"
)
qt_add_resources(WDLauncher launcher_icons PREFIX "/launcher/icons/CmakeLauncherIcon" BASE "CmakeLauncherIcon/${ICON_VER_DIR}" FILES ${COMMON_ICON_FILES})
qt_add_resources(WDLauncher splash_icons PREFIX "/launcher/splash/CmakeLauncherIcon" BASE "CmakeLauncherIcon/${ICON_VER_DIR}" FILES ${COMMON_ICON_FILES})

qt_add_resources(WDLauncher api_config PREFIX "/config" BASE "CMakeSplashScreen" FILES "CMakeSplashScreen/api.txt")
qt_add_resources(WDLauncher banner_config PREFIX "/config" BASE "Banner" FILES "Banner/banner.txt")

if(WIN32)
    set_target_properties(WDLauncher PROPERTIES WIN32_EXECUTABLE TRUE)

    # Robustly find windeployqt
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION) # Ensure _qmake_executable is defined
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY) # Ensure _qt_bin_dir is defined
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    # Link common Qt components and Windows-specific libraries for WDLauncher
    target_link_libraries(WDLauncher PRIVATE
        Qt6::Widgets Qt6::Core Qt6::Gui Qt6::QuickWidgets Qt6::Network
        ole32 # Required for COM initialization (WebView2)
    )
    # Version generation for .rc
    string(REPLACE "." ";" VER_LIST ${RC_VERSION_STR})
    list(LENGTH VER_LIST VER_LEN)
    if(VER_LEN GREATER_EQUAL 3) 
        list(GET VER_LIST 0 V1) 
        list(GET VER_LIST 1 V2) 
        list(GET VER_LIST 2 V3) 
        set(RC_VERSION_COMMA "${V1},${V2},${V3},0") 
    endif()

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

    if(NOT EXISTS "${APP_ICON_DST}")
        message(STATUS "WARNING: icon.ico not found for status '${BUILD_STATUS}'. Skipping resource compilation.")
    endif()

    # Add resources once
    target_sources(WDLauncher PRIVATE Windows/windows_resources.rc "${CMAKE_CURRENT_BINARY_DIR}/details_generated.rc")

    # Set specific output directory for the portable version (with all DLLs)
    set(PORTABLE_DIR "${CMAKE_BINARY_DIR}/Windows-Portable")
    set_target_properties(WDLauncher PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PORTABLE_DIR}")
    # --- Consolidated Post-Build for WDLauncher ---
    add_custom_command(TARGET WDLauncher POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --compiler-runtime --plugindir "$<TARGET_FILE_DIR:WDLauncher>/plugins" "$<TARGET_FILE:WDLauncher>"
        COMMENT "Finalizing build: Deploying dependencies..."
        VERBATIM
    )

    # --- RJL Installer Configuration ---
    # --- RJL Installer Configuration ---
    add_executable(RJLInstaller WIN32
        Windows/WinInstaller/installer.cpp
        LauncherUpdater/LauncherFetch/fetch.cpp
        LauncherUpdater/LauncherFetch/fetchurl.cpp
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
        LauncherPatch/version.cpp
    )
    
    # --- ThemeMaker Configuration ---
    add_executable(ThemeMaker WIN32 LTheme/ThemeMaker/ThemeMakerCore.cpp ConsoleOutput.cpp LauncherPatch/version.cpp)
    set_target_properties(ThemeMaker PROPERTIES
        OUTPUT_NAME "ThemeMaker"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Windows-Portable/Tools"
    )
    target_link_libraries(ThemeMaker PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network)
    add_dependencies(WDLauncher ThemeMaker)

    # Embed Metadata for ThemeMaker to resolve compile errors in version.cpp
    target_compile_definitions(ThemeMaker PRIVATE
        LAUNCHER_APP_NAME="${APP_NAME}"
        LAUNCHER_VERSION="${RC_VERSION_STR}"
        LAUNCHER_AUTHOR="${APP_AUTHOR}"
        LAUNCHER_SHORT_NAME="${APP_SHORT_NAME}"
        LAUNCHER_BUILD_TYPE_SHORT="${RC_BUILD_TYPE_SHORT}"
        LAUNCHER_BUILD_NUMBER=${LATEST_BUILD_NUM}
    )

    add_custom_command(TARGET ThemeMaker POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --compiler-runtime --plugindir "$<TARGET_FILE_DIR:ThemeMaker>/plugins" "$<TARGET_FILE:ThemeMaker>"
        COMMENT "Deploying ThemeMaker dependencies..."
        VERBATIM
    )

    # We put MadeChanges in a Tools subfolder of the portable directory
    set_target_properties(MadeChanges PROPERTIES
        OUTPUT_NAME "MadeChanges"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Windows-Portable/Tools"
    )
    target_link_libraries(MadeChanges PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network)
    add_dependencies(WDLauncher MadeChanges)

    # Compile definitions for MadeChanges
    target_compile_definitions(MadeChanges PRIVATE
        LAUNCHER_APP_NAME="${APP_NAME}"
        LAUNCHER_VERSION="${RC_VERSION_STR}"
        LAUNCHER_BUILD_TYPE_SHORT="${RC_BUILD_TYPE_SHORT}"
    LAUNCHER_BUILD_NUMBER=${LATEST_BUILD_NUM}
    )

    # PatchHistory for MadeChanges
    if(PATCH_FILES)
        qt_add_resources(MadeChanges "patches" PREFIX "/patches" BASE "${CMAKE_CURRENT_SOURCE_DIR}/LauncherPatch/PatchHistory" FILES ${PATCH_FILES})
    endif()

    # Output the installer to the root build folder for easy access
    set_target_properties(RJLInstaller PROPERTIES 
        OUTPUT_NAME "RJLInstaller"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

    # Linking and Resources
    target_link_libraries(RJLInstaller PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network)
    
    # PatchHistory for RJLInstaller
    if(PATCH_FILES)
        qt_add_resources(RJLInstaller "patches" PREFIX "/patches" BASE "${CMAKE_CURRENT_SOURCE_DIR}/LauncherPatch/PatchHistory" FILES ${PATCH_FILES})
    endif()

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

    # Compile definitions for RJLInstaller
    target_compile_definitions(RJLInstaller PRIVATE
        LAUNCHER_APP_NAME="${APP_NAME}"
        LAUNCHER_VERSION="${RC_VERSION_STR}"
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
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --compiler-runtime --plugindir "$<TARGET_FILE_DIR:MadeChanges>/plugins" "$<TARGET_FILE:MadeChanges>"
        COMMENT "Deploying MadeChanges dependencies..."
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

elseif(APPLE)
    set_target_properties(WDLauncher PROPERTIES MACOSX_BUNDLE TRUE RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Mac-Portable")
else() # Generic Linux (Portable Configuration)
    # 1. Set specific output directory for the portable version
    set(LINUX_PORTABLE_DIR "${CMAKE_BINARY_DIR}/Linux-Portable")
    
    set_target_properties(WDLauncher PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY "${LINUX_PORTABLE_DIR}"
        # 2. Set RPATH to prioritize local 'lib' folder. 
        # Using '$ORIGIN/lib' tells the OS to look for .so files in the folder 'lib' next to the exe.
        INSTALL_RPATH "$ORIGIN/lib:$ORIGIN"
        BUILD_WITH_INSTALL_RPATH TRUE
        SKIP_BUILD_RPATH FALSE
    )

    # 3. Define common link options for Linux portability
    set(LINUX_PORTABLE_LINK_OPTIONS -static-libstdc++ -static-libgcc)
    target_link_options(WDLauncher PRIVATE ${LINUX_PORTABLE_LINK_OPTIONS})

    target_link_libraries(WDLauncher PRIVATE
        Qt6::Widgets Qt6::Core Qt6::Gui Qt6::QuickWidgets Qt6::Network
    )
    if(Qt6WebView_FOUND) 
        target_link_libraries(WDLauncher PRIVATE Qt6::WebView) 
    endif()
    # 4. Linux Deployment logic (Bundling and AppImage)
    find_program(LINUXDEPLOY_EXECUTABLE NAMES linuxdeploy)
    find_program(LINUXDEPLOY_QT_PLUGIN NAMES linuxdeploy-plugin-qt)

    if(LINUX_PORTABLE_DIR AND LINUXDEPLOY_EXECUTABLE)
        # Get Qt bin directory to help the plugin find Qt tools
        get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

        set(LINUX_DEPLOY_EXTRA_ARGS "")
        if(LINUXDEPLOY_QT_PLUGIN)
            list(APPEND LINUX_DEPLOY_EXTRA_ARGS "--plugin" "qt")
        else()
            message(WARNING "linuxdeploy-plugin-qt not found. Qt dependencies (platforms, images formats) will not be bundled. Please install linuxdeploy-plugin-qt.")
        endif()

        add_custom_command(TARGET WDLauncher POST_BUILD
            # Create standard ELF portable bundle
            COMMAND ${CMAKE_COMMAND} -E env "PATH='${_qt_bin_dir}:$ENV{PATH}'" NO_STRIP=1 
                    "${LINUXDEPLOY_EXECUTABLE}" --appdir "${LINUX_PORTABLE_DIR}" 
                    --executable "$<TARGET_FILE:WDLauncher>" 
                    --icon-file "${CMAKE_CURRENT_SOURCE_DIR}/CmakeLauncherIcon/${ICON_VER_DIR}/512/${BUILD_STATUS}/icon.png"
                    --icon-filename "${APP_SHORT_NAME}"
                    --create-desktop-file
                    ${LINUX_DEPLOY_EXTRA_ARGS}
            COMMAND ${CMAKE_COMMAND} -E echo "Syncing filesystem to prevent SquashFS corruption..." && sync
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMENT "Creating Linux-Portable ELF bundle..."
        )

        # Build ThemeMaker for Linux
        add_executable(ThemeMaker LTheme/ThemeMaker/ThemeMakerCore.cpp ConsoleOutput.cpp LauncherPatch/version.cpp)
        set_target_properties(ThemeMaker PROPERTIES 
            OUTPUT_NAME "ThemeMaker"
            RUNTIME_OUTPUT_DIRECTORY "${LINUX_PORTABLE_DIR}/Tools"
            INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
        target_link_libraries(ThemeMaker PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network)
        target_compile_options(ThemeMaker PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/modapi.h")
        target_link_options(ThemeMaker PRIVATE ${LINUX_PORTABLE_LINK_OPTIONS})
        target_compile_definitions(ThemeMaker PRIVATE
            LAUNCHER_APP_NAME="${APP_NAME}"
            LAUNCHER_VERSION="${RC_VERSION_STR}"
            LAUNCHER_BUILD_TYPE_SHORT="${RC_BUILD_TYPE_SHORT}"
            LAUNCHER_BUILD_NUMBER=${LATEST_BUILD_NUM}
        )

        # Create standalone AppImage
        add_custom_command(TARGET WDLauncher POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E env "PATH='${_qt_bin_dir}:$ENV{PATH}'" VERSION=${APP_VERSION} ARCH=x86_64 NO_STRIP=1 
                    "${LINUXDEPLOY_EXECUTABLE}" --appdir "${LINUX_PORTABLE_DIR}" 
                    --executable "$<TARGET_FILE:WDLauncher>" 
                    --icon-file "${CMAKE_CURRENT_SOURCE_DIR}/CmakeLauncherIcon/${ICON_VER_DIR}/512/${BUILD_STATUS}/icon.png" 
                    --icon-filename "${APP_SHORT_NAME}"
                    --create-desktop-file 
                    ${LINUX_DEPLOY_EXTRA_ARGS} --output appimage
            COMMAND ${CMAKE_COMMAND} -E echo "Finalizing AppImage and syncing..." && sync
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            COMMENT "Creating standalone Linux AppImage..."
        )
    else()
        message(STATUS "linuxdeploy not found. Portable Linux builds and AppImage generation will be skipped.")
    endif()
endif()


# --- Mac Portable ---
if(APPLE)
    set_target_properties(WDLauncher PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.xrejenx.rjml"
        MACOSX_BUNDLE_BUNDLE_NAME "${APP_NAME}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_LONG_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2026 ${APP_AUTHOR}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Mac-Portable"
    )
endif()

# --- Linux Specifics (UNIX includes Linux, macOS, etc. but we handle macOS separately) ---
if(UNIX AND NOT APPLE)
    # Add any Linux-specific configurations here if needed
endif()
