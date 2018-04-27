# Why am I bothering.

cmake_minimum_required (VERSION 3.6.0)
project (neotags C)

if (MSVC)
    set (CMAKE_C_FLAGS_RELEASE "/Ox /favor:INTEL64 /MP12")
    set (CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_RELEASE})
    set (CMAKE_C_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_RELEASE})
    set (CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELEASE})
else()
    include (FindPkgConfig)
    include (CheckFunctionExists)

    pkg_check_modules (neotags REQUIRED libpcre2-8)
    CHECK_FUNCTION_EXISTS (strlcpy HAS_STRLCPY)

    if (NOT DEFINED(HAS_STRLCPY))
        pkg_check_modules(neotags_libbsd libbsd)
        if (neotags_libbsd_FOUND EQUAL 1)
            set (HAS_LIBBSD 1)
        endif()
    endif()

    set (CMAKE_C_FLAGS_DEBUG "-Og -g -Wall -fdiagnostics-color=always")
    set (CMAKE_C_FLAGS_MINSIZEREL "-Os -s -Wall -fdiagnostics-color=always")
    set (CMAKE_C_FLAGS_RELWITHDEBINFO "-Ofast -g -Wall -fdiagnostics-color=always")
    set (CMAKE_C_FLAGS_RELEASE "-Ofast -s -Wall -fdiagnostics-color=always")
endif()

add_definitions(-D_GNU_SOURCE -DHAVE_CONFIG_H)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set (DEBUG 1)
    message ("whore")
endif()

configure_file(cmake-config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


add_subdirectory(src)
