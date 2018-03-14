/*********************************************************************************************
 *
 * gwgl.h
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

#ifndef __GWGL_H__
#define __GWGL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <gl/gl.h>
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

struct _GPU_DEVICE {
    DWORD  cb;
    CHAR   DeviceName[32];
    CHAR   DeviceString[128];
    DWORD  Flags;
    RECT   rcVirtualScreen;
};
DECLARE_HANDLE(HPBUFFERARB);
DECLARE_HANDLE(HPBUFFEREXT);
DECLARE_HANDLE(HVIDEOOUTPUTDEVICENV);
DECLARE_HANDLE(HPVIDEODEV);
DECLARE_HANDLE(HPGPUNV);
DECLARE_HANDLE(HGPUNV);
DECLARE_HANDLE(HVIDEOINPUTDEVICENV);
typedef struct _GPU_DEVICE GPU_DEVICE;
typedef struct _GPU_DEVICE *PGPU_DEVICE;

#ifndef WGL_3DFX_multisample
#define WGL_3DFX_multisample
#define WGL_SAMPLE_BUFFERS_3DFX 0x2060
#define WGL_SAMPLES_3DFX 0x2061
#endif

#ifndef WGL_3DL_stereo_control
#define WGL_3DL_stereo_control
#define WGL_STEREO_EMITTER_ENABLE_3DL 0x2055
#define WGL_STEREO_EMITTER_DISABLE_3DL 0x2056
#define WGL_STEREO_POLARITY_NORMAL_3DL 0x2057
#define WGL_STEREO_POLARITY_INVERT_3DL 0x2058
#endif
extern BOOL (APIENTRYP gwglSetStereoEmitterState3DL)(HDC hDC, UINT uState);

#ifndef WGL_AMD_gpu_association
#define WGL_AMD_gpu_association
#define WGL_GPU_VENDOR_AMD 0x1F00
#define WGL_GPU_RENDERER_STRING_AMD 0x1F01
#define WGL_GPU_OPENGL_VERSION_STRING_AMD 0x1F02
#define WGL_GPU_FASTEST_TARGET_GPUS_AMD 0x21A2
#define WGL_GPU_RAM_AMD 0x21A3
#define WGL_GPU_CLOCK_AMD 0x21A4
#define WGL_GPU_NUM_PIPES_AMD 0x21A5
#define WGL_GPU_NUM_SIMD_AMD 0x21A6
#define WGL_GPU_NUM_RB_AMD 0x21A7
#define WGL_GPU_NUM_SPI_AMD 0x21A8
#endif
extern UINT (APIENTRYP gwglGetGPUIDsAMD)(UINT maxCount, UINT *ids);
extern INT (APIENTRYP gwglGetGPUInfoAMD)(UINT id, int property, GLenum dataType, UINT size, void *data);
extern UINT (APIENTRYP gwglGetContextGPUIDAMD)(HGLRC hglrc);
extern HGLRC (APIENTRYP gwglCreateAssociatedContextAMD)(UINT id);
extern HGLRC (APIENTRYP gwglCreateAssociatedContextAttribsAMD)(UINT id, HGLRC hShareContext, const int *attribList);
extern BOOL (APIENTRYP gwglDeleteAssociatedContextAMD)(HGLRC hglrc);
extern BOOL (APIENTRYP gwglMakeAssociatedContextCurrentAMD)(HGLRC hglrc);
extern HGLRC (APIENTRYP gwglGetCurrentAssociatedContextAMD)();
extern VOID (APIENTRYP gwglBlitContextFramebufferAMD)(HGLRC dstCtx, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

#ifndef WGL_ARB_buffer_region
#define WGL_ARB_buffer_region
#define WGL_FRONT_COLOR_BUFFER_BIT_ARB 0x00000001
#define WGL_BACK_COLOR_BUFFER_BIT_ARB 0x00000002
#define WGL_DEPTH_BUFFER_BIT_ARB 0x00000004
#define WGL_STENCIL_BUFFER_BIT_ARB 0x00000008
#endif
extern HANDLE (APIENTRYP gwglCreateBufferRegionARB)(HDC hDC, int iLayerPlane, UINT uType);
extern VOID (APIENTRYP gwglDeleteBufferRegionARB)(HANDLE hRegion);
extern BOOL (APIENTRYP gwglSaveBufferRegionARB)(HANDLE hRegion, int x, int y, int width, int height);
extern BOOL (APIENTRYP gwglRestoreBufferRegionARB)(HANDLE hRegion, int x, int y, int width, int height, int xSrc, int ySrc);

#ifndef WGL_ARB_context_flush_control
#define WGL_ARB_context_flush_control
#define WGL_CONTEXT_RELEASE_BEHAVIOR_ARB 0x2097
#define WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB 0
#define WGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB 0x2098
#endif

#ifndef WGL_ARB_create_context
#define WGL_ARB_create_context
#define WGL_CONTEXT_DEBUG_BIT_ARB 0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB 0x2093
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define ERROR_INVALID_VERSION_ARB 0x2095
#endif
extern HGLRC (APIENTRYP gwglCreateContextAttribsARB)(HDC hDC, HGLRC hShareContext, const int *attribList);

#ifndef WGL_ARB_create_context_profile
#define WGL_ARB_create_context_profile
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#define ERROR_INVALID_PROFILE_ARB 0x2096
#endif

#ifndef WGL_ARB_create_context_robustness
#define WGL_ARB_create_context_robustness
#define WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB 0x00000004
#define WGL_LOSE_CONTEXT_ON_RESET_ARB 0x8252
#define WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define WGL_NO_RESET_NOTIFICATION_ARB 0x8261
#endif

#ifndef WGL_ARB_extensions_string
#define WGL_ARB_extensions_string
#endif
extern const char * (APIENTRYP gwglGetExtensionsStringARB)(HDC hdc);

#ifndef WGL_ARB_framebuffer_sRGB
#define WGL_ARB_framebuffer_sRGB
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB 0x20A9
#endif

#ifndef WGL_ARB_make_current_read
#define WGL_ARB_make_current_read
#define ERROR_INVALID_PIXEL_TYPE_ARB 0x2043
#define ERROR_INCOMPATIBLE_DEVICE_CONTEXTS_ARB 0x2054
#endif
extern BOOL (APIENTRYP gwglMakeContextCurrentARB)(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
extern HDC (APIENTRYP gwglGetCurrentReadDCARB)();

#ifndef WGL_ARB_multisample
#define WGL_ARB_multisample
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042
#endif

#ifndef WGL_ARB_pbuffer
#define WGL_ARB_pbuffer
#define WGL_DRAW_TO_PBUFFER_ARB 0x202D
#define WGL_MAX_PBUFFER_PIXELS_ARB 0x202E
#define WGL_MAX_PBUFFER_WIDTH_ARB 0x202F
#define WGL_MAX_PBUFFER_HEIGHT_ARB 0x2030
#define WGL_PBUFFER_LARGEST_ARB 0x2033
#define WGL_PBUFFER_WIDTH_ARB 0x2034
#define WGL_PBUFFER_HEIGHT_ARB 0x2035
#define WGL_PBUFFER_LOST_ARB 0x2036
#endif
extern HPBUFFERARB (APIENTRYP gwglCreatePbufferARB)(HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
extern HDC (APIENTRYP gwglGetPbufferDCARB)(HPBUFFERARB hPbuffer);
extern int (APIENTRYP gwglReleasePbufferDCARB)(HPBUFFERARB hPbuffer, HDC hDC);
extern BOOL (APIENTRYP gwglDestroyPbufferARB)(HPBUFFERARB hPbuffer);
extern BOOL (APIENTRYP gwglQueryPbufferARB)(HPBUFFERARB hPbuffer, int iAttribute, int *piValue);

#ifndef WGL_ARB_pixel_format
#define WGL_ARB_pixel_format
#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_DRAW_TO_BITMAP_ARB 0x2002
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_NEED_PALETTE_ARB 0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB 0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB 0x2006
#define WGL_SWAP_METHOD_ARB 0x2007
#define WGL_NUMBER_OVERLAYS_ARB 0x2008
#define WGL_NUMBER_UNDERLAYS_ARB 0x2009
#define WGL_TRANSPARENT_ARB 0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB 0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB 0x200C
#define WGL_SHARE_STENCIL_ARB 0x200D
#define WGL_SHARE_ACCUM_ARB 0x200E
#define WGL_SUPPORT_GDI_ARB 0x200F
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_STEREO_ARB 0x2012
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_RED_BITS_ARB 0x2015
#define WGL_RED_SHIFT_ARB 0x2016
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_GREEN_SHIFT_ARB 0x2018
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_BLUE_SHIFT_ARB 0x201A
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_ALPHA_SHIFT_ARB 0x201C
#define WGL_ACCUM_BITS_ARB 0x201D
#define WGL_ACCUM_RED_BITS_ARB 0x201E
#define WGL_ACCUM_GREEN_BITS_ARB 0x201F
#define WGL_ACCUM_BLUE_BITS_ARB 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB 0x2021
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_AUX_BUFFERS_ARB 0x2024
#define WGL_NO_ACCELERATION_ARB 0x2025
#define WGL_GENERIC_ACCELERATION_ARB 0x2026
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_SWAP_EXCHANGE_ARB 0x2028
#define WGL_SWAP_COPY_ARB 0x2029
#define WGL_SWAP_UNDEFINED_ARB 0x202A
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_TYPE_COLORINDEX_ARB 0x202C
#endif
extern BOOL (APIENTRYP gwglGetPixelFormatAttribivARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
extern BOOL (APIENTRYP gwglGetPixelFormatAttribfvARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
extern BOOL (APIENTRYP gwglChoosePixelFormatARB)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

#ifndef WGL_ARB_pixel_format_float
#define WGL_ARB_pixel_format_float
#define WGL_TYPE_RGBA_FLOAT_ARB 0x21A0
#endif

#ifndef WGL_ARB_render_texture
#define WGL_ARB_render_texture
#define WGL_BIND_TO_TEXTURE_RGB_ARB 0x2070
#define WGL_BIND_TO_TEXTURE_RGBA_ARB 0x2071
#define WGL_TEXTURE_FORMAT_ARB 0x2072
#define WGL_TEXTURE_TARGET_ARB 0x2073
#define WGL_MIPMAP_TEXTURE_ARB 0x2074
#define WGL_TEXTURE_RGB_ARB 0x2075
#define WGL_TEXTURE_RGBA_ARB 0x2076
#define WGL_NO_TEXTURE_ARB 0x2077
#define WGL_TEXTURE_CUBE_MAP_ARB 0x2078
#define WGL_TEXTURE_1D_ARB 0x2079
#define WGL_TEXTURE_2D_ARB 0x207A
#define WGL_MIPMAP_LEVEL_ARB 0x207B
#define WGL_CUBE_MAP_FACE_ARB 0x207C
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x207D
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x207E
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x207F
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x2080
#define WGL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x2081
#define WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x2082
#define WGL_FRONT_LEFT_ARB 0x2083
#define WGL_FRONT_RIGHT_ARB 0x2084
#define WGL_BACK_LEFT_ARB 0x2085
#define WGL_BACK_RIGHT_ARB 0x2086
#define WGL_AUX0_ARB 0x2087
#define WGL_AUX1_ARB 0x2088
#define WGL_AUX2_ARB 0x2089
#define WGL_AUX3_ARB 0x208A
#define WGL_AUX4_ARB 0x208B
#define WGL_AUX5_ARB 0x208C
#define WGL_AUX6_ARB 0x208D
#define WGL_AUX7_ARB 0x208E
#define WGL_AUX8_ARB 0x208F
#define WGL_AUX9_ARB 0x2090
#endif
extern BOOL (APIENTRYP gwglBindTexImageARB)(HPBUFFERARB hPbuffer, int iBuffer);
extern BOOL (APIENTRYP gwglReleaseTexImageARB)(HPBUFFERARB hPbuffer, int iBuffer);
extern BOOL (APIENTRYP gwglSetPbufferAttribARB)(HPBUFFERARB hPbuffer, const int *piAttribList);

#ifndef WGL_ARB_robustness_application_isolation
#define WGL_ARB_robustness_application_isolation
#define WGL_CONTEXT_RESET_ISOLATION_BIT_ARB 0x00000008
#endif

#ifndef WGL_ARB_robustness_share_group_isolation
#define WGL_ARB_robustness_share_group_isolation
/* reuse WGL_CONTEXT_RESET_ISOLATION_BIT_ARB */
#endif

