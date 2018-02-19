/*********************************************************************************************
 *
 * gegl.h
 * ggl (OpenGL glue code library)
 * Version: 0.5
 *
 * Copyright 2011 Ju Hyung Lee. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *     1. Redistributions of source code must retain the above copyright notice, this list of
 *        conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright notice, this list
 *        of conditions and the following disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY JU HYUNG LEE ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL JU HYUNG LEE OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Ju Hyung Lee.
 *
 ***********************************************************************************************/

#ifndef __GEGL_H__
#define __GEGL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "EGL/egl.h"
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

#include <EGL/eglplatform.h>
typedef unsigned int EGLBoolean;
typedef unsigned int EGLenum;
typedef intptr_t EGLAttribKHR;
typedef intptr_t EGLAttrib;
typedef void *EGLClientBuffer;
typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLDeviceEXT;
typedef void *EGLDisplay;
typedef void *EGLImage;
typedef void *EGLImageKHR;
typedef void *EGLLabelKHR;
typedef void *EGLObjectKHR;
typedef void *EGLOutputLayerEXT;
typedef void *EGLOutputPortEXT;
typedef void *EGLStreamKHR;
typedef void *EGLSurface;
typedef void *EGLSync;
typedef void *EGLSyncKHR;
typedef void *EGLSyncNV;
typedef void (*__eglMustCastToProperFunctionPointerType)(void);
typedef khronos_utime_nanoseconds_t EGLTimeKHR;
typedef khronos_utime_nanoseconds_t EGLTime;
typedef khronos_utime_nanoseconds_t EGLTimeNV;
typedef khronos_utime_nanoseconds_t EGLuint64NV;
typedef khronos_uint64_t EGLuint64KHR;
typedef khronos_stime_nanoseconds_t EGLnsecsANDROID;
typedef int EGLNativeFileDescriptorKHR;
typedef khronos_ssize_t EGLsizeiANDROID;
typedef void (*EGLSetBlobFuncANDROID) (const void *key, EGLsizeiANDROID keySize, const void *value, EGLsizeiANDROID valueSize);
typedef EGLsizeiANDROID (*EGLGetBlobFuncANDROID) (const void *key, EGLsizeiANDROID keySize, void *value, EGLsizeiANDROID valueSize);
struct EGLClientPixmapHI {
    void  *pData;
    EGLint iWidth;
    EGLint iHeight;
    EGLint iStride;
};
typedef void (APIENTRY *EGLDEBUGPROCKHR)(EGLenum error,const char *command,EGLint messageType,EGLLabelKHR threadLabel,EGLLabelKHR objectLabel,const char* message);

