cmake_minimum_required (VERSION 3.6.0)
project (neotags C)
include (FindPkgConfig)
include (CheckFunctionExists)
include (CheckSymbolExists)

# Windows is too hard to support properly.
if (MSVC OR MINGW)
    message(FATAL_ERROR "\
Native Microsoft Windows is not supported through cmake. Please use the included \
Visual Studio project solution and libraries. Believe me, this is saving you \
hours of effort.")
endif()

set (REQUIRED_LZMA_VERSION 5.3.1)
option(USE_LIBBSD "Use libbsd on linux/cygwin if available.")
option(BUILD_DIST "\
Include the required alpha (and therefore uncommon) liblzma version as a static
// library, and use bundled BSD extensions instead of libbsd even if it is available.")
option(STATIC_LIBS_ONLY "Build with only static libraries.")

###############################################################################
# Libraries

find_package (ZLIB REQUIRED)
find_package (LibLZMA)
# pkg_check_modules (neotags_PCRE-8 libpcre2-8 REQUIRED)
CHECK_FUNCTION_EXISTS (strlcpy HAVE_STRLCPY)
CHECK_SYMBOL_EXISTS   (err "err.h" HAVE_ERR)

if (LIBLZMA_FOUND)
    if (${LIBLZMA_VERSION_STRING} STRLESS ${REQUIRED_LZMA_VERSION})
        message(WARNING "\
liblzma version ${REQUIRED_LZMA_VERSION} or greater is required for XZ support. \
XZ support will be disabled for this build.")
    else()
        set (LZMA_SUPPORT 1)
    endif()
endif()

if (BUILD_DIST)
    set (USE_LIBBSD "OFF") 
endif()

if (NOT HAVE_STRLCPY AND USE_LIBBSD)
    pkg_check_modules(neotags_LIBBSD libbsd)
    if (neotags_LIBBSD_FOUND)
        set (HAVE_LIBBSD 1)
    endif()
endif()

###############################################################################
# Flags

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set (DEBUG 1)
endif()

if (NOT BUILD_DIST) 
    set (MARCH_SETTING "-march=native")
endif()
set (WARNS "-fdiagnostics-color=always -Wall -Wno-macro-redefined")

set (CMAKE_C_FLAGS_DEBUG         "${MARCH_SETTING} ${WARNS} -Og -g -Wextra -Wpedantic -Wformat")
set (CMAKE_C_FLAGS_MINSIZEREL    "${MARCH_SETTING} ${WARNS} -Os")
set (CMAKE_C_FLAGS_RELWITHDEBINFO"${MARCH_SETTING} ${WARNS} -Ofast -g")
set (CMAKE_C_FLAGS_RELEASE       "${MARCH_SETTING} ${WARNS} -Ofast")

set (CMAKE_EXE_LINKER_FLAGS "-L/usr/local/lib")
set (CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
set (CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-s")
add_definitions(-D_GNU_SOURCE -DHAVE_CONFIG_H)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
configure_file(cmake-config.h.in config.h)

add_subdirectory(src)