#ifndef WGL_ATI_pixel_format_float
#define WGL_ATI_pixel_format_float
#define WGL_TYPE_RGBA_FLOAT_ATI 0x21A0
#endif

#ifndef WGL_EXT_colorspace
#define WGL_EXT_colorspace
#define WGL_COLORSPACE_EXT 0x3087
#define WGL_COLORSPACE_SRGB_EXT 0x3089
#define WGL_COLORSPACE_LINEAR_EXT 0x308A
#endif

#ifndef WGL_EXT_create_context_es_profile
#define WGL_EXT_create_context_es_profile
#define WGL_CONTEXT_ES_PROFILE_BIT_EXT 0x00000004
#endif

#ifndef WGL_EXT_create_context_es2_profile
#define WGL_EXT_create_context_es2_profile
#define WGL_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004
#endif

#ifndef WGL_EXT_depth_float
#define WGL_EXT_depth_float
#define WGL_DEPTH_FLOAT_EXT 0x2040
#endif

#ifndef WGL_EXT_display_color_table
#define WGL_EXT_display_color_table
#endif
extern GLboolean (APIENTRYP gwglCreateDisplayColorTableEXT)(GLushort id);
extern GLboolean (APIENTRYP gwglLoadDisplayColorTableEXT)(const GLushort *table, GLuint length);
extern GLboolean (APIENTRYP gwglBindDisplayColorTableEXT)(GLushort id);
extern VOID (APIENTRYP gwglDestroyDisplayColorTableEXT)(GLushort id);

