/*********************************************************************************************
 *
 * gglx.h
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

#ifndef __GGLX_H__
#define __GGLX_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

#ifndef GLEXT_64_TYPES_DEFINED
/* This code block is duplicated in glext.h, so must be protected */
#define GLEXT_64_TYPES_DEFINED
/* Define int32_t, int64_t, and uint64_t types for UST/MSC */
/* (as used in the GLX_OML_sync_control extension). */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
#include <inttypes.h>
#elif defined(__sun__) || defined(__digital__)
#include <inttypes.h>
#if defined(__STDC__)
#if defined(__arch64__) || defined(_LP64)
typedef long int int64_t;
typedef unsigned long int uint64_t;
#else
typedef long long int int64_t;
typedef unsigned long long int uint64_t;
#endif /* __arch64__ */
#endif /* __STDC__ */
#elif defined( __VMS ) || defined(__sgi)
#include <inttypes.h>
#elif defined(__SCO__) || defined(__USLC__)
#include <stdint.h>
#elif defined(__UNIXOS2__) || defined(__SOL64__)
typedef long int int32_t;
typedef long long int int64_t;
typedef unsigned long long int uint64_t;
#elif defined(_WIN32) && defined(__GNUC__)
#include <stdint.h>
#elif defined(_WIN32)
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
/* Fallback if nothing above works */
#include <inttypes.h>
#endif
#endif
typedef XID GLXFBConfigID;
typedef struct __GLXFBConfigRec *GLXFBConfig;
typedef XID GLXContextID;
typedef struct __GLXcontextRec *GLXContext;
typedef XID GLXPixmap;
typedef XID GLXDrawable;
typedef XID GLXWindow;
typedef XID GLXPbuffer;
typedef void (APIENTRY *__GLXextFuncPtr)(void);
typedef XID GLXVideoCaptureDeviceNV;
typedef unsigned int GLXVideoDeviceNV;
typedef XID GLXVideoSourceSGIX;
typedef XID GLXFBConfigIDSGIX;
typedef struct __GLXFBConfigRec *GLXFBConfigSGIX;
typedef XID GLXPbufferSGIX;
typedef struct {
    int event_type;             /* GLX_DAMAGED or GLX_SAVED */
    int draw_type;              /* GLX_WINDOW or GLX_PBUFFER */
    unsigned long serial;       /* # of last request processed by server */
    Bool send_event;            /* true if this came for SendEvent request */
    Display *display;           /* display the event was read from */
    GLXDrawable drawable;       /* XID of Drawable */
    unsigned int buffer_mask;   /* mask indicating which buffers are affected */
    unsigned int aux_buffer;    /* which aux buffer was affected */
    int x, y;
    int width, height;
    int count;                  /* if nonzero, at least this many more */
} GLXPbufferClobberEvent;
typedef struct {
    int type;
    unsigned long serial;       /* # of last request processed by server */
    Bool send_event;            /* true if this came from a SendEvent request */
    Display *display;           /* Display the event was read from */
    GLXDrawable drawable;       /* drawable on which event was requested in event mask */
    int event_type;
    int64_t ust;
    int64_t msc;
    int64_t sbc;
} GLXBufferSwapComplete;
typedef union __GLXEvent {
    GLXPbufferClobberEvent glxpbufferclobber;
    GLXBufferSwapComplete glxbufferswapcomplete;
    long pad[24];
} GLXEvent;
typedef struct {
    int type;
    unsigned long serial;
    Bool send_event;
    Display *display;
    int extension;
    int evtype;
    GLXDrawable window;
    Bool stereo_tree;
} GLXStereoNotifyEventEXT;
typedef struct {
    int type;
    unsigned long serial;   /* # of last request processed by server */
    Bool send_event;        /* true if this came for SendEvent request */
    Display *display;       /* display the event was read from */
    GLXDrawable drawable;   /* i.d. of Drawable */
    int event_type;         /* GLX_DAMAGED_SGIX or GLX_SAVED_SGIX */
    int draw_type;          /* GLX_WINDOW_SGIX or GLX_PBUFFER_SGIX */
    unsigned int mask;      /* mask indicating which buffers are affected*/
    int x, y;
    int width, height;
    int count;              /* if nonzero, at least this many more */
} GLXBufferClobberEventSGIX;
typedef struct {
    char    pipeName[80]; /* Should be [GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX] */
    int     networkId;
} GLXHyperpipeNetworkSGIX;
typedef struct {
    char    pipeName[80]; /* Should be [GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX] */
    int     channel;
    unsigned int participationType;
    int     timeSlice;
} GLXHyperpipeConfigSGIX;
typedef struct {
    char pipeName[80]; /* Should be [GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX] */
    int srcXOrigin, srcYOrigin, srcWidth, srcHeight;
    int destXOrigin, destYOrigin, destWidth, destHeight;
} GLXPipeRect;
typedef struct {
    char pipeName[80]; /* Should be [GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX] */
    int XOrigin, YOrigin, maxHeight, maxWidth;
} GLXPipeRectLimits;

