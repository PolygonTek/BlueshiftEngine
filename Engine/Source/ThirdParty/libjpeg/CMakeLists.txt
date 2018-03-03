cmake_minimum_required(VERSION 2.8.12)

project(libjpeg)

set(SRC_FILES
	cderror.h
	jcinit.c
	jdapimin.c
	jdinput.c
	jfdctflt.c
	jmorecfg.h
	cdjpeg.h
	jcmainct.c
	jdapistd.c
	jdmainct.c
	jfdctfst.c
	jpegint.h
	jaricom.c
	jcmarker.c
	jdarith.c
	jdmarker.c
	jfdctint.c
	jpeglib.h
	jcapimin.c
	jcmaster.c
	jdatadst.c
	jdmaster.c
	jidctflt.c
	jquant1.c
	jcapistd.c
	jcomapi.c
	jdatasrc.c
	jdmerge.c
	jidctfst.c
	jquant2.c
	jcarith.c
	jconfig.h
	jdcoefct.c
	jdpostct.c
	jidctint.c
	jutils.c
	jccoefct.c
	jcparam.c
	jdcolor.c
	jdsample.c
	jinclude.h
	jversion.h
	jccolor.c
	jcprepct.c
	jdct.h
	jdtrans.c
	jmemmgr.c
	transupp.c
	jcdctmgr.c
	jcsample.c
	jddctmgr.c
	jerror.c
	jmemnobs.c
	transupp.h
	jchuff.c
	jctrans.c
	jdhuff.c
	jerror.h
	jmemsys.h
)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES 
    FOLDER ThirdParty
    PREFIX ""
    #DEBUG_POSTFIX "_d"
    OUTPUT_NAME ${PROJECT_NAME}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Library/${ENGINE_BUILD_PLATFORM_DIR})