#ifndef WGL_EXT_extensions_string
#define WGL_EXT_extensions_string
#endif
extern const char * (APIENTRYP gwglGetExtensionsStringEXT)();

#ifndef WGL_EXT_framebuffer_sRGB
#define WGL_EXT_framebuffer_sRGB
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT 0x20A9
#endif

#ifndef WGL_EXT_make_current_read
#define WGL_EXT_make_current_read
#define ERROR_INVALID_PIXEL_TYPE_EXT 0x2043
#endif
extern BOOL (APIENTRYP gwglMakeContextCurrentEXT)(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
extern HDC (APIENTRYP gwglGetCurrentReadDCEXT)();

#ifndef WGL_EXT_multisample
#define WGL_EXT_multisample
#define WGL_SAMPLE_BUFFERS_EXT 0x2041
#define WGL_SAMPLES_EXT 0x2042
#endif

#ifndef WGL_EXT_pbuffer
#define WGL_EXT_pbuffer
#define WGL_DRAW_TO_PBUFFER_EXT 0x202D
#define WGL_MAX_PBUFFER_PIXELS_EXT 0x202E
#define WGL_MAX_PBUFFER_WIDTH_EXT 0x202F
#define WGL_MAX_PBUFFER_HEIGHT_EXT 0x2030
#define WGL_OPTIMAL_PBUFFER_WIDTH_EXT 0x2031
#define WGL_OPTIMAL_PBUFFER_HEIGHT_EXT 0x2032
#define WGL_PBUFFER_LARGEST_EXT 0x2033
#define WGL_PBUFFER_WIDTH_EXT 0x2034
#define WGL_PBUFFER_HEIGHT_EXT 0x2035
#endif
extern HPBUFFEREXT (APIENTRYP gwglCreatePbufferEXT)(HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
extern HDC (APIENTRYP gwglGetPbufferDCEXT)(HPBUFFEREXT hPbuffer);
extern int (APIENTRYP gwglReleasePbufferDCEXT)(HPBUFFEREXT hPbuffer, HDC hDC);
extern BOOL (APIENTRYP gwglDestroyPbufferEXT)(HPBUFFEREXT hPbuffer);
extern BOOL (APIENTRYP gwglQueryPbufferEXT)(HPBUFFEREXT hPbuffer, int iAttribute, int *piValue);

#ifndef WGL_EXT_pixel_format
#define WGL_EXT_pixel_format
#define WGL_NUMBER_PIXEL_FORMATS_EXT 0x2000
#define WGL_DRAW_TO_WINDOW_EXT 0x2001
#define WGL_DRAW_TO_BITMAP_EXT 0x2002
#define WGL_ACCELERATION_EXT 0x2003
#define WGL_NEED_PALETTE_EXT 0x2004
#define WGL_NEED_SYSTEM_PALETTE_EXT 0x2005
#define WGL_SWAP_LAYER_BUFFERS_EXT 0x2006
#define WGL_SWAP_METHOD_EXT 0x2007
#define WGL_NUMBER_OVERLAYS_EXT 0x2008
#define WGL_NUMBER_UNDERLAYS_EXT 0x2009
#define WGL_TRANSPARENT_EXT 0x200A
#define WGL_TRANSPARENT_VALUE_EXT 0x200B
#define WGL_SHARE_DEPTH_EXT 0x200C
#define WGL_SHARE_STENCIL_EXT 0x200D
#define WGL_SHARE_ACCUM_EXT 0x200E
#define WGL_SUPPORT_GDI_EXT 0x200F
#define WGL_SUPPORT_OPENGL_EXT 0x2010
#define WGL_DOUBLE_BUFFER_EXT 0x2011
#define WGL_STEREO_EXT 0x2012
#define WGL_PIXEL_TYPE_EXT 0x2013
#define WGL_COLOR_BITS_EXT 0x2014
#define WGL_RED_BITS_EXT 0x2015
#define WGL_RED_SHIFT_EXT 0x2016
#define WGL_GREEN_BITS_EXT 0x2017
#define WGL_GREEN_SHIFT_EXT 0x2018
#define WGL_BLUE_BITS_EXT 0x2019
#define WGL_BLUE_SHIFT_EXT 0x201A
#define WGL_ALPHA_BITS_EXT 0x201B
#define WGL_ALPHA_SHIFT_EXT 0x201C
#define WGL_ACCUM_BITS_EXT 0x201D
#define WGL_ACCUM_RED_BITS_EXT 0x201E
#define WGL_ACCUM_GREEN_BITS_EXT 0x201F
#define WGL_ACCUM_BLUE_BITS_EXT 0x2020
#define WGL_ACCUM_ALPHA_BITS_EXT 0x2021
#define WGL_DEPTH_BITS_EXT 0x2022
#define WGL_STENCIL_BITS_EXT 0x2023
#define WGL_AUX_BUFFERS_EXT 0x2024
#define WGL_NO_ACCELERATION_EXT 0x2025
#define WGL_GENERIC_ACCELERATION_EXT 0x2026
#define WGL_FULL_ACCELERATION_EXT 0x2027
#define WGL_SWAP_EXCHANGE_EXT 0x2028
#define WGL_SWAP_COPY_EXT 0x2029
#define WGL_SWAP_UNDEFINED_EXT 0x202A
#define WGL_TYPE_RGBA_EXT 0x202B
#define WGL_TYPE_COLORINDEX_EXT 0x202C
#endif
extern BOOL (APIENTRYP gwglGetPixelFormatAttribivEXT)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, int *piValues);
extern BOOL (APIENTRYP gwglGetPixelFormatAttribfvEXT)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, FLOAT *pfValues);
extern BOOL (APIENTRYP gwglChoosePixelFormatEXT)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

