cmake_minimum_required(VERSION 2.8.12)

if (COMMAND cmake_policy)
    if (POLICY CMP0053)
        # Simplify variable reference and escape sequence evaluation.
        cmake_policy(SET CMP0053 OLD)
    endif ()
endif ()

project(LuaJIT)

include(FindLuaJIT)

file(GLOB_RECURSE ALL_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/lua*.h*)

auto_source_group(${ALL_FILES})

add_custom_target(${PROJECT_NAME} SOURCES ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ThirdParty)

# Skip LuaJIT compilation if library already exists
if (LUAJIT_FOUND)
    #return ()
endif ()

set(STATIC_LIB TRUE)

if (APPLE)
    if (NOT IOS)
        set($ENV{MACOSX_DEPLOYMENT_TARGET} ${CMAKE_OSX_DEPLOYMENT_TARGET})
        set(LJLIBNAME libluajit.a)
        set(LJARCH x86_64)

        execute_process(COMMAND make clean
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
            RESULT_VARIABLE EXIT_CODE)

        execute_process(COMMAND make "TARGET_FLAGS=-arch ${LJARCH}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
            RESULT_VARIABLE EXIT_CODE)

        # Copy library and interpreter executable to lib folder
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../lib/${LUAJIT_LIBDIR})

        execute_process(COMMAND ${CMAKE_COMMAND} -E copy luajit ../lib/${LUAJIT_LIBDIR}/luajit
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
            RESULT_VARIABLE EXIT_CODE)

        execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${LJLIBNAME} ../lib/${LUAJIT_LIBDIR}/${LJLIBNAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
            RESULT_VARIABLE EXIT_CODE)

        # Clean 
        execute_process(COMMAND make clean
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
            RESULT_VARIABLE EXIT_CODE)

        message(STATUS "=== Successfully built LuaJIT for MacOS/${LJARCH} ===")
    endif ()
