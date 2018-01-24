set(JSONCPP_INCLUDE_DIR ../../include )

set(PUBLIC_HEADERS
    ${JSONCPP_INCLUDE_DIR}/json/config.h
    ${JSONCPP_INCLUDE_DIR}/json/forwards.h
    ${JSONCPP_INCLUDE_DIR}/json/features.h
    ${JSONCPP_INCLUDE_DIR}/json/value.h
    ${JSONCPP_INCLUDE_DIR}/json/reader.h
    ${JSONCPP_INCLUDE_DIR}/json/writer.h
    ${JSONCPP_INCLUDE_DIR}/json/assertions.h
    ${JSONCPP_INCLUDE_DIR}/json/version.h)

source_group("Public API" FILES ${PUBLIC_HEADERS})

add_library(${PROJECT_NAME} STATIC
    ${PUBLIC_HEADERS}
    json_tool.h
    json_reader.cpp
    json_batchallocator.h
    json_valueiterator.inl
    json_value.cpp
    json_writer.cpp
    version.h.in)

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER Dependencies
    PREFIX ""
    DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    VERSION ${JSONCPP_VERSION} SOVERSION ${JSONCPP_VERSION_MAJOR})
