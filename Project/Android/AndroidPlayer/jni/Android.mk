#----------------------------------------------------------------------------------
# File:        native_basic/jni/Android.mk
# SDK Version: v10.14 
# Email:       tegradev@nvidia.com
# Site:        http://developer.nvidia.com/
#
# Copyright (c) 2007-2012, NVIDIA CORPORATION.  All rights reserved.
#
# TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
# *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
# OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA OR ITS SUPPLIERS
# BE  LIABLE  FOR  ANY  SPECIAL,  INCIDENTAL,  INDIRECT,  OR  CONSEQUENTIAL DAMAGES
# WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS OF BUSINESS PROFITS,
# BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
# ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
# BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
#
#
#----------------------------------------------------------------------------------
LOCAL_PATH := $(subst //,/,$(call my-dir))
include $(CLEAR_VARS)
LOCAL_MODULE := AndroidPlayer

LOCAL_SRC_FILES := $(wildcard *.cpp)
LOCAL_SRC_FILES += $(wildcard *.c)
LOCAL_C_INCLUDES  += ../../../../blueshift/Engine/Source/Core/Public 
LOCAL_C_INCLUDES  += ../../../../blueshift/Engine/Source/Dependencies
LOCAL_C_INCLUDES  += ../../../../blueshift/Engine/Source/BLib/Public
LOCAL_C_INCLUDES  += ../../../../blueshift/Engine/Source/Renderer/Public
LOCAL_CFLAGS += -std=c++11

LOCAL_ARM_MODE   := arm

LOCAL_LDLIBS :=  -lstdc++ -lc -lm -llog -landroid -ldl -lGLESv3 -lEGL
LOCAL_STATIC_LIBRARIES := nv_and_util nv_egl_util nv_bitfont nv_math nv_glesutil nv_hhdds nv_log nv_shader nv_file nv_thread
LOCAL_STATIC_LIBRARIES += BLib

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path, ../libs/jni)

$(call import-module,nv_and_util)
$(call import-module,nv_egl_util)
$(call import-module,nv_bitfont)
$(call import-module,nv_math)
$(call import-module,nv_glesutil)
$(call import-module,nv_hhdds)
$(call import-module,nv_log)
$(call import-module,nv_shader)
$(call import-module,nv_file)
$(call import-module,nv_thread)
