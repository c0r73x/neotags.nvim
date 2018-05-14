# /src
set (neotags_SOURCES neotags.c utility.c linked_list.c)

if (NOT "${HAS_STRLCPY}" EQUAL "1")
    if (NOT "${HAS_LIBBSD}" EQUAL "1")
        set (neotags_EXTRA_SOURCES bsd_funcs.c)
    endif()
endif()

add_executable(neotags ${neotags_SOURCES} ${neotags_EXTRA_SOURCES})

if (MSVC)
    # Again, wild guesses.
    get_filename_component(MSVC_PCRE_FULL_PATH "../" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    find_library(MSVC_FIND_PCRE pcre2-8.dll pcre2-8.a pcre2-8.dll.a pcre2-8.lib HINTS "C:/Program Files/PCRE2/lib" "C:/Program Files/PCRE2/bin" ${MSVC_PCRE_FULL_PATH})
    find_library(MSVC_FIND_LIBZ libz HINTS "D:/libs/lib")
    target_link_libraries(neotags ${MSVC_FIND_PCRE})
else()
    target_link_libraries(neotags -lpcre2-8 -lz)
    if ("${HAS_LIBBSD}" EQUAL "1")
        target_link_libraries(neotags -lbsd)
    endif()
endif()
