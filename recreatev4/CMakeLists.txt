cmake_minimum_required(VERSION 3.16)

project(RJL VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prefer Qt6 but allow Qt5 fallback
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

# -------------------------
# Main application sources
# -------------------------
set(MAIN_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    changeuser.cpp
    changeuser.h
    changeuser.ui
    version.cpp
    version.h
    updater.cpp
    updater.h
)

# Create main target (Qt6 vs Qt5)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(RJL
        MANUAL_FINALIZATION
        ${MAIN_SOURCES}
    )
else()
    if(ANDROID)
        add_library(RJL SHARED ${MAIN_SOURCES})
    else()
        add_executable(RJL ${MAIN_SOURCES})
    endif()
endif()

target_link_libraries(RJL PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

# Keep main app as GUI subsystem on Windows
set_target_properties(RJL PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# -------------------------
# MadeChanges tool target
# -------------------------
# Source file names (allow for common naming variants)
set(MADECHANGES_UI MadeChanges.ui)
set(MADECHANGES_HDR MadeChanges.h)
set(MADECHANGES_SRC MadeChanges.cpp)
set(MADECHANGES_MAIN1 MadeChanges_main.cpp)
set(MADECHANGES_MAIN2 MadeChange_main.cpp)  # fallback if file was named with typo

# Resolve which main file exists and fail early if none found
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${MADECHANGES_MAIN1}")
    set(MADECHANGES_MAIN ${MADECHANGES_MAIN1})
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${MADECHANGES_MAIN2}")
    set(MADECHANGES_MAIN ${MADECHANGES_MAIN2})
else()
    message(FATAL_ERROR "MadeChanges main file not found. Add ${MADECHANGES_MAIN1} or ${MADECHANGES_MAIN2} to the source tree.")
endif()

set(MADECHANGES_SOURCES
    ${MADECHANGES_SRC}
    ${MADECHANGES_HDR}
    ${MADECHANGES_MAIN}
    ${MADECHANGES_UI}
)

# Optionally include updater_assets.cpp if present (non-fatal)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/updater_assets.cpp")
    list(APPEND MADECHANGES_SOURCES updater_assets.cpp)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(MadeChanges
        MANUAL_FINALIZATION
        ${MADECHANGES_SOURCES}
    )
else()
    add_executable(MadeChanges ${MADECHANGES_SOURCES})
endif()

# Ensure headers/ui are found
target_include_directories(MadeChanges PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(MadeChanges PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

# Place the tool into build/.../Tools and force console subsystem (use main)
set_target_properties(MadeChanges PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Tools"
    WIN32_EXECUTABLE OFF
)

# Defensive: ensure linker looks for main() on Windows toolchains
if (MINGW)
    # prefer console subsystem for MinGW
    set_property(TARGET MadeChanges APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-subsystem,console")
elseif (MSVC)
    set_property(TARGET MadeChanges APPEND_STRING PROPERTY LINK_FLAGS " /SUBSYSTEM:CONSOLE")
endif()

# Build MadeChanges together with main app
add_dependencies(RJL MadeChanges)

# -------------------------
# Install rules
# -------------------------
include(GNUInstallDirs)
install(TARGETS RJL
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS MadeChanges RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/Tools)

# Finalize Qt6 executables
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(RJL)
    qt_finalize_executable(MadeChanges)
endif()