#ifndef WGL_EXT_pixel_format_packed_float
#define WGL_EXT_pixel_format_packed_float
#define WGL_TYPE_RGBA_UNSIGNED_FLOAT_EXT 0x20A8
#endif

#ifndef WGL_EXT_swap_control
#define WGL_EXT_swap_control
#endif
extern BOOL (APIENTRYP gwglSwapIntervalEXT)(int interval);
extern int (APIENTRYP gwglGetSwapIntervalEXT)();

#ifndef WGL_EXT_swap_control_tear
#define WGL_EXT_swap_control_tear
#endif

#ifndef WGL_I3D_digital_video_control
#define WGL_I3D_digital_video_control
#define WGL_DIGITAL_VIDEO_CURSOR_ALPHA_FRAMEBUFFER_I3D 0x2050
#define WGL_DIGITAL_VIDEO_CURSOR_ALPHA_VALUE_I3D 0x2051
#define WGL_DIGITAL_VIDEO_CURSOR_INCLUDED_I3D 0x2052
#define WGL_DIGITAL_VIDEO_GAMMA_CORRECTED_I3D 0x2053
#endif
extern BOOL (APIENTRYP gwglGetDigitalVideoParametersI3D)(HDC hDC, int iAttribute, int *piValue);
extern BOOL (APIENTRYP gwglSetDigitalVideoParametersI3D)(HDC hDC, int iAttribute, const int *piValue);

