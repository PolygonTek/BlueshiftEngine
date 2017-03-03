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
endfunction(enable_precompiled_header)

function(add_framework appname fwname)
  find_library(FRAMEWORK_${fwname} ${fwname})
  #find_library(FRAMEWORK_${fwname} NAMES ${fwname} PATHS ${CMAKE_OSX_SYSROOT}/System/Library PATH_SUFFIXES Frameworks NO_DEFAULT_PATH)
  if (${FRAMEWORK_${fwname}} STREQUAL FRAMEWORK_${fwname}-NOTFOUND)
    message(ERROR ": Framework ${fwname} not found")
  else ()
    target_link_libraries(${appname} "${FRAMEWORK_${fwname}}")
    message(STATUS "Found framework ${fwname} at ${FRAMEWORK_${fwname}}")
  endif()
endfunction(add_framework)
