cmake_minimum_required(VERSION 2.8.12)

project(luasocket)

set(SRC_FILES
    luasocket.c
    luasocket.h
    timeout.c
    timeout.h
    buffer.c
    buffer.h
    io.c
    io.h
    auxiliar.c
    auxiliar.h
    compat.c
    compat.h
    options.c
    options.h
    inet.c
    inet.h
    except.c
    except.h
    select.c
    select.h
    tcp.c
    tcp.h
    udp.c
    udp.h
    mime.c
    mime.h
    socket.h
)

if (WIN32 AND NOT ANDROID)
    list(APPEND SRC_FILES
        wsocket.c
        wsocket.h)
elseif (UNIX)
    list(APPEND SRC_FILES
        usocket.c
        usocket.h
        unixstream.c
        unixstream.h
        unixdgram.c
        unixdgram.h
        unix.c
        unix.h
        #serial.c
        )
endif ()
    
if (USE_LUAJIT)
    include_directories(${ENGINE_INCLUDE_DIR}/ThirdParty/luaJIT/src)
else ()
    include_directories(${ENGINE_INCLUDE_DIR}/ThirdParty/lua)
endif ()

#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DLUASOCKET_DEBUG")

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

if (WIN32 AND NOT ANDROID)
    target_link_libraries(${PROJECT_NAME} "wsock32")
elseif (APPLE)
    if (NOT IOS)
        target_link_libraries(${PROJECT_NAME} "-undefined dynamic_lookup")
    endif ()

    set_target_properties(${PROJECT_NAME} PROPERTIES 
        COMPILE_FLAGS "-fno-common")
    add_definitions("-DUNIX_HAS_SUN_LEN")
endif ()

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER ThirdParty
    PREFIX ""
    #DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
