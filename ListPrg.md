
Project CodeName WDLauncher, RJ Launcher

/project/WDLauncher
Profile_java
Profile
Java
Instance_mc
Account
LauncherPatch
./Profile_java/JavaProfile.cpp - Instances manager
./Profile/Profile.cpp - Manage user account
./Java/. - Java for runing minecrafts
./Instance_mc - Instance_built and manager
./Account/AddAccount.cpp - Manage account login
./LauncherPatch/. - Manage Launcher Version and build.
[FILES]
/project/WDLauncher
├── ./Account
│   ├── AddAccount.cpp
│   └── OFFLINE
│       ├── offline_auth.cpp
│       └── offline_handle.cpp
├── ./Build.sh
├── ./CMakeLists.txt
├── ./ConsoleOutput.cpp
├── ./Core.cpp
├── ./Instance_mc
├── ./Java
├── ./ListPrg.md
├── ./Profile
│   └── Profile.cpp
├── ./Profile_java
│   └── JavaProfile.cpp
├── ./Settings.cpp
└── ./RMBuild.sh
./Core.cpp - Main app
./ConsoleOutput.cpp - Console in Settings/Debug


[Build Batch/Sh]
./Build.sh - Linux "Use to build folder without remove the build folder"
./RMBuild.sh - Linux "Use for remove build folder and re-build entirely"
