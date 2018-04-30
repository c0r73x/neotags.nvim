cmake_minimum_required (VERSION 3.6.0)
project (neotags C)

if (MSVC)
    set (CMAKE_C_FLAGS_RELEASE "/Ox /Oi /Ot /GL /MD /favor:INTEL64 /MP")
    set (CMAKE_BUILD_TYPE Release)
    set (CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_RELEASE})
    set (CMAKE_C_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_RELEASE})
    set (CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELEASE})
    include_directories("C:/Program Files/PCRE2/include")
    include_directories("D:/libs/include")
else()
    include (FindPkgConfig)
    include (CheckFunctionExists)

    pkg_check_modules (neotags REQUIRED libpcre2-8)
    CHECK_FUNCTION_EXISTS (strlcpy HAS_STRLCPY)

    if (NOT "${HAS_STRLCPY}" EQUAL "1")
        pkg_check_modules(neotags_libbsd libbsd)
        if (neotags_libbsd_FOUND EQUAL 1)
            set (TEST_COMPILE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/.tmp-test.c")
            file(WRITE ${TEST_COMPILE_FILE} "int main(void) { return 0; }")
            try_compile(COMPILE_SUCCEEDED "${CMAKE_BINARY_DIR}/compile_tests" ${TEST_COMPILE_FILE} LINK_LIBRARIES "-lbsd")
            file(REMOVE ${TEST_COMPILE_FILE})
            
            if (COMPILE_SUCCEEDED)
                set (HAS_LIBBSD 1)
            endif()
        endif()
    endif()
    
    pkg_check_modules (neotags REQUIRED zlib)

    set (CMAKE_C_FLAGS_DEBUG "-Og -g -Wall -fdiagnostics-color=always")
    set (CMAKE_C_FLAGS_MINSIZEREL "-Os -s -Wall -fdiagnostics-color=always")
    set (CMAKE_C_FLAGS_RELWITHDEBINFO "-Ofast -g -Wall -fdiagnostics-color=always")
    set (CMAKE_C_FLAGS_RELEASE "-Ofast -s -Wall -fdiagnostics-color=always")
endif()

add_definitions(-D_GNU_SOURCE -DHAVE_CONFIG_H)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set (DEBUG 1)
endif()

configure_file(cmake-config.h.in config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})


add_subdirectory(src)
