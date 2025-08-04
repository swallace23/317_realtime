# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/realtime_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/realtime_autogen.dir/ParseCache.txt"
  "realtime_autogen"
  )
endif()
