#!/bin/bash

# WDLauncher Build Menu
while true; do
    echo " "
    echo "WDLauncher Build Menu"
    echo " "
    echo "Type 'Build'   or 'B'  to Build and Run (default)"
    echo "Type 'RBuild'  or 'b'  to Rebuild and Run"
    echo "Type 'Run'     or 'R'  to Run existing build"
    echo "Type 'All'     or 'a'  to Build ALL platforms"
    echo "Type 'Deps'    or 'u'  to Install dependencies"
    echo "Type 'Linux'   or 'L'  to Build for Linux"
    echo "Type 'Windows' or 'W'  to Build for Windows (Vista+)"
    echo "Type 'Apple'   or 'A'  to Build for macOS"
    echo "Type 'Exit'    or 'x'  to Quit"
    echo " "
    read -p "Select an option: " choice

    case $choice in
        Build|B)
            clear
            echo "[*] Configuring with CMake..."
            cmake -S . -B BuildLinux
            cd BuildLinux || exit 1
            make
            if [ $? -eq 0 ]; then
                echo "[*] Build successful. Running WDLauncher..."
                cd RJML-Linux && ./WDLauncher
                cd ..
            else
                echo "[!] Build failed."
                cd ..
            fi
            ;;
        RBuild|b)
            clear
            echo "Removing Old Build folders..."
            rm -rf BuildLinux BuildWindows BuildMac BuildWin32 BuildWin64 BuildMacIntel BuildMacArm
            echo "[*] Configuring with CMake..."
            cmake -S . -B BuildLinux
            cd BuildLinux || exit 1
            make
            if [ $? -eq 0 ]; then
                cd RJML-Linux && ./WDLauncher
                cd ..
            else
                cd ..
            fi
            ;;
        Run|R)
            clear
            if [ -f "BuildLinux/RJML-Linux/WDLauncher" ]; then
                echo "[*] Running existing WDLauncher (Linux)..."
                cd BuildLinux/RJML-Linux && ./WDLauncher
                cd ../..
            elif [ -f "BuildWindows/RJML-Windows/WDLauncher.exe" ]; then
                echo "[*] Running existing WDLauncher (Windows)..."
                wine BuildWindows/RJML-Windows/WDLauncher.exe
            elif [ -f "BuildMac/RJML-Linux/WDLauncher" ]; then
                echo "[*] Running existing WDLauncher (macOS)..."
                cd BuildMac/RJML-Linux && ./WDLauncher
                cd ../..
            else
                echo "[!] No build found. Please run 'Build' first."
            fi
            ;;
        All|a)
            clear
            echo "[*] Building ALL platforms (Linux, Windows, macOS)..."

            # Linux build
            echo "[*] Building for Linux..."
            cmake -S . -B BuildLinux
            cd BuildLinux || exit 1
            make -j$(nproc)
            cd ..

            # Windows build (both 32-bit and 64-bit)
            echo "[*] Building Windows 32-bit..."
            cmake -S . -B BuildWin32 \
              -DCMAKE_SYSTEM_NAME=Windows \
              -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc \
              -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++
            cd BuildWin32 || exit 1
            make -j$(nproc)
            cd ..

            echo "[*] Building Windows 64-bit..."
            cmake -S . -B BuildWin64 \
              -DCMAKE_SYSTEM_NAME=Windows \
              -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
              -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
            cd BuildWin64 || exit 1
            make -j$(nproc)
            cd ..

            # macOS build (Intel + ARM)
            echo "[*] Building macOS Intel..."
            cmake -S . -B BuildMacIntel -DCMAKE_OSX_ARCHITECTURES=x86_64
            cd BuildMacIntel || exit 1
            make -j$(sysctl -n hw.ncpu)
            cd ..

            echo "[*] Building macOS ARM (Apple Silicon)..."
            cmake -S . -B BuildMacArm -DCMAKE_OSX_ARCHITECTURES=arm64
            cd BuildMacArm || exit 1
            make -j$(sysctl -n hw.ncpu)
            cd ..

            echo "[*] All builds completed."
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
        Linux|L)
            clear
            echo "[*] Building for Linux..."
            cmake -S . -B BuildLinux
            cd BuildLinux || exit 1
            make
            ;;
        Windows|W)
            clear
            echo "[*] Windows Build Menu"
            echo "Select architectures to build (separate with spaces):"
            echo "  1 - Win32 (32-bit)"
            echo "  2 - Win64 (64-bit)"
            read -p "[Enter choices]: " win_choices

            # IMPORTANT: You must point this to a cross-compiled Qt6 for Windows!
            # Example: /usr/lib/mxe/usr/x86_64-w64-mingw32.shared/qt6
            QT_WIN_PREFIX="/usr/i686-w64-mingw32/qt6" 

            for choice in $win_choices; do
                case $choice in
                    1)
                        echo "[*] Building Windows 32-bit..."
                        cmake -S . -B BuildWin32 \
                          -DCMAKE_SYSTEM_NAME=Windows \
                          -DCMAKE_PREFIX_PATH="$QT_WIN_PREFIX" \
                          -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc \
                          -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++
                        cd BuildWin32 || exit 1
                        make -j$(nproc)
                        cd ..
                        ;;
                    2)
                        # Update path for 64-bit if different
                        QT_WIN64_PREFIX="/usr/x86_64-w64-mingw32/qt6"
                        
                        echo "[*] Building Windows 64-bit..."
                        cmake -S . -B BuildWin64 \
                          -DCMAKE_SYSTEM_NAME=Windows \
                          -DCMAKE_PREFIX_PATH="$QT_WIN64_PREFIX" \
                          -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
                          -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
                        cd BuildWin64 || exit 1
                        make -j$(nproc)
                        cd ..
                        ;;
                    *)
                        echo "[!] Invalid Windows choice: $choice"
                        ;;
                esac
            done
            ;;
        Apple|A)
            clear
            echo "[*] macOS Build Menu"
            echo "Select architectures to build (separate with spaces):"
            echo "  1 - MacIntel (x86_64)"
            echo "  2 - Mac M1/M2 (arm64)"
            read -p "[Enter choices]: " mac_choices

            for choice in $mac_choices; do
                case $choice in
                    1)
                        echo "[*] Building macOS Intel (x86_64)..."
                        cmake -S . -B BuildMacIntel -DCMAKE_OSX_ARCHITECTURES=x86_64
                        cd BuildMacIntel || exit 1
                        make -j$(sysctl -n hw.ncpu)
                        cd ..
                        ;;
                    2)
                        echo "[*] Building macOS Apple Silicon (arm64)..."
                        cmake -S . -B BuildMacArm -DCMAKE_OSX_ARCHITECTURES=arm64
                        cd BuildMacArm || exit 1
                        make -j$(sysctl -n hw.ncpu)
                        cd ..
                        ;;
                    *)
                        echo "[!] Invalid macOS choice: $choice"
                        ;;
                esac
            done
            ;;
        Exit|x)
            echo "Exiting..."
            exit 0
            ;;
        *)
            echo "Invalid command."
            sleep 1
            ;;
    esac
done
