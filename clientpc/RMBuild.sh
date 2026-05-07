#!/bin/bash

# WDLauncher Build Menu
while true; do
    echo " "
    echo "WDLauncher Build Menu"
    echo " "
    echo "Type 'Build'  or 'B'  to Build to exist"
    echo "Type 'RBuild' or 'b'  to Rebuild"
    echo "Type 'Win64'  or '64' to Cross-Build for Windows (x64)"
    echo "Type 'Win32'  or '32' to Cross-Build for Windows (x86)"
    echo "Type 'Deps'   or 'u'  to Install dependencies"
    echo "Type 'Exit'   or 'x'  to Quit"
    echo " "
    read -p "Select an option: " choice

    case $choice in
        Build|B)
            clear
            echo "[*] Configuring with Cmake and make..."
            cmake -B Build

            echo "[*] Building project..."
            cd Build || exit 1
            make

            if [ $? -eq 0 ]; then
                echo "[*] Build successful. Running WDLauncher..."
                if [ -f "RJML-Linux/WDLauncher" ]; then
                    ./RJML-Linux/WDLauncher
                elif [ -f "RJML-Windows/WDLauncher.exe" ]; then
                    ./RJML-Windows/WDLauncher.exe
                else
                    ./WDLauncher
                fi
                echo
                echo "[*] Program exited. Returning to menu..."
                cd ..
            else
                echo "[!] Build failed. Returning to menu..."
                cd ..
            fi
            ;;
        RBuild|b)
            clear
            echo "Removing Old Build folder..."
            rm -rf Build

            echo "[*] Configuring with CMake..."
            cmake -B Build

            echo "[*] Building project..."
            cd Build || exit 1
            make

            if [ $? -eq 0 ]; then
                echo "[*] Rebuild successful. Running WDLauncher..."
                if [ -f "RJML-Linux/WDLauncher" ]; then
                    ./RJML-Linux/WDLauncher
                elif [ -f "RJML-Windows/WDLauncher.exe" ]; then
                    ./RJML-Windows/WDLauncher.exe
                else
                    ./WDLauncher
                fi
                echo
                echo "[*] Program exited. Returning to menu..."
                cd ..
            else
                echo "[!] Rebuild failed. Returning to menu..."
                cd ..
            fi
            ;;
        Win64|64)
            clear
            echo "[*] Cross-configuring for Windows x64..."
            # Update this path to your cross-compiled Qt6
            QT_WIN_PREFIX="/usr/x86_64-w64-mingw32/qt6"
            cmake -S . -B BuildWin64 -DCMAKE_SYSTEM_NAME=Windows \
                  -DCMAKE_PREFIX_PATH="$QT_WIN_PREFIX" \
                  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
                  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
            echo "[*] Building..."
            cd BuildWin64 && make
            if [ $? -eq 0 ]; then
                echo "[*] Build successful: BuildWin64/RJML-Windows/WDLauncher.exe"
            fi
            cd ..
            ;;
        Win32|32)
            clear
            echo "[*] Cross-configuring for Windows x86..."
            # Update this path to your cross-compiled Qt6
            QT_WIN_PREFIX="/usr/i686-w64-mingw32/qt6"
            cmake -S . -B BuildWin32 -DCMAKE_SYSTEM_NAME=Windows \
                  -DCMAKE_PREFIX_PATH="$QT_WIN_PREFIX" \
                  -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc \
                  -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++
            echo "[*] Building..."
            cd BuildWin32 && make
            if [ $? -eq 0 ]; then
                echo "[*] Build successful: BuildWin32/RJML-Windows/WDLauncher.exe"
            fi
            cd ..
            ;;
        Deps|u)
            clear
            echo "[*] Installing dependencies..."
            if command -v apt >/dev/null 2>&1; then
                sudo apt update
                sudo apt install build-essential cmake qt6-base-dev qt6-webengine-dev g++-mingw-w64-i686 g++-mingw-w64-x86-64
            elif command -v dnf >/dev/null 2>&1; then
                sudo dnf install cmake gcc-c++ qt6-qtbase-devel qt6-qtwebengine-devel mingw32-gcc-c++ mingw64-gcc-c++
            elif command -v brew >/dev/null 2>&1; then
                brew install cmake qt
            else
                echo "[!] Package manager not detected. Please install dependencies manually."
            fi
            ;;
        Exit|x)
            echo "Exiting..."
            exit 0
            ;;
        *)
            echo "Invalid command. Type 'Build'/'B', 'RBuild'/'b', or 'Exit'/'x'."
            sleep 1
            ;;
    esac
done
