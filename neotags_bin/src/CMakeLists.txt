# /src
set (neotags_SOURCES neotags.c utility.c linked_list.c)

if (NOT "${HAS_STRLCPY}" EQUAL "1")
    if (NOT "${HAS_LIBBSD}" EQUAL "1")
        set (neotags_EXTRA_SOURCES bsd_funcs.c)
    endif()
endif()

add_executable(neotags ${neotags_SOURCES} ${neotags_EXTRA_SOURCES})

if (MSVC)
    get_filename_component(MSVC_PCRE_FULL_PATH "../lib/" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    link_directories(${MSVC_PCRE_FULL_PATH})
    target_link_libraries(neotags pcre2-8)
else()
    target_link_libraries(neotags -lpcre2-8)
    if ("${HAS_LIBBSD}" EQUAL "1")
        target_link_libraries(neotags -lbsd)
    endif()
endif()
