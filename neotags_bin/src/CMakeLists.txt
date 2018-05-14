# /src
set (neotags_SOURCES
  archive_util_gzip.c
  getlines.c
  linked_list.c
  neotags.c
  strip.c
  util.c)

if (LZMA_SUPPORT)
    set (neotags_SOURCES ${neotags_SOURCES} archive_util_xz.c)
endif()
if (NOT HAVE_STRLCPY AND NOT HAVE_LIBBSD) 
    set (neotags_SOURCES ${neotags_SOURCES} bsd_funcs.c)
endif()

add_executable(neotags ${neotags_SOURCES})

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

if (STATIC_LIBS_ONLY)
  # add_library(neo_PCRE2_STATIC STATIC IMPORTED)
  add_library(neo_ZLIB_STATIC STATIC IMPORTED)
  # set_property(TARGET neo_PCRE2_STATIC PROPERTY IMPORTED_LOCATION /usr/lib64/libpcre2-8.a)
  set_property(TARGET neo_ZLIB_STATIC PROPERTY IMPORTED_LOCATION /usr/lib64/libz.a)
  # target_link_libraries(neotags neo_PCRE2_STATIC neo_ZLIB_STATIC)
else()
  # include_directories(${neotags_PCRE-8_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
  # target_link_libraries(neotags ${neotags_PCRE-8_LIBRARIES} ${ZLIB_LIBRARIES})
  include_directories(${ZLIB_INCLUDE_DIRS})
  target_link_libraries(neotags ${ZLIB_LIBRARIES})
endif()

if (neotags_LIBBSD_FOUND)
  include_directories(neotags ${neotags_LIBBSD_INCLUDE_DIRS})
  target_link_libraries(neotags ${neotags_LIBBSD_LIBRARIES})
endif()

