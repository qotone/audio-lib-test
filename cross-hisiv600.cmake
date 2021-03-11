#
# CMake Toolchain file for crosscompiling on ARM.
#
# This can be used when running cmake in the following way:
#  cd build-hisiv600/
#  cmake .. -DCMAKE_TOOLCHAIN_FILE=../cross-hisiv600.cmake
#

set(CROSS_PATH /home/luoyang/App/GCC4Embed/hisi-linux/x86-arm/arm-hisiv600-linux)
set(CROSS arm-hisiv600-linux-)

# Target operating system name.
set(CMAKE_SYSTEM_NAME Linux)

# Name of C compiler.
set(CMAKE_C_COMPILER "${CROSS}gcc")
set(CMAKE_CXX_COMPILER "${CROSS}g++")
SET(CMAKE_AR "${CROSS}ar" CACHE FILEPATH "Archiver")
SET(CMAKE_AS "${CROSS}as" CACHE FILEPATH "Archiver")
SET(CMAKE_LD  ${CROSS}ld CACHE FILEPATH "Archiver")
SET(CMAKE_NM "${CROSS}nm" CACHE FILEPATH "Archiver")
SET(CMAKE_STRIP  "${CROSS}strip" CACHE FILEPATH "Archiver")


set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access" CACHE STRING "Set C Compiler Flags" FORCE)
# link flags
set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -mcpu=cortex-a9 -mfloat-abi=softfp -mfpu=neon -mno-unaligned-access -fno-aggressive-loop-optimizations"  CACHE STRING "Set link Flags" FORCE)




# Where to look for the target environment. (More paths can be added here)
set(CMAKE_FIND_ROOT_PATH "${CROSS_PATH}")

# Adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search headers and libraries in the target environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
