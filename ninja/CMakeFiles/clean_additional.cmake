# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\MadeChanges_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\MadeChanges_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\RJLInstaller_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\RJLInstaller_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\WDLauncher_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\WDLauncher_autogen.dir\\ParseCache.txt"
  "MadeChanges_autogen"
  "RJLInstaller_autogen"
  "WDLauncher_autogen"
  )
endif()
