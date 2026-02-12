# the name of the target operating system
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER   $ENV{CC})
set(CMAKE_CXX_COMPILER $ENV{CXX})

# add compile options
add_compile_options("-march=armv7-a")
add_compile_options("-mtune=cortex-a9")
add_compile_options("-mfpu=neon")
add_compile_options("-mfloat-abi=hard")
add_compile_options("-ftree-vectorize")
add_compile_options("-fno-builtin")
add_compile_options("-fno-common")
add_compile_options("-Wformat=1")
add_compile_options("-D_BSP_NA51089_")
add_compile_options("$<$<COMPILE_LANGUAGE:C>:-Wstrict-prototypes>")
add_compile_options("$<$<COMPILE_LANGUAGE:C>:-fno-exceptions>")

# set include path
include_directories($ENV{NVT_HDAL_DIR}/include)
include_directories($ENV{NVT_VOS_DIR}/include)
include_directories(api/common)
include_directories(api/writer)
include_directories(api/reader)

# where is the target environment located
set(CMAKE_FIND_ROOT_PATH  ${CROSS_TOOLCHAIN_PATH})

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# search headers and libraries in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Prevent CMake from testing the toolchain
set(CMAKE_C_COMPILER_FORCED 1)
set(CMAKE_CXX_COMPILER_FORCED 1)
