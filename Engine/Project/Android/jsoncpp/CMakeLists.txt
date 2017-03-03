Output Directory: ..\..\..\Build\Android\$(Platform)\$(Configuration)\
Intermidiate Directory: ..\..\..\Build\Android\$(ProjectName)\$(Platform)\$(Configuration)\
Min Android API Level : Android 5.0 (android-21)
Target Android API Level : Android 5.0 (android-21)
Additional Include Directory: ..\..\..\Source\Dependencies\jsoncpp\include
Preprocesson Definition: JSON_USE_EXCEPTION=0



CMAKE_MINIMUM_REQUIRED(VERSION 2.8.5)

PROJECT(jsoncpp)

SET(LIB_SUFFIX "" CACHE STRING "Optional arch-dependent suffix for the library installation directory")

# Set variable named ${VAR_NAME} to value ${VALUE}
FUNCTION(set_using_dynamic_name VAR_NAME VALUE)
    SET( "${VAR_NAME}" "${VALUE}" PARENT_SCOPE)
ENDFUNCTION(set_using_dynamic_name)

# Extract major, minor, patch from version text
# Parse a version string "X.Y.Z" and outputs
# version parts in ${OUPUT_PREFIX}_MAJOR, _MINOR, _PATCH.
# If parse succeeds then ${OUPUT_PREFIX}_FOUND is TRUE.
MACRO(jsoncpp_parse_version VERSION_TEXT OUPUT_PREFIX)
    SET(VERSION_REGEX "[0-9]+\\.[0-9]+\\.[0-9]+(-[a-zA-Z0-9_]+)?")
    IF( ${VERSION_TEXT} MATCHES ${VERSION_REGEX} )
        STRING(REGEX MATCHALL "[0-9]+|-([A-Za-z0-9_]+)" VERSION_PARTS ${VERSION_TEXT})
        LIST(GET VERSION_PARTS 0 ${OUPUT_PREFIX}_MAJOR)
        LIST(GET VERSION_PARTS 1 ${OUPUT_PREFIX}_MINOR)
        LIST(GET VERSION_PARTS 2 ${OUPUT_PREFIX}_PATCH)
        set_using_dynamic_name( "${OUPUT_PREFIX}_FOUND" TRUE )
    ELSE( ${VERSION_TEXT} MATCHES ${VERSION_REGEX} )
        set_using_dynamic_name( "${OUPUT_PREFIX}_FOUND" FALSE )
    ENDIF( ${VERSION_TEXT} MATCHES ${VERSION_REGEX} )
ENDMACRO(jsoncpp_parse_version)

# Read out version from "version" file
FILE(STRINGS "version" JSONCPP_VERSION)

jsoncpp_parse_version( ${JSONCPP_VERSION} JSONCPP_VERSION )
IF(NOT JSONCPP_VERSION_FOUND)
    MESSAGE(FATAL_ERROR "Failed to parse version string properly. Expect X.Y.Z")
ENDIF(NOT JSONCPP_VERSION_FOUND)

MESSAGE(STATUS "JsonCpp Version: ${JSONCPP_VERSION_MAJOR}.${JSONCPP_VERSION_MINOR}.${JSONCPP_VERSION_PATCH}")
# File version.h is only regenerated on CMake configure step
CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/src/lib_json/version.h.in"
                "${PROJECT_SOURCE_DIR}/include/json/version.h" )

# Include our configuration header
INCLUDE_DIRECTORIES( ${PROJECT_SOURCE_DIR}/include )

if ( MSVC )
    # Only enabled in debug because some old versions of VS STL generate
    # unreachable code warning when compiled in release configuration.
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W4 ")
endif( MSVC )

# Build the different applications
ADD_SUBDIRECTORY( src/lib_json )
