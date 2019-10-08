cmake_minimum_required (VERSION 3.6.0)

set(PROJECT_NAME neotags)
project(${PROJECT_NAME})

include (FindPkgConfig)
include (CheckFunctionExists)
include (CheckSymbolExists)

if (MSVC OR MINGW)
    set(ISWIN 1)
endif()

set (REQUIRED_LZMA_VERSION 5.3.1)
option(USE_LIBBSD "Use libbsd on linux/cygwin if available.")
option(BUILD_DIST "\
Include the required alpha (and therefore uncommon) liblzma version as a static\
library, and use bundled BSD extensions instead of libbsd even if it is available.")
option(STATIC_LIBS_ONLY "Build with only static libraries.")

###############################################################################
# Libraries

set(THREADS_PREFER_PTHREAD_FLAG ON)

if (ISWIN)
    set (LZMA_SUPPORT 1)
    set (USE_PTHREADS 1)
else()
    find_package (ZLIB REQUIRED)
    find_package (Threads)
    find_package (LibLZMA)
endif()

set (CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE -DHAVE_CONFIG_H)

CHECK_SYMBOL_EXISTS (reallocarray "stdlib.h" HAVE_REALLOCARRAY)
CHECK_SYMBOL_EXISTS (asprintf     "stdio.h"  HAVE_ASPRINTF)
CHECK_SYMBOL_EXISTS (strlcpy      "string.h" HAVE_STRLCPY)
CHECK_SYMBOL_EXISTS (strdupa      "string.h" HAVE_STRDUPA)
CHECK_SYMBOL_EXISTS (strsep       "string.h" HAVE_STRSEP)
CHECK_SYMBOL_EXISTS (err          "err.h"    HAVE_ERR)

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

if (THREADS_HAVE_PTHREAD_ARG)
    set (USE_PTHREADS 1)
endif()

###############################################################################
# Flags

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set (DEBUG 1)
endif()

if (MSVC)
    set (CMAKE_C_FLAGS_RELEASE "/MP /GS /TC /GL /W3 /Gm- /Ox /Ob2 /Zc:inline /fp:fast /Gd /Oi /MD /FC /Ot /arch:AVX2")
    
    set (CMAKE_BUILD_TYPE Release)
    set (CMAKE_C_FLAGS_DEBUG "/GS /TC /Wall /ZI /Gm- /Od /RTC1 /Gd /Oi /MD /FC")
    set (CMAKE_C_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_RELEASE})
    set (CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELEASE})
else()
    if (NOT BUILD_DIST) 
        set (MARCH_SETTING "-march=native")
    endif()

    set (WARNS "-fdiagnostics-color=always -Wall")

    set (CMAKE_C_FLAGS_DEBUG          "${MARCH_SETTING} ${WARNS} -Og -g -Wextra -Wpedantic -Wformat")
    set (CMAKE_C_FLAGS_MINSIZEREL     "${MARCH_SETTING} ${WARNS} -Os")
    set (CMAKE_C_FLAGS_RELWITHDEBINFO "${MARCH_SETTING} ${WARNS} -Ofast -g")
    set (CMAKE_C_FLAGS_RELEASE        "${MARCH_SETTING} ${WARNS} -Ofast")

    set (CMAKE_EXE_LINKER_FLAGS "-L/usr/local/lib")
    if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set (CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")
        set (CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "-s")
    endif()
endif()

add_definitions(-D_GNU_SOURCE -DHAVE_CONFIG_H)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
configure_file(cmake-config.h.in config.h)

add_subdirectory(src)

