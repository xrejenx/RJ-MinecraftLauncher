cmake_minimum_required(VERSION 3.20)
# MADE IN THAILAND
project(RJML VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui WebView QuickWidgets Network)
set(APP_VERSION ${PROJECT_VERSION})

# App Meta Details
set(APP_NAME "RJMinecraftLauncher")
set(APP_SHORT_NAME "RJML")
set(APP_AUTHOR "xrejenx")

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
)

# Made App
add_executable(WDLauncher
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
    CMakeSplashScreen/Fetch/splashfetchupdate.cpp
    CMakeSplashScreen/Fetch/fetchhelper.cpp
    CMakeSplashScreen/Downloader/splashdownloadmanager.cpp
    LTheme/theme.cpp
    LTheme/Themeloader.cpp
    Profile/InstanceSettings.cpp
    Profile/Java_download.cpp
    Profile/Java_settings.cpp
    Profile/Profile.cpp
    Profile_java/JavaProfile.cpp
    Profile_java/lgwl-handlelib.cpp
    Profile_java/lgwl-lib.cpp
    Profile_java/INSTANCES_MANAGER/insta_handle.cpp
    Profile_java/INSTANCES_MANAGER/instance-javaRequirement.cpp
    Profile_java/INSTANCES_MANAGER/launchargs_creator.cpp
    Profile_java/INSTANCES_MANAGER/vanilla.cpp
    Profile_java/INSTANCES_MANAGER/vanilla_installer.cpp
    Profile_java/INSTANCES_MANAGER/vanilla_mc_downloader.cpp
    Profile_java/INSTANCES_MANAGER/vanilla_mc_handle.cpp
    Windows/windows_resources.rc
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
    PREFIX "/launcher/icons"
    FILES
        "CmakeLauncherIcon/undev/icon.png"
        "CmakeLauncherIcon/beta/icon.png"
        "CmakeLauncherIcon/release/icon.png"
)

# Splash Icons based on Build Legit Prefixes
qt_add_resources(WDLauncher "splash_icons"
    PREFIX "/launcher/splash"
    FILES
        "CmakeLauncherIcon/undev/icon.png"
        "CmakeLauncherIcon/beta/icon.png"
        "CmakeLauncherIcon/release/icon.png"
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

# Embed Metadata inside the executable binary via compile definitions
target_compile_definitions(WDLauncher PRIVATE
    LAUNCHER_APP_NAME="${APP_NAME}"
    LAUNCHER_VERSION="${APP_VERSION}"
    LAUNCHER_AUTHOR="${APP_AUTHOR}"
    LAUNCHER_SHORT_NAME="${APP_SHORT_NAME}"
)

target_link_libraries(WDLauncher PRIVATE
    Qt6::Widgets Qt6::Core Qt6::Gui Qt6::WebView Qt6::QuickWidgets Qt6::Network
)

# --- Windows Portable ---
if(WIN32)
    set_target_properties(WDLauncher PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Windows-Portable")

    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

    add_custom_command(TARGET WDLauncher POST_BUILD
        COMMAND "${_qt_bin_dir}/windeployqt" --no-translations --compiler-runtime $<TARGET_FILE:WDLauncher>
        COMMENT "Deploying for ${APP_NAME} by ${APP_AUTHOR}..."
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