elseif (MSVC)
    if (DEFINED $ENV{VSWHERE})
        set(VSWHERE $ENV{VSWHERE})
    else ()
        set(VSWHERE "$ENV{ProgramFiles(x86)}\\Microsoft Visual Studio\\Installer\\vswhere.exe")
    endif ()
        
    execute_process(COMMAND ${VSWHERE} -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
        OUTPUT_VARIABLE VSINSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

    if (DEFINED VSINSTALL_DIR)
        set(VCVARS64 "${VSINSTALL_DIR}\\VC\\Auxiliary\\Build\\vcvars64.bat")
        # Prevent vcvarsxx.bat call from changing the current working directory
        set(ENV{VSCMD_START_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
    elseif (DEFINED ENV{VS140COMNTOOLS})
        set(VCVARS64 "$ENV{VS140COMNTOOLS}..\\..\\VC\\bin\\amd64\\vcvars64.bat")
    else ()
        message(FATAL_ERROR "Not found vcvars64.bat")
    endif ()

    set(LJCOMPILE cl /MD /nologo /c /O2 /W3 /D_CRT_SECURE_NO_DEPRECATE "/D_CRT_STDIO_INLINE=__declspec(dllexport)__inline") # NOTE: Add /MD because we need it !
    set(LJLINK link /nologo)
    set(LJMT mt /nologo)
    set(LJLIB lib /nologo /nodefaultlib)
    set(DASMDIR ../dynasm)
    set(DASM ${DASMDIR}/dynasm.lua)
    set(LJDLLNAME lua51.dll)
    set(LJLIBNAME lua51.lib)
    set(ALL_LIB lib_base.c lib_math.c lib_bit.c lib_string.c lib_table.c lib_io.c lib_os.c lib_package.c lib_debug.c lib_jit.c lib_ffi.c)
    
    # Compile minilua
    execute_process(COMMAND ${VCVARS64} && ${LJCOMPILE} host/minilua.c && ${LJLINK} /out:minilua.exe minilua.obj
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        RESULT_VARIABLE EXIT_CODE)

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/minilua.exe.manifest")
        execute_process(COMMAND ${VCVARS64} && ${LJMT} -manifest minilua.exe.manifest -outputresource:minilua.exe
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
            RESULT_VARIABLE EXIT_CODE)
    endif ()

    # Generate buildvm_arch.h using minilua
    set(DASMFLAGS -D WIN -D JIT -D FFI -D P64)
    set(LJARCH x64)

    execute_process(COMMAND minilua
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        RESULT_VARIABLE EXIT_CODE)

    execute_process(COMMAND minilua ${DASM} -LN ${DASMFLAGS} -o host/buildvm_arch.h vm_x86.dasc
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        RESULT_VARIABLE EXIT_CODE)

    # Compile buildvm
    execute_process(COMMAND ${VCVARS64} && ${LJCOMPILE} /I "." /I ${DASMDIR} host/buildvm*.c && ${LJLINK} /out:buildvm.exe buildvm*.obj
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        RESULT_VARIABLE EXIT_CODE)

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/buildvm.exe.manifest")
        execute_process(COMMAND ${VCVARS64} && ${LJMT} -manifest buildvm.exe.manifest -outputresource:buildvm.exe
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
            RESULT_VARIABLE EXIT_CODE)
    endif ()

    # Generate luaJIT header files using buildvm
    execute_process(COMMAND buildvm -m peobj -o lj_vm.obj
        COMMAND buildvm -m bcdef -o lj_bcdef.h ${ALL_LIB}
        COMMAND buildvm -m ffdef -o lj_ffdef.h ${ALL_LIB}
        COMMAND buildvm -m libdef -o lj_libdef.h ${ALL_LIB}
        COMMAND buildvm -m recdef -o lj_recdef.h ${ALL_LIB}
        COMMAND buildvm -m vmdef -o jit/vmdef.lua ${ALL_LIB}
        COMMAND buildvm -m folddef -o lj_folddef.h lj_opt_fold.c
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        RESULT_VARIABLE EXIT_CODE)

    # Compile library
    if (STATIC_LIB)
        set(LUAJIT_LIB_OPTIONS /OUT:${LJLIBNAME})
    else ()
        set(LUAJIT_COMPILE_OPTIONS /DLUA_BUILD_AS_DLL)
        set(LUAJIT_LIB_OPTIONS /DLL /OUT:${LJDLLNAME})
    endif ()

    execute_process(COMMAND ${VCVARS64} && ${LJCOMPILE} ${LUAJIT_COMPILE_OPTIONS} lj_*.c lib_*.c && ${LJLIB} ${LUAJIT_LIB_OPTIONS} lj_*.obj lib_*.obj
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        RESULT_VARIABLE EXIT_CODE)

    # Compile interpreter executable
    execute_process(COMMAND ${VCVARS64} && ${LJCOMPILE} luajit.c && ${LJLINK} /out:luajit.exe luajit.obj ${LJLIBNAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        RESULT_VARIABLE EXIT_CODE)

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/luajit.exe.manifest")
        execute_process(COMMAND ${VCVARS64} && ${LJMT} -manifest luajit.exe.manifest -outputresource:luajit.exe
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
            RESULT_VARIABLE EXIT_CODE)
    endif ()

    # Remove intermediate files
    file(GLOB OBJ_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.obj)
    file(GLOB EXP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.exp)
    file(GLOB MANIFEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.manifest)
    
    execute_process(COMMAND ${CMAKE_COMMAND} -E remove ${OBJ_FILES} ${EXP_FILES} ${MANIFEST_FILES} minilua.exe buildvm.exe host/buildvm_arch.h lj_bcdef.h lj_ffdef.h lj_libdef.h lj_recdef.h lj_folddef.h jit/vmdef.lua
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        RESULT_VARIABLE EXIT_CODE)

    # Copy library and interpreter executable to lib folder
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../lib/${LUAJIT_LIBDIR})

    execute_process(COMMAND ${CMAKE_COMMAND} -E copy luajit.exe ../lib/${LUAJIT_LIBDIR}/luajit.exe
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
        RESULT_VARIABLE EXIT_CODE)

    if (STATIC_LIB)
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${LJLIBNAME} ../lib/${LUAJIT_LIBDIR}/${LJLIBNAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
            RESULT_VARIABLE EXIT_CODE)
    else ()
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${LJDLLNAME} ../lib/${LUAJIT_LIBDIR}/${LJDLLNAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
            RESULT_VARIABLE EXIT_CODE)
    endif ()

    message(STATUS "=== Successfully built LuaJIT for Windows/${LJARCH} ===")
endif ()
