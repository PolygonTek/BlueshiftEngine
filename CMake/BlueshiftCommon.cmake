################################################################################
# Useful functions and macros
################################################################################
function(auto_source_group SRC_FILES)
  foreach(FILE ${ARGV})
    # get the base path
    get_filename_component(BASE_PATH "${FILE}" PATH)

    # changes /'s to \\'s
    string(REPLACE "/" "\\" GROUP "${BASE_PATH}")

    source_group("${GROUP}" FILES "${FILE}")
  endforeach()
endfunction()

function(enable_precompiled_header ARG_PCH_HEADER ARG_PCH_SOURCE ARG_PCH_USE_SOURCES)
  if (MSVC)
    set(FILES ${${ARG_PCH_USE_SOURCES}})

    foreach (FILE ${FILES})
      if (FILE MATCHES ".*\\.(c|cpp)$")
        if (FILE STREQUAL ${ARG_PCH_SOURCE})
          set_source_files_properties(${FILE} PROPERTIES COMPILE_FLAGS "/Yc\"${ARG_PCH_HEADER}\"")
        else()
          set_source_files_properties(${FILE} PROPERTIES COMPILE_FLAGS "/Yu\"${ARG_PCH_HEADER}\"")
        endif()
      endif()
    endforeach(FILE)
  endif()
endfunction()

function(add_framework APP_NAME FW_NAME)
  find_library(FRAMEWORK_${FW_NAME} ${FW_NAME})
  #find_library(FRAMEWORK_${FW_NAME} NAMES ${FW_NAME} PATHS ${CMAKE_OSX_SYSROOT}/System/Library PATH_SUFFIXES Frameworks NO_DEFAULT_PATH)
  if (${FRAMEWORK_${FW_NAME}} STREQUAL FRAMEWORK_${FW_NAME}-NOTFOUND)
    message(ERROR ": Framework ${FW_NAME} not found")
  else ()
    target_link_libraries(${APP_NAME} "${FRAMEWORK_${FW_NAME}}")
    message(STATUS "Found framework ${FW_NAME} at ${FRAMEWORK_${FW_NAME}}")
  endif()
endfunction()