#ifndef WGL_I3D_gamma
#define WGL_I3D_gamma
#define WGL_GAMMA_TABLE_SIZE_I3D 0x204E
#define WGL_GAMMA_EXCLUDE_DESKTOP_I3D 0x204F
#endif
extern BOOL (APIENTRYP gwglGetGammaTableParametersI3D)(HDC hDC, int iAttribute, int *piValue);
extern BOOL (APIENTRYP gwglSetGammaTableParametersI3D)(HDC hDC, int iAttribute, const int *piValue);
extern BOOL (APIENTRYP gwglGetGammaTableI3D)(HDC hDC, int iEntries, USHORT *puRed, USHORT *puGreen, USHORT *puBlue);
extern BOOL (APIENTRYP gwglSetGammaTableI3D)(HDC hDC, int iEntries, const USHORT *puRed, const USHORT *puGreen, const USHORT *puBlue);

#ifndef WGL_I3D_genlock
#define WGL_I3D_genlock
#define WGL_GENLOCK_SOURCE_MULTIVIEW_I3D 0x2044
#define WGL_GENLOCK_SOURCE_EXTERNAL_SYNC_I3D 0x2045
#define WGL_GENLOCK_SOURCE_EXTERNAL_FIELD_I3D 0x2046
#define WGL_GENLOCK_SOURCE_EXTERNAL_TTL_I3D 0x2047
#define WGL_GENLOCK_SOURCE_DIGITAL_SYNC_I3D 0x2048
#define WGL_GENLOCK_SOURCE_DIGITAL_FIELD_I3D 0x2049
#define WGL_GENLOCK_SOURCE_EDGE_FALLING_I3D 0x204A
#define WGL_GENLOCK_SOURCE_EDGE_RISING_I3D 0x204B
#define WGL_GENLOCK_SOURCE_EDGE_BOTH_I3D 0x204C
#endif
extern BOOL (APIENTRYP gwglEnableGenlockI3D)(HDC hDC);
extern BOOL (APIENTRYP gwglDisableGenlockI3D)(HDC hDC);
extern BOOL (APIENTRYP gwglIsEnabledGenlockI3D)(HDC hDC, BOOL *pFlag);
extern BOOL (APIENTRYP gwglGenlockSourceI3D)(HDC hDC, UINT uSource);
extern BOOL (APIENTRYP gwglGetGenlockSourceI3D)(HDC hDC, UINT *uSource);
extern BOOL (APIENTRYP gwglGenlockSourceEdgeI3D)(HDC hDC, UINT uEdge);
extern BOOL (APIENTRYP gwglGetGenlockSourceEdgeI3D)(HDC hDC, UINT *uEdge);
extern BOOL (APIENTRYP gwglGenlockSampleRateI3D)(HDC hDC, UINT uRate);
extern BOOL (APIENTRYP gwglGetGenlockSampleRateI3D)(HDC hDC, UINT *uRate);
extern BOOL (APIENTRYP gwglGenlockSourceDelayI3D)(HDC hDC, UINT uDelay);
extern BOOL (APIENTRYP gwglGetGenlockSourceDelayI3D)(HDC hDC, UINT *uDelay);
extern BOOL (APIENTRYP gwglQueryGenlockMaxSourceDelayI3D)(HDC hDC, UINT *uMaxLineDelay, UINT *uMaxPixelDelay);

#ifndef WGL_I3D_image_buffer
#define WGL_I3D_image_buffer
#define WGL_IMAGE_BUFFER_MIN_ACCESS_I3D 0x00000001
#define WGL_IMAGE_BUFFER_LOCK_I3D 0x00000002
#endif
extern LPVOID (APIENTRYP gwglCreateImageBufferI3D)(HDC hDC, DWORD dwSize, UINT uFlags);
extern BOOL (APIENTRYP gwglDestroyImageBufferI3D)(HDC hDC, LPVOID pAddress);
extern BOOL (APIENTRYP gwglAssociateImageBufferEventsI3D)(HDC hDC, const HANDLE *pEvent, const LPVOID *pAddress, const DWORD *pSize, UINT count);
extern BOOL (APIENTRYP gwglReleaseImageBufferEventsI3D)(HDC hDC, const LPVOID *pAddress, UINT count);