#ifndef GLX_VERSION_1_3
#define GLX_VERSION_1_3
#define GLX_WINDOW_BIT 0x00000001
#define GLX_PIXMAP_BIT 0x00000002
#define GLX_PBUFFER_BIT 0x00000004
#define GLX_RGBA_BIT 0x00000001
#define GLX_COLOR_INDEX_BIT 0x00000002
#define GLX_PBUFFER_CLOBBER_MASK 0x08000000
#define GLX_FRONT_LEFT_BUFFER_BIT 0x00000001
#define GLX_FRONT_RIGHT_BUFFER_BIT 0x00000002
#define GLX_BACK_LEFT_BUFFER_BIT 0x00000004
#define GLX_BACK_RIGHT_BUFFER_BIT 0x00000008
#define GLX_AUX_BUFFERS_BIT 0x00000010
#define GLX_DEPTH_BUFFER_BIT 0x00000020
#define GLX_STENCIL_BUFFER_BIT 0x00000040
#define GLX_ACCUM_BUFFER_BIT 0x00000080
#define GLX_CONFIG_CAVEAT 0x20
#define GLX_X_VISUAL_TYPE 0x22
#define GLX_TRANSPARENT_TYPE 0x23
#define GLX_TRANSPARENT_INDEX_VALUE 0x24
#define GLX_TRANSPARENT_RED_VALUE 0x25
#define GLX_TRANSPARENT_GREEN_VALUE 0x26
#define GLX_TRANSPARENT_BLUE_VALUE 0x27
#define GLX_TRANSPARENT_ALPHA_VALUE 0x28
#define GLX_DONT_CARE 0xFFFFFFFF
#define GLX_NONE 0x8000
#define GLX_SLOW_CONFIG 0x8001
#define GLX_TRUE_COLOR 0x8002
#define GLX_DIRECT_COLOR 0x8003
#define GLX_PSEUDO_COLOR 0x8004
#define GLX_STATIC_COLOR 0x8005
#define GLX_GRAY_SCALE 0x8006
#define GLX_STATIC_GRAY 0x8007
#define GLX_TRANSPARENT_RGB 0x8008
#define GLX_TRANSPARENT_INDEX 0x8009
#define GLX_VISUAL_ID 0x800B
#define GLX_SCREEN 0x800C
#define GLX_NON_CONFORMANT_CONFIG 0x800D
#define GLX_DRAWABLE_TYPE 0x8010
#define GLX_RENDER_TYPE 0x8011
#define GLX_X_RENDERABLE 0x8012
#define GLX_FBCONFIG_ID 0x8013
#define GLX_RGBA_TYPE 0x8014
#define GLX_COLOR_INDEX_TYPE 0x8015
#define GLX_MAX_PBUFFER_WIDTH 0x8016
#define GLX_MAX_PBUFFER_HEIGHT 0x8017
#define GLX_MAX_PBUFFER_PIXELS 0x8018
#define GLX_PRESERVED_CONTENTS 0x801B
#define GLX_LARGEST_PBUFFER 0x801C
#define GLX_WIDTH 0x801D
#define GLX_HEIGHT 0x801E
#define GLX_EVENT_MASK 0x801F
#define GLX_DAMAGED 0x8020
#define GLX_SAVED 0x8021
#define GLX_WINDOW 0x8022
#define GLX_PBUFFER 0x8023
#define GLX_PBUFFER_HEIGHT 0x8040
#define GLX_PBUFFER_WIDTH 0x8041
#endif
extern GLXFBConfig * (APIENTRYP gglXGetFBConfigs)(Display *dpy, int screen, int *nelements);
extern GLXFBConfig * (APIENTRYP gglXChooseFBConfig)(Display *dpy, int screen, const int *attrib_list, int *nelements);
extern int (APIENTRYP gglXGetFBConfigAttrib)(Display *dpy, GLXFBConfig config, int attribute, int *value);
extern XVisualInfo * (APIENTRYP gglXGetVisualFromFBConfig)(Display *dpy, GLXFBConfig config);
extern GLXWindow (APIENTRYP gglXCreateWindow)(Display *dpy, GLXFBConfig config, Window win, const int *attrib_list);
extern void (APIENTRYP gglXDestroyWindow)(Display *dpy, GLXWindow win);
extern GLXPixmap (APIENTRYP gglXCreatePixmap)(Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attrib_list);
extern void (APIENTRYP gglXDestroyPixmap)(Display *dpy, GLXPixmap pixmap);
extern GLXPbuffer (APIENTRYP gglXCreatePbuffer)(Display *dpy, GLXFBConfig config, const int *attrib_list);
extern void (APIENTRYP gglXDestroyPbuffer)(Display *dpy, GLXPbuffer pbuf);
extern void (APIENTRYP gglXQueryDrawable)(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value);
extern GLXContext (APIENTRYP gglXCreateNewContext)(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);
extern Bool (APIENTRYP gglXMakeContextCurrent)(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
extern GLXDrawable (APIENTRYP gglXGetCurrentReadDrawable)();
extern int (APIENTRYP gglXQueryContext)(Display *dpy, GLXContext ctx, int attribute, int *value);
extern void (APIENTRYP gglXSelectEvent)(Display *dpy, GLXDrawable draw, unsigned long event_mask);
extern void (APIENTRYP gglXGetSelectedEvent)(Display *dpy, GLXDrawable draw, unsigned long *event_mask);

#ifndef GLX_VERSION_1_4
#define GLX_VERSION_1_4
#define GLX_SAMPLE_BUFFERS 100000
#define GLX_SAMPLES 100001
#endif
extern __GLXextFuncPtr (APIENTRYP gglXGetProcAddress)(const GLubyte *procName);

#ifndef GLX_3DFX_multisample
#define GLX_3DFX_multisample
#define GLX_SAMPLE_BUFFERS_3DFX 0x8050
#define GLX_SAMPLES_3DFX 0x8051
#endif

#ifndef GLX_AMD_gpu_association
#define GLX_AMD_gpu_association
#define GLX_GPU_VENDOR_AMD 0x1F00
#define GLX_GPU_RENDERER_STRING_AMD 0x1F01
#define GLX_GPU_OPENGL_VERSION_STRING_AMD 0x1F02
#define GLX_GPU_FASTEST_TARGET_GPUS_AMD 0x21A2
#define GLX_GPU_RAM_AMD 0x21A3
#define GLX_GPU_CLOCK_AMD 0x21A4
#define GLX_GPU_NUM_PIPES_AMD 0x21A5
#define GLX_GPU_NUM_SIMD_AMD 0x21A6
#define GLX_GPU_NUM_RB_AMD 0x21A7
#define GLX_GPU_NUM_SPI_AMD 0x21A8
#endif
extern unsigned int (APIENTRYP gglXGetGPUIDsAMD)(unsigned int maxCount, unsigned int *ids);
extern int (APIENTRYP gglXGetGPUInfoAMD)(unsigned int id, int property, GLenum dataType, unsigned int size, void *data);
extern unsigned int (APIENTRYP gglXGetContextGPUIDAMD)(GLXContext ctx);
extern GLXContext (APIENTRYP gglXCreateAssociatedContextAMD)(unsigned int id, GLXContext share_list);
extern GLXContext (APIENTRYP gglXCreateAssociatedContextAttribsAMD)(unsigned int id, GLXContext share_context, const int *attribList);
extern Bool (APIENTRYP gglXDeleteAssociatedContextAMD)(GLXContext ctx);
extern Bool (APIENTRYP gglXMakeAssociatedContextCurrentAMD)(GLXContext ctx);
extern GLXContext (APIENTRYP gglXGetCurrentAssociatedContextAMD)();
extern void (APIENTRYP gglXBlitContextFramebufferAMD)(GLXContext dstCtx, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

#ifndef GLX_ARB_context_flush_control
#define GLX_ARB_context_flush_control
#define GLX_CONTEXT_RELEASE_BEHAVIOR_ARB 0x2097
#define GLX_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB 0
#define GLX_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB 0x2098
#endif

#ifndef GLX_ARB_create_context
#define GLX_ARB_create_context
#define GLX_CONTEXT_DEBUG_BIT_ARB 0x00000001
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092
#define GLX_CONTEXT_FLAGS_ARB 0x2094
#endif
extern GLXContext (APIENTRYP gglXCreateContextAttribsARB)(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);

#ifndef GLX_ARB_create_context_profile
#define GLX_ARB_create_context_profile
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define GLX_CONTEXT_PROFILE_MASK_ARB 0x9126
#endif

#ifndef GLX_ARB_create_context_robustness
#define GLX_ARB_create_context_robustness
#define GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB 0x00000004
#define GLX_LOSE_CONTEXT_ON_RESET_ARB 0x8252
#define GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define GLX_NO_RESET_NOTIFICATION_ARB 0x8261
#endif

#ifndef GLX_ARB_fbconfig_float
#define GLX_ARB_fbconfig_float
#define GLX_RGBA_FLOAT_TYPE_ARB 0x20B9
#define GLX_RGBA_FLOAT_BIT_ARB 0x00000004
#endif

#ifndef GLX_ARB_framebuffer_sRGB
#define GLX_ARB_framebuffer_sRGB
#define GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20B2
#endif

#ifndef GLX_ARB_get_proc_address
#define GLX_ARB_get_proc_address
#endif
extern __GLXextFuncPtr (APIENTRYP gglXGetProcAddressARB)(const GLubyte *procName);

#ifndef GLX_ARB_multisample
#define GLX_ARB_multisample
#define GLX_SAMPLE_BUFFERS_ARB 100000
#define GLX_SAMPLES_ARB 100001
#endif

#ifndef GLX_ARB_robustness_application_isolation
#define GLX_ARB_robustness_application_isolation
#define GLX_CONTEXT_RESET_ISOLATION_BIT_ARB 0x00000008
#endif

#ifndef GLX_ARB_robustness_share_group_isolation
#define GLX_ARB_robustness_share_group_isolation
/* reuse GLX_CONTEXT_RESET_ISOLATION_BIT_ARB */
#endif

#ifndef GLX_ARB_vertex_buffer_object
#define GLX_ARB_vertex_buffer_object
#define GLX_CONTEXT_ALLOW_BUFFER_BYTE_ORDER_MISMATCH_ARB 0x2095
#endif

#ifndef GLX_EXT_buffer_age
#define GLX_EXT_buffer_age
#define GLX_BACK_BUFFER_AGE_EXT 0x20F4
#endif

#ifndef GLX_EXT_create_context_es_profile
#define GLX_EXT_create_context_es_profile
#define GLX_CONTEXT_ES_PROFILE_BIT_EXT 0x00000004
#endif

#ifndef GLX_EXT_create_context_es2_profile
#define GLX_EXT_create_context_es2_profile
#define GLX_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004
#endif

#ifndef GLX_EXT_fbconfig_packed_float
#define GLX_EXT_fbconfig_packed_float
#define GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT 0x20B1
#define GLX_RGBA_UNSIGNED_FLOAT_BIT_EXT 0x00000008
#endif

#ifndef GLX_EXT_framebuffer_sRGB
#define GLX_EXT_framebuffer_sRGB
#define GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT 0x20B2
#endif

#ifndef GLX_EXT_import_context
#define GLX_EXT_import_context
#define GLX_SHARE_CONTEXT_EXT 0x800A
#define GLX_VISUAL_ID_EXT 0x800B
#define GLX_SCREEN_EXT 0x800C
#endif
extern Display * (APIENTRYP gglXGetCurrentDisplayEXT)();
extern int (APIENTRYP gglXQueryContextInfoEXT)(Display *dpy, GLXContext context, int attribute, int *value);
extern GLXContextID (APIENTRYP gglXGetContextIDEXT)(const GLXContext context);
extern GLXContext (APIENTRYP gglXImportContextEXT)(Display *dpy, GLXContextID contextID);
extern void (APIENTRYP gglXFreeContextEXT)(Display *dpy, GLXContext context);

#ifndef GLX_EXT_libglvnd
#define GLX_EXT_libglvnd
#define GLX_VENDOR_NAMES_EXT 0x20F6
#endif

#ifndef GLX_EXT_stereo_tree
#define GLX_EXT_stereo_tree
#define GLX_STEREO_TREE_EXT 0x20F5
#define GLX_STEREO_NOTIFY_MASK_EXT 0x00000001
#define GLX_STEREO_NOTIFY_EXT 0x00000000
#endif

#ifndef GLX_EXT_swap_control
#define GLX_EXT_swap_control
#define GLX_SWAP_INTERVAL_EXT 0x20F1
#define GLX_MAX_SWAP_INTERVAL_EXT 0x20F2
#endif
extern void (APIENTRYP gglXSwapIntervalEXT)(Display *dpy, GLXDrawable drawable, int interval);

#ifndef GLX_EXT_swap_control_tear
#define GLX_EXT_swap_control_tear
#define GLX_LATE_SWAPS_TEAR_EXT 0x20F3
#endif

#ifndef GLX_EXT_texture_from_pixmap
#define GLX_EXT_texture_from_pixmap
#define GLX_TEXTURE_1D_BIT_EXT 0x00000001
#define GLX_TEXTURE_2D_BIT_EXT 0x00000002
#define GLX_TEXTURE_RECTANGLE_BIT_EXT 0x00000004
#define GLX_BIND_TO_TEXTURE_RGB_EXT 0x20D0
#define GLX_BIND_TO_TEXTURE_RGBA_EXT 0x20D1
#define GLX_BIND_TO_MIPMAP_TEXTURE_EXT 0x20D2
#define GLX_BIND_TO_TEXTURE_TARGETS_EXT 0x20D3
#define GLX_Y_INVERTED_EXT 0x20D4
#define GLX_TEXTURE_FORMAT_EXT 0x20D5
#define GLX_TEXTURE_TARGET_EXT 0x20D6
#define GLX_MIPMAP_TEXTURE_EXT 0x20D7
#define GLX_TEXTURE_FORMAT_NONE_EXT 0x20D8
#define GLX_TEXTURE_FORMAT_RGB_EXT 0x20D9
#define GLX_TEXTURE_FORMAT_RGBA_EXT 0x20DA
#define GLX_TEXTURE_1D_EXT 0x20DB
#define GLX_TEXTURE_2D_EXT 0x20DC
#define GLX_TEXTURE_RECTANGLE_EXT 0x20DD
#define GLX_FRONT_LEFT_EXT 0x20DE
#define GLX_FRONT_RIGHT_EXT 0x20DF
#define GLX_BACK_LEFT_EXT 0x20E0
#define GLX_BACK_RIGHT_EXT 0x20E1
#define GLX_FRONT_EXT 0x20DE
#define GLX_BACK_EXT 0x20E0
#define GLX_AUX0_EXT 0x20E2
#define GLX_AUX1_EXT 0x20E3
#define GLX_AUX2_EXT 0x20E4
#define GLX_AUX3_EXT 0x20E5
#define GLX_AUX4_EXT 0x20E6
#define GLX_AUX5_EXT 0x20E7
#define GLX_AUX6_EXT 0x20E8
#define GLX_AUX7_EXT 0x20E9
#define GLX_AUX8_EXT 0x20EA
#define GLX_AUX9_EXT 0x20EB
#endif
extern void (APIENTRYP gglXBindTexImageEXT)(Display *dpy, GLXDrawable drawable, int buffer, const int *attrib_list);
extern void (APIENTRYP gglXReleaseTexImageEXT)(Display *dpy, GLXDrawable drawable, int buffer);

#ifndef GLX_EXT_visual_info
#define GLX_EXT_visual_info
#define GLX_X_VISUAL_TYPE_EXT 0x22
#define GLX_TRANSPARENT_TYPE_EXT 0x23
#define GLX_TRANSPARENT_INDEX_VALUE_EXT 0x24
#define GLX_TRANSPARENT_RED_VALUE_EXT 0x25
#define GLX_TRANSPARENT_GREEN_VALUE_EXT 0x26
#define GLX_TRANSPARENT_BLUE_VALUE_EXT 0x27
#define GLX_TRANSPARENT_ALPHA_VALUE_EXT 0x28
#define GLX_NONE_EXT 0x8000
#define GLX_TRUE_COLOR_EXT 0x8002
#define GLX_DIRECT_COLOR_EXT 0x8003
#define GLX_PSEUDO_COLOR_EXT 0x8004
#define GLX_STATIC_COLOR_EXT 0x8005
#define GLX_GRAY_SCALE_EXT 0x8006
#define GLX_STATIC_GRAY_EXT 0x8007
#define GLX_TRANSPARENT_RGB_EXT 0x8008
#define GLX_TRANSPARENT_INDEX_EXT 0x8009
#endif

#ifndef GLX_EXT_visual_rating
#define GLX_EXT_visual_rating
#define GLX_VISUAL_CAVEAT_EXT 0x20
#define GLX_SLOW_VISUAL_EXT 0x8001
#define GLX_NON_CONFORMANT_VISUAL_EXT 0x800D
/* reuse GLX_NONE_EXT */
#endif

#ifndef GLX_INTEL_swap_event
#define GLX_INTEL_swap_event
#define GLX_BUFFER_SWAP_COMPLETE_INTEL_MASK 0x04000000
#define GLX_EXCHANGE_COMPLETE_INTEL 0x8180
#define GLX_COPY_COMPLETE_INTEL 0x8181
#define GLX_FLIP_COMPLETE_INTEL 0x8182
#endif

#ifndef GLX_MESA_agp_offset
#define GLX_MESA_agp_offset
#endif
extern unsigned int (APIENTRYP gglXGetAGPOffsetMESA)(const void *pointer);

#ifndef GLX_MESA_copy_sub_buffer
#define GLX_MESA_copy_sub_buffer
#endif
extern void (APIENTRYP gglXCopySubBufferMESA)(Display *dpy, GLXDrawable drawable, int x, int y, int width, int height);

#ifndef GLX_MESA_pixmap_colormap
#define GLX_MESA_pixmap_colormap
#endif
extern GLXPixmap (APIENTRYP gglXCreateGLXPixmapMESA)(Display *dpy, XVisualInfo *visual, Pixmap pixmap, Colormap cmap);

#ifndef GLX_MESA_query_renderer
#define GLX_MESA_query_renderer
#define GLX_RENDERER_VENDOR_ID_MESA 0x8183
#define GLX_RENDERER_DEVICE_ID_MESA 0x8184
#define GLX_RENDERER_VERSION_MESA 0x8185
#define GLX_RENDERER_ACCELERATED_MESA 0x8186
#define GLX_RENDERER_VIDEO_MEMORY_MESA 0x8187
#define GLX_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_MESA 0x8188
#define GLX_RENDERER_PREFERRED_PROFILE_MESA 0x8189
#define GLX_RENDERER_OPENGL_CORE_PROFILE_VERSION_MESA 0x818A
#define GLX_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION_MESA 0x818B
#define GLX_RENDERER_OPENGL_ES_PROFILE_VERSION_MESA 0x818C
#define GLX_RENDERER_OPENGL_ES2_PROFILE_VERSION_MESA 0x818D
#define GLX_RENDERER_ID_MESA 0x818E
#endif
extern Bool (APIENTRYP gglXQueryCurrentRendererIntegerMESA)(int attribute, unsigned int *value);
extern const char * (APIENTRYP gglXQueryCurrentRendererStringMESA)(int attribute);
extern Bool (APIENTRYP gglXQueryRendererIntegerMESA)(Display *dpy, int screen, int renderer, int attribute, unsigned int *value);
extern const char * (APIENTRYP gglXQueryRendererStringMESA)(Display *dpy, int screen, int renderer, int attribute);

#ifndef GLX_MESA_release_buffers
#define GLX_MESA_release_buffers
#endif
extern Bool (APIENTRYP gglXReleaseBuffersMESA)(Display *dpy, GLXDrawable drawable);

#ifndef GLX_MESA_set_3dfx_mode
#define GLX_MESA_set_3dfx_mode
#define GLX_3DFX_WINDOW_MODE_MESA 0x1
#define GLX_3DFX_FULLSCREEN_MODE_MESA 0x2
#endif
extern Bool (APIENTRYP gglXSet3DfxModeMESA)(int mode);

#ifndef GLX_NV_copy_buffer
#define GLX_NV_copy_buffer
#endif
extern void (APIENTRYP gglXCopyBufferSubDataNV)(Display *dpy, GLXContext readCtx, GLXContext writeCtx, GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
extern void (APIENTRYP gglXNamedCopyBufferSubDataNV)(Display *dpy, GLXContext readCtx, GLXContext writeCtx, GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);

#ifndef GLX_NV_copy_image
#define GLX_NV_copy_image
#endif
extern void (APIENTRYP gglXCopyImageSubDataNV)(Display *dpy, GLXContext srcCtx, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLXContext dstCtx, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth);

#ifndef GLX_NV_delay_before_swap
#define GLX_NV_delay_before_swap
#endif
extern Bool (APIENTRYP gglXDelayBeforeSwapNV)(Display *dpy, GLXDrawable drawable, GLfloat seconds);

#ifndef GLX_NV_float_buffer
#define GLX_NV_float_buffer
#define GLX_FLOAT_COMPONENTS_NV 0x20B0
#endif

#ifndef GLX_NV_multisample_coverage
#define GLX_NV_multisample_coverage
#define GLX_COVERAGE_SAMPLES_NV 100001
#define GLX_COLOR_SAMPLES_NV 0x20B3
#endif

#ifndef GLX_NV_present_video
#define GLX_NV_present_video
#define GLX_NUM_VIDEO_SLOTS_NV 0x20F0
#endif
extern unsigned int * (APIENTRYP gglXEnumerateVideoDevicesNV)(Display *dpy, int screen, int *nelements);
extern int (APIENTRYP gglXBindVideoDeviceNV)(Display *dpy, unsigned int video_slot, unsigned int video_device, const int *attrib_list);

#ifndef GLX_NV_robustness_video_memory_purge
#define GLX_NV_robustness_video_memory_purge
#define GLX_GENERATE_RESET_ON_VIDEO_MEMORY_PURGE_NV 0x20F7
#endif

#ifndef GLX_NV_swap_group
#define GLX_NV_swap_group
#endif
extern Bool (APIENTRYP gglXJoinSwapGroupNV)(Display *dpy, GLXDrawable drawable, GLuint group);
extern Bool (APIENTRYP gglXBindSwapBarrierNV)(Display *dpy, GLuint group, GLuint barrier);
extern Bool (APIENTRYP gglXQuerySwapGroupNV)(Display *dpy, GLXDrawable drawable, GLuint *group, GLuint *barrier);
extern Bool (APIENTRYP gglXQueryMaxSwapGroupsNV)(Display *dpy, int screen, GLuint *maxGroups, GLuint *maxBarriers);
extern Bool (APIENTRYP gglXQueryFrameCountNV)(Display *dpy, int screen, GLuint *count);
extern Bool (APIENTRYP gglXResetFrameCountNV)(Display *dpy, int screen);

#ifndef GLX_NV_video_capture
#define GLX_NV_video_capture
#define GLX_DEVICE_ID_NV 0x20CD
#define GLX_UNIQUE_ID_NV 0x20CE
#define GLX_NUM_VIDEO_CAPTURE_SLOTS_NV 0x20CF
#endif
extern int (APIENTRYP gglXBindVideoCaptureDeviceNV)(Display *dpy, unsigned int video_capture_slot, GLXVideoCaptureDeviceNV device);
extern GLXVideoCaptureDeviceNV * (APIENTRYP gglXEnumerateVideoCaptureDevicesNV)(Display *dpy, int screen, int *nelements);
extern void (APIENTRYP gglXLockVideoCaptureDeviceNV)(Display *dpy, GLXVideoCaptureDeviceNV device);
extern int (APIENTRYP gglXQueryVideoCaptureDeviceNV)(Display *dpy, GLXVideoCaptureDeviceNV device, int attribute, int *value);
extern void (APIENTRYP gglXReleaseVideoCaptureDeviceNV)(Display *dpy, GLXVideoCaptureDeviceNV device);

#ifndef GLX_NV_video_out
#define GLX_NV_video_out
#define GLX_VIDEO_OUT_COLOR_NV 0x20C3
#define GLX_VIDEO_OUT_ALPHA_NV 0x20C4
#define GLX_VIDEO_OUT_DEPTH_NV 0x20C5
#define GLX_VIDEO_OUT_COLOR_AND_ALPHA_NV 0x20C6
#define GLX_VIDEO_OUT_COLOR_AND_DEPTH_NV 0x20C7
#define GLX_VIDEO_OUT_FRAME_NV 0x20C8
#define GLX_VIDEO_OUT_FIELD_1_NV 0x20C9
#define GLX_VIDEO_OUT_FIELD_2_NV 0x20CA
#define GLX_VIDEO_OUT_STACKED_FIELDS_1_2_NV 0x20CB
#define GLX_VIDEO_OUT_STACKED_FIELDS_2_1_NV 0x20CC
#endif
extern int (APIENTRYP gglXGetVideoDeviceNV)(Display *dpy, int screen, int numVideoDevices, GLXVideoDeviceNV *pVideoDevice);
extern int (APIENTRYP gglXReleaseVideoDeviceNV)(Display *dpy, int screen, GLXVideoDeviceNV VideoDevice);
extern int (APIENTRYP gglXBindVideoImageNV)(Display *dpy, GLXVideoDeviceNV VideoDevice, GLXPbuffer pbuf, int iVideoBuffer);
extern int (APIENTRYP gglXReleaseVideoImageNV)(Display *dpy, GLXPbuffer pbuf);
extern int (APIENTRYP gglXSendPbufferToVideoNV)(Display *dpy, GLXPbuffer pbuf, int iBufferType, unsigned long *pulCounterPbuffer, GLboolean bBlock);
extern int (APIENTRYP gglXGetVideoInfoNV)(Display *dpy, int screen, GLXVideoDeviceNV VideoDevice, unsigned long *pulCounterOutputPbuffer, unsigned long *pulCounterOutputVideo);

#ifndef GLX_OML_swap_method
#define GLX_OML_swap_method
#define GLX_SWAP_METHOD_OML 0x8060
#define GLX_SWAP_EXCHANGE_OML 0x8061
#define GLX_SWAP_COPY_OML 0x8062
#define GLX_SWAP_UNDEFINED_OML 0x8063
#endif

#ifndef GLX_OML_sync_control
#define GLX_OML_sync_control
#endif
extern Bool (APIENTRYP gglXGetSyncValuesOML)(Display *dpy, GLXDrawable drawable, int64_t *ust, int64_t *msc, int64_t *sbc);
extern Bool (APIENTRYP gglXGetMscRateOML)(Display *dpy, GLXDrawable drawable, int32_t *numerator, int32_t *denominator);
extern int64_t (APIENTRYP gglXSwapBuffersMscOML)(Display *dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor, int64_t remainder);
extern Bool (APIENTRYP gglXWaitForMscOML)(Display *dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor, int64_t remainder, int64_t *ust, int64_t *msc, int64_t *sbc);
extern Bool (APIENTRYP gglXWaitForSbcOML)(Display *dpy, GLXDrawable drawable, int64_t target_sbc, int64_t *ust, int64_t *msc, int64_t *sbc);

#ifndef GLX_SGI_cushion
#define GLX_SGI_cushion
#endif
extern void (APIENTRYP gglXCushionSGI)(Display *dpy, Window window, float cushion);

#ifndef GLX_SGI_make_current_read
#define GLX_SGI_make_current_read
#endif
extern Bool (APIENTRYP gglXMakeCurrentReadSGI)(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
extern GLXDrawable (APIENTRYP gglXGetCurrentReadDrawableSGI)();

#ifndef GLX_SGI_swap_control
#define GLX_SGI_swap_control
#endif
extern int (APIENTRYP gglXSwapIntervalSGI)(int interval);

#ifndef GLX_SGI_video_sync
#define GLX_SGI_video_sync
#endif
extern int (APIENTRYP gglXGetVideoSyncSGI)(unsigned int *count);
extern int (APIENTRYP gglXWaitVideoSyncSGI)(int divisor, int remainder, unsigned int *count);

#ifndef GLX_SGIS_blended_overlay
#define GLX_SGIS_blended_overlay
#define GLX_BLENDED_RGBA_SGIS 0x8025
#endif

#ifndef GLX_SGIS_multisample
#define GLX_SGIS_multisample
#define GLX_SAMPLE_BUFFERS_SGIS 100000
#define GLX_SAMPLES_SGIS 100001
#endif

#ifndef GLX_SGIS_shared_multisample
#define GLX_SGIS_shared_multisample
#define GLX_MULTISAMPLE_SUB_RECT_WIDTH_SGIS 0x8026
#define GLX_MULTISAMPLE_SUB_RECT_HEIGHT_SGIS 0x8027
#endif

#ifndef GLX_SGIX_dmbuffer
#define GLX_SGIX_dmbuffer
#define GLX_DIGITAL_MEDIA_PBUFFER_SGIX 0x8024
#endif
extern Bool (APIENTRYP gglXAssociateDMPbufferSGIX)(Display *dpy, GLXPbufferSGIX pbuffer, DMparams *params, DMbuffer dmbuffer);

#ifndef GLX_SGIX_fbconfig
#define GLX_SGIX_fbconfig
#define GLX_WINDOW_BIT_SGIX 0x00000001
#define GLX_PIXMAP_BIT_SGIX 0x00000002
#define GLX_RGBA_BIT_SGIX 0x00000001
#define GLX_COLOR_INDEX_BIT_SGIX 0x00000002
#define GLX_DRAWABLE_TYPE_SGIX 0x8010
#define GLX_RENDER_TYPE_SGIX 0x8011
#define GLX_X_RENDERABLE_SGIX 0x8012
#define GLX_FBCONFIG_ID_SGIX 0x8013
#define GLX_RGBA_TYPE_SGIX 0x8014
#define GLX_COLOR_INDEX_TYPE_SGIX 0x8015
/* reuse GLX_SCREEN_EXT */
#endif
extern int (APIENTRYP gglXGetFBConfigAttribSGIX)(Display *dpy, GLXFBConfigSGIX config, int attribute, int *value);
extern GLXFBConfigSGIX * (APIENTRYP gglXChooseFBConfigSGIX)(Display *dpy, int screen, int *attrib_list, int *nelements);
extern GLXPixmap (APIENTRYP gglXCreateGLXPixmapWithConfigSGIX)(Display *dpy, GLXFBConfigSGIX config, Pixmap pixmap);
extern GLXContext (APIENTRYP gglXCreateContextWithConfigSGIX)(Display *dpy, GLXFBConfigSGIX config, int render_type, GLXContext share_list, Bool direct);
extern XVisualInfo * (APIENTRYP gglXGetVisualFromFBConfigSGIX)(Display *dpy, GLXFBConfigSGIX config);
extern GLXFBConfigSGIX (APIENTRYP gglXGetFBConfigFromVisualSGIX)(Display *dpy, XVisualInfo *vis);

#ifndef GLX_SGIX_hyperpipe
#define GLX_SGIX_hyperpipe
#define GLX_HYPERPIPE_PIPE_NAME_LENGTH_SGIX 80
#define GLX_BAD_HYPERPIPE_CONFIG_SGIX 91
#define GLX_BAD_HYPERPIPE_SGIX 92
#define GLX_HYPERPIPE_DISPLAY_PIPE_SGIX 0x00000001
#define GLX_HYPERPIPE_RENDER_PIPE_SGIX 0x00000002
#define GLX_PIPE_RECT_SGIX 0x00000001
#define GLX_PIPE_RECT_LIMITS_SGIX 0x00000002
#define GLX_HYPERPIPE_STEREO_SGIX 0x00000003
#define GLX_HYPERPIPE_PIXEL_AVERAGE_SGIX 0x00000004
#define GLX_HYPERPIPE_ID_SGIX 0x8030
#endif
extern GLXHyperpipeNetworkSGIX * (APIENTRYP gglXQueryHyperpipeNetworkSGIX)(Display *dpy, int *npipes);
extern int (APIENTRYP gglXHyperpipeConfigSGIX)(Display *dpy, int networkId, int npipes, GLXHyperpipeConfigSGIX *cfg, int *hpId);
extern GLXHyperpipeConfigSGIX * (APIENTRYP gglXQueryHyperpipeConfigSGIX)(Display *dpy, int hpId, int *npipes);
extern int (APIENTRYP gglXDestroyHyperpipeConfigSGIX)(Display *dpy, int hpId);
extern int (APIENTRYP gglXBindHyperpipeSGIX)(Display *dpy, int hpId);
extern int (APIENTRYP gglXQueryHyperpipeBestAttribSGIX)(Display *dpy, int timeSlice, int attrib, int size, void *attribList, void *returnAttribList);
extern int (APIENTRYP gglXHyperpipeAttribSGIX)(Display *dpy, int timeSlice, int attrib, int size, void *attribList);
extern int (APIENTRYP gglXQueryHyperpipeAttribSGIX)(Display *dpy, int timeSlice, int attrib, int size, void *returnAttribList);

#ifndef GLX_SGIX_pbuffer
#define GLX_SGIX_pbuffer
#define GLX_PBUFFER_BIT_SGIX 0x00000004
#define GLX_BUFFER_CLOBBER_MASK_SGIX 0x08000000
#define GLX_FRONT_LEFT_BUFFER_BIT_SGIX 0x00000001
#define GLX_FRONT_RIGHT_BUFFER_BIT_SGIX 0x00000002
#define GLX_BACK_LEFT_BUFFER_BIT_SGIX 0x00000004
#define GLX_BACK_RIGHT_BUFFER_BIT_SGIX 0x00000008
#define GLX_AUX_BUFFERS_BIT_SGIX 0x00000010
#define GLX_DEPTH_BUFFER_BIT_SGIX 0x00000020
#define GLX_STENCIL_BUFFER_BIT_SGIX 0x00000040
#define GLX_ACCUM_BUFFER_BIT_SGIX 0x00000080
#define GLX_SAMPLE_BUFFERS_BIT_SGIX 0x00000100
#define GLX_MAX_PBUFFER_WIDTH_SGIX 0x8016
#define GLX_MAX_PBUFFER_HEIGHT_SGIX 0x8017
#define GLX_MAX_PBUFFER_PIXELS_SGIX 0x8018
#define GLX_OPTIMAL_PBUFFER_WIDTH_SGIX 0x8019
#define GLX_OPTIMAL_PBUFFER_HEIGHT_SGIX 0x801A
#define GLX_PRESERVED_CONTENTS_SGIX 0x801B
#define GLX_LARGEST_PBUFFER_SGIX 0x801C
#define GLX_WIDTH_SGIX 0x801D
#define GLX_HEIGHT_SGIX 0x801E
#define GLX_EVENT_MASK_SGIX 0x801F
#define GLX_DAMAGED_SGIX 0x8020
#define GLX_SAVED_SGIX 0x8021
#define GLX_WINDOW_SGIX 0x8022
#define GLX_PBUFFER_SGIX 0x8023
#endif
extern GLXPbufferSGIX (APIENTRYP gglXCreateGLXPbufferSGIX)(Display *dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height, int *attrib_list);
extern void (APIENTRYP gglXDestroyGLXPbufferSGIX)(Display *dpy, GLXPbufferSGIX pbuf);
extern int (APIENTRYP gglXQueryGLXPbufferSGIX)(Display *dpy, GLXPbufferSGIX pbuf, int attribute, unsigned int *value);
extern void (APIENTRYP gglXSelectEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long mask);
extern void (APIENTRYP gglXGetSelectedEventSGIX)(Display *dpy, GLXDrawable drawable, unsigned long *mask);

#ifndef GLX_SGIX_swap_barrier
#define GLX_SGIX_swap_barrier
#endif
extern void (APIENTRYP gglXBindSwapBarrierSGIX)(Display *dpy, GLXDrawable drawable, int barrier);
extern Bool (APIENTRYP gglXQueryMaxSwapBarriersSGIX)(Display *dpy, int screen, int *max);

#ifndef GLX_SGIX_swap_group
#define GLX_SGIX_swap_group
#endif
extern void (APIENTRYP gglXJoinSwapGroupSGIX)(Display *dpy, GLXDrawable drawable, GLXDrawable member);

#ifndef GLX_SGIX_video_resize
#define GLX_SGIX_video_resize
#define GLX_SYNC_FRAME_SGIX 0x00000000
#define GLX_SYNC_SWAP_SGIX 0x00000001
#endif
extern int (APIENTRYP gglXBindChannelToWindowSGIX)(Display *display, int screen, int channel, Window window);
extern int (APIENTRYP gglXChannelRectSGIX)(Display *display, int screen, int channel, int x, int y, int w, int h);
extern int (APIENTRYP gglXQueryChannelRectSGIX)(Display *display, int screen, int channel, int *dx, int *dy, int *dw, int *dh);
extern int (APIENTRYP gglXQueryChannelDeltasSGIX)(Display *display, int screen, int channel, int *x, int *y, int *w, int *h);
extern int (APIENTRYP gglXChannelRectSyncSGIX)(Display *display, int screen, int channel, GLenum synctype);

#ifndef GLX_SGIX_video_source
#define GLX_SGIX_video_source
#endif
extern GLXVideoSourceSGIX (APIENTRYP gglXCreateGLXVideoSourceSGIX)(Display *display, int screen, VLServer server, VLPath path, int nodeClass, VLNode drainNode);
extern void (APIENTRYP gglXDestroyGLXVideoSourceSGIX)(Display *dpy, GLXVideoSourceSGIX glxvideosource);

#ifndef GLX_SGIX_visual_select_group
#define GLX_SGIX_visual_select_group
#define GLX_VISUAL_SELECT_GROUP_SGIX 0x8028
#endif

#ifndef GLX_SUN_get_transparent_index
#define GLX_SUN_get_transparent_index
#endif
extern Status (APIENTRYP gglXGetTransparentIndexSUN)(Display *dpy, Window overlay, Window underlay, long *pTransparentIndex);

/* 62 extensions */
typedef struct {
	int _GLX_3DFX_multisample : 1;
	int _GLX_AMD_gpu_association : 1;
	int _GLX_ARB_context_flush_control : 1;
	int _GLX_ARB_create_context : 1;
	int _GLX_ARB_create_context_profile : 1;
	int _GLX_ARB_create_context_robustness : 1;
	int _GLX_ARB_fbconfig_float : 1;
	int _GLX_ARB_framebuffer_sRGB : 1;
	int _GLX_ARB_get_proc_address : 1;
	int _GLX_ARB_multisample : 1;
	int _GLX_ARB_robustness_application_isolation : 1;
	int _GLX_ARB_robustness_share_group_isolation : 1;
	int _GLX_ARB_vertex_buffer_object : 1;
	int _GLX_EXT_buffer_age : 1;
	int _GLX_EXT_create_context_es_profile : 1;
	int _GLX_EXT_create_context_es2_profile : 1;
	int _GLX_EXT_fbconfig_packed_float : 1;
	int _GLX_EXT_framebuffer_sRGB : 1;
	int _GLX_EXT_import_context : 1;
	int _GLX_EXT_libglvnd : 1;
	int _GLX_EXT_stereo_tree : 1;
	int _GLX_EXT_swap_control : 1;
	int _GLX_EXT_swap_control_tear : 1;
	int _GLX_EXT_texture_from_pixmap : 1;
	int _GLX_EXT_visual_info : 1;
	int _GLX_EXT_visual_rating : 1;
	int _GLX_INTEL_swap_event : 1;
	int _GLX_MESA_agp_offset : 1;
	int _GLX_MESA_copy_sub_buffer : 1;
	int _GLX_MESA_pixmap_colormap : 1;
	int _GLX_MESA_query_renderer : 1;
	int _GLX_MESA_release_buffers : 1;
	int _GLX_MESA_set_3dfx_mode : 1;
	int _GLX_NV_copy_buffer : 1;
	int _GLX_NV_copy_image : 1;
	int _GLX_NV_delay_before_swap : 1;
	int _GLX_NV_float_buffer : 1;
	int _GLX_NV_multisample_coverage : 1;
	int _GLX_NV_present_video : 1;
	int _GLX_NV_robustness_video_memory_purge : 1;
	int _GLX_NV_swap_group : 1;
	int _GLX_NV_video_capture : 1;
	int _GLX_NV_video_out : 1;
	int _GLX_OML_swap_method : 1;
	int _GLX_OML_sync_control : 1;
	int _GLX_SGI_cushion : 1;
	int _GLX_SGI_make_current_read : 1;
	int _GLX_SGI_swap_control : 1;
	int _GLX_SGI_video_sync : 1;
	int _GLX_SGIS_blended_overlay : 1;
	int _GLX_SGIS_multisample : 1;
	int _GLX_SGIS_shared_multisample : 1;
	int _GLX_SGIX_dmbuffer : 1;
	int _GLX_SGIX_fbconfig : 1;
	int _GLX_SGIX_hyperpipe : 1;
	int _GLX_SGIX_pbuffer : 1;
	int _GLX_SGIX_swap_barrier : 1;
	int _GLX_SGIX_swap_group : 1;
	int _GLX_SGIX_video_resize : 1;
	int _GLX_SGIX_video_source : 1;
	int _GLX_SGIX_visual_select_group : 1;
	int _GLX_SUN_get_transparent_index : 1;
} gglxext_t;

extern gglxext_t gglxext;

extern void gglx_init(int enableDebug);
extern void gglx_rebind(int enableDebug);

#ifdef __cplusplus
}
#endif

#endif /* __GGLX_H__ */