#ifndef EGL_VERSION_1_0
#define EGL_VERSION_1_0
#define EGL_ALPHA_SIZE 0x3021
#define EGL_BAD_ACCESS 0x3002
#define EGL_BAD_ALLOC 0x3003
#define EGL_BAD_ATTRIBUTE 0x3004
#define EGL_BAD_CONFIG 0x3005
#define EGL_BAD_CONTEXT 0x3006
#define EGL_BAD_CURRENT_SURFACE 0x3007
#define EGL_BAD_DISPLAY 0x3008
#define EGL_BAD_MATCH 0x3009
#define EGL_BAD_NATIVE_PIXMAP 0x300A
#define EGL_BAD_NATIVE_WINDOW 0x300B
#define EGL_BAD_PARAMETER 0x300C
#define EGL_BAD_SURFACE 0x300D
#define EGL_BLUE_SIZE 0x3022
#define EGL_BUFFER_SIZE 0x3020
#define EGL_CONFIG_CAVEAT 0x3027
#define EGL_CONFIG_ID 0x3028
#define EGL_CORE_NATIVE_ENGINE 0x305B
#define EGL_DEPTH_SIZE 0x3025
#define EGL_DONT_CARE ((EGLint)-1)
#define EGL_DRAW 0x3059
#define EGL_EXTENSIONS 0x3055
#define EGL_FALSE 0
#define EGL_GREEN_SIZE 0x3023
#define EGL_HEIGHT 0x3056
#define EGL_LARGEST_PBUFFER 0x3058
#define EGL_LEVEL 0x3029
#define EGL_MAX_PBUFFER_HEIGHT 0x302A
#define EGL_MAX_PBUFFER_PIXELS 0x302B
#define EGL_MAX_PBUFFER_WIDTH 0x302C
#define EGL_NATIVE_RENDERABLE 0x302D
#define EGL_NATIVE_VISUAL_ID 0x302E
#define EGL_NATIVE_VISUAL_TYPE 0x302F
#define EGL_NONE 0x3038
#define EGL_NON_CONFORMANT_CONFIG 0x3051
#define EGL_NOT_INITIALIZED 0x3001
#define EGL_NO_CONTEXT ((EGLContext)0)
#define EGL_NO_DISPLAY ((EGLDisplay)0)
#define EGL_NO_SURFACE ((EGLSurface)0)
#define EGL_PBUFFER_BIT 0x0001
#define EGL_PIXMAP_BIT 0x0002
#define EGL_READ 0x305A
#define EGL_RED_SIZE 0x3024
#define EGL_SAMPLES 0x3031
#define EGL_SAMPLE_BUFFERS 0x3032
#define EGL_SLOW_CONFIG 0x3050
#define EGL_STENCIL_SIZE 0x3026
#define EGL_SUCCESS 0x3000
#define EGL_SURFACE_TYPE 0x3033
#define EGL_TRANSPARENT_BLUE_VALUE 0x3035
#define EGL_TRANSPARENT_GREEN_VALUE 0x3036
#define EGL_TRANSPARENT_RED_VALUE 0x3037
#define EGL_TRANSPARENT_RGB 0x3052
#define EGL_TRANSPARENT_TYPE 0x3034
#define EGL_TRUE 1
#define EGL_VENDOR 0x3053
#define EGL_VERSION 0x3054
#define EGL_WIDTH 0x3057
#define EGL_WINDOW_BIT 0x0004
#endif
extern EGLBoolean (APIENTRYP geglChooseConfig)(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
extern EGLBoolean (APIENTRYP geglCopyBuffers)(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target);
extern EGLContext (APIENTRYP geglCreateContext)(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
extern EGLSurface (APIENTRYP geglCreatePbufferSurface)(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
extern EGLSurface (APIENTRYP geglCreatePixmapSurface)(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list);
extern EGLSurface (APIENTRYP geglCreateWindowSurface)(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list);
extern EGLBoolean (APIENTRYP geglDestroyContext)(EGLDisplay dpy, EGLContext ctx);
extern EGLBoolean (APIENTRYP geglDestroySurface)(EGLDisplay dpy, EGLSurface surface);
extern EGLBoolean (APIENTRYP geglGetConfigAttrib)(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
extern EGLBoolean (APIENTRYP geglGetConfigs)(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
extern EGLDisplay (APIENTRYP geglGetCurrentDisplay)();
extern EGLSurface (APIENTRYP geglGetCurrentSurface)(EGLint readdraw);
extern EGLDisplay (APIENTRYP geglGetDisplay)(EGLNativeDisplayType display_id);
extern EGLint (APIENTRYP geglGetError)();
extern __eglMustCastToProperFunctionPointerType (APIENTRYP geglGetProcAddress)(const char *procname);
extern EGLBoolean (APIENTRYP geglInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor);
extern EGLBoolean (APIENTRYP geglMakeCurrent)(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
extern EGLBoolean (APIENTRYP geglQueryContext)(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value);
extern const char * (APIENTRYP geglQueryString)(EGLDisplay dpy, EGLint name);
extern EGLBoolean (APIENTRYP geglQuerySurface)(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);
extern EGLBoolean (APIENTRYP geglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
extern EGLBoolean (APIENTRYP geglTerminate)(EGLDisplay dpy);
extern EGLBoolean (APIENTRYP geglWaitGL)();
extern EGLBoolean (APIENTRYP geglWaitNative)(EGLint engine);

#ifndef EGL_VERSION_1_1
#define EGL_VERSION_1_1
#define EGL_BACK_BUFFER 0x3084
#define EGL_BIND_TO_TEXTURE_RGB 0x3039
#define EGL_BIND_TO_TEXTURE_RGBA 0x303A
#define EGL_CONTEXT_LOST 0x300E
#define EGL_MIN_SWAP_INTERVAL 0x303B
#define EGL_MAX_SWAP_INTERVAL 0x303C
#define EGL_MIPMAP_TEXTURE 0x3082
#define EGL_MIPMAP_LEVEL 0x3083
#define EGL_NO_TEXTURE 0x305C
#define EGL_TEXTURE_2D 0x305F
#define EGL_TEXTURE_FORMAT 0x3080
#define EGL_TEXTURE_RGB 0x305D
#define EGL_TEXTURE_RGBA 0x305E
#define EGL_TEXTURE_TARGET 0x3081
#endif
extern EGLBoolean (APIENTRYP geglBindTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
extern EGLBoolean (APIENTRYP geglReleaseTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
extern EGLBoolean (APIENTRYP geglSurfaceAttrib)(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
extern EGLBoolean (APIENTRYP geglSwapInterval)(EGLDisplay dpy, EGLint interval);

#ifndef EGL_VERSION_1_2
#define EGL_VERSION_1_2
#define EGL_ALPHA_FORMAT 0x3088
#define EGL_ALPHA_FORMAT_NONPRE 0x308B
#define EGL_ALPHA_FORMAT_PRE 0x308C
#define EGL_ALPHA_MASK_SIZE 0x303E
#define EGL_BUFFER_PRESERVED 0x3094
#define EGL_BUFFER_DESTROYED 0x3095
#define EGL_CLIENT_APIS 0x308D
#define EGL_COLORSPACE 0x3087
#define EGL_COLORSPACE_sRGB 0x3089
#define EGL_COLORSPACE_LINEAR 0x308A
#define EGL_COLOR_BUFFER_TYPE 0x303F
#define EGL_CONTEXT_CLIENT_TYPE 0x3097
#define EGL_DISPLAY_SCALING 10000
#define EGL_HORIZONTAL_RESOLUTION 0x3090
#define EGL_LUMINANCE_BUFFER 0x308F
#define EGL_LUMINANCE_SIZE 0x303D
#define EGL_OPENGL_ES_BIT 0x0001
#define EGL_OPENVG_BIT 0x0002
#define EGL_OPENGL_ES_API 0x30A0
#define EGL_OPENVG_API 0x30A1
#define EGL_OPENVG_IMAGE 0x3096
#define EGL_PIXEL_ASPECT_RATIO 0x3092
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_RENDER_BUFFER 0x3086
#define EGL_RGB_BUFFER 0x308E
#define EGL_SINGLE_BUFFER 0x3085
#define EGL_SWAP_BEHAVIOR 0x3093
#define EGL_UNKNOWN ((EGLint)-1)
#define EGL_VERTICAL_RESOLUTION 0x3091
#endif
extern EGLBoolean (APIENTRYP geglBindAPI)(EGLenum api);
extern EGLenum (APIENTRYP geglQueryAPI)();
extern EGLSurface (APIENTRYP geglCreatePbufferFromClientBuffer)(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list);
extern EGLBoolean (APIENTRYP geglReleaseThread)();
extern EGLBoolean (APIENTRYP geglWaitClient)();

#ifndef EGL_VERSION_1_3
#define EGL_VERSION_1_3
#define EGL_CONFORMANT 0x3042
#define EGL_CONTEXT_CLIENT_VERSION 0x3098
#define EGL_MATCH_NATIVE_PIXMAP 0x3041
#define EGL_OPENGL_ES2_BIT 0x0004
#define EGL_VG_ALPHA_FORMAT 0x3088
#define EGL_VG_ALPHA_FORMAT_NONPRE 0x308B
#define EGL_VG_ALPHA_FORMAT_PRE 0x308C
#define EGL_VG_ALPHA_FORMAT_PRE_BIT 0x0040
#define EGL_VG_COLORSPACE 0x3087
#define EGL_VG_COLORSPACE_sRGB 0x3089
#define EGL_VG_COLORSPACE_LINEAR 0x308A
#define EGL_VG_COLORSPACE_LINEAR_BIT 0x0020
#endif

#ifndef EGL_VERSION_1_4
#define EGL_VERSION_1_4
#define EGL_DEFAULT_DISPLAY ((EGLNativeDisplayType)0)
#define EGL_MULTISAMPLE_RESOLVE_BOX_BIT 0x0200
#define EGL_MULTISAMPLE_RESOLVE 0x3099
#define EGL_MULTISAMPLE_RESOLVE_DEFAULT 0x309A
#define EGL_MULTISAMPLE_RESOLVE_BOX 0x309B
#define EGL_OPENGL_API 0x30A2
#define EGL_OPENGL_BIT 0x0008
#define EGL_SWAP_BEHAVIOR_PRESERVED_BIT 0x0400
#endif
extern EGLContext (APIENTRYP geglGetCurrentContext)();

#ifndef EGL_VERSION_1_5
#define EGL_VERSION_1_5
#define EGL_CONTEXT_MAJOR_VERSION 0x3098
#define EGL_CONTEXT_MINOR_VERSION 0x30FB
#define EGL_CONTEXT_OPENGL_PROFILE_MASK 0x30FD
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY 0x31BD
#define EGL_NO_RESET_NOTIFICATION 0x31BE
#define EGL_LOSE_CONTEXT_ON_RESET 0x31BF
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT 0x00000001
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT 0x00000002
#define EGL_CONTEXT_OPENGL_DEBUG 0x31B0
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE 0x31B1
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS 0x31B2
#define EGL_OPENGL_ES3_BIT 0x00000040
#define EGL_CL_EVENT_HANDLE 0x309C
#define EGL_SYNC_CL_EVENT 0x30FE
#define EGL_SYNC_CL_EVENT_COMPLETE 0x30FF
#define EGL_SYNC_PRIOR_COMMANDS_COMPLETE 0x30F0
#define EGL_SYNC_TYPE 0x30F7
#define EGL_SYNC_STATUS 0x30F1
#define EGL_SYNC_CONDITION 0x30F8
#define EGL_SIGNALED 0x30F2
#define EGL_UNSIGNALED 0x30F3
#define EGL_SYNC_FLUSH_COMMANDS_BIT 0x0001
#define EGL_FOREVER 0xFFFFFFFFFFFFFFFF
#define EGL_TIMEOUT_EXPIRED 0x30F5
#define EGL_CONDITION_SATISFIED 0x30F6
#define EGL_NO_SYNC ((EGLSync)0)
#define EGL_SYNC_FENCE 0x30F9
#define EGL_GL_COLORSPACE 0x309D
#define EGL_GL_COLORSPACE_SRGB 0x3089
#define EGL_GL_COLORSPACE_LINEAR 0x308A
#define EGL_GL_RENDERBUFFER 0x30B9
#define EGL_GL_TEXTURE_2D 0x30B1
#define EGL_GL_TEXTURE_LEVEL 0x30BC
#define EGL_GL_TEXTURE_3D 0x30B2
#define EGL_GL_TEXTURE_ZOFFSET 0x30BD
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x30B3
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x30B4
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x30B5
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x30B6
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x30B7
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x30B8
#define EGL_IMAGE_PRESERVED 0x30D2
#define EGL_NO_IMAGE ((EGLImage)0)
#endif
extern EGLSync (APIENTRYP geglCreateSync)(EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list);
extern EGLBoolean (APIENTRYP geglDestroySync)(EGLDisplay dpy, EGLSync sync);
extern EGLint (APIENTRYP geglClientWaitSync)(EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout);
extern EGLBoolean (APIENTRYP geglGetSyncAttrib)(EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib *value);
extern EGLImage (APIENTRYP geglCreateImage)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLAttrib *attrib_list);
extern EGLBoolean (APIENTRYP geglDestroyImage)(EGLDisplay dpy, EGLImage image);
extern EGLDisplay (APIENTRYP geglGetPlatformDisplay)(EGLenum platform, void *native_display, const EGLAttrib *attrib_list);
extern EGLSurface (APIENTRYP geglCreatePlatformWindowSurface)(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLAttrib *attrib_list);
extern EGLSurface (APIENTRYP geglCreatePlatformPixmapSurface)(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list);
extern EGLBoolean (APIENTRYP geglWaitSync)(EGLDisplay dpy, EGLSync sync, EGLint flags);

#ifndef EGL_ANDROID_blob_cache
#define EGL_ANDROID_blob_cache
#endif
extern void (APIENTRYP geglSetBlobCacheFuncsANDROID)(EGLDisplay dpy, EGLSetBlobFuncANDROID set, EGLGetBlobFuncANDROID get);

#ifndef EGL_ANDROID_create_native_client_buffer
#define EGL_ANDROID_create_native_client_buffer
#define EGL_NATIVE_BUFFER_USAGE_ANDROID 0x3143
#define EGL_NATIVE_BUFFER_USAGE_PROTECTED_BIT_ANDROID 0x00000001
#define EGL_NATIVE_BUFFER_USAGE_RENDERBUFFER_BIT_ANDROID 0x00000002
#define EGL_NATIVE_BUFFER_USAGE_TEXTURE_BIT_ANDROID 0x00000004
#endif
extern EGLClientBuffer (APIENTRYP geglCreateNativeClientBufferANDROID)(const EGLint *attrib_list);

#ifndef EGL_ANDROID_framebuffer_target
#define EGL_ANDROID_framebuffer_target
#define EGL_FRAMEBUFFER_TARGET_ANDROID 0x3147
#endif

#ifndef EGL_ANDROID_front_buffer_auto_refresh
#define EGL_ANDROID_front_buffer_auto_refresh
#define EGL_FRONT_BUFFER_AUTO_REFRESH_ANDROID 0x314C
#endif

#ifndef EGL_ANDROID_image_native_buffer
#define EGL_ANDROID_image_native_buffer
#define EGL_NATIVE_BUFFER_ANDROID 0x3140
#endif

#ifndef EGL_ANDROID_native_fence_sync
#define EGL_ANDROID_native_fence_sync
#define EGL_SYNC_NATIVE_FENCE_ANDROID 0x3144
#define EGL_SYNC_NATIVE_FENCE_FD_ANDROID 0x3145
#define EGL_SYNC_NATIVE_FENCE_SIGNALED_ANDROID 0x3146
#define EGL_NO_NATIVE_FENCE_FD_ANDROID -1
#endif
extern EGLint (APIENTRYP geglDupNativeFenceFDANDROID)(EGLDisplay dpy, EGLSyncKHR sync);

#ifndef EGL_ANDROID_presentation_time
#define EGL_ANDROID_presentation_time
#endif
extern EGLBoolean (APIENTRYP geglPresentationTimeANDROID)(EGLDisplay dpy, EGLSurface surface, EGLnsecsANDROID time);

#ifndef EGL_ANDROID_recordable
#define EGL_ANDROID_recordable
#define EGL_RECORDABLE_ANDROID 0x3142
#endif

#ifndef EGL_ANGLE_d3d_share_handle_client_buffer
#define EGL_ANGLE_d3d_share_handle_client_buffer
#define EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE 0x3200
#endif

#ifndef EGL_ANGLE_device_d3d
#define EGL_ANGLE_device_d3d
#define EGL_D3D9_DEVICE_ANGLE 0x33A0
#define EGL_D3D11_DEVICE_ANGLE 0x33A1
#endif

#ifndef EGL_ANGLE_query_surface_pointer
#define EGL_ANGLE_query_surface_pointer
#endif
extern EGLBoolean (APIENTRYP geglQuerySurfacePointerANGLE)(EGLDisplay dpy, EGLSurface surface, EGLint attribute, void **value);

#ifndef EGL_ANGLE_surface_d3d_texture_2d_share_handle
#define EGL_ANGLE_surface_d3d_texture_2d_share_handle
/* reuse EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE */
#endif

#ifndef EGL_ANGLE_window_fixed_size
#define EGL_ANGLE_window_fixed_size
#define EGL_FIXED_SIZE_ANGLE 0x3201
#endif

#ifndef EGL_ARM_implicit_external_sync
#define EGL_ARM_implicit_external_sync
#define EGL_SYNC_PRIOR_COMMANDS_IMPLICIT_EXTERNAL_ARM 0x328A
#endif

#ifndef EGL_ARM_pixmap_multisample_discard
#define EGL_ARM_pixmap_multisample_discard
#define EGL_DISCARD_SAMPLES_ARM 0x3286
#endif

#ifndef EGL_EXT_buffer_age
#define EGL_EXT_buffer_age
#define EGL_BUFFER_AGE_EXT 0x313D
#endif

#ifndef EGL_EXT_client_extensions
#define EGL_EXT_client_extensions
#endif

#ifndef EGL_EXT_create_context_robustness
#define EGL_EXT_create_context_robustness
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT 0x30BF
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT 0x3138
#define EGL_NO_RESET_NOTIFICATION_EXT 0x31BE
#define EGL_LOSE_CONTEXT_ON_RESET_EXT 0x31BF
#endif

#ifndef EGL_EXT_device_base
#define EGL_EXT_device_base
#define EGL_NO_DEVICE_EXT ((EGLDeviceEXT)(0))
#define EGL_BAD_DEVICE_EXT 0x322B
#define EGL_DEVICE_EXT 0x322C
#endif
extern EGLBoolean (APIENTRYP geglQueryDeviceAttribEXT)(EGLDeviceEXT device, EGLint attribute, EGLAttrib *value);
extern const char * (APIENTRYP geglQueryDeviceStringEXT)(EGLDeviceEXT device, EGLint name);
extern EGLBoolean (APIENTRYP geglQueryDevicesEXT)(EGLint max_devices, EGLDeviceEXT *devices, EGLint *num_devices);
extern EGLBoolean (APIENTRYP geglQueryDisplayAttribEXT)(EGLDisplay dpy, EGLint attribute, EGLAttrib *value);

#ifndef EGL_EXT_device_drm
#define EGL_EXT_device_drm
#define EGL_DRM_DEVICE_FILE_EXT 0x3233
#endif

#ifndef EGL_EXT_device_enumeration
#define EGL_EXT_device_enumeration
#endif
/* reuse EGLBoolean (APIENTRYP geglQueryDevicesEXT)(EGLint max_devices, EGLDeviceEXT *devices, EGLint *num_devices) */

#ifndef EGL_EXT_device_openwf
#define EGL_EXT_device_openwf
#define EGL_OPENWF_DEVICE_ID_EXT 0x3237
#endif

#ifndef EGL_EXT_device_query
#define EGL_EXT_device_query
/* reuse EGL_NO_DEVICE_EXT */
/* reuse EGL_BAD_DEVICE_EXT */
/* reuse EGL_DEVICE_EXT */
#endif
/* reuse EGLBoolean (APIENTRYP geglQueryDeviceAttribEXT)(EGLDeviceEXT device, EGLint attribute, EGLAttrib *value) */
/* reuse const char * (APIENTRYP geglQueryDeviceStringEXT)(EGLDeviceEXT device, EGLint name) */
/* reuse EGLBoolean (APIENTRYP geglQueryDisplayAttribEXT)(EGLDisplay dpy, EGLint attribute, EGLAttrib *value) */

#ifndef EGL_EXT_gl_colorspace_bt2020_linear
#define EGL_EXT_gl_colorspace_bt2020_linear
#define EGL_GL_COLORSPACE_BT2020_LINEAR_EXT 0x333F
#endif

#ifndef EGL_EXT_gl_colorspace_bt2020_pq
#define EGL_EXT_gl_colorspace_bt2020_pq
#define EGL_GL_COLORSPACE_BT2020_PQ_EXT 0x3340
#endif

#ifndef EGL_EXT_gl_colorspace_scrgb_linear
#define EGL_EXT_gl_colorspace_scrgb_linear
#define EGL_GL_COLORSPACE_SCRGB_LINEAR_EXT 0x3350
#endif

#ifndef EGL_EXT_image_dma_buf_import
#define EGL_EXT_image_dma_buf_import
#define EGL_LINUX_DMA_BUF_EXT 0x3270
#define EGL_LINUX_DRM_FOURCC_EXT 0x3271
#define EGL_DMA_BUF_PLANE0_FD_EXT 0x3272
#define EGL_DMA_BUF_PLANE0_OFFSET_EXT 0x3273
#define EGL_DMA_BUF_PLANE0_PITCH_EXT 0x3274
#define EGL_DMA_BUF_PLANE1_FD_EXT 0x3275
#define EGL_DMA_BUF_PLANE1_OFFSET_EXT 0x3276
#define EGL_DMA_BUF_PLANE1_PITCH_EXT 0x3277
#define EGL_DMA_BUF_PLANE2_FD_EXT 0x3278
#define EGL_DMA_BUF_PLANE2_OFFSET_EXT 0x3279
#define EGL_DMA_BUF_PLANE2_PITCH_EXT 0x327A
#define EGL_YUV_COLOR_SPACE_HINT_EXT 0x327B
#define EGL_SAMPLE_RANGE_HINT_EXT 0x327C
#define EGL_YUV_CHROMA_HORIZONTAL_SITING_HINT_EXT 0x327D
#define EGL_YUV_CHROMA_VERTICAL_SITING_HINT_EXT 0x327E
#define EGL_ITU_REC601_EXT 0x327F
#define EGL_ITU_REC709_EXT 0x3280
#define EGL_ITU_REC2020_EXT 0x3281
#define EGL_YUV_FULL_RANGE_EXT 0x3282
#define EGL_YUV_NARROW_RANGE_EXT 0x3283
#define EGL_YUV_CHROMA_SITING_0_EXT 0x3284
#define EGL_YUV_CHROMA_SITING_0_5_EXT 0x3285
#endif

#ifndef EGL_EXT_image_dma_buf_import_modifiers
#define EGL_EXT_image_dma_buf_import_modifiers
#define EGL_DMA_BUF_PLANE3_FD_EXT 0x3440
#define EGL_DMA_BUF_PLANE3_OFFSET_EXT 0x3441
#define EGL_DMA_BUF_PLANE3_PITCH_EXT 0x3442
#define EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT 0x3443
#define EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT 0x3444
#define EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT 0x3445
#define EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT 0x3446
#define EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT 0x3447
#define EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT 0x3448
#define EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT 0x3449
#define EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT 0x344A
#endif
extern EGLBoolean (APIENTRYP geglQueryDmaBufFormatsEXT)(EGLDisplay dpy, EGLint max_formats, EGLint *formats, EGLint *num_formats);
extern EGLBoolean (APIENTRYP geglQueryDmaBufModifiersEXT)(EGLDisplay dpy, EGLint format, EGLint max_modifiers, EGLuint64KHR *modifiers, EGLBoolean *external_only, EGLint *num_modifiers);

#ifndef EGL_EXT_multiview_window
#define EGL_EXT_multiview_window
#define EGL_MULTIVIEW_VIEW_COUNT_EXT 0x3134
#endif

#ifndef EGL_EXT_output_base
#define EGL_EXT_output_base
#define EGL_NO_OUTPUT_LAYER_EXT ((EGLOutputLayerEXT)0)
#define EGL_NO_OUTPUT_PORT_EXT ((EGLOutputPortEXT)0)
#define EGL_BAD_OUTPUT_LAYER_EXT 0x322D
#define EGL_BAD_OUTPUT_PORT_EXT 0x322E
#define EGL_SWAP_INTERVAL_EXT 0x322F
#endif
extern EGLBoolean (APIENTRYP geglGetOutputLayersEXT)(EGLDisplay dpy, const EGLAttrib *attrib_list, EGLOutputLayerEXT *layers, EGLint max_layers, EGLint *num_layers);
extern EGLBoolean (APIENTRYP geglGetOutputPortsEXT)(EGLDisplay dpy, const EGLAttrib *attrib_list, EGLOutputPortEXT *ports, EGLint max_ports, EGLint *num_ports);
extern EGLBoolean (APIENTRYP geglOutputLayerAttribEXT)(EGLDisplay dpy, EGLOutputLayerEXT layer, EGLint attribute, EGLAttrib value);
extern EGLBoolean (APIENTRYP geglQueryOutputLayerAttribEXT)(EGLDisplay dpy, EGLOutputLayerEXT layer, EGLint attribute, EGLAttrib *value);
extern const char * (APIENTRYP geglQueryOutputLayerStringEXT)(EGLDisplay dpy, EGLOutputLayerEXT layer, EGLint name);
extern EGLBoolean (APIENTRYP geglOutputPortAttribEXT)(EGLDisplay dpy, EGLOutputPortEXT port, EGLint attribute, EGLAttrib value);
extern EGLBoolean (APIENTRYP geglQueryOutputPortAttribEXT)(EGLDisplay dpy, EGLOutputPortEXT port, EGLint attribute, EGLAttrib *value);
extern const char * (APIENTRYP geglQueryOutputPortStringEXT)(EGLDisplay dpy, EGLOutputPortEXT port, EGLint name);

#ifndef EGL_EXT_output_drm
#define EGL_EXT_output_drm
#define EGL_DRM_CRTC_EXT 0x3234
#define EGL_DRM_PLANE_EXT 0x3235
#define EGL_DRM_CONNECTOR_EXT 0x3236
#endif

#ifndef EGL_EXT_output_openwf
#define EGL_EXT_output_openwf
#define EGL_OPENWF_PIPELINE_ID_EXT 0x3238
#define EGL_OPENWF_PORT_ID_EXT 0x3239
#endif

#ifndef EGL_EXT_pixel_format_float
#define EGL_EXT_pixel_format_float
#define EGL_COLOR_COMPONENT_TYPE_EXT 0x3339
#define EGL_COLOR_COMPONENT_TYPE_FIXED_EXT 0x333A
#define EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT 0x333B
#endif

#ifndef EGL_EXT_platform_base
#define EGL_EXT_platform_base
#endif
extern EGLDisplay (APIENTRYP geglGetPlatformDisplayEXT)(EGLenum platform, void *native_display, const EGLint *attrib_list);
extern EGLSurface (APIENTRYP geglCreatePlatformWindowSurfaceEXT)(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
extern EGLSurface (APIENTRYP geglCreatePlatformPixmapSurfaceEXT)(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);

#ifndef EGL_EXT_platform_device
#define EGL_EXT_platform_device
#define EGL_PLATFORM_DEVICE_EXT 0x313F
#endif

#ifndef EGL_EXT_platform_wayland
#define EGL_EXT_platform_wayland
#define EGL_PLATFORM_WAYLAND_EXT 0x31D8
#endif

#ifndef EGL_EXT_platform_x11
#define EGL_EXT_platform_x11
#define EGL_PLATFORM_X11_EXT 0x31D5
#define EGL_PLATFORM_X11_SCREEN_EXT 0x31D6
#endif

#ifndef EGL_EXT_protected_content
#define EGL_EXT_protected_content
#define EGL_PROTECTED_CONTENT_EXT 0x32C0
#endif

#ifndef EGL_EXT_protected_surface
#define EGL_EXT_protected_surface
/* reuse EGL_PROTECTED_CONTENT_EXT */
#endif

#ifndef EGL_EXT_stream_consumer_egloutput
#define EGL_EXT_stream_consumer_egloutput
#endif
extern EGLBoolean (APIENTRYP geglStreamConsumerOutputEXT)(EGLDisplay dpy, EGLStreamKHR stream, EGLOutputLayerEXT layer);

#ifndef EGL_EXT_surface_SMPTE2086_metadata
#define EGL_EXT_surface_SMPTE2086_metadata
#define EGL_SMPTE2086_DISPLAY_PRIMARY_RX_EXT 0x3341
#define EGL_SMPTE2086_DISPLAY_PRIMARY_RY_EXT 0x3342
#define EGL_SMPTE2086_DISPLAY_PRIMARY_GX_EXT 0x3343
#define EGL_SMPTE2086_DISPLAY_PRIMARY_GY_EXT 0x3344
#define EGL_SMPTE2086_DISPLAY_PRIMARY_BX_EXT 0x3345
#define EGL_SMPTE2086_DISPLAY_PRIMARY_BY_EXT 0x3346
#define EGL_SMPTE2086_WHITE_POINT_X_EXT 0x3347
#define EGL_SMPTE2086_WHITE_POINT_Y_EXT 0x3348
#define EGL_SMPTE2086_MAX_LUMINANCE_EXT 0x3349
#define EGL_SMPTE2086_MIN_LUMINANCE_EXT 0x334A
#endif

#ifndef EGL_EXT_swap_buffers_with_damage
#define EGL_EXT_swap_buffers_with_damage
#endif
extern EGLBoolean (APIENTRYP geglSwapBuffersWithDamageEXT)(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects);

#ifndef EGL_EXT_yuv_surface
#define EGL_EXT_yuv_surface
#define EGL_YUV_ORDER_EXT 0x3301
#define EGL_YUV_NUMBER_OF_PLANES_EXT 0x3311
#define EGL_YUV_SUBSAMPLE_EXT 0x3312
#define EGL_YUV_DEPTH_RANGE_EXT 0x3317
#define EGL_YUV_CSC_STANDARD_EXT 0x330A
#define EGL_YUV_PLANE_BPP_EXT 0x331A
#define EGL_YUV_BUFFER_EXT 0x3300
#define EGL_YUV_ORDER_YUV_EXT 0x3302
#define EGL_YUV_ORDER_YVU_EXT 0x3303
#define EGL_YUV_ORDER_YUYV_EXT 0x3304
#define EGL_YUV_ORDER_UYVY_EXT 0x3305
#define EGL_YUV_ORDER_YVYU_EXT 0x3306
#define EGL_YUV_ORDER_VYUY_EXT 0x3307
#define EGL_YUV_ORDER_AYUV_EXT 0x3308
#define EGL_YUV_SUBSAMPLE_4_2_0_EXT 0x3313
#define EGL_YUV_SUBSAMPLE_4_2_2_EXT 0x3314
#define EGL_YUV_SUBSAMPLE_4_4_4_EXT 0x3315
#define EGL_YUV_DEPTH_RANGE_LIMITED_EXT 0x3318
#define EGL_YUV_DEPTH_RANGE_FULL_EXT 0x3319
#define EGL_YUV_CSC_STANDARD_601_EXT 0x330B
#define EGL_YUV_CSC_STANDARD_709_EXT 0x330C
#define EGL_YUV_CSC_STANDARD_2020_EXT 0x330D
#define EGL_YUV_PLANE_BPP_0_EXT 0x331B
#define EGL_YUV_PLANE_BPP_8_EXT 0x331C
#define EGL_YUV_PLANE_BPP_10_EXT 0x331D
#endif

#ifndef EGL_HI_clientpixmap
#define EGL_HI_clientpixmap
#define EGL_CLIENT_PIXMAP_POINTER_HI 0x8F74
#endif
extern EGLSurface (APIENTRYP geglCreatePixmapSurfaceHI)(EGLDisplay dpy, EGLConfig config, struct EGLClientPixmapHI *pixmap);

#ifndef EGL_HI_colorformats
#define EGL_HI_colorformats
#define EGL_COLOR_FORMAT_HI 0x8F70
#define EGL_COLOR_RGB_HI 0x8F71
#define EGL_COLOR_RGBA_HI 0x8F72
#define EGL_COLOR_ARGB_HI 0x8F73
#endif

#ifndef EGL_IMG_context_priority
#define EGL_IMG_context_priority
#define EGL_CONTEXT_PRIORITY_LEVEL_IMG 0x3100
#define EGL_CONTEXT_PRIORITY_HIGH_IMG 0x3101
#define EGL_CONTEXT_PRIORITY_MEDIUM_IMG 0x3102
#define EGL_CONTEXT_PRIORITY_LOW_IMG 0x3103
#endif

#ifndef EGL_IMG_image_plane_attribs
#define EGL_IMG_image_plane_attribs
#define EGL_NATIVE_BUFFER_MULTIPLANE_SEPARATE_IMG 0x3105
#define EGL_NATIVE_BUFFER_PLANE_OFFSET_IMG 0x3106
#endif

#ifndef EGL_KHR_cl_event
#define EGL_KHR_cl_event
#define EGL_CL_EVENT_HANDLE_KHR 0x309C
#define EGL_SYNC_CL_EVENT_KHR 0x30FE
#define EGL_SYNC_CL_EVENT_COMPLETE_KHR 0x30FF
#endif

#ifndef EGL_KHR_cl_event2
#define EGL_KHR_cl_event2
/* reuse EGL_CL_EVENT_HANDLE_KHR */
/* reuse EGL_SYNC_CL_EVENT_KHR */
/* reuse EGL_SYNC_CL_EVENT_COMPLETE_KHR */
#endif
extern EGLSyncKHR (APIENTRYP geglCreateSync64KHR)(EGLDisplay dpy, EGLenum type, const EGLAttribKHR *attrib_list);

#ifndef EGL_KHR_config_attribs
#define EGL_KHR_config_attribs
#define EGL_CONFORMANT_KHR 0x3042
#define EGL_VG_COLORSPACE_LINEAR_BIT_KHR 0x0020
#define EGL_VG_ALPHA_FORMAT_PRE_BIT_KHR 0x0040
#endif

#ifndef EGL_KHR_client_get_all_proc_addresses
#define EGL_KHR_client_get_all_proc_addresses
#endif

#ifndef EGL_KHR_context_flush_control
#define EGL_KHR_context_flush_control
#define EGL_CONTEXT_RELEASE_BEHAVIOR_NONE_KHR 0
#define EGL_CONTEXT_RELEASE_BEHAVIOR_KHR 0x2097
#define EGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_KHR 0x2098
#endif

#ifndef EGL_KHR_create_context
#define EGL_KHR_create_context
#define EGL_CONTEXT_MAJOR_VERSION_KHR 0x3098
#define EGL_CONTEXT_MINOR_VERSION_KHR 0x30FB
#define EGL_CONTEXT_FLAGS_KHR 0x30FC
#define EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR 0x30FD
#define EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR 0x31BD
#define EGL_NO_RESET_NOTIFICATION_KHR 0x31BE
#define EGL_LOSE_CONTEXT_ON_RESET_KHR 0x31BF
#define EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR 0x00000001
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR 0x00000002
#define EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR 0x00000004
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR 0x00000001
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR 0x00000002
/* reuse EGL_OPENGL_ES3_BIT */
#define EGL_OPENGL_ES3_BIT_KHR 0x00000040
#endif

#ifndef EGL_KHR_create_context_no_error
#define EGL_KHR_create_context_no_error
#define EGL_CONTEXT_OPENGL_NO_ERROR_KHR 0x31B3
#endif

#ifndef EGL_KHR_debug
#define EGL_KHR_debug
#define EGL_OBJECT_THREAD_KHR 0x33B0
#define EGL_OBJECT_DISPLAY_KHR 0x33B1
#define EGL_OBJECT_CONTEXT_KHR 0x33B2
#define EGL_OBJECT_SURFACE_KHR 0x33B3
#define EGL_OBJECT_IMAGE_KHR 0x33B4
#define EGL_OBJECT_SYNC_KHR 0x33B5
#define EGL_OBJECT_STREAM_KHR 0x33B6
#define EGL_DEBUG_MSG_CRITICAL_KHR 0x33B9
#define EGL_DEBUG_MSG_ERROR_KHR 0x33BA
#define EGL_DEBUG_MSG_WARN_KHR 0x33BB
#define EGL_DEBUG_MSG_INFO_KHR 0x33BC
#define EGL_DEBUG_CALLBACK_KHR 0x33B8
#endif
extern EGLint (APIENTRYP geglDebugMessageControlKHR)(EGLDEBUGPROCKHR callback, const EGLAttrib *attrib_list);
extern EGLBoolean (APIENTRYP geglQueryDebugKHR)(EGLint attribute, EGLAttrib *value);
extern EGLint (APIENTRYP geglLabelObjectKHR)(EGLDisplay display, EGLenum objectType, EGLObjectKHR object, EGLLabelKHR label);

#ifndef EGL_KHR_fence_sync
#define EGL_KHR_fence_sync
#define EGL_SYNC_PRIOR_COMMANDS_COMPLETE_KHR 0x30F0
#define EGL_SYNC_CONDITION_KHR 0x30F8
#define EGL_SYNC_FENCE_KHR 0x30F9
#endif
extern EGLSyncKHR (APIENTRYP geglCreateSyncKHR)(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list);
extern EGLBoolean (APIENTRYP geglDestroySyncKHR)(EGLDisplay dpy, EGLSyncKHR sync);
extern EGLint (APIENTRYP geglClientWaitSyncKHR)(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout);
extern EGLBoolean (APIENTRYP geglGetSyncAttribKHR)(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint *value);

#ifndef EGL_KHR_get_all_proc_addresses
#define EGL_KHR_get_all_proc_addresses
#endif

#ifndef EGL_KHR_gl_colorspace
#define EGL_KHR_gl_colorspace
#define EGL_GL_COLORSPACE_KHR 0x309D
#define EGL_GL_COLORSPACE_SRGB_KHR 0x3089
#define EGL_GL_COLORSPACE_LINEAR_KHR 0x308A
#endif

#ifndef EGL_KHR_gl_renderbuffer_image
#define EGL_KHR_gl_renderbuffer_image
#define EGL_GL_RENDERBUFFER_KHR 0x30B9
#endif

#ifndef EGL_KHR_gl_texture_2D_image
#define EGL_KHR_gl_texture_2D_image
#define EGL_GL_TEXTURE_2D_KHR 0x30B1
#define EGL_GL_TEXTURE_LEVEL_KHR 0x30BC
#endif

#ifndef EGL_KHR_gl_texture_3D_image
#define EGL_KHR_gl_texture_3D_image
#define EGL_GL_TEXTURE_3D_KHR 0x30B2
#define EGL_GL_TEXTURE_ZOFFSET_KHR 0x30BD
#endif

#ifndef EGL_KHR_gl_texture_cubemap_image
#define EGL_KHR_gl_texture_cubemap_image
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR 0x30B3
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR 0x30B4
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR 0x30B5
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR 0x30B6
#define EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR 0x30B7
#define EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR 0x30B8
#endif

#ifndef EGL_KHR_image
#define EGL_KHR_image
#define EGL_NATIVE_PIXMAP_KHR 0x30B0
#define EGL_NO_IMAGE_KHR ((EGLImageKHR)0)
#endif
extern EGLImageKHR (APIENTRYP geglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
extern EGLBoolean (APIENTRYP geglDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image);

#ifndef EGL_KHR_image_base
#define EGL_KHR_image_base
#define EGL_IMAGE_PRESERVED_KHR 0x30D2
/* reuse EGL_NO_IMAGE_KHR */
#endif
/* reuse EGLImageKHR (APIENTRYP geglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list) */
/* reuse EGLBoolean (APIENTRYP geglDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image) */

#ifndef EGL_KHR_image_pixmap
#define EGL_KHR_image_pixmap
/* reuse EGL_NATIVE_PIXMAP_KHR */
#endif

#ifndef EGL_KHR_lock_surface
#define EGL_KHR_lock_surface
#define EGL_READ_SURFACE_BIT_KHR 0x0001
#define EGL_WRITE_SURFACE_BIT_KHR 0x0002
#define EGL_LOCK_SURFACE_BIT_KHR 0x0080
#define EGL_OPTIMAL_FORMAT_BIT_KHR 0x0100
#define EGL_MATCH_FORMAT_KHR 0x3043
#define EGL_FORMAT_RGB_565_EXACT_KHR 0x30C0
#define EGL_FORMAT_RGB_565_KHR 0x30C1
#define EGL_FORMAT_RGBA_8888_EXACT_KHR 0x30C2
#define EGL_FORMAT_RGBA_8888_KHR 0x30C3
#define EGL_MAP_PRESERVE_PIXELS_KHR 0x30C4
#define EGL_LOCK_USAGE_HINT_KHR 0x30C5
#define EGL_BITMAP_POINTER_KHR 0x30C6
#define EGL_BITMAP_PITCH_KHR 0x30C7
#define EGL_BITMAP_ORIGIN_KHR 0x30C8
#define EGL_BITMAP_PIXEL_RED_OFFSET_KHR 0x30C9
#define EGL_BITMAP_PIXEL_GREEN_OFFSET_KHR 0x30CA
#define EGL_BITMAP_PIXEL_BLUE_OFFSET_KHR 0x30CB
#define EGL_BITMAP_PIXEL_ALPHA_OFFSET_KHR 0x30CC
#define EGL_BITMAP_PIXEL_LUMINANCE_OFFSET_KHR 0x30CD
#define EGL_LOWER_LEFT_KHR 0x30CE
#define EGL_UPPER_LEFT_KHR 0x30CF
#endif
extern EGLBoolean (APIENTRYP geglLockSurfaceKHR)(EGLDisplay dpy, EGLSurface surface, const EGLint *attrib_list);
extern EGLBoolean (APIENTRYP geglUnlockSurfaceKHR)(EGLDisplay dpy, EGLSurface surface);

#ifndef EGL_KHR_lock_surface2
#define EGL_KHR_lock_surface2
#define EGL_BITMAP_PIXEL_SIZE_KHR 0x3110
#endif

#ifndef EGL_KHR_lock_surface3
#define EGL_KHR_lock_surface3
/* reuse EGL_READ_SURFACE_BIT_KHR */
/* reuse EGL_WRITE_SURFACE_BIT_KHR */
/* reuse EGL_LOCK_SURFACE_BIT_KHR */
/* reuse EGL_OPTIMAL_FORMAT_BIT_KHR */
/* reuse EGL_MATCH_FORMAT_KHR */
/* reuse EGL_FORMAT_RGB_565_EXACT_KHR */
/* reuse EGL_FORMAT_RGB_565_KHR */
/* reuse EGL_FORMAT_RGBA_8888_EXACT_KHR */
/* reuse EGL_FORMAT_RGBA_8888_KHR */
/* reuse EGL_MAP_PRESERVE_PIXELS_KHR */
/* reuse EGL_LOCK_USAGE_HINT_KHR */
/* reuse EGL_BITMAP_PITCH_KHR */
/* reuse EGL_BITMAP_ORIGIN_KHR */
/* reuse EGL_BITMAP_PIXEL_RED_OFFSET_KHR */
/* reuse EGL_BITMAP_PIXEL_GREEN_OFFSET_KHR */
/* reuse EGL_BITMAP_PIXEL_BLUE_OFFSET_KHR */
/* reuse EGL_BITMAP_PIXEL_ALPHA_OFFSET_KHR */
/* reuse EGL_BITMAP_PIXEL_LUMINANCE_OFFSET_KHR */
/* reuse EGL_BITMAP_PIXEL_SIZE_KHR */
/* reuse EGL_BITMAP_POINTER_KHR */
/* reuse EGL_LOWER_LEFT_KHR */
/* reuse EGL_UPPER_LEFT_KHR */
#endif
/* reuse EGLBoolean (APIENTRYP geglLockSurfaceKHR)(EGLDisplay dpy, EGLSurface surface, const EGLint *attrib_list) */
/* reuse EGLBoolean (APIENTRYP geglUnlockSurfaceKHR)(EGLDisplay dpy, EGLSurface surface) */
extern EGLBoolean (APIENTRYP geglQuerySurface64KHR)(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLAttribKHR *value);

#ifndef EGL_KHR_mutable_render_buffer
#define EGL_KHR_mutable_render_buffer
#define EGL_MUTABLE_RENDER_BUFFER_BIT_KHR 0x1000
#endif

#ifndef EGL_KHR_no_config_context
#define EGL_KHR_no_config_context
#define EGL_NO_CONFIG_KHR ((EGLConfig)0)
#endif

#ifndef EGL_KHR_partial_update
#define EGL_KHR_partial_update
#define EGL_BUFFER_AGE_KHR 0x313D
#endif
extern EGLBoolean (APIENTRYP geglSetDamageRegionKHR)(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects);

#ifndef EGL_KHR_platform_android
#define EGL_KHR_platform_android
#define EGL_PLATFORM_ANDROID_KHR 0x3141
#endif

#ifndef EGL_KHR_platform_gbm
#define EGL_KHR_platform_gbm
#define EGL_PLATFORM_GBM_KHR 0x31D7
#endif

#ifndef EGL_KHR_platform_wayland
#define EGL_KHR_platform_wayland
#define EGL_PLATFORM_WAYLAND_KHR 0x31D8
#endif

#ifndef EGL_KHR_platform_x11
#define EGL_KHR_platform_x11
#define EGL_PLATFORM_X11_KHR 0x31D5
#define EGL_PLATFORM_X11_SCREEN_KHR 0x31D6
#endif

#ifndef EGL_KHR_reusable_sync
#define EGL_KHR_reusable_sync
#define EGL_SYNC_STATUS_KHR 0x30F1
#define EGL_SIGNALED_KHR 0x30F2
#define EGL_UNSIGNALED_KHR 0x30F3
#define EGL_TIMEOUT_EXPIRED_KHR 0x30F5
#define EGL_CONDITION_SATISFIED_KHR 0x30F6
#define EGL_SYNC_TYPE_KHR 0x30F7
#define EGL_SYNC_REUSABLE_KHR 0x30FA
#define EGL_SYNC_FLUSH_COMMANDS_BIT_KHR 0x0001
#define EGL_FOREVER_KHR 0xFFFFFFFFFFFFFFFF
#define EGL_NO_SYNC_KHR ((EGLSyncKHR)0)
#endif
/* reuse EGLSyncKHR (APIENTRYP geglCreateSyncKHR)(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list) */
/* reuse EGLBoolean (APIENTRYP geglDestroySyncKHR)(EGLDisplay dpy, EGLSyncKHR sync) */
/* reuse EGLint (APIENTRYP geglClientWaitSyncKHR)(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout) */
extern EGLBoolean (APIENTRYP geglSignalSyncKHR)(EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode);
/* reuse EGLBoolean (APIENTRYP geglGetSyncAttribKHR)(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint *value) */

#ifndef EGL_KHR_stream
#define EGL_KHR_stream
#define EGL_NO_STREAM_KHR ((EGLStreamKHR)0)
#define EGL_CONSUMER_LATENCY_USEC_KHR 0x3210
#define EGL_PRODUCER_FRAME_KHR 0x3212
#define EGL_CONSUMER_FRAME_KHR 0x3213
#define EGL_STREAM_STATE_KHR 0x3214
#define EGL_STREAM_STATE_CREATED_KHR 0x3215
#define EGL_STREAM_STATE_CONNECTING_KHR 0x3216
#define EGL_STREAM_STATE_EMPTY_KHR 0x3217
#define EGL_STREAM_STATE_NEW_FRAME_AVAILABLE_KHR 0x3218
#define EGL_STREAM_STATE_OLD_FRAME_AVAILABLE_KHR 0x3219
#define EGL_STREAM_STATE_DISCONNECTED_KHR 0x321A
#define EGL_BAD_STREAM_KHR 0x321B
#define EGL_BAD_STATE_KHR 0x321C
#endif
extern EGLStreamKHR (APIENTRYP geglCreateStreamKHR)(EGLDisplay dpy, const EGLint *attrib_list);
extern EGLBoolean (APIENTRYP geglDestroyStreamKHR)(EGLDisplay dpy, EGLStreamKHR stream);
extern EGLBoolean (APIENTRYP geglStreamAttribKHR)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint value);
extern EGLBoolean (APIENTRYP geglQueryStreamKHR)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint *value);
extern EGLBoolean (APIENTRYP geglQueryStreamu64KHR)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLuint64KHR *value);

#ifndef EGL_KHR_stream_attrib
#define EGL_KHR_stream_attrib
/* reuse EGL_CONSUMER_LATENCY_USEC_KHR */
/* reuse EGL_STREAM_STATE_KHR */
/* reuse EGL_STREAM_STATE_CREATED_KHR */
/* reuse EGL_STREAM_STATE_CONNECTING_KHR */
#endif
extern EGLStreamKHR (APIENTRYP geglCreateStreamAttribKHR)(EGLDisplay dpy, const EGLAttrib *attrib_list);
extern EGLBoolean (APIENTRYP geglSetStreamAttribKHR)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLAttrib value);
extern EGLBoolean (APIENTRYP geglQueryStreamAttribKHR)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLAttrib *value);
extern EGLBoolean (APIENTRYP geglStreamConsumerAcquireAttribKHR)(EGLDisplay dpy, EGLStreamKHR stream, const EGLAttrib *attrib_list);
extern EGLBoolean (APIENTRYP geglStreamConsumerReleaseAttribKHR)(EGLDisplay dpy, EGLStreamKHR stream, const EGLAttrib *attrib_list);

#ifndef EGL_KHR_stream_consumer_gltexture
#define EGL_KHR_stream_consumer_gltexture
#define EGL_CONSUMER_ACQUIRE_TIMEOUT_USEC_KHR 0x321E
#endif
extern EGLBoolean (APIENTRYP geglStreamConsumerGLTextureExternalKHR)(EGLDisplay dpy, EGLStreamKHR stream);
extern EGLBoolean (APIENTRYP geglStreamConsumerAcquireKHR)(EGLDisplay dpy, EGLStreamKHR stream);
extern EGLBoolean (APIENTRYP geglStreamConsumerReleaseKHR)(EGLDisplay dpy, EGLStreamKHR stream);

#ifndef EGL_KHR_stream_cross_process_fd
#define EGL_KHR_stream_cross_process_fd
#define EGL_NO_FILE_DESCRIPTOR_KHR ((EGLNativeFileDescriptorKHR)(-1))
#endif
extern EGLNativeFileDescriptorKHR (APIENTRYP geglGetStreamFileDescriptorKHR)(EGLDisplay dpy, EGLStreamKHR stream);
extern EGLStreamKHR (APIENTRYP geglCreateStreamFromFileDescriptorKHR)(EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor);

#ifndef EGL_KHR_stream_fifo
#define EGL_KHR_stream_fifo
#define EGL_STREAM_FIFO_LENGTH_KHR 0x31FC
#define EGL_STREAM_TIME_NOW_KHR 0x31FD
#define EGL_STREAM_TIME_CONSUMER_KHR 0x31FE
#define EGL_STREAM_TIME_PRODUCER_KHR 0x31FF
#endif
extern EGLBoolean (APIENTRYP geglQueryStreamTimeKHR)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLTimeKHR *value);

#ifndef EGL_KHR_stream_producer_aldatalocator
#define EGL_KHR_stream_producer_aldatalocator
#endif

#ifndef EGL_KHR_stream_producer_eglsurface
#define EGL_KHR_stream_producer_eglsurface
#define EGL_STREAM_BIT_KHR 0x0800
#endif
extern EGLSurface (APIENTRYP geglCreateStreamProducerSurfaceKHR)(EGLDisplay dpy, EGLConfig config, EGLStreamKHR stream, const EGLint *attrib_list);

#ifndef EGL_KHR_surfaceless_context
#define EGL_KHR_surfaceless_context
#endif

#ifndef EGL_KHR_swap_buffers_with_damage
#define EGL_KHR_swap_buffers_with_damage
#endif
extern EGLBoolean (APIENTRYP geglSwapBuffersWithDamageKHR)(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects);

#ifndef EGL_KHR_vg_parent_image
#define EGL_KHR_vg_parent_image
#define EGL_VG_PARENT_IMAGE_KHR 0x30BA
#endif

#ifndef EGL_KHR_wait_sync
#define EGL_KHR_wait_sync
#endif
extern EGLint (APIENTRYP geglWaitSyncKHR)(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags);

#ifndef EGL_MESA_drm_image
#define EGL_MESA_drm_image
#define EGL_DRM_BUFFER_FORMAT_MESA 0x31D0
#define EGL_DRM_BUFFER_USE_MESA 0x31D1
#define EGL_DRM_BUFFER_FORMAT_ARGB32_MESA 0x31D2
#define EGL_DRM_BUFFER_MESA 0x31D3
#define EGL_DRM_BUFFER_STRIDE_MESA 0x31D4
#define EGL_DRM_BUFFER_USE_SCANOUT_MESA 0x00000001
#define EGL_DRM_BUFFER_USE_SHARE_MESA 0x00000002
#endif
extern EGLImageKHR (APIENTRYP geglCreateDRMImageMESA)(EGLDisplay dpy, const EGLint *attrib_list);
extern EGLBoolean (APIENTRYP geglExportDRMImageMESA)(EGLDisplay dpy, EGLImageKHR image, EGLint *name, EGLint *handle, EGLint *stride);

#ifndef EGL_MESA_image_dma_buf_export
#define EGL_MESA_image_dma_buf_export
#endif
extern EGLBoolean (APIENTRYP geglExportDMABUFImageQueryMESA)(EGLDisplay dpy, EGLImageKHR image, int *fourcc, int *num_planes, EGLuint64KHR *modifiers);
extern EGLBoolean (APIENTRYP geglExportDMABUFImageMESA)(EGLDisplay dpy, EGLImageKHR image, int *fds, EGLint *strides, EGLint *offsets);

#ifndef EGL_MESA_platform_gbm
#define EGL_MESA_platform_gbm
#define EGL_PLATFORM_GBM_MESA 0x31D7
#endif

#ifndef EGL_MESA_platform_surfaceless
#define EGL_MESA_platform_surfaceless
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD
#endif

#ifndef EGL_NOK_swap_region
#define EGL_NOK_swap_region
#endif
extern EGLBoolean (APIENTRYP geglSwapBuffersRegionNOK)(EGLDisplay dpy, EGLSurface surface, EGLint numRects, const EGLint *rects);

#ifndef EGL_NOK_swap_region2
#define EGL_NOK_swap_region2
#endif
extern EGLBoolean (APIENTRYP geglSwapBuffersRegion2NOK)(EGLDisplay dpy, EGLSurface surface, EGLint numRects, const EGLint *rects);

#ifndef EGL_NOK_texture_from_pixmap
#define EGL_NOK_texture_from_pixmap
#define EGL_Y_INVERTED_NOK 0x307F
#endif

#ifndef EGL_NV_3dvision_surface
#define EGL_NV_3dvision_surface
#define EGL_AUTO_STEREO_NV 0x3136
#endif

#ifndef EGL_NV_coverage_sample
#define EGL_NV_coverage_sample
#define EGL_COVERAGE_BUFFERS_NV 0x30E0
#define EGL_COVERAGE_SAMPLES_NV 0x30E1
#endif

#ifndef EGL_NV_coverage_sample_resolve
#define EGL_NV_coverage_sample_resolve
#define EGL_COVERAGE_SAMPLE_RESOLVE_NV 0x3131
#define EGL_COVERAGE_SAMPLE_RESOLVE_DEFAULT_NV 0x3132
#define EGL_COVERAGE_SAMPLE_RESOLVE_NONE_NV 0x3133
#endif

#ifndef EGL_NV_cuda_event
#define EGL_NV_cuda_event
#define EGL_CUDA_EVENT_HANDLE_NV 0x323B
#define EGL_SYNC_CUDA_EVENT_NV 0x323C
#define EGL_SYNC_CUDA_EVENT_COMPLETE_NV 0x323D
#endif

#ifndef EGL_NV_depth_nonlinear
#define EGL_NV_depth_nonlinear
#define EGL_DEPTH_ENCODING_NV 0x30E2
#define EGL_DEPTH_ENCODING_NONE_NV 0
#define EGL_DEPTH_ENCODING_NONLINEAR_NV 0x30E3
#endif

#ifndef EGL_NV_device_cuda
#define EGL_NV_device_cuda
#define EGL_CUDA_DEVICE_NV 0x323A
#endif

#ifndef EGL_NV_native_query
#define EGL_NV_native_query
#endif
extern EGLBoolean (APIENTRYP geglQueryNativeDisplayNV)(EGLDisplay dpy, EGLNativeDisplayType *display_id);
extern EGLBoolean (APIENTRYP geglQueryNativeWindowNV)(EGLDisplay dpy, EGLSurface surf, EGLNativeWindowType *window);
extern EGLBoolean (APIENTRYP geglQueryNativePixmapNV)(EGLDisplay dpy, EGLSurface surf, EGLNativePixmapType *pixmap);

#ifndef EGL_NV_post_convert_rounding
#define EGL_NV_post_convert_rounding
#endif

#ifndef EGL_NV_post_sub_buffer
#define EGL_NV_post_sub_buffer
#define EGL_POST_SUB_BUFFER_SUPPORTED_NV 0x30BE
#endif
extern EGLBoolean (APIENTRYP geglPostSubBufferNV)(EGLDisplay dpy, EGLSurface surface, EGLint x, EGLint y, EGLint width, EGLint height);

#ifndef EGL_NV_robustness_video_memory_purge
#define EGL_NV_robustness_video_memory_purge
#define EGL_GENERATE_RESET_ON_VIDEO_MEMORY_PURGE_NV 0x334C
#endif

#ifndef EGL_NV_stream_consumer_gltexture_yuv
#define EGL_NV_stream_consumer_gltexture_yuv
#define EGL_YUV_PLANE0_TEXTURE_UNIT_NV 0x332C
#define EGL_YUV_PLANE1_TEXTURE_UNIT_NV 0x332D
#define EGL_YUV_PLANE2_TEXTURE_UNIT_NV 0x332E
/* reuse EGL_YUV_NUMBER_OF_PLANES_EXT */
/* reuse EGL_YUV_BUFFER_EXT */
#endif
extern EGLBoolean (APIENTRYP geglStreamConsumerGLTextureExternalAttribsNV)(EGLDisplay dpy, EGLStreamKHR stream, EGLAttrib *attrib_list);

#ifndef EGL_NV_stream_cross_object
#define EGL_NV_stream_cross_object
#define EGL_STREAM_CROSS_OBJECT_NV 0x334D
#endif

#ifndef EGL_NV_stream_cross_display
#define EGL_NV_stream_cross_display
#define EGL_STREAM_CROSS_DISPLAY_NV 0x334E
#endif

#ifndef EGL_NV_stream_cross_partition
#define EGL_NV_stream_cross_partition
#define EGL_STREAM_CROSS_PARTITION_NV 0x323F
#endif

#ifndef EGL_NV_stream_cross_process
#define EGL_NV_stream_cross_process
#define EGL_STREAM_CROSS_PROCESS_NV 0x3245
#endif

#ifndef EGL_NV_stream_cross_system
#define EGL_NV_stream_cross_system
#define EGL_STREAM_CROSS_SYSTEM_NV 0x334F
#endif

#ifndef EGL_NV_stream_fifo_next
#define EGL_NV_stream_fifo_next
#define EGL_PENDING_FRAME_NV 0x3329
#define EGL_STREAM_TIME_PENDING_NV 0x332A
#endif

#ifndef EGL_NV_stream_fifo_synchronous
#define EGL_NV_stream_fifo_synchronous
#define EGL_STREAM_FIFO_SYNCHRONOUS_NV 0x3336
#endif

#ifndef EGL_NV_stream_frame_limits
#define EGL_NV_stream_frame_limits
#define EGL_PRODUCER_MAX_FRAME_HINT_NV 0x3337
#define EGL_CONSUMER_MAX_FRAME_HINT_NV 0x3338
#endif

#ifndef EGL_NV_stream_metadata
#define EGL_NV_stream_metadata
#define EGL_MAX_STREAM_METADATA_BLOCKS_NV 0x3250
#define EGL_MAX_STREAM_METADATA_BLOCK_SIZE_NV 0x3251
#define EGL_MAX_STREAM_METADATA_TOTAL_SIZE_NV 0x3252
#define EGL_PRODUCER_METADATA_NV 0x3253
#define EGL_CONSUMER_METADATA_NV 0x3254
#define EGL_PENDING_METADATA_NV 0x3328
#define EGL_METADATA0_SIZE_NV 0x3255
#define EGL_METADATA1_SIZE_NV 0x3256
#define EGL_METADATA2_SIZE_NV 0x3257
#define EGL_METADATA3_SIZE_NV 0x3258
#define EGL_METADATA0_TYPE_NV 0x3259
#define EGL_METADATA1_TYPE_NV 0x325A
#define EGL_METADATA2_TYPE_NV 0x325B
#define EGL_METADATA3_TYPE_NV 0x325C
#endif
extern EGLBoolean (APIENTRYP geglQueryDisplayAttribNV)(EGLDisplay dpy, EGLint attribute, EGLAttrib *value);
extern EGLBoolean (APIENTRYP geglSetStreamMetadataNV)(EGLDisplay dpy, EGLStreamKHR stream, EGLint n, EGLint offset, EGLint size, const void *data);
extern EGLBoolean (APIENTRYP geglQueryStreamMetadataNV)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum name, EGLint n, EGLint offset, EGLint size, void *data);

#ifndef EGL_NV_stream_reset
#define EGL_NV_stream_reset
#define EGL_SUPPORT_RESET_NV 0x3334
#define EGL_SUPPORT_REUSE_NV 0x3335
#endif
extern EGLBoolean (APIENTRYP geglResetStreamNV)(EGLDisplay dpy, EGLStreamKHR stream);

#ifndef EGL_NV_stream_remote
#define EGL_NV_stream_remote
#define EGL_STREAM_STATE_INITIALIZING_NV 0x3240
#define EGL_STREAM_TYPE_NV 0x3241
#define EGL_STREAM_PROTOCOL_NV 0x3242
#define EGL_STREAM_ENDPOINT_NV 0x3243
#define EGL_STREAM_LOCAL_NV 0x3244
#define EGL_STREAM_PRODUCER_NV 0x3247
#define EGL_STREAM_CONSUMER_NV 0x3248
#define EGL_STREAM_PROTOCOL_FD_NV 0x3246
#endif

#ifndef EGL_NV_stream_socket
#define EGL_NV_stream_socket
#define EGL_STREAM_PROTOCOL_SOCKET_NV 0x324B
#define EGL_SOCKET_HANDLE_NV 0x324C
#define EGL_SOCKET_TYPE_NV 0x324D
#endif

#ifndef EGL_NV_stream_socket_inet
#define EGL_NV_stream_socket_inet
#define EGL_SOCKET_TYPE_INET_NV 0x324F
#endif

#ifndef EGL_NV_stream_socket_unix
#define EGL_NV_stream_socket_unix
#define EGL_SOCKET_TYPE_UNIX_NV 0x324E
#endif

#ifndef EGL_NV_stream_sync
#define EGL_NV_stream_sync
/* reuse EGL_SYNC_TYPE_KHR */
#define EGL_SYNC_NEW_FRAME_NV 0x321F
#endif
extern EGLSyncKHR (APIENTRYP geglCreateStreamSyncNV)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum type, const EGLint *attrib_list);

#ifndef EGL_NV_sync
#define EGL_NV_sync
#define EGL_SYNC_PRIOR_COMMANDS_COMPLETE_NV 0x30E6
#define EGL_SYNC_STATUS_NV 0x30E7
#define EGL_SIGNALED_NV 0x30E8
#define EGL_UNSIGNALED_NV 0x30E9
#define EGL_SYNC_FLUSH_COMMANDS_BIT_NV 0x0001
#define EGL_FOREVER_NV 0xFFFFFFFFFFFFFFFF
#define EGL_ALREADY_SIGNALED_NV 0x30EA
#define EGL_TIMEOUT_EXPIRED_NV 0x30EB
#define EGL_CONDITION_SATISFIED_NV 0x30EC
#define EGL_SYNC_TYPE_NV 0x30ED
#define EGL_SYNC_CONDITION_NV 0x30EE
#define EGL_SYNC_FENCE_NV 0x30EF
#define EGL_NO_SYNC_NV ((EGLSyncNV)0)
#endif
extern EGLSyncNV (APIENTRYP geglCreateFenceSyncNV)(EGLDisplay dpy, EGLenum condition, const EGLint *attrib_list);
extern EGLBoolean (APIENTRYP geglDestroySyncNV)(EGLSyncNV sync);
extern EGLBoolean (APIENTRYP geglFenceNV)(EGLSyncNV sync);
extern EGLint (APIENTRYP geglClientWaitSyncNV)(EGLSyncNV sync, EGLint flags, EGLTimeNV timeout);
extern EGLBoolean (APIENTRYP geglSignalSyncNV)(EGLSyncNV sync, EGLenum mode);
extern EGLBoolean (APIENTRYP geglGetSyncAttribNV)(EGLSyncNV sync, EGLint attribute, EGLint *value);

#ifndef EGL_NV_system_time
#define EGL_NV_system_time
#endif
extern EGLuint64NV (APIENTRYP geglGetSystemTimeFrequencyNV)();
extern EGLuint64NV (APIENTRYP geglGetSystemTimeNV)();

#ifndef EGL_TIZEN_image_native_buffer
#define EGL_TIZEN_image_native_buffer
#define EGL_NATIVE_BUFFER_TIZEN 0x32A0
#endif

#ifndef EGL_TIZEN_image_native_surface
#define EGL_TIZEN_image_native_surface
#define EGL_NATIVE_SURFACE_TIZEN 0x32A1
#endif

/* 124 extensions */
typedef struct {
	int _EGL_ANDROID_blob_cache : 1;
	int _EGL_ANDROID_create_native_client_buffer : 1;
	int _EGL_ANDROID_framebuffer_target : 1;
	int _EGL_ANDROID_front_buffer_auto_refresh : 1;
	int _EGL_ANDROID_image_native_buffer : 1;
	int _EGL_ANDROID_native_fence_sync : 1;
	int _EGL_ANDROID_presentation_time : 1;
	int _EGL_ANDROID_recordable : 1;
	int _EGL_ANGLE_d3d_share_handle_client_buffer : 1;
	int _EGL_ANGLE_device_d3d : 1;
	int _EGL_ANGLE_query_surface_pointer : 1;
	int _EGL_ANGLE_surface_d3d_texture_2d_share_handle : 1;
	int _EGL_ANGLE_window_fixed_size : 1;
	int _EGL_ARM_implicit_external_sync : 1;
	int _EGL_ARM_pixmap_multisample_discard : 1;
	int _EGL_EXT_buffer_age : 1;
	int _EGL_EXT_client_extensions : 1;
	int _EGL_EXT_create_context_robustness : 1;
	int _EGL_EXT_device_base : 1;
	int _EGL_EXT_device_drm : 1;
	int _EGL_EXT_device_enumeration : 1;
	int _EGL_EXT_device_openwf : 1;
	int _EGL_EXT_device_query : 1;
	int _EGL_EXT_gl_colorspace_bt2020_linear : 1;
	int _EGL_EXT_gl_colorspace_bt2020_pq : 1;
	int _EGL_EXT_gl_colorspace_scrgb_linear : 1;
	int _EGL_EXT_image_dma_buf_import : 1;
	int _EGL_EXT_image_dma_buf_import_modifiers : 1;
	int _EGL_EXT_multiview_window : 1;
	int _EGL_EXT_output_base : 1;
	int _EGL_EXT_output_drm : 1;
	int _EGL_EXT_output_openwf : 1;
	int _EGL_EXT_pixel_format_float : 1;
	int _EGL_EXT_platform_base : 1;
	int _EGL_EXT_platform_device : 1;
	int _EGL_EXT_platform_wayland : 1;
	int _EGL_EXT_platform_x11 : 1;
	int _EGL_EXT_protected_content : 1;
	int _EGL_EXT_protected_surface : 1;
	int _EGL_EXT_stream_consumer_egloutput : 1;
	int _EGL_EXT_surface_SMPTE2086_metadata : 1;
	int _EGL_EXT_swap_buffers_with_damage : 1;
	int _EGL_EXT_yuv_surface : 1;
	int _EGL_HI_clientpixmap : 1;
	int _EGL_HI_colorformats : 1;
	int _EGL_IMG_context_priority : 1;
	int _EGL_IMG_image_plane_attribs : 1;
	int _EGL_KHR_cl_event : 1;
	int _EGL_KHR_cl_event2 : 1;
	int _EGL_KHR_config_attribs : 1;
	int _EGL_KHR_client_get_all_proc_addresses : 1;
	int _EGL_KHR_context_flush_control : 1;
	int _EGL_KHR_create_context : 1;
	int _EGL_KHR_create_context_no_error : 1;
	int _EGL_KHR_debug : 1;
	int _EGL_KHR_fence_sync : 1;
	int _EGL_KHR_get_all_proc_addresses : 1;
	int _EGL_KHR_gl_colorspace : 1;
	int _EGL_KHR_gl_renderbuffer_image : 1;
	int _EGL_KHR_gl_texture_2D_image : 1;
	int _EGL_KHR_gl_texture_3D_image : 1;
	int _EGL_KHR_gl_texture_cubemap_image : 1;
	int _EGL_KHR_image : 1;
	int _EGL_KHR_image_base : 1;
	int _EGL_KHR_image_pixmap : 1;
	int _EGL_KHR_lock_surface : 1;
	int _EGL_KHR_lock_surface2 : 1;
	int _EGL_KHR_lock_surface3 : 1;
	int _EGL_KHR_mutable_render_buffer : 1;
	int _EGL_KHR_no_config_context : 1;
	int _EGL_KHR_partial_update : 1;
	int _EGL_KHR_platform_android : 1;
	int _EGL_KHR_platform_gbm : 1;
	int _EGL_KHR_platform_wayland : 1;
	int _EGL_KHR_platform_x11 : 1;
	int _EGL_KHR_reusable_sync : 1;
	int _EGL_KHR_stream : 1;
	int _EGL_KHR_stream_attrib : 1;
	int _EGL_KHR_stream_consumer_gltexture : 1;
	int _EGL_KHR_stream_cross_process_fd : 1;
	int _EGL_KHR_stream_fifo : 1;
	int _EGL_KHR_stream_producer_aldatalocator : 1;
	int _EGL_KHR_stream_producer_eglsurface : 1;
	int _EGL_KHR_surfaceless_context : 1;
	int _EGL_KHR_swap_buffers_with_damage : 1;
	int _EGL_KHR_vg_parent_image : 1;
	int _EGL_KHR_wait_sync : 1;
	int _EGL_MESA_drm_image : 1;
	int _EGL_MESA_image_dma_buf_export : 1;
	int _EGL_MESA_platform_gbm : 1;
	int _EGL_MESA_platform_surfaceless : 1;
	int _EGL_NOK_swap_region : 1;
	int _EGL_NOK_swap_region2 : 1;
	int _EGL_NOK_texture_from_pixmap : 1;
	int _EGL_NV_3dvision_surface : 1;
	int _EGL_NV_coverage_sample : 1;
	int _EGL_NV_coverage_sample_resolve : 1;
	int _EGL_NV_cuda_event : 1;
	int _EGL_NV_depth_nonlinear : 1;
	int _EGL_NV_device_cuda : 1;
	int _EGL_NV_native_query : 1;
	int _EGL_NV_post_convert_rounding : 1;
	int _EGL_NV_post_sub_buffer : 1;
	int _EGL_NV_robustness_video_memory_purge : 1;
	int _EGL_NV_stream_consumer_gltexture_yuv : 1;
	int _EGL_NV_stream_cross_object : 1;
	int _EGL_NV_stream_cross_display : 1;
	int _EGL_NV_stream_cross_partition : 1;
	int _EGL_NV_stream_cross_process : 1;
	int _EGL_NV_stream_cross_system : 1;
	int _EGL_NV_stream_fifo_next : 1;
	int _EGL_NV_stream_fifo_synchronous : 1;
	int _EGL_NV_stream_frame_limits : 1;
	int _EGL_NV_stream_metadata : 1;
	int _EGL_NV_stream_reset : 1;
	int _EGL_NV_stream_remote : 1;
	int _EGL_NV_stream_socket : 1;
	int _EGL_NV_stream_socket_inet : 1;
	int _EGL_NV_stream_socket_unix : 1;
	int _EGL_NV_stream_sync : 1;
	int _EGL_NV_sync : 1;
	int _EGL_NV_system_time : 1;
	int _EGL_TIZEN_image_native_buffer : 1;
	int _EGL_TIZEN_image_native_surface : 1;
} geglext_t;

extern geglext_t geglext;

extern void gegl_init(EGLDisplay display, int enableDebug);
extern void gegl_rebind(int enableDebug);

#ifdef __cplusplus
}
#endif

#endif /* __GEGL_H__ */