#ifndef WGL_I3D_swap_frame_lock
#define WGL_I3D_swap_frame_lock
#endif
extern BOOL (APIENTRYP gwglEnableFrameLockI3D)();
extern BOOL (APIENTRYP gwglDisableFrameLockI3D)();
extern BOOL (APIENTRYP gwglIsEnabledFrameLockI3D)(BOOL *pFlag);
extern BOOL (APIENTRYP gwglQueryFrameLockMasterI3D)(BOOL *pFlag);

#ifndef WGL_I3D_swap_frame_usage
#define WGL_I3D_swap_frame_usage
#endif
extern BOOL (APIENTRYP gwglGetFrameUsageI3D)(float *pUsage);
extern BOOL (APIENTRYP gwglBeginFrameTrackingI3D)();
extern BOOL (APIENTRYP gwglEndFrameTrackingI3D)();
extern BOOL (APIENTRYP gwglQueryFrameTrackingI3D)(DWORD *pFrameCount, DWORD *pMissedFrames, float *pLastMissedUsage);

#ifndef WGL_NV_copy_image
#define WGL_NV_copy_image
#endif
extern BOOL (APIENTRYP gwglCopyImageSubDataNV)(HGLRC hSrcRC, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, HGLRC hDstRC, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth);

#ifndef WGL_NV_delay_before_swap
#define WGL_NV_delay_before_swap
#endif
extern BOOL (APIENTRYP gwglDelayBeforeSwapNV)(HDC hDC, GLfloat seconds);

#ifndef WGL_NV_DX_interop
#define WGL_NV_DX_interop
#define WGL_ACCESS_READ_ONLY_NV 0x00000000
#define WGL_ACCESS_READ_WRITE_NV 0x00000001
#define WGL_ACCESS_WRITE_DISCARD_NV 0x00000002
#endif
extern BOOL (APIENTRYP gwglDXSetResourceShareHandleNV)(void *dxObject, HANDLE shareHandle);
extern HANDLE (APIENTRYP gwglDXOpenDeviceNV)(void *dxDevice);
extern BOOL (APIENTRYP gwglDXCloseDeviceNV)(HANDLE hDevice);
extern HANDLE (APIENTRYP gwglDXRegisterObjectNV)(HANDLE hDevice, void *dxObject, GLuint name, GLenum type, GLenum access);
extern BOOL (APIENTRYP gwglDXUnregisterObjectNV)(HANDLE hDevice, HANDLE hObject);
extern BOOL (APIENTRYP gwglDXObjectAccessNV)(HANDLE hObject, GLenum access);
extern BOOL (APIENTRYP gwglDXLockObjectsNV)(HANDLE hDevice, GLint count, HANDLE *hObjects);
extern BOOL (APIENTRYP gwglDXUnlockObjectsNV)(HANDLE hDevice, GLint count, HANDLE *hObjects);

#ifndef WGL_NV_DX_interop2
#define WGL_NV_DX_interop2
#endif

#ifndef WGL_NV_float_buffer
#define WGL_NV_float_buffer
#define WGL_FLOAT_COMPONENTS_NV 0x20B0
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_R_NV 0x20B1
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RG_NV 0x20B2
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGB_NV 0x20B3
#define WGL_BIND_TO_TEXTURE_RECTANGLE_FLOAT_RGBA_NV 0x20B4
#define WGL_TEXTURE_FLOAT_R_NV 0x20B5
#define WGL_TEXTURE_FLOAT_RG_NV 0x20B6
#define WGL_TEXTURE_FLOAT_RGB_NV 0x20B7
#define WGL_TEXTURE_FLOAT_RGBA_NV 0x20B8
#endif

#ifndef WGL_NV_gpu_affinity
#define WGL_NV_gpu_affinity
#define ERROR_INCOMPATIBLE_AFFINITY_MASKS_NV 0x20D0
#define ERROR_MISSING_AFFINITY_MASK_NV 0x20D1
#endif
extern BOOL (APIENTRYP gwglEnumGpusNV)(UINT iGpuIndex, HGPUNV *phGpu);
extern BOOL (APIENTRYP gwglEnumGpuDevicesNV)(HGPUNV hGpu, UINT iDeviceIndex, PGPU_DEVICE lpGpuDevice);
extern HDC (APIENTRYP gwglCreateAffinityDCNV)(const HGPUNV *phGpuList);
extern BOOL (APIENTRYP gwglEnumGpusFromAffinityDCNV)(HDC hAffinityDC, UINT iGpuIndex, HGPUNV *hGpu);
extern BOOL (APIENTRYP gwglDeleteDCNV)(HDC hdc);

#ifndef WGL_NV_multisample_coverage
#define WGL_NV_multisample_coverage
#define WGL_COVERAGE_SAMPLES_NV 0x2042
#define WGL_COLOR_SAMPLES_NV 0x20B9
#endif

