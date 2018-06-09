# /src
set (neotags_SOURCES
    archive_util_gzip.c
    getlines.c
    neotags.c
    strip.c
    tok.c
    util.c)

if (LZMA_SUPPORT)
    set (neotags_SOURCES ${neotags_SOURCES} archive_util_xz.c)
endif()
if (NOT HAVE_STRLCPY AND NOT HAVE_LIBBSD) 
    set (neotags_SOURCES ${neotags_SOURCES} bsd_funcs.c)
endif()

add_executable(neotags ${neotags_SOURCES})

if (ISWIN)
    include_directories(neotags "${CMAKE_SOURCE_DIR}/contrib/include")
    target_link_libraries(neotags
        "${CMAKE_SOURCE_DIR}/contrib/libz.lib"
        "${CMAKE_SOURCE_DIR}/contrib/liblzma.lib"
        "${CMAKE_SOURCE_DIR}/contrib/libpthread.lib")
else()
    if (LZMA_SUPPORT)
        include_directories(${LIBLZMA_INCLUDE_DIRS})
        if (BUILD_DIST OR STATIC_LIBS_ONLY)
            add_library(neo_LZMA_STATIC STATIC IMPORTED)
            set_property(TARGET neo_LZMA_STATIC PROPERTY IMPORTED_LOCATION /usr/lib64/liblzma.a)
            target_link_libraries(neotags neo_LZMA_STATIC)
        else()
            target_link_libraries(neotags ${LIBLZMA_LIBRARIES})
        endif()
    endif()

    if(THREADS_HAVE_PTHREAD_ARG)
        target_link_libraries(neotags -pthread)
    endif()

    include_directories(${ZLIB_INCLUDE_DIRS})
    target_link_libraries(neotags ${ZLIB_LIBRARIES})
endif()

if (neotags_LIBBSD_FOUND)
    include_directories(neotags ${neotags_LIBBSD_INCLUDE_DIRS})
    target_link_libraries(neotags ${neotags_LIBBSD_LIBRARIES})
endif()
