cmake_minimum_required(VERSION 3.20)
# MADE IN THAILAND
project(RJML LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui WebEngineWidgets Network)

# App Meta Details
set(APP_NAME "RJMinecraftLauncher")
set(APP_SHORT_NAME "RJML")
set(APP_AUTHOR "xrejenx")

include_directories(
    .
    Account
    Account/OFFLINE
    LTheme
    Profile
    Profile_java
    Profile_java/INSTANCES_MANAGER
    Windows
)

# PatchHistory
file(GLOB PATCH_FILES "LauncherPatch/PatchHistory/*.txt")
if(PATCH_FILES)
    qt_add_resources(WDLauncher "patches"
        PREFIX "/patches"
        FILES ${PATCH_FILES}
    )
endif()

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

# App name change from Codename To Production
set_target_properties(WDLauncher PROPERTIES
    OUTPUT_NAME "${APP_SHORT_NAME}"
    PROJECT_LABEL "${APP_NAME}"
)

target_link_libraries(WDLauncher PRIVATE
    Qt6::Widgets Qt6::Core Qt6::Gui Qt6::WebEngineWidgets Qt6::Network
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