#ifndef WGL_NV_present_video
#define WGL_NV_present_video
#define WGL_NUM_VIDEO_SLOTS_NV 0x20F0
#endif
extern int (APIENTRYP gwglEnumerateVideoDevicesNV)(HDC hDC, HVIDEOOUTPUTDEVICENV *phDeviceList);
extern BOOL (APIENTRYP gwglBindVideoDeviceNV)(HDC hDC, unsigned int uVideoSlot, HVIDEOOUTPUTDEVICENV hVideoDevice, const int *piAttribList);
extern BOOL (APIENTRYP gwglQueryCurrentContextNV)(int iAttribute, int *piValue);

#ifndef WGL_NV_render_depth_texture
#define WGL_NV_render_depth_texture
#define WGL_BIND_TO_TEXTURE_DEPTH_NV 0x20A3
#define WGL_BIND_TO_TEXTURE_RECTANGLE_DEPTH_NV 0x20A4
#define WGL_DEPTH_TEXTURE_FORMAT_NV 0x20A5
#define WGL_TEXTURE_DEPTH_COMPONENT_NV 0x20A6
#define WGL_DEPTH_COMPONENT_NV 0x20A7
#endif

#ifndef WGL_NV_render_texture_rectangle
#define WGL_NV_render_texture_rectangle
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGB_NV 0x20A0
#define WGL_BIND_TO_TEXTURE_RECTANGLE_RGBA_NV 0x20A1
#define WGL_TEXTURE_RECTANGLE_NV 0x20A2
#endif

#ifndef WGL_NV_swap_group
#define WGL_NV_swap_group
#endif
extern BOOL (APIENTRYP gwglJoinSwapGroupNV)(HDC hDC, GLuint group);
extern BOOL (APIENTRYP gwglBindSwapBarrierNV)(GLuint group, GLuint barrier);
extern BOOL (APIENTRYP gwglQuerySwapGroupNV)(HDC hDC, GLuint *group, GLuint *barrier);
extern BOOL (APIENTRYP gwglQueryMaxSwapGroupsNV)(HDC hDC, GLuint *maxGroups, GLuint *maxBarriers);
extern BOOL (APIENTRYP gwglQueryFrameCountNV)(HDC hDC, GLuint *count);
extern BOOL (APIENTRYP gwglResetFrameCountNV)(HDC hDC);

#ifndef WGL_NV_video_capture
#define WGL_NV_video_capture
#define WGL_UNIQUE_ID_NV 0x20CE
#define WGL_NUM_VIDEO_CAPTURE_SLOTS_NV 0x20CF
#endif
extern BOOL (APIENTRYP gwglBindVideoCaptureDeviceNV)(UINT uVideoSlot, HVIDEOINPUTDEVICENV hDevice);
extern UINT (APIENTRYP gwglEnumerateVideoCaptureDevicesNV)(HDC hDc, HVIDEOINPUTDEVICENV *phDeviceList);
extern BOOL (APIENTRYP gwglLockVideoCaptureDeviceNV)(HDC hDc, HVIDEOINPUTDEVICENV hDevice);
extern BOOL (APIENTRYP gwglQueryVideoCaptureDeviceNV)(HDC hDc, HVIDEOINPUTDEVICENV hDevice, int iAttribute, int *piValue);
extern BOOL (APIENTRYP gwglReleaseVideoCaptureDeviceNV)(HDC hDc, HVIDEOINPUTDEVICENV hDevice);

#ifndef WGL_NV_video_output
#define WGL_NV_video_output
#define WGL_BIND_TO_VIDEO_RGB_NV 0x20C0
#define WGL_BIND_TO_VIDEO_RGBA_NV 0x20C1
#define WGL_BIND_TO_VIDEO_RGB_AND_DEPTH_NV 0x20C2
#define WGL_VIDEO_OUT_COLOR_NV 0x20C3
#define WGL_VIDEO_OUT_ALPHA_NV 0x20C4
#define WGL_VIDEO_OUT_DEPTH_NV 0x20C5
#define WGL_VIDEO_OUT_COLOR_AND_ALPHA_NV 0x20C6
#define WGL_VIDEO_OUT_COLOR_AND_DEPTH_NV 0x20C7
#define WGL_VIDEO_OUT_FRAME 0x20C8
#define WGL_VIDEO_OUT_FIELD_1 0x20C9
#define WGL_VIDEO_OUT_FIELD_2 0x20CA
#define WGL_VIDEO_OUT_STACKED_FIELDS_1_2 0x20CB
#define WGL_VIDEO_OUT_STACKED_FIELDS_2_1 0x20CC
#endif
extern BOOL (APIENTRYP gwglGetVideoDeviceNV)(HDC hDC, int numDevices, HPVIDEODEV *hVideoDevice);
extern BOOL (APIENTRYP gwglReleaseVideoDeviceNV)(HPVIDEODEV hVideoDevice);
extern BOOL (APIENTRYP gwglBindVideoImageNV)(HPVIDEODEV hVideoDevice, HPBUFFERARB hPbuffer, int iVideoBuffer);
extern BOOL (APIENTRYP gwglReleaseVideoImageNV)(HPBUFFERARB hPbuffer, int iVideoBuffer);
extern BOOL (APIENTRYP gwglSendPbufferToVideoNV)(HPBUFFERARB hPbuffer, int iBufferType, unsigned long *pulCounterPbuffer, BOOL bBlock);
extern BOOL (APIENTRYP gwglGetVideoInfoNV)(HPVIDEODEV hpVideoDevice, unsigned long *pulCounterOutputPbuffer, unsigned long *pulCounterOutputVideo);

#ifndef WGL_NV_vertex_array_range
#define WGL_NV_vertex_array_range
#endif
extern void * (APIENTRYP gwglAllocateMemoryNV)(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
extern void (APIENTRYP gwglFreeMemoryNV)(void *pointer);

#ifndef WGL_OML_sync_control
#define WGL_OML_sync_control
#endif
extern BOOL (APIENTRYP gwglGetSyncValuesOML)(HDC hdc, INT64 *ust, INT64 *msc, INT64 *sbc);
extern BOOL (APIENTRYP gwglGetMscRateOML)(HDC hdc, INT32 *numerator, INT32 *denominator);
extern INT64 (APIENTRYP gwglSwapBuffersMscOML)(HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder);
extern INT64 (APIENTRYP gwglSwapLayerBuffersMscOML)(HDC hdc, int fuPlanes, INT64 target_msc, INT64 divisor, INT64 remainder);
extern BOOL (APIENTRYP gwglWaitForMscOML)(HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder, INT64 *ust, INT64 *msc, INT64 *sbc);
extern BOOL (APIENTRYP gwglWaitForSbcOML)(HDC hdc, INT64 target_sbc, INT64 *ust, INT64 *msc, INT64 *sbc);

/* 54 extensions */
typedef struct {
	int _WGL_3DFX_multisample : 1;
	int _WGL_3DL_stereo_control : 1;
	int _WGL_AMD_gpu_association : 1;
	int _WGL_ARB_buffer_region : 1;
	int _WGL_ARB_context_flush_control : 1;
	int _WGL_ARB_create_context : 1;
	int _WGL_ARB_create_context_profile : 1;
	int _WGL_ARB_create_context_robustness : 1;
	int _WGL_ARB_extensions_string : 1;
	int _WGL_ARB_framebuffer_sRGB : 1;
	int _WGL_ARB_make_current_read : 1;
	int _WGL_ARB_multisample : 1;
	int _WGL_ARB_pbuffer : 1;
	int _WGL_ARB_pixel_format : 1;
	int _WGL_ARB_pixel_format_float : 1;
	int _WGL_ARB_render_texture : 1;
	int _WGL_ARB_robustness_application_isolation : 1;
	int _WGL_ARB_robustness_share_group_isolation : 1;
	int _WGL_ATI_pixel_format_float : 1;
	int _WGL_EXT_colorspace : 1;
	int _WGL_EXT_create_context_es_profile : 1;
	int _WGL_EXT_create_context_es2_profile : 1;
	int _WGL_EXT_depth_float : 1;
	int _WGL_EXT_display_color_table : 1;
	int _WGL_EXT_extensions_string : 1;
	int _WGL_EXT_framebuffer_sRGB : 1;
	int _WGL_EXT_make_current_read : 1;
	int _WGL_EXT_multisample : 1;
	int _WGL_EXT_pbuffer : 1;
	int _WGL_EXT_pixel_format : 1;
	int _WGL_EXT_pixel_format_packed_float : 1;
	int _WGL_EXT_swap_control : 1;
	int _WGL_EXT_swap_control_tear : 1;
	int _WGL_I3D_digital_video_control : 1;
	int _WGL_I3D_gamma : 1;
	int _WGL_I3D_genlock : 1;
	int _WGL_I3D_image_buffer : 1;
	int _WGL_I3D_swap_frame_lock : 1;
	int _WGL_I3D_swap_frame_usage : 1;
	int _WGL_NV_copy_image : 1;
	int _WGL_NV_delay_before_swap : 1;
	int _WGL_NV_DX_interop : 1;
	int _WGL_NV_DX_interop2 : 1;
	int _WGL_NV_float_buffer : 1;
	int _WGL_NV_gpu_affinity : 1;
	int _WGL_NV_multisample_coverage : 1;
	int _WGL_NV_present_video : 1;
	int _WGL_NV_render_depth_texture : 1;
	int _WGL_NV_render_texture_rectangle : 1;
	int _WGL_NV_swap_group : 1;
	int _WGL_NV_video_capture : 1;
	int _WGL_NV_video_output : 1;
	int _WGL_NV_vertex_array_range : 1;
	int _WGL_OML_sync_control : 1;
} gwglext_t;

extern gwglext_t gwglext;

extern void gwgl_init(HDC hdc, int enableDebug);
extern void gwgl_rebind(int enableDebug);

#ifdef __cplusplus
}
#endif

#endif /* __GWGL_H__ */
