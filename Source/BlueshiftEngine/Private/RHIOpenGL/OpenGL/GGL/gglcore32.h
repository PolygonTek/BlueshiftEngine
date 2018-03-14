/*********************************************************************************************
 *
 * gglcore32.h
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

#ifndef __GGLCORE32_H__
#define __GGLCORE32_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <windows.h>
#include <gl/gl.h>
#endif

#if defined(__APPLE__)
#define GL_GLEXT_LEGACY
#include <OpenGL/gl.h>
#endif

#if defined(__linux__)
#include <gl/gl.h>
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

#include <stddef.h>
#ifndef GLEXT_64_TYPES_DEFINED
/* This code block is duplicated in glxext.h, so must be protected */
#define GLEXT_64_TYPES_DEFINED
/* Define int32_t, int64_t, and uint64_t types for UST/MSC */
/* (as used in the GL_EXT_timer_query extension). */
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
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef void GLvoid;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLclampx;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void *GLeglImageOES;
typedef char GLchar;
typedef char GLcharARB;
#ifdef __APPLE__
typedef void *GLhandleARB;
#else
typedef unsigned int GLhandleARB;
#endif
typedef unsigned short GLhalfARB;
typedef unsigned short GLhalf;
typedef GLint GLfixed;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
typedef int64_t GLint64;
typedef uint64_t GLuint64;
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
typedef int64_t GLint64EXT;
typedef uint64_t GLuint64EXT;
typedef struct __GLsync *GLsync;
struct _cl_context;
struct _cl_event;
typedef void (APIENTRY *GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef void (APIENTRY *GLDEBUGPROCARB)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef void (APIENTRY *GLDEBUGPROCKHR)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef void (APIENTRY *GLDEBUGPROCAMD)(GLuint id,GLenum category,GLenum severity,GLsizei length,const GLchar *message,void *userParam);
typedef unsigned short GLhalfNV;
typedef GLintptr GLvdpauSurfaceNV;

#ifndef GL_VERSION_1_0
#define GL_VERSION_1_0
#endif
extern void (APIENTRYP gglCullFace)(GLenum mode);
extern void (APIENTRYP gglFrontFace)(GLenum mode);
extern void (APIENTRYP gglHint)(GLenum target, GLenum mode);
extern void (APIENTRYP gglLineWidth)(GLfloat width);
extern void (APIENTRYP gglPointSize)(GLfloat size);
extern void (APIENTRYP gglPolygonMode)(GLenum face, GLenum mode);
extern void (APIENTRYP gglScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglTexParameterf)(GLenum target, GLenum pname, GLfloat param);
extern void (APIENTRYP gglTexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglTexParameteri)(GLenum target, GLenum pname, GLint param);
extern void (APIENTRYP gglTexParameteriv)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglTexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglDrawBuffer)(GLenum buf);
extern void (APIENTRYP gglClear)(GLbitfield mask);
extern void (APIENTRYP gglClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void (APIENTRYP gglClearStencil)(GLint s);
extern void (APIENTRYP gglClearDepth)(GLdouble depth);
extern void (APIENTRYP gglStencilMask)(GLuint mask);
extern void (APIENTRYP gglColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern void (APIENTRYP gglDepthMask)(GLboolean flag);
extern void (APIENTRYP gglDisable)(GLenum cap);
extern void (APIENTRYP gglEnable)(GLenum cap);
extern void (APIENTRYP gglFinish)();
extern void (APIENTRYP gglFlush)();
extern void (APIENTRYP gglBlendFunc)(GLenum sfactor, GLenum dfactor);
extern void (APIENTRYP gglLogicOp)(GLenum opcode);
extern void (APIENTRYP gglStencilFunc)(GLenum func, GLint ref, GLuint mask);
extern void (APIENTRYP gglStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
extern void (APIENTRYP gglDepthFunc)(GLenum func);
extern void (APIENTRYP gglPixelStoref)(GLenum pname, GLfloat param);
extern void (APIENTRYP gglPixelStorei)(GLenum pname, GLint param);
extern void (APIENTRYP gglReadBuffer)(GLenum src);
extern void (APIENTRYP gglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
extern void (APIENTRYP gglGetBooleanv)(GLenum pname, GLboolean *data);
extern void (APIENTRYP gglGetDoublev)(GLenum pname, GLdouble *data);
extern GLenum (APIENTRYP gglGetError)();
extern void (APIENTRYP gglGetFloatv)(GLenum pname, GLfloat *data);
extern void (APIENTRYP gglGetIntegerv)(GLenum pname, GLint *data);
extern const GLubyte * (APIENTRYP gglGetString)(GLenum name);
extern void (APIENTRYP gglGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, void *pixels);
extern void (APIENTRYP gglGetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
extern GLboolean (APIENTRYP gglIsEnabled)(GLenum cap);
extern void (APIENTRYP gglDepthRange)(GLdouble znear, GLdouble zfar);
extern void (APIENTRYP gglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

#ifndef GL_VERSION_1_1
#define GL_VERSION_1_1
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_QUADS 0x0007
#define GL_NEVER 0x0200
#define GL_LESS 0x0201
#define GL_EQUAL 0x0202
#define GL_LEQUAL 0x0203
#define GL_GREATER 0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL 0x0206
#define GL_ALWAYS 0x0207
#define GL_ZERO 0
#define GL_ONE 1
#define GL_SRC_COLOR 0x0300
#define GL_ONE_MINUS_SRC_COLOR 0x0301
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_DST_ALPHA 0x0304
#define GL_ONE_MINUS_DST_ALPHA 0x0305
#define GL_DST_COLOR 0x0306
#define GL_ONE_MINUS_DST_COLOR 0x0307
#define GL_SRC_ALPHA_SATURATE 0x0308
#define GL_NONE 0
#define GL_FRONT_LEFT 0x0400
#define GL_FRONT_RIGHT 0x0401
#define GL_BACK_LEFT 0x0402
#define GL_BACK_RIGHT 0x0403
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_LEFT 0x0406
#define GL_RIGHT 0x0407
#define GL_FRONT_AND_BACK 0x0408
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_OUT_OF_MEMORY 0x0505
#define GL_CW 0x0900
#define GL_CCW 0x0901
#define GL_POINT_SIZE 0x0B11
#define GL_POINT_SIZE_RANGE 0x0B12
#define GL_POINT_SIZE_GRANULARITY 0x0B13
#define GL_LINE_SMOOTH 0x0B20
#define GL_LINE_WIDTH 0x0B21
#define GL_LINE_WIDTH_RANGE 0x0B22
#define GL_LINE_WIDTH_GRANULARITY 0x0B23
#define GL_POLYGON_MODE 0x0B40
#define GL_POLYGON_SMOOTH 0x0B41
#define GL_CULL_FACE 0x0B44
#define GL_CULL_FACE_MODE 0x0B45
#define GL_FRONT_FACE 0x0B46
#define GL_DEPTH_RANGE 0x0B70
#define GL_DEPTH_TEST 0x0B71
#define GL_DEPTH_WRITEMASK 0x0B72
#define GL_DEPTH_CLEAR_VALUE 0x0B73
#define GL_DEPTH_FUNC 0x0B74
#define GL_STENCIL_TEST 0x0B90
#define GL_STENCIL_CLEAR_VALUE 0x0B91
#define GL_STENCIL_FUNC 0x0B92
#define GL_STENCIL_VALUE_MASK 0x0B93
#define GL_STENCIL_FAIL 0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL 0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS 0x0B96
#define GL_STENCIL_REF 0x0B97
#define GL_STENCIL_WRITEMASK 0x0B98
#define GL_VIEWPORT 0x0BA2
#define GL_DITHER 0x0BD0
#define GL_BLEND_DST 0x0BE0
#define GL_BLEND_SRC 0x0BE1
#define GL_BLEND 0x0BE2
#define GL_LOGIC_OP_MODE 0x0BF0
#define GL_COLOR_LOGIC_OP 0x0BF2
#define GL_DRAW_BUFFER 0x0C01
#define GL_READ_BUFFER 0x0C02
#define GL_SCISSOR_BOX 0x0C10
#define GL_SCISSOR_TEST 0x0C11
#define GL_COLOR_CLEAR_VALUE 0x0C22
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_DOUBLEBUFFER 0x0C32
#define GL_STEREO 0x0C33
#define GL_LINE_SMOOTH_HINT 0x0C52
#define GL_POLYGON_SMOOTH_HINT 0x0C53
#define GL_UNPACK_SWAP_BYTES 0x0CF0
#define GL_UNPACK_LSB_FIRST 0x0CF1
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_SWAP_BYTES 0x0D00
#define GL_PACK_LSB_FIRST 0x0D01
#define GL_PACK_ROW_LENGTH 0x0D02
#define GL_PACK_SKIP_ROWS 0x0D03
#define GL_PACK_SKIP_PIXELS 0x0D04
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_MAX_VIEWPORT_DIMS 0x0D3A
#define GL_SUBPIXEL_BITS 0x0D50
#define GL_TEXTURE_1D 0x0DE0
#define GL_TEXTURE_2D 0x0DE1
#define GL_POLYGON_OFFSET_UNITS 0x2A00
#define GL_POLYGON_OFFSET_POINT 0x2A01
#define GL_POLYGON_OFFSET_LINE 0x2A02
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_POLYGON_OFFSET_FACTOR 0x8038
#define GL_TEXTURE_BINDING_1D 0x8068
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_TEXTURE_WIDTH 0x1000
#define GL_TEXTURE_HEIGHT 0x1001
#define GL_TEXTURE_INTERNAL_FORMAT 0x1003
#define GL_TEXTURE_BORDER_COLOR 0x1004
#define GL_TEXTURE_RED_SIZE 0x805C
#define GL_TEXTURE_GREEN_SIZE 0x805D
#define GL_TEXTURE_BLUE_SIZE 0x805E
#define GL_TEXTURE_ALPHA_SIZE 0x805F
#define GL_DONT_CARE 0x1100
#define GL_FASTEST 0x1101
#define GL_NICEST 0x1102
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_DOUBLE 0x140A
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_CLEAR 0x1500
#define GL_AND 0x1501
#define GL_AND_REVERSE 0x1502
#define GL_COPY 0x1503
#define GL_AND_INVERTED 0x1504
#define GL_NOOP 0x1505
#define GL_XOR 0x1506
#define GL_OR 0x1507
#define GL_NOR 0x1508
#define GL_EQUIV 0x1509
#define GL_INVERT 0x150A
#define GL_OR_REVERSE 0x150B
#define GL_COPY_INVERTED 0x150C
#define GL_OR_INVERTED 0x150D
#define GL_NAND 0x150E
#define GL_SET 0x150F
#define GL_TEXTURE 0x1702
#define GL_COLOR 0x1800
#define GL_DEPTH 0x1801
#define GL_STENCIL 0x1802
#define GL_STENCIL_INDEX 0x1901
#define GL_DEPTH_COMPONENT 0x1902
#define GL_RED 0x1903
#define GL_GREEN 0x1904
#define GL_BLUE 0x1905
#define GL_ALPHA 0x1906
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_POINT 0x1B00
#define GL_LINE 0x1B01
#define GL_FILL 0x1B02
#define GL_KEEP 0x1E00
#define GL_REPLACE 0x1E01
#define GL_INCR 0x1E02
#define GL_DECR 0x1E03
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_PROXY_TEXTURE_1D 0x8063
#define GL_PROXY_TEXTURE_2D 0x8064
#define GL_REPEAT 0x2901
#define GL_R3_G3_B2 0x2A10
#define GL_RGB4 0x804F
#define GL_RGB5 0x8050
#define GL_RGB8 0x8051
#define GL_RGB10 0x8052
#define GL_RGB12 0x8053
#define GL_RGB16 0x8054
#define GL_RGBA2 0x8055
#define GL_RGBA4 0x8056
#define GL_RGB5_A1 0x8057
#define GL_RGBA8 0x8058
#define GL_RGB10_A2 0x8059
#define GL_RGBA12 0x805A
#define GL_RGBA16 0x805B
#define GL_FOG 0x0B60
#define GL_VERTEX_ARRAY 0x8074
#define GL_EYE_PLANE 0x2502
#endif
extern void (APIENTRYP gglDrawArrays)(GLenum mode, GLint first, GLsizei count);
extern void (APIENTRYP gglDrawElements)(GLenum mode, GLsizei count, GLenum type, const void *indices);
extern void (APIENTRYP gglGetPointerv)(GLenum pname, void **params);
extern void (APIENTRYP gglPolygonOffset)(GLfloat factor, GLfloat units);
extern void (APIENTRYP gglCopyTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
extern void (APIENTRYP gglCopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern void (APIENTRYP gglCopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern void (APIENTRYP gglCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglBindTexture)(GLenum target, GLuint texture);
extern void (APIENTRYP gglDeleteTextures)(GLsizei n, const GLuint *textures);
extern void (APIENTRYP gglGenTextures)(GLsizei n, GLuint *textures);
extern GLboolean (APIENTRYP gglIsTexture)(GLuint texture);

#ifndef GL_VERSION_1_2
#define GL_VERSION_1_2
#define GL_UNSIGNED_BYTE_3_3_2 0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_UNSIGNED_INT_8_8_8_8 0x8035
#define GL_UNSIGNED_INT_10_10_10_2 0x8036
#define GL_TEXTURE_BINDING_3D 0x806A
#define GL_PACK_SKIP_IMAGES 0x806B
#define GL_PACK_IMAGE_HEIGHT 0x806C
#define GL_UNPACK_SKIP_IMAGES 0x806D
#define GL_UNPACK_IMAGE_HEIGHT 0x806E
#define GL_TEXTURE_3D 0x806F
#define GL_PROXY_TEXTURE_3D 0x8070
#define GL_TEXTURE_DEPTH 0x8071
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_MAX_3D_TEXTURE_SIZE 0x8073
#define GL_UNSIGNED_BYTE_2_3_3_REV 0x8362
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV 0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV 0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV 0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#define GL_BGR 0x80E0
#define GL_BGRA 0x80E1
#define GL_MAX_ELEMENTS_VERTICES 0x80E8
#define GL_MAX_ELEMENTS_INDICES 0x80E9
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_TEXTURE_MIN_LOD 0x813A
#define GL_TEXTURE_MAX_LOD 0x813B
#define GL_TEXTURE_BASE_LEVEL 0x813C
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_SMOOTH_POINT_SIZE_RANGE 0x0B12
#define GL_SMOOTH_POINT_SIZE_GRANULARITY 0x0B13
#define GL_SMOOTH_LINE_WIDTH_RANGE 0x0B22
#define GL_SMOOTH_LINE_WIDTH_GRANULARITY 0x0B23
#define GL_ALIASED_LINE_WIDTH_RANGE 0x846E
#endif
extern void (APIENTRYP gglDrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
extern void (APIENTRYP gglTexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglCopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);

#ifndef GL_VERSION_1_3
#define GL_VERSION_1_3
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5
#define GL_TEXTURE6 0x84C6
#define GL_TEXTURE7 0x84C7
#define GL_TEXTURE8 0x84C8
#define GL_TEXTURE9 0x84C9
#define GL_TEXTURE10 0x84CA
#define GL_TEXTURE11 0x84CB
#define GL_TEXTURE12 0x84CC
#define GL_TEXTURE13 0x84CD
#define GL_TEXTURE14 0x84CE
#define GL_TEXTURE15 0x84CF
#define GL_TEXTURE16 0x84D0
#define GL_TEXTURE17 0x84D1
#define GL_TEXTURE18 0x84D2
#define GL_TEXTURE19 0x84D3
#define GL_TEXTURE20 0x84D4
#define GL_TEXTURE21 0x84D5
#define GL_TEXTURE22 0x84D6
#define GL_TEXTURE23 0x84D7
#define GL_TEXTURE24 0x84D8
#define GL_TEXTURE25 0x84D9
#define GL_TEXTURE26 0x84DA
#define GL_TEXTURE27 0x84DB
#define GL_TEXTURE28 0x84DC
#define GL_TEXTURE29 0x84DD
#define GL_TEXTURE30 0x84DE
#define GL_TEXTURE31 0x84DF
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_MULTISAMPLE 0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_SAMPLE_ALPHA_TO_ONE 0x809F
#define GL_SAMPLE_COVERAGE 0x80A0
#define GL_SAMPLE_BUFFERS 0x80A8
#define GL_SAMPLES 0x80A9
#define GL_SAMPLE_COVERAGE_VALUE 0x80AA
#define GL_SAMPLE_COVERAGE_INVERT 0x80AB
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP 0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP 0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE 0x851C
#define GL_COMPRESSED_RGB 0x84ED
#define GL_COMPRESSED_RGBA 0x84EE
#define GL_TEXTURE_COMPRESSION_HINT 0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE 0x86A0
#define GL_TEXTURE_COMPRESSED 0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS 0x86A3
#define GL_CLAMP_TO_BORDER 0x812D
#define GL_PRIMARY_COLOR 0x8577
#endif
extern void (APIENTRYP gglActiveTexture)(GLenum texture);
extern void (APIENTRYP gglSampleCoverage)(GLfloat value, GLboolean invert);
extern void (APIENTRYP gglCompressedTexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglGetCompressedTexImage)(GLenum target, GLint level, void *img);

#ifndef GL_VERSION_1_4
#define GL_VERSION_1_4
#define GL_BLEND_DST_RGB 0x80C8
#define GL_BLEND_SRC_RGB 0x80C9
#define GL_BLEND_DST_ALPHA 0x80CA
#define GL_BLEND_SRC_ALPHA 0x80CB
#define GL_POINT_FADE_THRESHOLD_SIZE 0x8128
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_DEPTH_COMPONENT32 0x81A7
#define GL_MIRRORED_REPEAT 0x8370
#define GL_MAX_TEXTURE_LOD_BIAS 0x84FD
#define GL_TEXTURE_LOD_BIAS 0x8501
#define GL_INCR_WRAP 0x8507
#define GL_DECR_WRAP 0x8508
#define GL_TEXTURE_DEPTH_SIZE 0x884A
#define GL_TEXTURE_COMPARE_MODE 0x884C
#define GL_TEXTURE_COMPARE_FUNC 0x884D
#define GL_FUNC_ADD 0x8006
#define GL_FUNC_SUBTRACT 0x800A
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_MIN 0x8007
#define GL_MAX 0x8008
#define GL_CONSTANT_COLOR 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_CONSTANT_ALPHA 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#endif
extern void (APIENTRYP gglBlendFuncSeparate)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
extern void (APIENTRYP gglMultiDrawArrays)(GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount);
extern void (APIENTRYP gglMultiDrawElements)(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount);
extern void (APIENTRYP gglPointParameterf)(GLenum pname, GLfloat param);
extern void (APIENTRYP gglPointParameterfv)(GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglPointParameteri)(GLenum pname, GLint param);
extern void (APIENTRYP gglPointParameteriv)(GLenum pname, const GLint *params);
extern void (APIENTRYP gglBlendColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void (APIENTRYP gglBlendEquation)(GLenum mode);

#ifndef GL_VERSION_1_5
#define GL_VERSION_1_5
#define GL_BUFFER_SIZE 0x8764
#define GL_BUFFER_USAGE 0x8765
#define GL_QUERY_COUNTER_BITS 0x8864
#define GL_CURRENT_QUERY 0x8865
#define GL_QUERY_RESULT 0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_ARRAY_BUFFER_BINDING 0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING 0x889F
#define GL_READ_ONLY 0x88B8
#define GL_WRITE_ONLY 0x88B9
#define GL_READ_WRITE 0x88BA
#define GL_BUFFER_ACCESS 0x88BB
#define GL_BUFFER_MAPPED 0x88BC
#define GL_BUFFER_MAP_POINTER 0x88BD
#define GL_STREAM_DRAW 0x88E0
#define GL_STREAM_READ 0x88E1
#define GL_STREAM_COPY 0x88E2
#define GL_STATIC_DRAW 0x88E4
#define GL_STATIC_READ 0x88E5
#define GL_STATIC_COPY 0x88E6
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_DYNAMIC_READ 0x88E9
#define GL_DYNAMIC_COPY 0x88EA
#define GL_SAMPLES_PASSED 0x8914
#define GL_SRC1_ALPHA 0x8589
#endif
extern void (APIENTRYP gglGenQueries)(GLsizei n, GLuint *ids);
extern void (APIENTRYP gglDeleteQueries)(GLsizei n, const GLuint *ids);
extern GLboolean (APIENTRYP gglIsQuery)(GLuint id);
extern void (APIENTRYP gglBeginQuery)(GLenum target, GLuint id);
extern void (APIENTRYP gglEndQuery)(GLenum target);
extern void (APIENTRYP gglGetQueryiv)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetQueryObjectiv)(GLuint id, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetQueryObjectuiv)(GLuint id, GLenum pname, GLuint *params);
extern void (APIENTRYP gglBindBuffer)(GLenum target, GLuint buffer);
extern void (APIENTRYP gglDeleteBuffers)(GLsizei n, const GLuint *buffers);
extern void (APIENTRYP gglGenBuffers)(GLsizei n, GLuint *buffers);
extern GLboolean (APIENTRYP gglIsBuffer)(GLuint buffer);
extern void (APIENTRYP gglBufferData)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
extern void (APIENTRYP gglBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
extern void (APIENTRYP gglGetBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, void *data);
extern void * (APIENTRYP gglMapBuffer)(GLenum target, GLenum access);
extern GLboolean (APIENTRYP gglUnmapBuffer)(GLenum target);
extern void (APIENTRYP gglGetBufferParameteriv)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetBufferPointerv)(GLenum target, GLenum pname, void **params);

#ifndef GL_VERSION_2_0
#define GL_VERSION_2_0
#define GL_BLEND_EQUATION_RGB 0x8009
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE 0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE 0x8625
#define GL_CURRENT_VERTEX_ATTRIB 0x8626
#define GL_VERTEX_PROGRAM_POINT_SIZE 0x8642
#define GL_VERTEX_ATTRIB_ARRAY_POINTER 0x8645
#define GL_STENCIL_BACK_FUNC 0x8800
#define GL_STENCIL_BACK_FAIL 0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL 0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS 0x8803
#define GL_MAX_DRAW_BUFFERS 0x8824
#define GL_DRAW_BUFFER0 0x8825
#define GL_DRAW_BUFFER1 0x8826
#define GL_DRAW_BUFFER2 0x8827
#define GL_DRAW_BUFFER3 0x8828
#define GL_DRAW_BUFFER4 0x8829
#define GL_DRAW_BUFFER5 0x882A
#define GL_DRAW_BUFFER6 0x882B
#define GL_DRAW_BUFFER7 0x882C
#define GL_DRAW_BUFFER8 0x882D
#define GL_DRAW_BUFFER9 0x882E
#define GL_DRAW_BUFFER10 0x882F
#define GL_DRAW_BUFFER11 0x8830
#define GL_DRAW_BUFFER12 0x8831
#define GL_DRAW_BUFFER13 0x8832
#define GL_DRAW_BUFFER14 0x8833
#define GL_DRAW_BUFFER15 0x8834
#define GL_BLEND_EQUATION_ALPHA 0x883D
#define GL_MAX_VERTEX_ATTRIBS 0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED 0x886A
#define GL_MAX_TEXTURE_IMAGE_UNITS 0x8872
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS 0x8B4A
#define GL_MAX_VARYING_FLOATS 0x8B4B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_SHADER_TYPE 0x8B4F
#define GL_FLOAT_VEC2 0x8B50
#define GL_FLOAT_VEC3 0x8B51
#define GL_FLOAT_VEC4 0x8B52
#define GL_INT_VEC2 0x8B53
#define GL_INT_VEC3 0x8B54
#define GL_INT_VEC4 0x8B55
#define GL_BOOL 0x8B56
#define GL_BOOL_VEC2 0x8B57
#define GL_BOOL_VEC3 0x8B58
#define GL_BOOL_VEC4 0x8B59
#define GL_FLOAT_MAT2 0x8B5A
#define GL_FLOAT_MAT3 0x8B5B
#define GL_FLOAT_MAT4 0x8B5C
#define GL_SAMPLER_1D 0x8B5D
#define GL_SAMPLER_2D 0x8B5E
#define GL_SAMPLER_3D 0x8B5F
#define GL_SAMPLER_CUBE 0x8B60
#define GL_SAMPLER_1D_SHADOW 0x8B61
#define GL_SAMPLER_2D_SHADOW 0x8B62
#define GL_DELETE_STATUS 0x8B80
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_VALIDATE_STATUS 0x8B83
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_ATTACHED_SHADERS 0x8B85
#define GL_ACTIVE_UNIFORMS 0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH 0x8B87
#define GL_SHADER_SOURCE_LENGTH 0x8B88
#define GL_ACTIVE_ATTRIBUTES 0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH 0x8B8A
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT 0x8B8B
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_POINT_SPRITE_COORD_ORIGIN 0x8CA0
#define GL_LOWER_LEFT 0x8CA1
#define GL_UPPER_LEFT 0x8CA2
#define GL_STENCIL_BACK_REF 0x8CA3
#define GL_STENCIL_BACK_VALUE_MASK 0x8CA4
#define GL_STENCIL_BACK_WRITEMASK 0x8CA5
#endif
extern void (APIENTRYP gglBlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
extern void (APIENTRYP gglDrawBuffers)(GLsizei n, const GLenum *bufs);
extern void (APIENTRYP gglStencilOpSeparate)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
extern void (APIENTRYP gglStencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
extern void (APIENTRYP gglStencilMaskSeparate)(GLenum face, GLuint mask);
extern void (APIENTRYP gglAttachShader)(GLuint program, GLuint shader);
extern void (APIENTRYP gglBindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
extern void (APIENTRYP gglCompileShader)(GLuint shader);
extern GLuint (APIENTRYP gglCreateProgram)();
extern GLuint (APIENTRYP gglCreateShader)(GLenum type);
extern void (APIENTRYP gglDeleteProgram)(GLuint program);
extern void (APIENTRYP gglDeleteShader)(GLuint shader);
extern void (APIENTRYP gglDetachShader)(GLuint program, GLuint shader);
extern void (APIENTRYP gglDisableVertexAttribArray)(GLuint index);
extern void (APIENTRYP gglEnableVertexAttribArray)(GLuint index);
extern void (APIENTRYP gglGetActiveAttrib)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
extern void (APIENTRYP gglGetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
extern void (APIENTRYP gglGetAttachedShaders)(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders);
extern GLint (APIENTRYP gglGetAttribLocation)(GLuint program, const GLchar *name);
extern void (APIENTRYP gglGetProgramiv)(GLuint program, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
extern void (APIENTRYP gglGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
extern void (APIENTRYP gglGetShaderSource)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);
extern GLint (APIENTRYP gglGetUniformLocation)(GLuint program, const GLchar *name);
extern void (APIENTRYP gglGetUniformfv)(GLuint program, GLint location, GLfloat *params);
extern void (APIENTRYP gglGetUniformiv)(GLuint program, GLint location, GLint *params);
extern void (APIENTRYP gglGetVertexAttribdv)(GLuint index, GLenum pname, GLdouble *params);
extern void (APIENTRYP gglGetVertexAttribfv)(GLuint index, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetVertexAttribiv)(GLuint index, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetVertexAttribPointerv)(GLuint index, GLenum pname, void **pointer);
extern GLboolean (APIENTRYP gglIsProgram)(GLuint program);
extern GLboolean (APIENTRYP gglIsShader)(GLuint shader);
extern void (APIENTRYP gglLinkProgram)(GLuint program);
extern void (APIENTRYP gglShaderSource)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
extern void (APIENTRYP gglUseProgram)(GLuint program);
extern void (APIENTRYP gglUniform1f)(GLint location, GLfloat v0);
extern void (APIENTRYP gglUniform2f)(GLint location, GLfloat v0, GLfloat v1);
extern void (APIENTRYP gglUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern void (APIENTRYP gglUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void (APIENTRYP gglUniform1i)(GLint location, GLint v0);
extern void (APIENTRYP gglUniform2i)(GLint location, GLint v0, GLint v1);
extern void (APIENTRYP gglUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
extern void (APIENTRYP gglUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
extern void (APIENTRYP gglUniform1fv)(GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglUniform2fv)(GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglUniform3fv)(GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglUniform4fv)(GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglUniform1iv)(GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglUniform2iv)(GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglUniform3iv)(GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglUniform4iv)(GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglValidateProgram)(GLuint program);
extern void (APIENTRYP gglVertexAttrib1d)(GLuint index, GLdouble x);
extern void (APIENTRYP gglVertexAttrib1dv)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttrib1f)(GLuint index, GLfloat x);
extern void (APIENTRYP gglVertexAttrib1fv)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib1s)(GLuint index, GLshort x);
extern void (APIENTRYP gglVertexAttrib1sv)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib2d)(GLuint index, GLdouble x, GLdouble y);
extern void (APIENTRYP gglVertexAttrib2dv)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttrib2f)(GLuint index, GLfloat x, GLfloat y);
extern void (APIENTRYP gglVertexAttrib2fv)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib2s)(GLuint index, GLshort x, GLshort y);
extern void (APIENTRYP gglVertexAttrib2sv)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib3d)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglVertexAttrib3dv)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttrib3f)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglVertexAttrib3fv)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib3s)(GLuint index, GLshort x, GLshort y, GLshort z);
extern void (APIENTRYP gglVertexAttrib3sv)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib4Nbv)(GLuint index, const GLbyte *v);
extern void (APIENTRYP gglVertexAttrib4Niv)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttrib4Nsv)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib4Nub)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
extern void (APIENTRYP gglVertexAttrib4Nubv)(GLuint index, const GLubyte *v);
extern void (APIENTRYP gglVertexAttrib4Nuiv)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglVertexAttrib4Nusv)(GLuint index, const GLushort *v);
extern void (APIENTRYP gglVertexAttrib4bv)(GLuint index, const GLbyte *v);
extern void (APIENTRYP gglVertexAttrib4d)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglVertexAttrib4dv)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttrib4f)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglVertexAttrib4fv)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib4iv)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttrib4s)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
extern void (APIENTRYP gglVertexAttrib4sv)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib4ubv)(GLuint index, const GLubyte *v);
extern void (APIENTRYP gglVertexAttrib4uiv)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglVertexAttrib4usv)(GLuint index, const GLushort *v);
extern void (APIENTRYP gglVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);

#ifndef GL_VERSION_2_1
#define GL_VERSION_2_1
#define GL_PIXEL_PACK_BUFFER 0x88EB
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#define GL_PIXEL_PACK_BUFFER_BINDING 0x88ED
#define GL_PIXEL_UNPACK_BUFFER_BINDING 0x88EF
#define GL_FLOAT_MAT2x3 0x8B65
#define GL_FLOAT_MAT2x4 0x8B66
#define GL_FLOAT_MAT3x2 0x8B67
#define GL_FLOAT_MAT3x4 0x8B68
#define GL_FLOAT_MAT4x2 0x8B69
#define GL_FLOAT_MAT4x3 0x8B6A
#define GL_SRGB 0x8C40
#define GL_SRGB8 0x8C41
#define GL_SRGB_ALPHA 0x8C42
#define GL_SRGB8_ALPHA8 0x8C43
#define GL_COMPRESSED_SRGB 0x8C48
#define GL_COMPRESSED_SRGB_ALPHA 0x8C49
#endif
extern void (APIENTRYP gglUniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

#ifndef GL_VERSION_3_0
#define GL_VERSION_3_0
#define GL_COMPARE_REF_TO_TEXTURE 0x884E
#define GL_CLIP_DISTANCE0 0x3000
#define GL_CLIP_DISTANCE1 0x3001
#define GL_CLIP_DISTANCE2 0x3002
#define GL_CLIP_DISTANCE3 0x3003
#define GL_CLIP_DISTANCE4 0x3004
#define GL_CLIP_DISTANCE5 0x3005
#define GL_CLIP_DISTANCE6 0x3006
#define GL_CLIP_DISTANCE7 0x3007
#define GL_MAX_CLIP_DISTANCES 0x0D32
#define GL_MAJOR_VERSION 0x821B
#define GL_MINOR_VERSION 0x821C
#define GL_NUM_EXTENSIONS 0x821D
#define GL_CONTEXT_FLAGS 0x821E
#define GL_COMPRESSED_RED 0x8225
#define GL_COMPRESSED_RG 0x8226
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT 0x00000001
#define GL_RGBA32F 0x8814
#define GL_RGB32F 0x8815
#define GL_RGBA16F 0x881A
#define GL_RGB16F 0x881B
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER 0x88FD
#define GL_MAX_ARRAY_TEXTURE_LAYERS 0x88FF
#define GL_MIN_PROGRAM_TEXEL_OFFSET 0x8904
#define GL_MAX_PROGRAM_TEXEL_OFFSET 0x8905
#define GL_CLAMP_READ_COLOR 0x891C
#define GL_FIXED_ONLY 0x891D
#define GL_MAX_VARYING_COMPONENTS 0x8B4B
#define GL_TEXTURE_1D_ARRAY 0x8C18
#define GL_PROXY_TEXTURE_1D_ARRAY 0x8C19
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#define GL_PROXY_TEXTURE_2D_ARRAY 0x8C1B
#define GL_TEXTURE_BINDING_1D_ARRAY 0x8C1C
#define GL_TEXTURE_BINDING_2D_ARRAY 0x8C1D
#define GL_R11F_G11F_B10F 0x8C3A
#define GL_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define GL_RGB9_E5 0x8C3D
#define GL_UNSIGNED_INT_5_9_9_9_REV 0x8C3E
#define GL_TEXTURE_SHARED_SIZE 0x8C3F
#define GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH 0x8C76
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE 0x8C7F
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS 0x8C80
#define GL_TRANSFORM_FEEDBACK_VARYINGS 0x8C83
#define GL_TRANSFORM_FEEDBACK_BUFFER_START 0x8C84
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE 0x8C85
#define GL_PRIMITIVES_GENERATED 0x8C87
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN 0x8C88
#define GL_RASTERIZER_DISCARD 0x8C89
#define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS 0x8C8A
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS 0x8C8B
#define GL_INTERLEAVED_ATTRIBS 0x8C8C
#define GL_SEPARATE_ATTRIBS 0x8C8D
#define GL_TRANSFORM_FEEDBACK_BUFFER 0x8C8E
#define GL_TRANSFORM_FEEDBACK_BUFFER_BINDING 0x8C8F
#define GL_RGBA32UI 0x8D70
#define GL_RGB32UI 0x8D71
#define GL_RGBA16UI 0x8D76
#define GL_RGB16UI 0x8D77
#define GL_RGBA8UI 0x8D7C
#define GL_RGB8UI 0x8D7D
#define GL_RGBA32I 0x8D82
#define GL_RGB32I 0x8D83
#define GL_RGBA16I 0x8D88
#define GL_RGB16I 0x8D89
#define GL_RGBA8I 0x8D8E
#define GL_RGB8I 0x8D8F
#define GL_RED_INTEGER 0x8D94
#define GL_GREEN_INTEGER 0x8D95
#define GL_BLUE_INTEGER 0x8D96
#define GL_RGB_INTEGER 0x8D98
#define GL_RGBA_INTEGER 0x8D99
#define GL_BGR_INTEGER 0x8D9A
#define GL_BGRA_INTEGER 0x8D9B
#define GL_SAMPLER_1D_ARRAY 0x8DC0
#define GL_SAMPLER_2D_ARRAY 0x8DC1
#define GL_SAMPLER_1D_ARRAY_SHADOW 0x8DC3
#define GL_SAMPLER_2D_ARRAY_SHADOW 0x8DC4
#define GL_SAMPLER_CUBE_SHADOW 0x8DC5
#define GL_UNSIGNED_INT_VEC2 0x8DC6
#define GL_UNSIGNED_INT_VEC3 0x8DC7
#define GL_UNSIGNED_INT_VEC4 0x8DC8
#define GL_INT_SAMPLER_1D 0x8DC9
#define GL_INT_SAMPLER_2D 0x8DCA
#define GL_INT_SAMPLER_3D 0x8DCB
#define GL_INT_SAMPLER_CUBE 0x8DCC
#define GL_INT_SAMPLER_1D_ARRAY 0x8DCE
#define GL_INT_SAMPLER_2D_ARRAY 0x8DCF
#define GL_UNSIGNED_INT_SAMPLER_1D 0x8DD1
#define GL_UNSIGNED_INT_SAMPLER_2D 0x8DD2
#define GL_UNSIGNED_INT_SAMPLER_3D 0x8DD3
#define GL_UNSIGNED_INT_SAMPLER_CUBE 0x8DD4
#define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY 0x8DD6
#define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY 0x8DD7
#define GL_QUERY_WAIT 0x8E13
#define GL_QUERY_NO_WAIT 0x8E14
#define GL_QUERY_BY_REGION_WAIT 0x8E15
#define GL_QUERY_BY_REGION_NO_WAIT 0x8E16
#define GL_BUFFER_ACCESS_FLAGS 0x911F
#define GL_BUFFER_MAP_LENGTH 0x9120
#define GL_BUFFER_MAP_OFFSET 0x9121
#define GL_DEPTH_COMPONENT32F 0x8CAC
#define GL_DEPTH32F_STENCIL8 0x8CAD
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING 0x8210
#define GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE 0x8211
#define GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE 0x8212
#define GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE 0x8213
#define GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE 0x8214
#define GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE 0x8215
#define GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE 0x8216
#define GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE 0x8217
#define GL_FRAMEBUFFER_DEFAULT 0x8218
#define GL_FRAMEBUFFER_UNDEFINED 0x8219
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_MAX_RENDERBUFFER_SIZE 0x84E8
#define GL_DEPTH_STENCIL 0x84F9
#define GL_UNSIGNED_INT_24_8 0x84FA
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_TEXTURE_STENCIL_SIZE 0x88F1
#define GL_TEXTURE_RED_TYPE 0x8C10
#define GL_TEXTURE_GREEN_TYPE 0x8C11
#define GL_TEXTURE_BLUE_TYPE 0x8C12
#define GL_TEXTURE_ALPHA_TYPE 0x8C13
#define GL_TEXTURE_DEPTH_TYPE 0x8C16
#define GL_UNSIGNED_NORMALIZED 0x8C17
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#define GL_RENDERBUFFER_BINDING 0x8CA7
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#define GL_RENDERBUFFER_SAMPLES 0x8CAB
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER 0x8CD4
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS 0x8CDF
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_COLOR_ATTACHMENT1 0x8CE1
#define GL_COLOR_ATTACHMENT2 0x8CE2
#define GL_COLOR_ATTACHMENT3 0x8CE3
#define GL_COLOR_ATTACHMENT4 0x8CE4
#define GL_COLOR_ATTACHMENT5 0x8CE5
#define GL_COLOR_ATTACHMENT6 0x8CE6
#define GL_COLOR_ATTACHMENT7 0x8CE7
#define GL_COLOR_ATTACHMENT8 0x8CE8
#define GL_COLOR_ATTACHMENT9 0x8CE9
#define GL_COLOR_ATTACHMENT10 0x8CEA
#define GL_COLOR_ATTACHMENT11 0x8CEB
#define GL_COLOR_ATTACHMENT12 0x8CEC
#define GL_COLOR_ATTACHMENT13 0x8CED
#define GL_COLOR_ATTACHMENT14 0x8CEE
#define GL_COLOR_ATTACHMENT15 0x8CEF
#define GL_COLOR_ATTACHMENT16 0x8CF0
#define GL_COLOR_ATTACHMENT17 0x8CF1
#define GL_COLOR_ATTACHMENT18 0x8CF2
#define GL_COLOR_ATTACHMENT19 0x8CF3
#define GL_COLOR_ATTACHMENT20 0x8CF4
#define GL_COLOR_ATTACHMENT21 0x8CF5
#define GL_COLOR_ATTACHMENT22 0x8CF6
#define GL_COLOR_ATTACHMENT23 0x8CF7
#define GL_COLOR_ATTACHMENT24 0x8CF8
#define GL_COLOR_ATTACHMENT25 0x8CF9
#define GL_COLOR_ATTACHMENT26 0x8CFA
#define GL_COLOR_ATTACHMENT27 0x8CFB
#define GL_COLOR_ATTACHMENT28 0x8CFC
#define GL_COLOR_ATTACHMENT29 0x8CFD
#define GL_COLOR_ATTACHMENT30 0x8CFE
#define GL_COLOR_ATTACHMENT31 0x8CFF
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_RENDERBUFFER_WIDTH 0x8D42
#define GL_RENDERBUFFER_HEIGHT 0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT 0x8D44
#define GL_STENCIL_INDEX1 0x8D46
#define GL_STENCIL_INDEX4 0x8D47
#define GL_STENCIL_INDEX8 0x8D48
#define GL_STENCIL_INDEX16 0x8D49
#define GL_RENDERBUFFER_RED_SIZE 0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE 0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE 0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE 0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE 0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE 0x8D55
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56
#define GL_MAX_SAMPLES 0x8D57
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#define GL_HALF_FLOAT 0x140B
#define GL_MAP_READ_BIT 0x0001
#define GL_MAP_WRITE_BIT 0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT 0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT 0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT 0x0020
#define GL_COMPRESSED_RED_RGTC1 0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#define GL_COMPRESSED_RG_RGTC2 0x8DBD
#define GL_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#define GL_RG 0x8227
#define GL_RG_INTEGER 0x8228
#define GL_R8 0x8229
#define GL_R16 0x822A
#define GL_RG8 0x822B
#define GL_RG16 0x822C
#define GL_R16F 0x822D
#define GL_R32F 0x822E
#define GL_RG16F 0x822F
#define GL_RG32F 0x8230
#define GL_R8I 0x8231
#define GL_R8UI 0x8232
#define GL_R16I 0x8233
#define GL_R16UI 0x8234
#define GL_R32I 0x8235
#define GL_R32UI 0x8236
#define GL_RG8I 0x8237
#define GL_RG8UI 0x8238
#define GL_RG16I 0x8239
#define GL_RG16UI 0x823A
#define GL_RG32I 0x823B
#define GL_RG32UI 0x823C
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#endif
extern void (APIENTRYP gglColorMaski)(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a);
extern void (APIENTRYP gglGetBooleani_v)(GLenum target, GLuint index, GLboolean *data);
extern void (APIENTRYP gglGetIntegeri_v)(GLenum target, GLuint index, GLint *data);
extern void (APIENTRYP gglEnablei)(GLenum target, GLuint index);
extern void (APIENTRYP gglDisablei)(GLenum target, GLuint index);
extern GLboolean (APIENTRYP gglIsEnabledi)(GLenum target, GLuint index);
extern void (APIENTRYP gglBeginTransformFeedback)(GLenum primitiveMode);
extern void (APIENTRYP gglEndTransformFeedback)();
extern void (APIENTRYP gglBindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
extern void (APIENTRYP gglBindBufferBase)(GLenum target, GLuint index, GLuint buffer);
extern void (APIENTRYP gglTransformFeedbackVaryings)(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode);
extern void (APIENTRYP gglGetTransformFeedbackVarying)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
extern void (APIENTRYP gglClampColor)(GLenum target, GLenum clamp);
extern void (APIENTRYP gglBeginConditionalRender)(GLuint id, GLenum mode);
extern void (APIENTRYP gglEndConditionalRender)();
extern void (APIENTRYP gglVertexAttribIPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
extern void (APIENTRYP gglGetVertexAttribIiv)(GLuint index, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetVertexAttribIuiv)(GLuint index, GLenum pname, GLuint *params);
extern void (APIENTRYP gglVertexAttribI1i)(GLuint index, GLint x);
extern void (APIENTRYP gglVertexAttribI2i)(GLuint index, GLint x, GLint y);
extern void (APIENTRYP gglVertexAttribI3i)(GLuint index, GLint x, GLint y, GLint z);
extern void (APIENTRYP gglVertexAttribI4i)(GLuint index, GLint x, GLint y, GLint z, GLint w);
extern void (APIENTRYP gglVertexAttribI1ui)(GLuint index, GLuint x);
extern void (APIENTRYP gglVertexAttribI2ui)(GLuint index, GLuint x, GLuint y);
extern void (APIENTRYP gglVertexAttribI3ui)(GLuint index, GLuint x, GLuint y, GLuint z);
extern void (APIENTRYP gglVertexAttribI4ui)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
extern void (APIENTRYP gglVertexAttribI1iv)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttribI2iv)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttribI3iv)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttribI4iv)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttribI1uiv)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglVertexAttribI2uiv)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglVertexAttribI3uiv)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglVertexAttribI4uiv)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglVertexAttribI4bv)(GLuint index, const GLbyte *v);
extern void (APIENTRYP gglVertexAttribI4sv)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttribI4ubv)(GLuint index, const GLubyte *v);
extern void (APIENTRYP gglVertexAttribI4usv)(GLuint index, const GLushort *v);
extern void (APIENTRYP gglGetUniformuiv)(GLuint program, GLint location, GLuint *params);
extern void (APIENTRYP gglBindFragDataLocation)(GLuint program, GLuint color, const GLchar *name);
extern GLint (APIENTRYP gglGetFragDataLocation)(GLuint program, const GLchar *name);
extern void (APIENTRYP gglUniform1ui)(GLint location, GLuint v0);
extern void (APIENTRYP gglUniform2ui)(GLint location, GLuint v0, GLuint v1);
extern void (APIENTRYP gglUniform3ui)(GLint location, GLuint v0, GLuint v1, GLuint v2);
extern void (APIENTRYP gglUniform4ui)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
extern void (APIENTRYP gglUniform1uiv)(GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglUniform2uiv)(GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglUniform3uiv)(GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglUniform4uiv)(GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglTexParameterIiv)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglTexParameterIuiv)(GLenum target, GLenum pname, const GLuint *params);
extern void (APIENTRYP gglGetTexParameterIiv)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetTexParameterIuiv)(GLenum target, GLenum pname, GLuint *params);
extern void (APIENTRYP gglClearBufferiv)(GLenum buffer, GLint drawbuffer, const GLint *value);
extern void (APIENTRYP gglClearBufferuiv)(GLenum buffer, GLint drawbuffer, const GLuint *value);
extern void (APIENTRYP gglClearBufferfv)(GLenum buffer, GLint drawbuffer, const GLfloat *value);
extern void (APIENTRYP gglClearBufferfi)(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
extern const GLubyte * (APIENTRYP gglGetStringi)(GLenum name, GLuint index);
extern GLboolean (APIENTRYP gglIsRenderbuffer)(GLuint renderbuffer);
extern void (APIENTRYP gglBindRenderbuffer)(GLenum target, GLuint renderbuffer);
extern void (APIENTRYP gglDeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
extern void (APIENTRYP gglGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
extern void (APIENTRYP gglRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint *params);
extern GLboolean (APIENTRYP gglIsFramebuffer)(GLuint framebuffer);
extern void (APIENTRYP gglBindFramebuffer)(GLenum target, GLuint framebuffer);
extern void (APIENTRYP gglDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
extern void (APIENTRYP gglGenFramebuffers)(GLsizei n, GLuint *framebuffers);
extern GLenum (APIENTRYP gglCheckFramebufferStatus)(GLenum target);
extern void (APIENTRYP gglFramebufferTexture1D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern void (APIENTRYP gglFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern void (APIENTRYP gglFramebufferTexture3D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
extern void (APIENTRYP gglFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
extern void (APIENTRYP gglGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
extern void (APIENTRYP gglGenerateMipmap)(GLenum target);
extern void (APIENTRYP gglBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
extern void (APIENTRYP gglRenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglFramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
extern void * (APIENTRYP gglMapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
extern void (APIENTRYP gglFlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length);
extern void (APIENTRYP gglBindVertexArray)(GLuint array);
extern void (APIENTRYP gglDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
extern void (APIENTRYP gglGenVertexArrays)(GLsizei n, GLuint *arrays);
extern GLboolean (APIENTRYP gglIsVertexArray)(GLuint array);

#ifndef GL_VERSION_3_1
#define GL_VERSION_3_1
#define GL_SAMPLER_2D_RECT 0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW 0x8B64
#define GL_SAMPLER_BUFFER 0x8DC2
#define GL_INT_SAMPLER_2D_RECT 0x8DCD
#define GL_INT_SAMPLER_BUFFER 0x8DD0
#define GL_UNSIGNED_INT_SAMPLER_2D_RECT 0x8DD5
#define GL_UNSIGNED_INT_SAMPLER_BUFFER 0x8DD8
#define GL_TEXTURE_BUFFER 0x8C2A
#define GL_MAX_TEXTURE_BUFFER_SIZE 0x8C2B
#define GL_TEXTURE_BINDING_BUFFER 0x8C2C
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING 0x8C2D
#define GL_TEXTURE_RECTANGLE 0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE 0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE 0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE 0x84F8
#define GL_R8_SNORM 0x8F94
#define GL_RG8_SNORM 0x8F95
#define GL_RGB8_SNORM 0x8F96
#define GL_RGBA8_SNORM 0x8F97
#define GL_R16_SNORM 0x8F98
#define GL_RG16_SNORM 0x8F99
#define GL_RGB16_SNORM 0x8F9A
#define GL_RGBA16_SNORM 0x8F9B
#define GL_SIGNED_NORMALIZED 0x8F9C
#define GL_PRIMITIVE_RESTART 0x8F9D
#define GL_PRIMITIVE_RESTART_INDEX 0x8F9E
#define GL_COPY_READ_BUFFER 0x8F36
#define GL_COPY_WRITE_BUFFER 0x8F37
#define GL_UNIFORM_BUFFER 0x8A11
#define GL_UNIFORM_BUFFER_BINDING 0x8A28
#define GL_UNIFORM_BUFFER_START 0x8A29
#define GL_UNIFORM_BUFFER_SIZE 0x8A2A
#define GL_MAX_VERTEX_UNIFORM_BLOCKS 0x8A2B
#define GL_MAX_GEOMETRY_UNIFORM_BLOCKS 0x8A2C
#define GL_MAX_FRAGMENT_UNIFORM_BLOCKS 0x8A2D
#define GL_MAX_COMBINED_UNIFORM_BLOCKS 0x8A2E
#define GL_MAX_UNIFORM_BUFFER_BINDINGS 0x8A2F
#define GL_MAX_UNIFORM_BLOCK_SIZE 0x8A30
#define GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS 0x8A31
#define GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS 0x8A32
#define GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS 0x8A33
#define GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT 0x8A34
#define GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH 0x8A35
#define GL_ACTIVE_UNIFORM_BLOCKS 0x8A36
#define GL_UNIFORM_TYPE 0x8A37
#define GL_UNIFORM_SIZE 0x8A38
#define GL_UNIFORM_NAME_LENGTH 0x8A39
#define GL_UNIFORM_BLOCK_INDEX 0x8A3A
#define GL_UNIFORM_OFFSET 0x8A3B
#define GL_UNIFORM_ARRAY_STRIDE 0x8A3C
#define GL_UNIFORM_MATRIX_STRIDE 0x8A3D
#define GL_UNIFORM_IS_ROW_MAJOR 0x8A3E
#define GL_UNIFORM_BLOCK_BINDING 0x8A3F
#define GL_UNIFORM_BLOCK_DATA_SIZE 0x8A40
#define GL_UNIFORM_BLOCK_NAME_LENGTH 0x8A41
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS 0x8A42
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES 0x8A43
#define GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER 0x8A44
#define GL_UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER 0x8A45
#define GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER 0x8A46
#define GL_INVALID_INDEX 0xFFFFFFFF
#endif
extern void (APIENTRYP gglDrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
extern void (APIENTRYP gglDrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
extern void (APIENTRYP gglTexBuffer)(GLenum target, GLenum internalformat, GLuint buffer);
extern void (APIENTRYP gglPrimitiveRestartIndex)(GLuint index);
extern void (APIENTRYP gglCopyBufferSubData)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
extern void (APIENTRYP gglGetUniformIndices)(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices);
extern void (APIENTRYP gglGetActiveUniformsiv)(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetActiveUniformName)(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName);
extern GLuint (APIENTRYP gglGetUniformBlockIndex)(GLuint program, const GLchar *uniformBlockName);
extern void (APIENTRYP gglGetActiveUniformBlockiv)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetActiveUniformBlockName)(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
extern void (APIENTRYP gglUniformBlockBinding)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
extern void (APIENTRYP gglBindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
extern void (APIENTRYP gglBindBufferBase)(GLenum target, GLuint index, GLuint buffer);
extern void (APIENTRYP gglGetIntegeri_v)(GLenum target, GLuint index, GLint *data);

#ifndef GL_VERSION_3_2
#define GL_VERSION_3_2
#define GL_CONTEXT_CORE_PROFILE_BIT 0x00000001
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#define GL_LINES_ADJACENCY 0x000A
#define GL_LINE_STRIP_ADJACENCY 0x000B
#define GL_TRIANGLES_ADJACENCY 0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY 0x000D
#define GL_PROGRAM_POINT_SIZE 0x8642
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS 0x8C29
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED 0x8DA7
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS 0x8DA8
#define GL_GEOMETRY_SHADER 0x8DD9
#define GL_GEOMETRY_VERTICES_OUT 0x8916
#define GL_GEOMETRY_INPUT_TYPE 0x8917
#define GL_GEOMETRY_OUTPUT_TYPE 0x8918
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS 0x8DDF
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES 0x8DE0
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS 0x8DE1
#define GL_MAX_VERTEX_OUTPUT_COMPONENTS 0x9122
#define GL_MAX_GEOMETRY_INPUT_COMPONENTS 0x9123
#define GL_MAX_GEOMETRY_OUTPUT_COMPONENTS 0x9124
#define GL_MAX_FRAGMENT_INPUT_COMPONENTS 0x9125
#define GL_CONTEXT_PROFILE_MASK 0x9126
#define GL_DEPTH_CLAMP 0x864F
#define GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION 0x8E4C
#define GL_FIRST_VERTEX_CONVENTION 0x8E4D
#define GL_LAST_VERTEX_CONVENTION 0x8E4E
#define GL_PROVOKING_VERTEX 0x8E4F
#define GL_TEXTURE_CUBE_MAP_SEAMLESS 0x884F
#define GL_MAX_SERVER_WAIT_TIMEOUT 0x9111
#define GL_OBJECT_TYPE 0x9112
#define GL_SYNC_CONDITION 0x9113
#define GL_SYNC_STATUS 0x9114
#define GL_SYNC_FLAGS 0x9115
#define GL_SYNC_FENCE 0x9116
#define GL_SYNC_GPU_COMMANDS_COMPLETE 0x9117
#define GL_UNSIGNALED 0x9118
#define GL_SIGNALED 0x9119
#define GL_ALREADY_SIGNALED 0x911A
#define GL_TIMEOUT_EXPIRED 0x911B
#define GL_CONDITION_SATISFIED 0x911C
#define GL_WAIT_FAILED 0x911D
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFF
#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
#define GL_SAMPLE_POSITION 0x8E50
#define GL_SAMPLE_MASK 0x8E51
#define GL_SAMPLE_MASK_VALUE 0x8E52
#define GL_MAX_SAMPLE_MASK_WORDS 0x8E59
#define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE 0x9101
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9102
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9103
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE 0x9104
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY 0x9105
#define GL_TEXTURE_SAMPLES 0x9106
#define GL_TEXTURE_FIXED_SAMPLE_LOCATIONS 0x9107
#define GL_SAMPLER_2D_MULTISAMPLE 0x9108
#define GL_INT_SAMPLER_2D_MULTISAMPLE 0x9109
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE 0x910A
#define GL_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910B
#define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910C
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910D
#define GL_MAX_COLOR_TEXTURE_SAMPLES 0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES 0x910F
#define GL_MAX_INTEGER_SAMPLES 0x9110
#endif
extern void (APIENTRYP gglDrawElementsBaseVertex)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
extern void (APIENTRYP gglDrawRangeElementsBaseVertex)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex);
extern void (APIENTRYP gglDrawElementsInstancedBaseVertex)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex);
extern void (APIENTRYP gglMultiDrawElementsBaseVertex)(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex);
extern void (APIENTRYP gglProvokingVertex)(GLenum mode);
extern GLsync (APIENTRYP gglFenceSync)(GLenum condition, GLbitfield flags);
extern GLboolean (APIENTRYP gglIsSync)(GLsync sync);
extern void (APIENTRYP gglDeleteSync)(GLsync sync);
extern GLenum (APIENTRYP gglClientWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
extern void (APIENTRYP gglWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
extern void (APIENTRYP gglGetInteger64v)(GLenum pname, GLint64 *data);
extern void (APIENTRYP gglGetSynciv)(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
extern void (APIENTRYP gglGetInteger64i_v)(GLenum target, GLuint index, GLint64 *data);
extern void (APIENTRYP gglGetBufferParameteri64v)(GLenum target, GLenum pname, GLint64 *params);
extern void (APIENTRYP gglFramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level);
extern void (APIENTRYP gglTexImage2DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
extern void (APIENTRYP gglTexImage3DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
extern void (APIENTRYP gglGetMultisamplefv)(GLenum pname, GLuint index, GLfloat *val);
extern void (APIENTRYP gglSampleMaski)(GLuint maskNumber, GLbitfield mask);

#ifndef GL_3DFX_multisample
#define GL_3DFX_multisample
#define GL_MULTISAMPLE_3DFX 0x86B2
#define GL_SAMPLE_BUFFERS_3DFX 0x86B3
#define GL_SAMPLES_3DFX 0x86B4
#define GL_MULTISAMPLE_BIT_3DFX 0x20000000
#endif

#ifndef GL_3DFX_tbuffer
#define GL_3DFX_tbuffer
#endif
extern void (APIENTRYP gglTbufferMask3DFX)(GLuint mask);

#ifndef GL_3DFX_texture_compression_FXT1
#define GL_3DFX_texture_compression_FXT1
#define GL_COMPRESSED_RGB_FXT1_3DFX 0x86B0
#define GL_COMPRESSED_RGBA_FXT1_3DFX 0x86B1
#endif

#ifndef GL_AMD_blend_minmax_factor
#define GL_AMD_blend_minmax_factor
#define GL_FACTOR_MIN_AMD 0x901C
#define GL_FACTOR_MAX_AMD 0x901D
#endif

#ifndef GL_AMD_conservative_depth
#define GL_AMD_conservative_depth
#endif

#ifndef GL_AMD_debug_output
#define GL_AMD_debug_output
#define GL_MAX_DEBUG_MESSAGE_LENGTH_AMD 0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES_AMD 0x9144
#define GL_DEBUG_LOGGED_MESSAGES_AMD 0x9145
#define GL_DEBUG_SEVERITY_HIGH_AMD 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM_AMD 0x9147
#define GL_DEBUG_SEVERITY_LOW_AMD 0x9148
#define GL_DEBUG_CATEGORY_API_ERROR_AMD 0x9149
#define GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD 0x914A
#define GL_DEBUG_CATEGORY_DEPRECATION_AMD 0x914B
#define GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD 0x914C
#define GL_DEBUG_CATEGORY_PERFORMANCE_AMD 0x914D
#define GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD 0x914E
#define GL_DEBUG_CATEGORY_APPLICATION_AMD 0x914F
#define GL_DEBUG_CATEGORY_OTHER_AMD 0x9150
#endif
extern void (APIENTRYP gglDebugMessageEnableAMD)(GLenum category, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
extern void (APIENTRYP gglDebugMessageInsertAMD)(GLenum category, GLenum severity, GLuint id, GLsizei length, const GLchar *buf);
extern void (APIENTRYP gglDebugMessageCallbackAMD)(GLDEBUGPROCAMD callback, void *userParam);
extern GLuint (APIENTRYP gglGetDebugMessageLogAMD)(GLuint count, GLsizei bufsize, GLenum *categories, GLuint *severities, GLuint *ids, GLsizei *lengths, GLchar *message);

#ifndef GL_AMD_depth_clamp_separate
#define GL_AMD_depth_clamp_separate
#define GL_DEPTH_CLAMP_NEAR_AMD 0x901E
#define GL_DEPTH_CLAMP_FAR_AMD 0x901F
#endif

#ifndef GL_AMD_draw_buffers_blend
#define GL_AMD_draw_buffers_blend
#endif
extern void (APIENTRYP gglBlendFuncIndexedAMD)(GLuint buf, GLenum src, GLenum dst);
extern void (APIENTRYP gglBlendFuncSeparateIndexedAMD)(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
extern void (APIENTRYP gglBlendEquationIndexedAMD)(GLuint buf, GLenum mode);
extern void (APIENTRYP gglBlendEquationSeparateIndexedAMD)(GLuint buf, GLenum modeRGB, GLenum modeAlpha);

#ifndef GL_AMD_gcn_shader
#define GL_AMD_gcn_shader
#endif

#ifndef GL_AMD_gpu_shader_half_float
#define GL_AMD_gpu_shader_half_float
#define GL_FLOAT16_NV 0x8FF8
#define GL_FLOAT16_VEC2_NV 0x8FF9
#define GL_FLOAT16_VEC3_NV 0x8FFA
#define GL_FLOAT16_VEC4_NV 0x8FFB
#define GL_FLOAT16_MAT2_AMD 0x91C5
#define GL_FLOAT16_MAT3_AMD 0x91C6
#define GL_FLOAT16_MAT4_AMD 0x91C7
#define GL_FLOAT16_MAT2x3_AMD 0x91C8
#define GL_FLOAT16_MAT2x4_AMD 0x91C9
#define GL_FLOAT16_MAT3x2_AMD 0x91CA
#define GL_FLOAT16_MAT3x4_AMD 0x91CB
#define GL_FLOAT16_MAT4x2_AMD 0x91CC
#define GL_FLOAT16_MAT4x3_AMD 0x91CD
#endif

#ifndef GL_AMD_gpu_shader_int64
#define GL_AMD_gpu_shader_int64
#define GL_INT64_NV 0x140E
#define GL_UNSIGNED_INT64_NV 0x140F
#define GL_INT8_NV 0x8FE0
#define GL_INT8_VEC2_NV 0x8FE1
#define GL_INT8_VEC3_NV 0x8FE2
#define GL_INT8_VEC4_NV 0x8FE3
#define GL_INT16_NV 0x8FE4
#define GL_INT16_VEC2_NV 0x8FE5
#define GL_INT16_VEC3_NV 0x8FE6
#define GL_INT16_VEC4_NV 0x8FE7
#define GL_INT64_VEC2_NV 0x8FE9
#define GL_INT64_VEC3_NV 0x8FEA
#define GL_INT64_VEC4_NV 0x8FEB
#define GL_UNSIGNED_INT8_NV 0x8FEC
#define GL_UNSIGNED_INT8_VEC2_NV 0x8FED
#define GL_UNSIGNED_INT8_VEC3_NV 0x8FEE
#define GL_UNSIGNED_INT8_VEC4_NV 0x8FEF
#define GL_UNSIGNED_INT16_NV 0x8FF0
#define GL_UNSIGNED_INT16_VEC2_NV 0x8FF1
#define GL_UNSIGNED_INT16_VEC3_NV 0x8FF2
#define GL_UNSIGNED_INT16_VEC4_NV 0x8FF3
#define GL_UNSIGNED_INT64_VEC2_NV 0x8FF5
#define GL_UNSIGNED_INT64_VEC3_NV 0x8FF6
#define GL_UNSIGNED_INT64_VEC4_NV 0x8FF7
/* reuse GL_FLOAT16_NV */
/* reuse GL_FLOAT16_VEC2_NV */
/* reuse GL_FLOAT16_VEC3_NV */
/* reuse GL_FLOAT16_VEC4_NV */
#endif
extern void (APIENTRYP gglUniform1i64NV)(GLint location, GLint64EXT x);
extern void (APIENTRYP gglUniform2i64NV)(GLint location, GLint64EXT x, GLint64EXT y);
extern void (APIENTRYP gglUniform3i64NV)(GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z);
extern void (APIENTRYP gglUniform4i64NV)(GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w);
extern void (APIENTRYP gglUniform1i64vNV)(GLint location, GLsizei count, const GLint64EXT *value);
extern void (APIENTRYP gglUniform2i64vNV)(GLint location, GLsizei count, const GLint64EXT *value);
extern void (APIENTRYP gglUniform3i64vNV)(GLint location, GLsizei count, const GLint64EXT *value);
extern void (APIENTRYP gglUniform4i64vNV)(GLint location, GLsizei count, const GLint64EXT *value);
extern void (APIENTRYP gglUniform1ui64NV)(GLint location, GLuint64EXT x);
extern void (APIENTRYP gglUniform2ui64NV)(GLint location, GLuint64EXT x, GLuint64EXT y);
extern void (APIENTRYP gglUniform3ui64NV)(GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z);
extern void (APIENTRYP gglUniform4ui64NV)(GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w);
extern void (APIENTRYP gglUniform1ui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value);
extern void (APIENTRYP gglUniform2ui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value);
extern void (APIENTRYP gglUniform3ui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value);
extern void (APIENTRYP gglUniform4ui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value);
extern void (APIENTRYP gglGetUniformi64vNV)(GLuint program, GLint location, GLint64EXT *params);
extern void (APIENTRYP gglGetUniformui64vNV)(GLuint program, GLint location, GLuint64EXT *params);
extern void (APIENTRYP gglProgramUniform1i64NV)(GLuint program, GLint location, GLint64EXT x);
extern void (APIENTRYP gglProgramUniform2i64NV)(GLuint program, GLint location, GLint64EXT x, GLint64EXT y);
extern void (APIENTRYP gglProgramUniform3i64NV)(GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z);
extern void (APIENTRYP gglProgramUniform4i64NV)(GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w);
extern void (APIENTRYP gglProgramUniform1i64vNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value);
extern void (APIENTRYP gglProgramUniform2i64vNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value);
extern void (APIENTRYP gglProgramUniform3i64vNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value);
extern void (APIENTRYP gglProgramUniform4i64vNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value);
extern void (APIENTRYP gglProgramUniform1ui64NV)(GLuint program, GLint location, GLuint64EXT x);
extern void (APIENTRYP gglProgramUniform2ui64NV)(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y);
extern void (APIENTRYP gglProgramUniform3ui64NV)(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z);
extern void (APIENTRYP gglProgramUniform4ui64NV)(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w);
extern void (APIENTRYP gglProgramUniform1ui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);
extern void (APIENTRYP gglProgramUniform2ui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);
extern void (APIENTRYP gglProgramUniform3ui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);
extern void (APIENTRYP gglProgramUniform4ui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);

#ifndef GL_AMD_interleaved_elements
#define GL_AMD_interleaved_elements
#define GL_VERTEX_ELEMENT_SWIZZLE_AMD 0x91A4
#define GL_VERTEX_ID_SWIZZLE_AMD 0x91A5
/* reuse GL_RED */
/* reuse GL_GREEN */
/* reuse GL_BLUE */
/* reuse GL_ALPHA */
/* reuse GL_RG8UI */
/* reuse GL_RG16UI */
/* reuse GL_RGBA8UI */
#endif
extern void (APIENTRYP gglVertexAttribParameteriAMD)(GLuint index, GLenum pname, GLint param);

#ifndef GL_AMD_multi_draw_indirect
#define GL_AMD_multi_draw_indirect
#endif
extern void (APIENTRYP gglMultiDrawArraysIndirectAMD)(GLenum mode, const void *indirect, GLsizei primcount, GLsizei stride);
extern void (APIENTRYP gglMultiDrawElementsIndirectAMD)(GLenum mode, GLenum type, const void *indirect, GLsizei primcount, GLsizei stride);

#ifndef GL_AMD_name_gen_delete
#define GL_AMD_name_gen_delete
#define GL_DATA_BUFFER_AMD 0x9151
#define GL_PERFORMANCE_MONITOR_AMD 0x9152
#define GL_QUERY_OBJECT_AMD 0x9153
#define GL_VERTEX_ARRAY_OBJECT_AMD 0x9154
#define GL_SAMPLER_OBJECT_AMD 0x9155
#endif
extern void (APIENTRYP gglGenNamesAMD)(GLenum identifier, GLuint num, GLuint *names);
extern void (APIENTRYP gglDeleteNamesAMD)(GLenum identifier, GLuint num, const GLuint *names);
extern GLboolean (APIENTRYP gglIsNameAMD)(GLenum identifier, GLuint name);

#ifndef GL_AMD_occlusion_query_event
#define GL_AMD_occlusion_query_event
#define GL_OCCLUSION_QUERY_EVENT_MASK_AMD 0x874F
#define GL_QUERY_DEPTH_PASS_EVENT_BIT_AMD 0x00000001
#define GL_QUERY_DEPTH_FAIL_EVENT_BIT_AMD 0x00000002
#define GL_QUERY_STENCIL_FAIL_EVENT_BIT_AMD 0x00000004
#define GL_QUERY_DEPTH_BOUNDS_FAIL_EVENT_BIT_AMD 0x00000008
#define GL_QUERY_ALL_EVENT_BITS_AMD 0xFFFFFFFF
#endif
extern void (APIENTRYP gglQueryObjectParameteruiAMD)(GLenum target, GLuint id, GLenum pname, GLuint param);

#ifndef GL_AMD_performance_monitor
#define GL_AMD_performance_monitor
#define GL_COUNTER_TYPE_AMD 0x8BC0
#define GL_COUNTER_RANGE_AMD 0x8BC1
#define GL_UNSIGNED_INT64_AMD 0x8BC2
#define GL_PERCENTAGE_AMD 0x8BC3
#define GL_PERFMON_RESULT_AVAILABLE_AMD 0x8BC4
#define GL_PERFMON_RESULT_SIZE_AMD 0x8BC5
#define GL_PERFMON_RESULT_AMD 0x8BC6
#endif
extern void (APIENTRYP gglGetPerfMonitorGroupsAMD)(GLint *numGroups, GLsizei groupsSize, GLuint *groups);
extern void (APIENTRYP gglGetPerfMonitorCountersAMD)(GLuint group, GLint *numCounters, GLint *maxActiveCounters, GLsizei counterSize, GLuint *counters);
extern void (APIENTRYP gglGetPerfMonitorGroupStringAMD)(GLuint group, GLsizei bufSize, GLsizei *length, GLchar *groupString);
extern void (APIENTRYP gglGetPerfMonitorCounterStringAMD)(GLuint group, GLuint counter, GLsizei bufSize, GLsizei *length, GLchar *counterString);
extern void (APIENTRYP gglGetPerfMonitorCounterInfoAMD)(GLuint group, GLuint counter, GLenum pname, void *data);
extern void (APIENTRYP gglGenPerfMonitorsAMD)(GLsizei n, GLuint *monitors);
extern void (APIENTRYP gglDeletePerfMonitorsAMD)(GLsizei n, GLuint *monitors);
extern void (APIENTRYP gglSelectPerfMonitorCountersAMD)(GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint *counterList);
extern void (APIENTRYP gglBeginPerfMonitorAMD)(GLuint monitor);
extern void (APIENTRYP gglEndPerfMonitorAMD)(GLuint monitor);
extern void (APIENTRYP gglGetPerfMonitorCounterDataAMD)(GLuint monitor, GLenum pname, GLsizei dataSize, GLuint *data, GLint *bytesWritten);

#ifndef GL_AMD_pinned_memory
#define GL_AMD_pinned_memory
#define GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD 0x9160
#endif

#ifndef GL_AMD_query_buffer_object
#define GL_AMD_query_buffer_object
#define GL_QUERY_BUFFER_AMD 0x9192
#define GL_QUERY_BUFFER_BINDING_AMD 0x9193
#define GL_QUERY_RESULT_NO_WAIT_AMD 0x9194
#endif

#ifndef GL_AMD_sample_positions
#define GL_AMD_sample_positions
#define GL_SUBSAMPLE_DISTANCE_AMD 0x883F
#endif
extern void (APIENTRYP gglSetMultisamplefvAMD)(GLenum pname, GLuint index, const GLfloat *val);

#ifndef GL_AMD_seamless_cubemap_per_texture
#define GL_AMD_seamless_cubemap_per_texture
/* reuse GL_TEXTURE_CUBE_MAP_SEAMLESS */
#endif

#ifndef GL_AMD_shader_atomic_counter_ops
#define GL_AMD_shader_atomic_counter_ops
#endif

#ifndef GL_AMD_shader_ballot
#define GL_AMD_shader_ballot
#endif

#ifndef GL_AMD_shader_stencil_export
#define GL_AMD_shader_stencil_export
#endif

#ifndef GL_AMD_shader_trinary_minmax
#define GL_AMD_shader_trinary_minmax
#endif

#ifndef GL_AMD_shader_explicit_vertex_parameter
#define GL_AMD_shader_explicit_vertex_parameter
#endif

#ifndef GL_AMD_sparse_texture
#define GL_AMD_sparse_texture
#define GL_VIRTUAL_PAGE_SIZE_X_AMD 0x9195
#define GL_VIRTUAL_PAGE_SIZE_Y_AMD 0x9196
#define GL_VIRTUAL_PAGE_SIZE_Z_AMD 0x9197
#define GL_MAX_SPARSE_TEXTURE_SIZE_AMD 0x9198
#define GL_MAX_SPARSE_3D_TEXTURE_SIZE_AMD 0x9199
#define GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS 0x919A
#define GL_MIN_SPARSE_LEVEL_AMD 0x919B
#define GL_MIN_LOD_WARNING_AMD 0x919C
#define GL_TEXTURE_STORAGE_SPARSE_BIT_AMD 0x00000001
#endif
extern void (APIENTRYP gglTexStorageSparseAMD)(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags);
extern void (APIENTRYP gglTextureStorageSparseAMD)(GLuint texture, GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags);

#ifndef GL_AMD_stencil_operation_extended
#define GL_AMD_stencil_operation_extended
#define GL_SET_AMD 0x874A
#define GL_REPLACE_VALUE_AMD 0x874B
#define GL_STENCIL_OP_VALUE_AMD 0x874C
#define GL_STENCIL_BACK_OP_VALUE_AMD 0x874D
#endif
extern void (APIENTRYP gglStencilOpValueAMD)(GLenum face, GLuint value);

#ifndef GL_AMD_texture_texture4
#define GL_AMD_texture_texture4
#endif

#ifndef GL_AMD_transform_feedback3_lines_triangles
#define GL_AMD_transform_feedback3_lines_triangles
#endif

#ifndef GL_AMD_transform_feedback4
#define GL_AMD_transform_feedback4
#define GL_STREAM_RASTERIZATION_AMD 0x91A0
#endif

#ifndef GL_AMD_vertex_shader_layer
#define GL_AMD_vertex_shader_layer
#endif

#ifndef GL_AMD_vertex_shader_tessellator
#define GL_AMD_vertex_shader_tessellator
#define GL_SAMPLER_BUFFER_AMD 0x9001
#define GL_INT_SAMPLER_BUFFER_AMD 0x9002
#define GL_UNSIGNED_INT_SAMPLER_BUFFER_AMD 0x9003
#define GL_TESSELLATION_MODE_AMD 0x9004
#define GL_TESSELLATION_FACTOR_AMD 0x9005
#define GL_DISCRETE_AMD 0x9006
#define GL_CONTINUOUS_AMD 0x9007
#endif
extern void (APIENTRYP gglTessellationFactorAMD)(GLfloat factor);
extern void (APIENTRYP gglTessellationModeAMD)(GLenum mode);

#ifndef GL_AMD_vertex_shader_viewport_index
#define GL_AMD_vertex_shader_viewport_index
#endif

#ifndef GL_APPLE_aux_depth_stencil
#define GL_APPLE_aux_depth_stencil
#define GL_AUX_DEPTH_STENCIL_APPLE 0x8A14
#endif

#ifndef GL_APPLE_client_storage
#define GL_APPLE_client_storage
#define GL_UNPACK_CLIENT_STORAGE_APPLE 0x85B2
#endif

#ifndef GL_APPLE_element_array
#define GL_APPLE_element_array
#define GL_ELEMENT_ARRAY_APPLE 0x8A0C
#define GL_ELEMENT_ARRAY_TYPE_APPLE 0x8A0D
#define GL_ELEMENT_ARRAY_POINTER_APPLE 0x8A0E
#endif
extern void (APIENTRYP gglElementPointerAPPLE)(GLenum type, const void *pointer);
extern void (APIENTRYP gglDrawElementArrayAPPLE)(GLenum mode, GLint first, GLsizei count);
extern void (APIENTRYP gglDrawRangeElementArrayAPPLE)(GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count);
extern void (APIENTRYP gglMultiDrawElementArrayAPPLE)(GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount);
extern void (APIENTRYP gglMultiDrawRangeElementArrayAPPLE)(GLenum mode, GLuint start, GLuint end, const GLint *first, const GLsizei *count, GLsizei primcount);

#ifndef GL_APPLE_fence
#define GL_APPLE_fence
#define GL_DRAW_PIXELS_APPLE 0x8A0A
#define GL_FENCE_APPLE 0x8A0B
#endif
extern void (APIENTRYP gglGenFencesAPPLE)(GLsizei n, GLuint *fences);
extern void (APIENTRYP gglDeleteFencesAPPLE)(GLsizei n, const GLuint *fences);
extern void (APIENTRYP gglSetFenceAPPLE)(GLuint fence);
extern GLboolean (APIENTRYP gglIsFenceAPPLE)(GLuint fence);
extern GLboolean (APIENTRYP gglTestFenceAPPLE)(GLuint fence);
extern void (APIENTRYP gglFinishFenceAPPLE)(GLuint fence);
extern GLboolean (APIENTRYP gglTestObjectAPPLE)(GLenum object, GLuint name);
extern void (APIENTRYP gglFinishObjectAPPLE)(GLenum object, GLint name);

#ifndef GL_APPLE_float_pixels
#define GL_APPLE_float_pixels
#define GL_HALF_APPLE 0x140B
#define GL_RGBA_FLOAT32_APPLE 0x8814
#define GL_RGB_FLOAT32_APPLE 0x8815
#define GL_ALPHA_FLOAT32_APPLE 0x8816
#define GL_INTENSITY_FLOAT32_APPLE 0x8817
#define GL_LUMINANCE_FLOAT32_APPLE 0x8818
#define GL_LUMINANCE_ALPHA_FLOAT32_APPLE 0x8819
#define GL_RGBA_FLOAT16_APPLE 0x881A
#define GL_RGB_FLOAT16_APPLE 0x881B
#define GL_ALPHA_FLOAT16_APPLE 0x881C
#define GL_INTENSITY_FLOAT16_APPLE 0x881D
#define GL_LUMINANCE_FLOAT16_APPLE 0x881E
#define GL_LUMINANCE_ALPHA_FLOAT16_APPLE 0x881F
#define GL_COLOR_FLOAT_APPLE 0x8A0F
#endif

#ifndef GL_APPLE_flush_buffer_range
#define GL_APPLE_flush_buffer_range
#define GL_BUFFER_SERIALIZED_MODIFY_APPLE 0x8A12
#define GL_BUFFER_FLUSHING_UNMAP_APPLE 0x8A13
#endif
extern void (APIENTRYP gglBufferParameteriAPPLE)(GLenum target, GLenum pname, GLint param);
extern void (APIENTRYP gglFlushMappedBufferRangeAPPLE)(GLenum target, GLintptr offset, GLsizeiptr size);

#ifndef GL_APPLE_object_purgeable
#define GL_APPLE_object_purgeable
#define GL_BUFFER_OBJECT_APPLE 0x85B3
#define GL_RELEASED_APPLE 0x8A19
#define GL_VOLATILE_APPLE 0x8A1A
#define GL_RETAINED_APPLE 0x8A1B
#define GL_UNDEFINED_APPLE 0x8A1C
#define GL_PURGEABLE_APPLE 0x8A1D
#endif
extern GLenum (APIENTRYP gglObjectPurgeableAPPLE)(GLenum objectType, GLuint name, GLenum option);
extern GLenum (APIENTRYP gglObjectUnpurgeableAPPLE)(GLenum objectType, GLuint name, GLenum option);
extern void (APIENTRYP gglGetObjectParameterivAPPLE)(GLenum objectType, GLuint name, GLenum pname, GLint *params);

#ifndef GL_APPLE_rgb_422
#define GL_APPLE_rgb_422
#define GL_RGB_422_APPLE 0x8A1F
#define GL_UNSIGNED_SHORT_8_8_APPLE 0x85BA
#define GL_UNSIGNED_SHORT_8_8_REV_APPLE 0x85BB
#define GL_RGB_RAW_422_APPLE 0x8A51
#endif

#ifndef GL_APPLE_row_bytes
#define GL_APPLE_row_bytes
#define GL_PACK_ROW_BYTES_APPLE 0x8A15
#define GL_UNPACK_ROW_BYTES_APPLE 0x8A16
#endif

#ifndef GL_APPLE_specular_vector
#define GL_APPLE_specular_vector
#define GL_LIGHT_MODEL_SPECULAR_VECTOR_APPLE 0x85B0
#endif

#ifndef GL_APPLE_texture_range
#define GL_APPLE_texture_range
#define GL_TEXTURE_RANGE_LENGTH_APPLE 0x85B7
#define GL_TEXTURE_RANGE_POINTER_APPLE 0x85B8
#define GL_TEXTURE_STORAGE_HINT_APPLE 0x85BC
#define GL_STORAGE_PRIVATE_APPLE 0x85BD
#define GL_STORAGE_CACHED_APPLE 0x85BE
#define GL_STORAGE_SHARED_APPLE 0x85BF
#endif
extern void (APIENTRYP gglTextureRangeAPPLE)(GLenum target, GLsizei length, const void *pointer);
extern void (APIENTRYP gglGetTexParameterPointervAPPLE)(GLenum target, GLenum pname, void **params);

#ifndef GL_APPLE_transform_hint
#define GL_APPLE_transform_hint
#define GL_TRANSFORM_HINT_APPLE 0x85B1
#endif

#ifndef GL_APPLE_vertex_array_object
#define GL_APPLE_vertex_array_object
#define GL_VERTEX_ARRAY_BINDING_APPLE 0x85B5
#endif
extern void (APIENTRYP gglBindVertexArrayAPPLE)(GLuint array);
extern void (APIENTRYP gglDeleteVertexArraysAPPLE)(GLsizei n, const GLuint *arrays);
extern void (APIENTRYP gglGenVertexArraysAPPLE)(GLsizei n, GLuint *arrays);
extern GLboolean (APIENTRYP gglIsVertexArrayAPPLE)(GLuint array);

#ifndef GL_APPLE_vertex_array_range
#define GL_APPLE_vertex_array_range
#define GL_VERTEX_ARRAY_RANGE_APPLE 0x851D
#define GL_VERTEX_ARRAY_RANGE_LENGTH_APPLE 0x851E
#define GL_VERTEX_ARRAY_STORAGE_HINT_APPLE 0x851F
#define GL_VERTEX_ARRAY_RANGE_POINTER_APPLE 0x8521
#define GL_STORAGE_CLIENT_APPLE 0x85B4
/* reuse GL_STORAGE_CACHED_APPLE */
/* reuse GL_STORAGE_SHARED_APPLE */
#endif
extern void (APIENTRYP gglVertexArrayRangeAPPLE)(GLsizei length, void *pointer);
extern void (APIENTRYP gglFlushVertexArrayRangeAPPLE)(GLsizei length, void *pointer);
extern void (APIENTRYP gglVertexArrayParameteriAPPLE)(GLenum pname, GLint param);

#ifndef GL_APPLE_vertex_program_evaluators
#define GL_APPLE_vertex_program_evaluators
#define GL_VERTEX_ATTRIB_MAP1_APPLE 0x8A00
#define GL_VERTEX_ATTRIB_MAP2_APPLE 0x8A01
#define GL_VERTEX_ATTRIB_MAP1_SIZE_APPLE 0x8A02
#define GL_VERTEX_ATTRIB_MAP1_COEFF_APPLE 0x8A03
#define GL_VERTEX_ATTRIB_MAP1_ORDER_APPLE 0x8A04
#define GL_VERTEX_ATTRIB_MAP1_DOMAIN_APPLE 0x8A05
#define GL_VERTEX_ATTRIB_MAP2_SIZE_APPLE 0x8A06
#define GL_VERTEX_ATTRIB_MAP2_COEFF_APPLE 0x8A07
#define GL_VERTEX_ATTRIB_MAP2_ORDER_APPLE 0x8A08
#define GL_VERTEX_ATTRIB_MAP2_DOMAIN_APPLE 0x8A09
#endif
extern void (APIENTRYP gglEnableVertexAttribAPPLE)(GLuint index, GLenum pname);
extern void (APIENTRYP gglDisableVertexAttribAPPLE)(GLuint index, GLenum pname);
extern GLboolean (APIENTRYP gglIsVertexAttribEnabledAPPLE)(GLuint index, GLenum pname);
extern void (APIENTRYP gglMapVertexAttrib1dAPPLE)(GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
extern void (APIENTRYP gglMapVertexAttrib1fAPPLE)(GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
extern void (APIENTRYP gglMapVertexAttrib2dAPPLE)(GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
extern void (APIENTRYP gglMapVertexAttrib2fAPPLE)(GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);

#ifndef GL_APPLE_ycbcr_422
#define GL_APPLE_ycbcr_422
#define GL_YCBCR_422_APPLE 0x85B9
/* reuse GL_UNSIGNED_SHORT_8_8_APPLE */
/* reuse GL_UNSIGNED_SHORT_8_8_REV_APPLE */
#endif

#ifndef GL_ARB_ES2_compatibility
#define GL_ARB_ES2_compatibility
#define GL_FIXED 0x140C
#define GL_IMPLEMENTATION_COLOR_READ_TYPE 0x8B9A
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT 0x8B9B
#define GL_LOW_FLOAT 0x8DF0
#define GL_MEDIUM_FLOAT 0x8DF1
#define GL_HIGH_FLOAT 0x8DF2
#define GL_LOW_INT 0x8DF3
#define GL_MEDIUM_INT 0x8DF4
#define GL_HIGH_INT 0x8DF5
#define GL_SHADER_COMPILER 0x8DFA
#define GL_SHADER_BINARY_FORMATS 0x8DF8
#define GL_NUM_SHADER_BINARY_FORMATS 0x8DF9
#define GL_MAX_VERTEX_UNIFORM_VECTORS 0x8DFB
#define GL_MAX_VARYING_VECTORS 0x8DFC
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS 0x8DFD
#define GL_RGB565 0x8D62
#endif
extern void (APIENTRYP gglReleaseShaderCompiler)();
extern void (APIENTRYP gglShaderBinary)(GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length);
extern void (APIENTRYP gglGetShaderPrecisionFormat)(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision);
extern void (APIENTRYP gglDepthRangef)(GLfloat n, GLfloat f);
extern void (APIENTRYP gglClearDepthf)(GLfloat d);

#ifndef GL_ARB_ES3_1_compatibility
#define GL_ARB_ES3_1_compatibility
/* reuse GL_BACK */
#endif
extern void (APIENTRYP gglMemoryBarrierByRegion)(GLbitfield barriers);

#ifndef GL_ARB_ES3_2_compatibility
#define GL_ARB_ES3_2_compatibility
#define GL_PRIMITIVE_BOUNDING_BOX_ARB 0x92BE
#define GL_MULTISAMPLE_LINE_WIDTH_RANGE_ARB 0x9381
#define GL_MULTISAMPLE_LINE_WIDTH_GRANULARITY_ARB 0x9382
#endif
extern void (APIENTRYP gglPrimitiveBoundingBoxARB)(GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW);

#ifndef GL_ARB_ES3_compatibility
#define GL_ARB_ES3_compatibility
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#define GL_COMPRESSED_SRGB8_ETC2 0x9275
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279
#define GL_COMPRESSED_R11_EAC 0x9270
#define GL_COMPRESSED_SIGNED_R11_EAC 0x9271
#define GL_COMPRESSED_RG11_EAC 0x9272
#define GL_COMPRESSED_SIGNED_RG11_EAC 0x9273
#define GL_PRIMITIVE_RESTART_FIXED_INDEX 0x8D69
#define GL_ANY_SAMPLES_PASSED_CONSERVATIVE 0x8D6A
#define GL_MAX_ELEMENT_INDEX 0x8D6B
#endif

#ifndef GL_ARB_arrays_of_arrays
#define GL_ARB_arrays_of_arrays
#endif

#ifndef GL_ARB_base_instance
#define GL_ARB_base_instance
#endif
extern void (APIENTRYP gglDrawArraysInstancedBaseInstance)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
extern void (APIENTRYP gglDrawElementsInstancedBaseInstance)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance);
extern void (APIENTRYP gglDrawElementsInstancedBaseVertexBaseInstance)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance);

#ifndef GL_ARB_bindless_texture
#define GL_ARB_bindless_texture
#define GL_UNSIGNED_INT64_ARB 0x140F
#endif
extern GLuint64 (APIENTRYP gglGetTextureHandleARB)(GLuint texture);
extern GLuint64 (APIENTRYP gglGetTextureSamplerHandleARB)(GLuint texture, GLuint sampler);
extern void (APIENTRYP gglMakeTextureHandleResidentARB)(GLuint64 handle);
extern void (APIENTRYP gglMakeTextureHandleNonResidentARB)(GLuint64 handle);
extern GLuint64 (APIENTRYP gglGetImageHandleARB)(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format);
extern void (APIENTRYP gglMakeImageHandleResidentARB)(GLuint64 handle, GLenum access);
extern void (APIENTRYP gglMakeImageHandleNonResidentARB)(GLuint64 handle);
extern void (APIENTRYP gglUniformHandleui64ARB)(GLint location, GLuint64 value);
extern void (APIENTRYP gglUniformHandleui64vARB)(GLint location, GLsizei count, const GLuint64 *value);
extern void (APIENTRYP gglProgramUniformHandleui64ARB)(GLuint program, GLint location, GLuint64 value);
extern void (APIENTRYP gglProgramUniformHandleui64vARB)(GLuint program, GLint location, GLsizei count, const GLuint64 *values);
extern GLboolean (APIENTRYP gglIsTextureHandleResidentARB)(GLuint64 handle);
extern GLboolean (APIENTRYP gglIsImageHandleResidentARB)(GLuint64 handle);
extern void (APIENTRYP gglVertexAttribL1ui64ARB)(GLuint index, GLuint64EXT x);
extern void (APIENTRYP gglVertexAttribL1ui64vARB)(GLuint index, const GLuint64EXT *v);
extern void (APIENTRYP gglGetVertexAttribLui64vARB)(GLuint index, GLenum pname, GLuint64EXT *params);

#ifndef GL_ARB_blend_func_extended
#define GL_ARB_blend_func_extended
#define GL_SRC1_COLOR 0x88F9
/* reuse GL_SRC1_ALPHA */
#define GL_ONE_MINUS_SRC1_COLOR 0x88FA
#define GL_ONE_MINUS_SRC1_ALPHA 0x88FB
#define GL_MAX_DUAL_SOURCE_DRAW_BUFFERS 0x88FC
#endif
extern void (APIENTRYP gglBindFragDataLocationIndexed)(GLuint program, GLuint colorNumber, GLuint index, const GLchar *name);
extern GLint (APIENTRYP gglGetFragDataIndex)(GLuint program, const GLchar *name);

#ifndef GL_ARB_buffer_storage
#define GL_ARB_buffer_storage
/* reuse GL_MAP_READ_BIT */
/* reuse GL_MAP_WRITE_BIT */
#define GL_MAP_PERSISTENT_BIT 0x0040
#define GL_MAP_COHERENT_BIT 0x0080
#define GL_DYNAMIC_STORAGE_BIT 0x0100
#define GL_CLIENT_STORAGE_BIT 0x0200
#define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT 0x00004000
#define GL_BUFFER_IMMUTABLE_STORAGE 0x821F
#define GL_BUFFER_STORAGE_FLAGS 0x8220
#endif
extern void (APIENTRYP gglBufferStorage)(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);

#ifndef GL_ARB_cl_event
#define GL_ARB_cl_event
#define GL_SYNC_CL_EVENT_ARB 0x8240
#define GL_SYNC_CL_EVENT_COMPLETE_ARB 0x8241
#endif
extern GLsync (APIENTRYP gglCreateSyncFromCLeventARB)(struct _cl_context *context, struct _cl_event *event, GLbitfield flags);

#ifndef GL_ARB_clear_buffer_object
#define GL_ARB_clear_buffer_object
#endif
extern void (APIENTRYP gglClearBufferData)(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data);
extern void (APIENTRYP gglClearBufferSubData)(GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data);

#ifndef GL_ARB_clear_texture
#define GL_ARB_clear_texture
#define GL_CLEAR_TEXTURE 0x9365
#endif
extern void (APIENTRYP gglClearTexImage)(GLuint texture, GLint level, GLenum format, GLenum type, const void *data);
extern void (APIENTRYP gglClearTexSubImage)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data);

#ifndef GL_ARB_clip_control
#define GL_ARB_clip_control
/* reuse GL_LOWER_LEFT */
/* reuse GL_UPPER_LEFT */
#define GL_NEGATIVE_ONE_TO_ONE 0x935E
#define GL_ZERO_TO_ONE 0x935F
#define GL_CLIP_ORIGIN 0x935C
#define GL_CLIP_DEPTH_MODE 0x935D
#endif
extern void (APIENTRYP gglClipControl)(GLenum origin, GLenum depth);

#ifndef GL_ARB_color_buffer_float
#define GL_ARB_color_buffer_float
#define GL_RGBA_FLOAT_MODE_ARB 0x8820
#define GL_CLAMP_VERTEX_COLOR_ARB 0x891A
#define GL_CLAMP_FRAGMENT_COLOR_ARB 0x891B
#define GL_CLAMP_READ_COLOR_ARB 0x891C
#define GL_FIXED_ONLY_ARB 0x891D
#endif
extern void (APIENTRYP gglClampColorARB)(GLenum target, GLenum clamp);

#ifndef GL_ARB_compatibility
#define GL_ARB_compatibility
#endif

#ifndef GL_ARB_compressed_texture_pixel_storage
#define GL_ARB_compressed_texture_pixel_storage
#define GL_UNPACK_COMPRESSED_BLOCK_WIDTH 0x9127
#define GL_UNPACK_COMPRESSED_BLOCK_HEIGHT 0x9128
#define GL_UNPACK_COMPRESSED_BLOCK_DEPTH 0x9129
#define GL_UNPACK_COMPRESSED_BLOCK_SIZE 0x912A
#define GL_PACK_COMPRESSED_BLOCK_WIDTH 0x912B
#define GL_PACK_COMPRESSED_BLOCK_HEIGHT 0x912C
#define GL_PACK_COMPRESSED_BLOCK_DEPTH 0x912D
#define GL_PACK_COMPRESSED_BLOCK_SIZE 0x912E
#endif

#ifndef GL_ARB_compute_shader
#define GL_ARB_compute_shader
#define GL_COMPUTE_SHADER 0x91B9
#define GL_MAX_COMPUTE_UNIFORM_BLOCKS 0x91BB
#define GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS 0x91BC
#define GL_MAX_COMPUTE_IMAGE_UNIFORMS 0x91BD
#define GL_MAX_COMPUTE_SHARED_MEMORY_SIZE 0x8262
#define GL_MAX_COMPUTE_UNIFORM_COMPONENTS 0x8263
#define GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS 0x8264
#define GL_MAX_COMPUTE_ATOMIC_COUNTERS 0x8265
#define GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS 0x8266
#define GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS 0x90EB
#define GL_MAX_COMPUTE_WORK_GROUP_COUNT 0x91BE
#define GL_MAX_COMPUTE_WORK_GROUP_SIZE 0x91BF
#define GL_COMPUTE_WORK_GROUP_SIZE 0x8267
#define GL_UNIFORM_BLOCK_REFERENCED_BY_COMPUTE_SHADER 0x90EC
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_COMPUTE_SHADER 0x90ED
#define GL_DISPATCH_INDIRECT_BUFFER 0x90EE
#define GL_DISPATCH_INDIRECT_BUFFER_BINDING 0x90EF
#define GL_COMPUTE_SHADER_BIT 0x00000020
#endif
extern void (APIENTRYP gglDispatchCompute)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
extern void (APIENTRYP gglDispatchComputeIndirect)(GLintptr indirect);

#ifndef GL_ARB_compute_variable_group_size
#define GL_ARB_compute_variable_group_size
#define GL_MAX_COMPUTE_VARIABLE_GROUP_INVOCATIONS_ARB 0x9344
#define GL_MAX_COMPUTE_FIXED_GROUP_INVOCATIONS_ARB 0x90EB
#define GL_MAX_COMPUTE_VARIABLE_GROUP_SIZE_ARB 0x9345
#define GL_MAX_COMPUTE_FIXED_GROUP_SIZE_ARB 0x91BF
#endif
extern void (APIENTRYP gglDispatchComputeGroupSizeARB)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z, GLuint group_size_x, GLuint group_size_y, GLuint group_size_z);

#ifndef GL_ARB_conditional_render_inverted
#define GL_ARB_conditional_render_inverted
#define GL_QUERY_WAIT_INVERTED 0x8E17
#define GL_QUERY_NO_WAIT_INVERTED 0x8E18
#define GL_QUERY_BY_REGION_WAIT_INVERTED 0x8E19
#define GL_QUERY_BY_REGION_NO_WAIT_INVERTED 0x8E1A
#endif

#ifndef GL_ARB_conservative_depth
#define GL_ARB_conservative_depth
#endif

#ifndef GL_ARB_copy_buffer
#define GL_ARB_copy_buffer
/* reuse GL_COPY_READ_BUFFER */
/* reuse GL_COPY_WRITE_BUFFER */
#endif
/* reuse void (APIENTRYP gglCopyBufferSubData)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) */

#ifndef GL_ARB_copy_image
#define GL_ARB_copy_image
#endif
extern void (APIENTRYP gglCopyImageSubData)(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);

#ifndef GL_ARB_cull_distance
#define GL_ARB_cull_distance
#define GL_MAX_CULL_DISTANCES 0x82F9
#define GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES 0x82FA
#endif

#ifndef GL_ARB_debug_output
#define GL_ARB_debug_output
#define GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB 0x8242
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_ARB 0x8243
#define GL_DEBUG_CALLBACK_FUNCTION_ARB 0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM_ARB 0x8245
#define GL_DEBUG_SOURCE_API_ARB 0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB 0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER_ARB 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY_ARB 0x8249
#define GL_DEBUG_SOURCE_APPLICATION_ARB 0x824A
#define GL_DEBUG_SOURCE_OTHER_ARB 0x824B
#define GL_DEBUG_TYPE_ERROR_ARB 0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB 0x824E
#define GL_DEBUG_TYPE_PORTABILITY_ARB 0x824F
#define GL_DEBUG_TYPE_PERFORMANCE_ARB 0x8250
#define GL_DEBUG_TYPE_OTHER_ARB 0x8251
#define GL_MAX_DEBUG_MESSAGE_LENGTH_ARB 0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES_ARB 0x9144
#define GL_DEBUG_LOGGED_MESSAGES_ARB 0x9145
#define GL_DEBUG_SEVERITY_HIGH_ARB 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM_ARB 0x9147
#define GL_DEBUG_SEVERITY_LOW_ARB 0x9148
#endif
extern void (APIENTRYP gglDebugMessageControlARB)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
extern void (APIENTRYP gglDebugMessageInsertARB)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf);
extern void (APIENTRYP gglDebugMessageCallbackARB)(GLDEBUGPROCARB callback, const void *userParam);
extern GLuint (APIENTRYP gglGetDebugMessageLogARB)(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog);

#ifndef GL_ARB_depth_buffer_float
#define GL_ARB_depth_buffer_float
/* reuse GL_DEPTH_COMPONENT32F */
/* reuse GL_DEPTH32F_STENCIL8 */
/* reuse GL_FLOAT_32_UNSIGNED_INT_24_8_REV */
#endif

#ifndef GL_ARB_depth_clamp
#define GL_ARB_depth_clamp
/* reuse GL_DEPTH_CLAMP */
#endif

#ifndef GL_ARB_depth_texture
#define GL_ARB_depth_texture
#define GL_DEPTH_COMPONENT16_ARB 0x81A5
#define GL_DEPTH_COMPONENT24_ARB 0x81A6
#define GL_DEPTH_COMPONENT32_ARB 0x81A7
#define GL_TEXTURE_DEPTH_SIZE_ARB 0x884A
#define GL_DEPTH_TEXTURE_MODE_ARB 0x884B
#endif

#ifndef GL_ARB_derivative_control
#define GL_ARB_derivative_control
#endif

#ifndef GL_ARB_direct_state_access
#define GL_ARB_direct_state_access
#define GL_TEXTURE_TARGET 0x1006
#define GL_QUERY_TARGET 0x82EA
/* reuse GL_TEXTURE_BINDING_1D */
/* reuse GL_TEXTURE_BINDING_1D_ARRAY */
/* reuse GL_TEXTURE_BINDING_2D */
/* reuse GL_TEXTURE_BINDING_2D_ARRAY */
/* reuse GL_TEXTURE_BINDING_2D_MULTISAMPLE */
/* reuse GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY */
/* reuse GL_TEXTURE_BINDING_3D */
/* reuse GL_TEXTURE_BINDING_BUFFER */
/* reuse GL_TEXTURE_BINDING_CUBE_MAP */
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY 0x900A
/* reuse GL_TEXTURE_BINDING_RECTANGLE */
#endif
extern void (APIENTRYP gglCreateTransformFeedbacks)(GLsizei n, GLuint *ids);
extern void (APIENTRYP gglTransformFeedbackBufferBase)(GLuint xfb, GLuint index, GLuint buffer);
extern void (APIENTRYP gglTransformFeedbackBufferRange)(GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
extern void (APIENTRYP gglGetTransformFeedbackiv)(GLuint xfb, GLenum pname, GLint *param);
extern void (APIENTRYP gglGetTransformFeedbacki_v)(GLuint xfb, GLenum pname, GLuint index, GLint *param);
extern void (APIENTRYP gglGetTransformFeedbacki64_v)(GLuint xfb, GLenum pname, GLuint index, GLint64 *param);
extern void (APIENTRYP gglCreateBuffers)(GLsizei n, GLuint *buffers);
extern void (APIENTRYP gglNamedBufferStorage)(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags);
extern void (APIENTRYP gglNamedBufferData)(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage);
extern void (APIENTRYP gglNamedBufferSubData)(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data);
extern void (APIENTRYP gglCopyNamedBufferSubData)(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
extern void (APIENTRYP gglClearNamedBufferData)(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data);
extern void (APIENTRYP gglClearNamedBufferSubData)(GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data);
extern void * (APIENTRYP gglMapNamedBuffer)(GLuint buffer, GLenum access);
extern void * (APIENTRYP gglMapNamedBufferRange)(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access);
extern GLboolean (APIENTRYP gglUnmapNamedBuffer)(GLuint buffer);
extern void (APIENTRYP gglFlushMappedNamedBufferRange)(GLuint buffer, GLintptr offset, GLsizeiptr length);
extern void (APIENTRYP gglGetNamedBufferParameteriv)(GLuint buffer, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetNamedBufferParameteri64v)(GLuint buffer, GLenum pname, GLint64 *params);
extern void (APIENTRYP gglGetNamedBufferPointerv)(GLuint buffer, GLenum pname, void **params);
extern void (APIENTRYP gglGetNamedBufferSubData)(GLuint buffer, GLintptr offset, GLsizeiptr size, void *data);
extern void (APIENTRYP gglCreateFramebuffers)(GLsizei n, GLuint *framebuffers);
extern void (APIENTRYP gglNamedFramebufferRenderbuffer)(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
extern void (APIENTRYP gglNamedFramebufferParameteri)(GLuint framebuffer, GLenum pname, GLint param);
extern void (APIENTRYP gglNamedFramebufferTexture)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
extern void (APIENTRYP gglNamedFramebufferTextureLayer)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer);
extern void (APIENTRYP gglNamedFramebufferDrawBuffer)(GLuint framebuffer, GLenum buf);
extern void (APIENTRYP gglNamedFramebufferDrawBuffers)(GLuint framebuffer, GLsizei n, const GLenum *bufs);
extern void (APIENTRYP gglNamedFramebufferReadBuffer)(GLuint framebuffer, GLenum src);
extern void (APIENTRYP gglInvalidateNamedFramebufferData)(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments);
extern void (APIENTRYP gglInvalidateNamedFramebufferSubData)(GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglClearNamedFramebufferiv)(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value);
extern void (APIENTRYP gglClearNamedFramebufferuiv)(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value);
extern void (APIENTRYP gglClearNamedFramebufferfv)(GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value);
extern void (APIENTRYP gglClearNamedFramebufferfi)(GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
extern void (APIENTRYP gglBlitNamedFramebuffer)(GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
extern GLenum (APIENTRYP gglCheckNamedFramebufferStatus)(GLuint framebuffer, GLenum target);
extern void (APIENTRYP gglGetNamedFramebufferParameteriv)(GLuint framebuffer, GLenum pname, GLint *param);
extern void (APIENTRYP gglGetNamedFramebufferAttachmentParameteriv)(GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params);
extern void (APIENTRYP gglCreateRenderbuffers)(GLsizei n, GLuint *renderbuffers);
extern void (APIENTRYP gglNamedRenderbufferStorage)(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglNamedRenderbufferStorageMultisample)(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglGetNamedRenderbufferParameteriv)(GLuint renderbuffer, GLenum pname, GLint *params);
extern void (APIENTRYP gglCreateTextures)(GLenum target, GLsizei n, GLuint *textures);
extern void (APIENTRYP gglTextureBuffer)(GLuint texture, GLenum internalformat, GLuint buffer);
extern void (APIENTRYP gglTextureBufferRange)(GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
extern void (APIENTRYP gglTextureStorage1D)(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width);
extern void (APIENTRYP gglTextureStorage2D)(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglTextureStorage3D)(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
extern void (APIENTRYP gglTextureStorage2DMultisample)(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
extern void (APIENTRYP gglTextureStorage3DMultisample)(GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
extern void (APIENTRYP gglTextureSubImage1D)(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTextureSubImage2D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTextureSubImage3D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglCompressedTextureSubImage1D)(GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTextureSubImage2D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTextureSubImage3D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCopyTextureSubImage1D)(GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern void (APIENTRYP gglCopyTextureSubImage2D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglCopyTextureSubImage3D)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglTextureParameterf)(GLuint texture, GLenum pname, GLfloat param);
extern void (APIENTRYP gglTextureParameterfv)(GLuint texture, GLenum pname, const GLfloat *param);
extern void (APIENTRYP gglTextureParameteri)(GLuint texture, GLenum pname, GLint param);
extern void (APIENTRYP gglTextureParameterIiv)(GLuint texture, GLenum pname, const GLint *params);
extern void (APIENTRYP gglTextureParameterIuiv)(GLuint texture, GLenum pname, const GLuint *params);
extern void (APIENTRYP gglTextureParameteriv)(GLuint texture, GLenum pname, const GLint *param);
extern void (APIENTRYP gglGenerateTextureMipmap)(GLuint texture);
extern void (APIENTRYP gglBindTextureUnit)(GLuint unit, GLuint texture);
extern void (APIENTRYP gglGetTextureImage)(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels);
extern void (APIENTRYP gglGetCompressedTextureImage)(GLuint texture, GLint level, GLsizei bufSize, void *pixels);
extern void (APIENTRYP gglGetTextureLevelParameterfv)(GLuint texture, GLint level, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetTextureLevelParameteriv)(GLuint texture, GLint level, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetTextureParameterfv)(GLuint texture, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetTextureParameterIiv)(GLuint texture, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetTextureParameterIuiv)(GLuint texture, GLenum pname, GLuint *params);
extern void (APIENTRYP gglGetTextureParameteriv)(GLuint texture, GLenum pname, GLint *params);
extern void (APIENTRYP gglCreateVertexArrays)(GLsizei n, GLuint *arrays);
extern void (APIENTRYP gglDisableVertexArrayAttrib)(GLuint vaobj, GLuint index);
extern void (APIENTRYP gglEnableVertexArrayAttrib)(GLuint vaobj, GLuint index);
extern void (APIENTRYP gglVertexArrayElementBuffer)(GLuint vaobj, GLuint buffer);
extern void (APIENTRYP gglVertexArrayVertexBuffer)(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
extern void (APIENTRYP gglVertexArrayVertexBuffers)(GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides);
extern void (APIENTRYP gglVertexArrayAttribBinding)(GLuint vaobj, GLuint attribindex, GLuint bindingindex);
extern void (APIENTRYP gglVertexArrayAttribFormat)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
extern void (APIENTRYP gglVertexArrayAttribIFormat)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
extern void (APIENTRYP gglVertexArrayAttribLFormat)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
extern void (APIENTRYP gglVertexArrayBindingDivisor)(GLuint vaobj, GLuint bindingindex, GLuint divisor);
extern void (APIENTRYP gglGetVertexArrayiv)(GLuint vaobj, GLenum pname, GLint *param);
extern void (APIENTRYP gglGetVertexArrayIndexediv)(GLuint vaobj, GLuint index, GLenum pname, GLint *param);
extern void (APIENTRYP gglGetVertexArrayIndexed64iv)(GLuint vaobj, GLuint index, GLenum pname, GLint64 *param);
extern void (APIENTRYP gglCreateSamplers)(GLsizei n, GLuint *samplers);
extern void (APIENTRYP gglCreateProgramPipelines)(GLsizei n, GLuint *pipelines);
extern void (APIENTRYP gglCreateQueries)(GLenum target, GLsizei n, GLuint *ids);
extern void (APIENTRYP gglGetQueryBufferObjecti64v)(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
extern void (APIENTRYP gglGetQueryBufferObjectiv)(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
extern void (APIENTRYP gglGetQueryBufferObjectui64v)(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
extern void (APIENTRYP gglGetQueryBufferObjectuiv)(GLuint id, GLuint buffer, GLenum pname, GLintptr offset);

#ifndef GL_ARB_draw_buffers
#define GL_ARB_draw_buffers
#define GL_MAX_DRAW_BUFFERS_ARB 0x8824
#define GL_DRAW_BUFFER0_ARB 0x8825
#define GL_DRAW_BUFFER1_ARB 0x8826
#define GL_DRAW_BUFFER2_ARB 0x8827
#define GL_DRAW_BUFFER3_ARB 0x8828
#define GL_DRAW_BUFFER4_ARB 0x8829
#define GL_DRAW_BUFFER5_ARB 0x882A
#define GL_DRAW_BUFFER6_ARB 0x882B
#define GL_DRAW_BUFFER7_ARB 0x882C
#define GL_DRAW_BUFFER8_ARB 0x882D
#define GL_DRAW_BUFFER9_ARB 0x882E
#define GL_DRAW_BUFFER10_ARB 0x882F
#define GL_DRAW_BUFFER11_ARB 0x8830
#define GL_DRAW_BUFFER12_ARB 0x8831
#define GL_DRAW_BUFFER13_ARB 0x8832
#define GL_DRAW_BUFFER14_ARB 0x8833
#define GL_DRAW_BUFFER15_ARB 0x8834
#endif
extern void (APIENTRYP gglDrawBuffersARB)(GLsizei n, const GLenum *bufs);

#ifndef GL_ARB_draw_buffers_blend
#define GL_ARB_draw_buffers_blend
#endif
extern void (APIENTRYP gglBlendEquationiARB)(GLuint buf, GLenum mode);
extern void (APIENTRYP gglBlendEquationSeparateiARB)(GLuint buf, GLenum modeRGB, GLenum modeAlpha);
extern void (APIENTRYP gglBlendFunciARB)(GLuint buf, GLenum src, GLenum dst);
extern void (APIENTRYP gglBlendFuncSeparateiARB)(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);

#ifndef GL_ARB_draw_elements_base_vertex
#define GL_ARB_draw_elements_base_vertex
#endif
/* reuse void (APIENTRYP gglDrawElementsBaseVertex)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) */
/* reuse void (APIENTRYP gglDrawRangeElementsBaseVertex)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex) */
/* reuse void (APIENTRYP gglDrawElementsInstancedBaseVertex)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex) */
/* reuse void (APIENTRYP gglMultiDrawElementsBaseVertex)(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex) */

#ifndef GL_ARB_draw_indirect
#define GL_ARB_draw_indirect
#define GL_DRAW_INDIRECT_BUFFER 0x8F3F
#define GL_DRAW_INDIRECT_BUFFER_BINDING 0x8F43
#endif
extern void (APIENTRYP gglDrawArraysIndirect)(GLenum mode, const void *indirect);
extern void (APIENTRYP gglDrawElementsIndirect)(GLenum mode, GLenum type, const void *indirect);

#ifndef GL_ARB_draw_instanced
#define GL_ARB_draw_instanced
#endif
extern void (APIENTRYP gglDrawArraysInstancedARB)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
extern void (APIENTRYP gglDrawElementsInstancedARB)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);

#ifndef GL_ARB_enhanced_layouts
#define GL_ARB_enhanced_layouts
#define GL_LOCATION_COMPONENT 0x934A
/* reuse GL_TRANSFORM_FEEDBACK_BUFFER */
#define GL_TRANSFORM_FEEDBACK_BUFFER_INDEX 0x934B
#define GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE 0x934C
#endif

#ifndef GL_ARB_explicit_attrib_location
#define GL_ARB_explicit_attrib_location
#endif

#ifndef GL_ARB_explicit_uniform_location
#define GL_ARB_explicit_uniform_location
#define GL_MAX_UNIFORM_LOCATIONS 0x826E
#endif

#ifndef GL_ARB_fragment_coord_conventions
#define GL_ARB_fragment_coord_conventions
#endif

#ifndef GL_ARB_fragment_layer_viewport
#define GL_ARB_fragment_layer_viewport
#endif

#ifndef GL_ARB_fragment_program
#define GL_ARB_fragment_program
#define GL_FRAGMENT_PROGRAM_ARB 0x8804
#define GL_PROGRAM_FORMAT_ASCII_ARB 0x8875
#define GL_PROGRAM_LENGTH_ARB 0x8627
#define GL_PROGRAM_FORMAT_ARB 0x8876
#define GL_PROGRAM_BINDING_ARB 0x8677
#define GL_PROGRAM_INSTRUCTIONS_ARB 0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB 0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB 0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB 0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A7
#define GL_PROGRAM_PARAMETERS_ARB 0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB 0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AB
#define GL_PROGRAM_ATTRIBS_ARB 0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB 0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AF
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB 0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB 0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB 0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB 0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB 0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB 0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB 0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB 0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x8810
#define GL_PROGRAM_STRING_ARB 0x8628
#define GL_PROGRAM_ERROR_POSITION_ARB 0x864B
#define GL_CURRENT_MATRIX_ARB 0x8641
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB 0x88B7
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB 0x8640
#define GL_MAX_PROGRAM_MATRICES_ARB 0x862F
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB 0x862E
#define GL_MAX_TEXTURE_COORDS_ARB 0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB 0x8872
#define GL_PROGRAM_ERROR_STRING_ARB 0x8874
#define GL_MATRIX0_ARB 0x88C0
#define GL_MATRIX1_ARB 0x88C1
#define GL_MATRIX2_ARB 0x88C2
#define GL_MATRIX3_ARB 0x88C3
#define GL_MATRIX4_ARB 0x88C4
#define GL_MATRIX5_ARB 0x88C5
#define GL_MATRIX6_ARB 0x88C6
#define GL_MATRIX7_ARB 0x88C7
#define GL_MATRIX8_ARB 0x88C8
#define GL_MATRIX9_ARB 0x88C9
#define GL_MATRIX10_ARB 0x88CA
#define GL_MATRIX11_ARB 0x88CB
#define GL_MATRIX12_ARB 0x88CC
#define GL_MATRIX13_ARB 0x88CD
#define GL_MATRIX14_ARB 0x88CE
#define GL_MATRIX15_ARB 0x88CF
#define GL_MATRIX16_ARB 0x88D0
#define GL_MATRIX17_ARB 0x88D1
#define GL_MATRIX18_ARB 0x88D2
#define GL_MATRIX19_ARB 0x88D3
#define GL_MATRIX20_ARB 0x88D4
#define GL_MATRIX21_ARB 0x88D5
#define GL_MATRIX22_ARB 0x88D6
#define GL_MATRIX23_ARB 0x88D7
#define GL_MATRIX24_ARB 0x88D8
#define GL_MATRIX25_ARB 0x88D9
#define GL_MATRIX26_ARB 0x88DA
#define GL_MATRIX27_ARB 0x88DB
#define GL_MATRIX28_ARB 0x88DC
#define GL_MATRIX29_ARB 0x88DD
#define GL_MATRIX30_ARB 0x88DE
#define GL_MATRIX31_ARB 0x88DF
#endif
extern void (APIENTRYP gglProgramStringARB)(GLenum target, GLenum format, GLsizei len, const void *string);
extern void (APIENTRYP gglBindProgramARB)(GLenum target, GLuint program);
extern void (APIENTRYP gglDeleteProgramsARB)(GLsizei n, const GLuint *programs);
extern void (APIENTRYP gglGenProgramsARB)(GLsizei n, GLuint *programs);
extern void (APIENTRYP gglProgramEnvParameter4dARB)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglProgramEnvParameter4dvARB)(GLenum target, GLuint index, const GLdouble *params);
extern void (APIENTRYP gglProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglProgramEnvParameter4fvARB)(GLenum target, GLuint index, const GLfloat *params);
extern void (APIENTRYP gglProgramLocalParameter4dARB)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglProgramLocalParameter4dvARB)(GLenum target, GLuint index, const GLdouble *params);
extern void (APIENTRYP gglProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglProgramLocalParameter4fvARB)(GLenum target, GLuint index, const GLfloat *params);
extern void (APIENTRYP gglGetProgramEnvParameterdvARB)(GLenum target, GLuint index, GLdouble *params);
extern void (APIENTRYP gglGetProgramEnvParameterfvARB)(GLenum target, GLuint index, GLfloat *params);
extern void (APIENTRYP gglGetProgramLocalParameterdvARB)(GLenum target, GLuint index, GLdouble *params);
extern void (APIENTRYP gglGetProgramLocalParameterfvARB)(GLenum target, GLuint index, GLfloat *params);
extern void (APIENTRYP gglGetProgramivARB)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetProgramStringARB)(GLenum target, GLenum pname, void *string);
extern GLboolean (APIENTRYP gglIsProgramARB)(GLuint program);

#ifndef GL_ARB_fragment_program_shadow
#define GL_ARB_fragment_program_shadow
#endif

#ifndef GL_ARB_fragment_shader
#define GL_ARB_fragment_shader
#define GL_FRAGMENT_SHADER_ARB 0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB 0x8B49
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB 0x8B8B
#endif

#ifndef GL_ARB_fragment_shader_interlock
#define GL_ARB_fragment_shader_interlock
#endif

#ifndef GL_ARB_framebuffer_no_attachments
#define GL_ARB_framebuffer_no_attachments
#define GL_FRAMEBUFFER_DEFAULT_WIDTH 0x9310
#define GL_FRAMEBUFFER_DEFAULT_HEIGHT 0x9311
#define GL_FRAMEBUFFER_DEFAULT_LAYERS 0x9312
#define GL_FRAMEBUFFER_DEFAULT_SAMPLES 0x9313
#define GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS 0x9314
#define GL_MAX_FRAMEBUFFER_WIDTH 0x9315
#define GL_MAX_FRAMEBUFFER_HEIGHT 0x9316
#define GL_MAX_FRAMEBUFFER_LAYERS 0x9317
#define GL_MAX_FRAMEBUFFER_SAMPLES 0x9318
#endif
extern void (APIENTRYP gglFramebufferParameteri)(GLenum target, GLenum pname, GLint param);
extern void (APIENTRYP gglGetFramebufferParameteriv)(GLenum target, GLenum pname, GLint *params);

#ifndef GL_ARB_framebuffer_object
#define GL_ARB_framebuffer_object
/* reuse GL_INVALID_FRAMEBUFFER_OPERATION */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE */
/* reuse GL_FRAMEBUFFER_DEFAULT */
/* reuse GL_FRAMEBUFFER_UNDEFINED */
/* reuse GL_DEPTH_STENCIL_ATTACHMENT */
/* reuse GL_MAX_RENDERBUFFER_SIZE */
/* reuse GL_DEPTH_STENCIL */
/* reuse GL_UNSIGNED_INT_24_8 */
/* reuse GL_DEPTH24_STENCIL8 */
/* reuse GL_TEXTURE_STENCIL_SIZE */
/* reuse GL_UNSIGNED_NORMALIZED */
/* reuse GL_FRAMEBUFFER_BINDING */
/* reuse GL_DRAW_FRAMEBUFFER_BINDING */
/* reuse GL_RENDERBUFFER_BINDING */
/* reuse GL_READ_FRAMEBUFFER */
/* reuse GL_DRAW_FRAMEBUFFER */
/* reuse GL_READ_FRAMEBUFFER_BINDING */
/* reuse GL_RENDERBUFFER_SAMPLES */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER */
/* reuse GL_FRAMEBUFFER_COMPLETE */
/* reuse GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT */
/* reuse GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT */
/* reuse GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER */
/* reuse GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER */
/* reuse GL_FRAMEBUFFER_UNSUPPORTED */
/* reuse GL_MAX_COLOR_ATTACHMENTS */
/* reuse GL_COLOR_ATTACHMENT0 */
/* reuse GL_COLOR_ATTACHMENT1 */
/* reuse GL_COLOR_ATTACHMENT2 */
/* reuse GL_COLOR_ATTACHMENT3 */
/* reuse GL_COLOR_ATTACHMENT4 */
/* reuse GL_COLOR_ATTACHMENT5 */
/* reuse GL_COLOR_ATTACHMENT6 */
/* reuse GL_COLOR_ATTACHMENT7 */
/* reuse GL_COLOR_ATTACHMENT8 */
/* reuse GL_COLOR_ATTACHMENT9 */
/* reuse GL_COLOR_ATTACHMENT10 */
/* reuse GL_COLOR_ATTACHMENT11 */
/* reuse GL_COLOR_ATTACHMENT12 */
/* reuse GL_COLOR_ATTACHMENT13 */
/* reuse GL_COLOR_ATTACHMENT14 */
/* reuse GL_COLOR_ATTACHMENT15 */
/* reuse GL_DEPTH_ATTACHMENT */
/* reuse GL_STENCIL_ATTACHMENT */
/* reuse GL_FRAMEBUFFER */
/* reuse GL_RENDERBUFFER */
/* reuse GL_RENDERBUFFER_WIDTH */
/* reuse GL_RENDERBUFFER_HEIGHT */
/* reuse GL_RENDERBUFFER_INTERNAL_FORMAT */
/* reuse GL_STENCIL_INDEX1 */
/* reuse GL_STENCIL_INDEX4 */
/* reuse GL_STENCIL_INDEX8 */
/* reuse GL_STENCIL_INDEX16 */
/* reuse GL_RENDERBUFFER_RED_SIZE */
/* reuse GL_RENDERBUFFER_GREEN_SIZE */
/* reuse GL_RENDERBUFFER_BLUE_SIZE */
/* reuse GL_RENDERBUFFER_ALPHA_SIZE */
/* reuse GL_RENDERBUFFER_DEPTH_SIZE */
/* reuse GL_RENDERBUFFER_STENCIL_SIZE */
/* reuse GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE */
/* reuse GL_MAX_SAMPLES */
#define GL_INDEX 0x8222
#endif
/* reuse GLboolean (APIENTRYP gglIsRenderbuffer)(GLuint renderbuffer) */
/* reuse void (APIENTRYP gglBindRenderbuffer)(GLenum target, GLuint renderbuffer) */
/* reuse void (APIENTRYP gglDeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers) */
/* reuse void (APIENTRYP gglGenRenderbuffers)(GLsizei n, GLuint *renderbuffers) */
/* reuse void (APIENTRYP gglRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) */
/* reuse void (APIENTRYP gglGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint *params) */
/* reuse GLboolean (APIENTRYP gglIsFramebuffer)(GLuint framebuffer) */
/* reuse void (APIENTRYP gglBindFramebuffer)(GLenum target, GLuint framebuffer) */
/* reuse void (APIENTRYP gglDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers) */
/* reuse void (APIENTRYP gglGenFramebuffers)(GLsizei n, GLuint *framebuffers) */
/* reuse GLenum (APIENTRYP gglCheckFramebufferStatus)(GLenum target) */
/* reuse void (APIENTRYP gglFramebufferTexture1D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) */
/* reuse void (APIENTRYP gglFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) */
/* reuse void (APIENTRYP gglFramebufferTexture3D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) */
/* reuse void (APIENTRYP gglFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) */
/* reuse void (APIENTRYP gglGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint *params) */
/* reuse void (APIENTRYP gglGenerateMipmap)(GLenum target) */
/* reuse void (APIENTRYP gglBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) */
/* reuse void (APIENTRYP gglRenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) */
/* reuse void (APIENTRYP gglFramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) */

#ifndef GL_ARB_framebuffer_sRGB
#define GL_ARB_framebuffer_sRGB
/* reuse GL_FRAMEBUFFER_SRGB */
#endif

#ifndef GL_ARB_geometry_shader4
#define GL_ARB_geometry_shader4
#define GL_LINES_ADJACENCY_ARB 0x000A
#define GL_LINE_STRIP_ADJACENCY_ARB 0x000B
#define GL_TRIANGLES_ADJACENCY_ARB 0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY_ARB 0x000D
#define GL_PROGRAM_POINT_SIZE_ARB 0x8642
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_ARB 0x8C29
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED_ARB 0x8DA7
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_ARB 0x8DA8
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_ARB 0x8DA9
#define GL_GEOMETRY_SHADER_ARB 0x8DD9
#define GL_GEOMETRY_VERTICES_OUT_ARB 0x8DDA
#define GL_GEOMETRY_INPUT_TYPE_ARB 0x8DDB
#define GL_GEOMETRY_OUTPUT_TYPE_ARB 0x8DDC
#define GL_MAX_GEOMETRY_VARYING_COMPONENTS_ARB 0x8DDD
#define GL_MAX_VERTEX_VARYING_COMPONENTS_ARB 0x8DDE
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_ARB 0x8DDF
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES_ARB 0x8DE0
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_ARB 0x8DE1
/* reuse GL_MAX_VARYING_COMPONENTS */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER */
#endif
extern void (APIENTRYP gglProgramParameteriARB)(GLuint program, GLenum pname, GLint value);
extern void (APIENTRYP gglFramebufferTextureARB)(GLenum target, GLenum attachment, GLuint texture, GLint level);
extern void (APIENTRYP gglFramebufferTextureLayerARB)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
extern void (APIENTRYP gglFramebufferTextureFaceARB)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face);

#ifndef GL_ARB_get_program_binary
#define GL_ARB_get_program_binary
#define GL_PROGRAM_BINARY_RETRIEVABLE_HINT 0x8257
#define GL_PROGRAM_BINARY_LENGTH 0x8741
#define GL_NUM_PROGRAM_BINARY_FORMATS 0x87FE
#define GL_PROGRAM_BINARY_FORMATS 0x87FF
#endif
extern void (APIENTRYP gglGetProgramBinary)(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
extern void (APIENTRYP gglProgramBinary)(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
extern void (APIENTRYP gglProgramParameteri)(GLuint program, GLenum pname, GLint value);

#ifndef GL_ARB_get_texture_sub_image
#define GL_ARB_get_texture_sub_image
#endif
extern void (APIENTRYP gglGetTextureSubImage)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels);
extern void (APIENTRYP gglGetCompressedTextureSubImage)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels);

#ifndef GL_ARB_gpu_shader5
#define GL_ARB_gpu_shader5
#define GL_GEOMETRY_SHADER_INVOCATIONS 0x887F
#define GL_MAX_GEOMETRY_SHADER_INVOCATIONS 0x8E5A
#define GL_MIN_FRAGMENT_INTERPOLATION_OFFSET 0x8E5B
#define GL_MAX_FRAGMENT_INTERPOLATION_OFFSET 0x8E5C
#define GL_FRAGMENT_INTERPOLATION_OFFSET_BITS 0x8E5D
#define GL_MAX_VERTEX_STREAMS 0x8E71
#endif

#ifndef GL_ARB_gpu_shader_fp64
#define GL_ARB_gpu_shader_fp64
/* reuse GL_DOUBLE */
#define GL_DOUBLE_VEC2 0x8FFC
#define GL_DOUBLE_VEC3 0x8FFD
#define GL_DOUBLE_VEC4 0x8FFE
#define GL_DOUBLE_MAT2 0x8F46
#define GL_DOUBLE_MAT3 0x8F47
#define GL_DOUBLE_MAT4 0x8F48
#define GL_DOUBLE_MAT2x3 0x8F49
#define GL_DOUBLE_MAT2x4 0x8F4A
#define GL_DOUBLE_MAT3x2 0x8F4B
#define GL_DOUBLE_MAT3x4 0x8F4C
#define GL_DOUBLE_MAT4x2 0x8F4D
#define GL_DOUBLE_MAT4x3 0x8F4E
#endif
extern void (APIENTRYP gglUniform1d)(GLint location, GLdouble x);
extern void (APIENTRYP gglUniform2d)(GLint location, GLdouble x, GLdouble y);
extern void (APIENTRYP gglUniform3d)(GLint location, GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglUniform4d)(GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglUniform1dv)(GLint location, GLsizei count, const GLdouble *value);
extern void (APIENTRYP gglUniform2dv)(GLint location, GLsizei count, const GLdouble *value);
extern void (APIENTRYP gglUniform3dv)(GLint location, GLsizei count, const GLdouble *value);
extern void (APIENTRYP gglUniform4dv)(GLint location, GLsizei count, const GLdouble *value);
extern void (APIENTRYP gglUniformMatrix2dv)(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglUniformMatrix3dv)(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglUniformMatrix4dv)(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglUniformMatrix2x3dv)(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglUniformMatrix2x4dv)(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglUniformMatrix3x2dv)(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglUniformMatrix3x4dv)(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglUniformMatrix4x2dv)(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglUniformMatrix4x3dv)(GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglGetUniformdv)(GLuint program, GLint location, GLdouble *params);

#ifndef GL_ARB_gpu_shader_int64
#define GL_ARB_gpu_shader_int64
#define GL_INT64_ARB 0x140E
/* reuse GL_UNSIGNED_INT64_ARB */
#define GL_INT64_VEC2_ARB 0x8FE9
#define GL_INT64_VEC3_ARB 0x8FEA
#define GL_INT64_VEC4_ARB 0x8FEB
#define GL_UNSIGNED_INT64_VEC2_ARB 0x8FF5
#define GL_UNSIGNED_INT64_VEC3_ARB 0x8FF6
#define GL_UNSIGNED_INT64_VEC4_ARB 0x8FF7
#endif
extern void (APIENTRYP gglUniform1i64ARB)(GLint location, GLint64 x);
extern void (APIENTRYP gglUniform2i64ARB)(GLint location, GLint64 x, GLint64 y);
extern void (APIENTRYP gglUniform3i64ARB)(GLint location, GLint64 x, GLint64 y, GLint64 z);
extern void (APIENTRYP gglUniform4i64ARB)(GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w);
extern void (APIENTRYP gglUniform1i64vARB)(GLint location, GLsizei count, const GLint64 *value);
extern void (APIENTRYP gglUniform2i64vARB)(GLint location, GLsizei count, const GLint64 *value);
extern void (APIENTRYP gglUniform3i64vARB)(GLint location, GLsizei count, const GLint64 *value);
extern void (APIENTRYP gglUniform4i64vARB)(GLint location, GLsizei count, const GLint64 *value);
extern void (APIENTRYP gglUniform1ui64ARB)(GLint location, GLuint64 x);
extern void (APIENTRYP gglUniform2ui64ARB)(GLint location, GLuint64 x, GLuint64 y);
extern void (APIENTRYP gglUniform3ui64ARB)(GLint location, GLuint64 x, GLuint64 y, GLuint64 z);
extern void (APIENTRYP gglUniform4ui64ARB)(GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w);
extern void (APIENTRYP gglUniform1ui64vARB)(GLint location, GLsizei count, const GLuint64 *value);
extern void (APIENTRYP gglUniform2ui64vARB)(GLint location, GLsizei count, const GLuint64 *value);
extern void (APIENTRYP gglUniform3ui64vARB)(GLint location, GLsizei count, const GLuint64 *value);
extern void (APIENTRYP gglUniform4ui64vARB)(GLint location, GLsizei count, const GLuint64 *value);
extern void (APIENTRYP gglGetUniformi64vARB)(GLuint program, GLint location, GLint64 *params);
extern void (APIENTRYP gglGetUniformui64vARB)(GLuint program, GLint location, GLuint64 *params);
extern void (APIENTRYP gglGetnUniformi64vARB)(GLuint program, GLint location, GLsizei bufSize, GLint64 *params);
extern void (APIENTRYP gglGetnUniformui64vARB)(GLuint program, GLint location, GLsizei bufSize, GLuint64 *params);
extern void (APIENTRYP gglProgramUniform1i64ARB)(GLuint program, GLint location, GLint64 x);
extern void (APIENTRYP gglProgramUniform2i64ARB)(GLuint program, GLint location, GLint64 x, GLint64 y);
extern void (APIENTRYP gglProgramUniform3i64ARB)(GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z);
extern void (APIENTRYP gglProgramUniform4i64ARB)(GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w);
extern void (APIENTRYP gglProgramUniform1i64vARB)(GLuint program, GLint location, GLsizei count, const GLint64 *value);
extern void (APIENTRYP gglProgramUniform2i64vARB)(GLuint program, GLint location, GLsizei count, const GLint64 *value);
extern void (APIENTRYP gglProgramUniform3i64vARB)(GLuint program, GLint location, GLsizei count, const GLint64 *value);
extern void (APIENTRYP gglProgramUniform4i64vARB)(GLuint program, GLint location, GLsizei count, const GLint64 *value);
extern void (APIENTRYP gglProgramUniform1ui64ARB)(GLuint program, GLint location, GLuint64 x);
extern void (APIENTRYP gglProgramUniform2ui64ARB)(GLuint program, GLint location, GLuint64 x, GLuint64 y);
extern void (APIENTRYP gglProgramUniform3ui64ARB)(GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z);
extern void (APIENTRYP gglProgramUniform4ui64ARB)(GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w);
extern void (APIENTRYP gglProgramUniform1ui64vARB)(GLuint program, GLint location, GLsizei count, const GLuint64 *value);
extern void (APIENTRYP gglProgramUniform2ui64vARB)(GLuint program, GLint location, GLsizei count, const GLuint64 *value);
extern void (APIENTRYP gglProgramUniform3ui64vARB)(GLuint program, GLint location, GLsizei count, const GLuint64 *value);
extern void (APIENTRYP gglProgramUniform4ui64vARB)(GLuint program, GLint location, GLsizei count, const GLuint64 *value);

#ifndef GL_ARB_half_float_pixel
#define GL_ARB_half_float_pixel
#define GL_HALF_FLOAT_ARB 0x140B
#endif

#ifndef GL_ARB_half_float_vertex
#define GL_ARB_half_float_vertex
/* reuse GL_HALF_FLOAT */
#endif

#ifndef GL_ARB_imaging
#define GL_ARB_imaging
/* reuse GL_CONSTANT_COLOR */
/* reuse GL_ONE_MINUS_CONSTANT_COLOR */
/* reuse GL_CONSTANT_ALPHA */
/* reuse GL_ONE_MINUS_CONSTANT_ALPHA */
#define GL_BLEND_COLOR 0x8005
/* reuse GL_FUNC_ADD */
/* reuse GL_MIN */
/* reuse GL_MAX */
#define GL_BLEND_EQUATION 0x8009
/* reuse GL_FUNC_SUBTRACT */
/* reuse GL_FUNC_REVERSE_SUBTRACT */
#define GL_CONVOLUTION_1D 0x8010
#define GL_CONVOLUTION_2D 0x8011
#define GL_SEPARABLE_2D 0x8012
#define GL_CONVOLUTION_BORDER_MODE 0x8013
#define GL_CONVOLUTION_FILTER_SCALE 0x8014
#define GL_CONVOLUTION_FILTER_BIAS 0x8015
#define GL_REDUCE 0x8016
#define GL_CONVOLUTION_FORMAT 0x8017
#define GL_CONVOLUTION_WIDTH 0x8018
#define GL_CONVOLUTION_HEIGHT 0x8019
#define GL_MAX_CONVOLUTION_WIDTH 0x801A
#define GL_MAX_CONVOLUTION_HEIGHT 0x801B
#define GL_POST_CONVOLUTION_RED_SCALE 0x801C
#define GL_POST_CONVOLUTION_GREEN_SCALE 0x801D
#define GL_POST_CONVOLUTION_BLUE_SCALE 0x801E
#define GL_POST_CONVOLUTION_ALPHA_SCALE 0x801F
#define GL_POST_CONVOLUTION_RED_BIAS 0x8020
#define GL_POST_CONVOLUTION_GREEN_BIAS 0x8021
#define GL_POST_CONVOLUTION_BLUE_BIAS 0x8022
#define GL_POST_CONVOLUTION_ALPHA_BIAS 0x8023
#define GL_HISTOGRAM 0x8024
#define GL_PROXY_HISTOGRAM 0x8025
#define GL_HISTOGRAM_WIDTH 0x8026
#define GL_HISTOGRAM_FORMAT 0x8027
#define GL_HISTOGRAM_RED_SIZE 0x8028
#define GL_HISTOGRAM_GREEN_SIZE 0x8029
#define GL_HISTOGRAM_BLUE_SIZE 0x802A
#define GL_HISTOGRAM_ALPHA_SIZE 0x802B
#define GL_HISTOGRAM_LUMINANCE_SIZE 0x802C
#define GL_HISTOGRAM_SINK 0x802D
#define GL_MINMAX 0x802E
#define GL_MINMAX_FORMAT 0x802F
#define GL_MINMAX_SINK 0x8030
#define GL_TABLE_TOO_LARGE 0x8031
#define GL_COLOR_MATRIX 0x80B1
#define GL_COLOR_MATRIX_STACK_DEPTH 0x80B2
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH 0x80B3
#define GL_POST_COLOR_MATRIX_RED_SCALE 0x80B4
#define GL_POST_COLOR_MATRIX_GREEN_SCALE 0x80B5
#define GL_POST_COLOR_MATRIX_BLUE_SCALE 0x80B6
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE 0x80B7
#define GL_POST_COLOR_MATRIX_RED_BIAS 0x80B8
#define GL_POST_COLOR_MATRIX_GREEN_BIAS 0x80B9
#define GL_POST_COLOR_MATRIX_BLUE_BIAS 0x80BA
#define GL_POST_COLOR_MATRIX_ALPHA_BIAS 0x80BB
#define GL_COLOR_TABLE 0x80D0
#define GL_POST_CONVOLUTION_COLOR_TABLE 0x80D1
#define GL_POST_COLOR_MATRIX_COLOR_TABLE 0x80D2
#define GL_PROXY_COLOR_TABLE 0x80D3
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE 0x80D4
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE 0x80D5
#define GL_COLOR_TABLE_SCALE 0x80D6
#define GL_COLOR_TABLE_BIAS 0x80D7
#define GL_COLOR_TABLE_FORMAT 0x80D8
#define GL_COLOR_TABLE_WIDTH 0x80D9
#define GL_COLOR_TABLE_RED_SIZE 0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE 0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE 0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE 0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE 0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE 0x80DF
#define GL_CONSTANT_BORDER 0x8151
#define GL_REPLICATE_BORDER 0x8153
#define GL_CONVOLUTION_BORDER_COLOR 0x8154
#endif
/* reuse void (APIENTRYP gglBlendColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) */
/* reuse void (APIENTRYP gglBlendEquation)(GLenum mode) */
extern void (APIENTRYP gglColorTable)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table);
extern void (APIENTRYP gglColorTableParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglColorTableParameteriv)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglCopyColorTable)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
extern void (APIENTRYP gglGetColorTable)(GLenum target, GLenum format, GLenum type, void *table);
extern void (APIENTRYP gglGetColorTableParameterfv)(GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetColorTableParameteriv)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglColorSubTable)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data);
extern void (APIENTRYP gglCopyColorSubTable)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
extern void (APIENTRYP gglConvolutionFilter1D)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image);
extern void (APIENTRYP gglConvolutionFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image);
extern void (APIENTRYP gglConvolutionParameterf)(GLenum target, GLenum pname, GLfloat params);
extern void (APIENTRYP gglConvolutionParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglConvolutionParameteri)(GLenum target, GLenum pname, GLint params);
extern void (APIENTRYP gglConvolutionParameteriv)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglCopyConvolutionFilter1D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
extern void (APIENTRYP gglCopyConvolutionFilter2D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglGetConvolutionFilter)(GLenum target, GLenum format, GLenum type, void *image);
extern void (APIENTRYP gglGetConvolutionParameterfv)(GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetConvolutionParameteriv)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetSeparableFilter)(GLenum target, GLenum format, GLenum type, void *row, void *column, void *span);
extern void (APIENTRYP gglSeparableFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column);
extern void (APIENTRYP gglGetHistogram)(GLenum target, GLboolean reset, GLenum format, GLenum type, void *values);
extern void (APIENTRYP gglGetHistogramParameterfv)(GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetHistogramParameteriv)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetMinmax)(GLenum target, GLboolean reset, GLenum format, GLenum type, void *values);
extern void (APIENTRYP gglGetMinmaxParameterfv)(GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetMinmaxParameteriv)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglHistogram)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
extern void (APIENTRYP gglMinmax)(GLenum target, GLenum internalformat, GLboolean sink);
extern void (APIENTRYP gglResetHistogram)(GLenum target);
extern void (APIENTRYP gglResetMinmax)(GLenum target);

#ifndef GL_ARB_indirect_parameters
#define GL_ARB_indirect_parameters
#define GL_PARAMETER_BUFFER_ARB 0x80EE
#define GL_PARAMETER_BUFFER_BINDING_ARB 0x80EF
#endif
extern void (APIENTRYP gglMultiDrawArraysIndirectCountARB)(GLenum mode, GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride);
extern void (APIENTRYP gglMultiDrawElementsIndirectCountARB)(GLenum mode, GLenum type, GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride);

#ifndef GL_ARB_instanced_arrays
#define GL_ARB_instanced_arrays
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ARB 0x88FE
#endif
extern void (APIENTRYP gglVertexAttribDivisorARB)(GLuint index, GLuint divisor);

#ifndef GL_ARB_internalformat_query
#define GL_ARB_internalformat_query
#define GL_NUM_SAMPLE_COUNTS 0x9380
#endif
extern void (APIENTRYP gglGetInternalformativ)(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params);

#ifndef GL_ARB_internalformat_query2
#define GL_ARB_internalformat_query2
#define GL_IMAGE_FORMAT_COMPATIBILITY_TYPE 0x90C7
/* reuse GL_NUM_SAMPLE_COUNTS */
/* reuse GL_RENDERBUFFER */
/* reuse GL_SAMPLES */
/* reuse GL_TEXTURE_1D */
/* reuse GL_TEXTURE_1D_ARRAY */
/* reuse GL_TEXTURE_2D */
/* reuse GL_TEXTURE_2D_ARRAY */
/* reuse GL_TEXTURE_3D */
/* reuse GL_TEXTURE_CUBE_MAP */
#define GL_TEXTURE_CUBE_MAP_ARRAY 0x9009
/* reuse GL_TEXTURE_RECTANGLE */
/* reuse GL_TEXTURE_BUFFER */
/* reuse GL_TEXTURE_2D_MULTISAMPLE */
/* reuse GL_TEXTURE_2D_MULTISAMPLE_ARRAY */
/* reuse GL_TEXTURE_COMPRESSED */
#define GL_INTERNALFORMAT_SUPPORTED 0x826F
#define GL_INTERNALFORMAT_PREFERRED 0x8270
#define GL_INTERNALFORMAT_RED_SIZE 0x8271
#define GL_INTERNALFORMAT_GREEN_SIZE 0x8272
#define GL_INTERNALFORMAT_BLUE_SIZE 0x8273
#define GL_INTERNALFORMAT_ALPHA_SIZE 0x8274
#define GL_INTERNALFORMAT_DEPTH_SIZE 0x8275
#define GL_INTERNALFORMAT_STENCIL_SIZE 0x8276
#define GL_INTERNALFORMAT_SHARED_SIZE 0x8277
#define GL_INTERNALFORMAT_RED_TYPE 0x8278
#define GL_INTERNALFORMAT_GREEN_TYPE 0x8279
#define GL_INTERNALFORMAT_BLUE_TYPE 0x827A
#define GL_INTERNALFORMAT_ALPHA_TYPE 0x827B
#define GL_INTERNALFORMAT_DEPTH_TYPE 0x827C
#define GL_INTERNALFORMAT_STENCIL_TYPE 0x827D
#define GL_MAX_WIDTH 0x827E
#define GL_MAX_HEIGHT 0x827F
#define GL_MAX_DEPTH 0x8280
#define GL_MAX_LAYERS 0x8281
#define GL_MAX_COMBINED_DIMENSIONS 0x8282
#define GL_COLOR_COMPONENTS 0x8283
#define GL_DEPTH_COMPONENTS 0x8284
#define GL_STENCIL_COMPONENTS 0x8285
#define GL_COLOR_RENDERABLE 0x8286
#define GL_DEPTH_RENDERABLE 0x8287
#define GL_STENCIL_RENDERABLE 0x8288
#define GL_FRAMEBUFFER_RENDERABLE 0x8289
#define GL_FRAMEBUFFER_RENDERABLE_LAYERED 0x828A
#define GL_FRAMEBUFFER_BLEND 0x828B
#define GL_READ_PIXELS 0x828C
#define GL_READ_PIXELS_FORMAT 0x828D
#define GL_READ_PIXELS_TYPE 0x828E
#define GL_TEXTURE_IMAGE_FORMAT 0x828F
#define GL_TEXTURE_IMAGE_TYPE 0x8290
#define GL_GET_TEXTURE_IMAGE_FORMAT 0x8291
#define GL_GET_TEXTURE_IMAGE_TYPE 0x8292
#define GL_MIPMAP 0x8293
#define GL_MANUAL_GENERATE_MIPMAP 0x8294
#define GL_AUTO_GENERATE_MIPMAP 0x8295
#define GL_COLOR_ENCODING 0x8296
#define GL_SRGB_READ 0x8297
#define GL_SRGB_WRITE 0x8298
#define GL_SRGB_DECODE_ARB 0x8299
#define GL_FILTER 0x829A
#define GL_VERTEX_TEXTURE 0x829B
#define GL_TESS_CONTROL_TEXTURE 0x829C
#define GL_TESS_EVALUATION_TEXTURE 0x829D
#define GL_GEOMETRY_TEXTURE 0x829E
#define GL_FRAGMENT_TEXTURE 0x829F
#define GL_COMPUTE_TEXTURE 0x82A0
#define GL_TEXTURE_SHADOW 0x82A1
#define GL_TEXTURE_GATHER 0x82A2
#define GL_TEXTURE_GATHER_SHADOW 0x82A3
#define GL_SHADER_IMAGE_LOAD 0x82A4
#define GL_SHADER_IMAGE_STORE 0x82A5
#define GL_SHADER_IMAGE_ATOMIC 0x82A6
#define GL_IMAGE_TEXEL_SIZE 0x82A7
#define GL_IMAGE_COMPATIBILITY_CLASS 0x82A8
#define GL_IMAGE_PIXEL_FORMAT 0x82A9
#define GL_IMAGE_PIXEL_TYPE 0x82AA
#define GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_TEST 0x82AC
#define GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_TEST 0x82AD
#define GL_SIMULTANEOUS_TEXTURE_AND_DEPTH_WRITE 0x82AE
#define GL_SIMULTANEOUS_TEXTURE_AND_STENCIL_WRITE 0x82AF
#define GL_TEXTURE_COMPRESSED_BLOCK_WIDTH 0x82B1
#define GL_TEXTURE_COMPRESSED_BLOCK_HEIGHT 0x82B2
#define GL_TEXTURE_COMPRESSED_BLOCK_SIZE 0x82B3
#define GL_CLEAR_BUFFER 0x82B4
#define GL_TEXTURE_VIEW 0x82B5
#define GL_VIEW_COMPATIBILITY_CLASS 0x82B6
#define GL_FULL_SUPPORT 0x82B7
#define GL_CAVEAT_SUPPORT 0x82B8
#define GL_IMAGE_CLASS_4_X_32 0x82B9
#define GL_IMAGE_CLASS_2_X_32 0x82BA
#define GL_IMAGE_CLASS_1_X_32 0x82BB
#define GL_IMAGE_CLASS_4_X_16 0x82BC
#define GL_IMAGE_CLASS_2_X_16 0x82BD
#define GL_IMAGE_CLASS_1_X_16 0x82BE
#define GL_IMAGE_CLASS_4_X_8 0x82BF
#define GL_IMAGE_CLASS_2_X_8 0x82C0
#define GL_IMAGE_CLASS_1_X_8 0x82C1
#define GL_IMAGE_CLASS_11_11_10 0x82C2
#define GL_IMAGE_CLASS_10_10_10_2 0x82C3
#define GL_VIEW_CLASS_128_BITS 0x82C4
#define GL_VIEW_CLASS_96_BITS 0x82C5
#define GL_VIEW_CLASS_64_BITS 0x82C6
#define GL_VIEW_CLASS_48_BITS 0x82C7
#define GL_VIEW_CLASS_32_BITS 0x82C8
#define GL_VIEW_CLASS_24_BITS 0x82C9
#define GL_VIEW_CLASS_16_BITS 0x82CA
#define GL_VIEW_CLASS_8_BITS 0x82CB
#define GL_VIEW_CLASS_S3TC_DXT1_RGB 0x82CC
#define GL_VIEW_CLASS_S3TC_DXT1_RGBA 0x82CD
#define GL_VIEW_CLASS_S3TC_DXT3_RGBA 0x82CE
#define GL_VIEW_CLASS_S3TC_DXT5_RGBA 0x82CF
#define GL_VIEW_CLASS_RGTC1_RED 0x82D0
#define GL_VIEW_CLASS_RGTC2_RG 0x82D1
#define GL_VIEW_CLASS_BPTC_UNORM 0x82D2
#define GL_VIEW_CLASS_BPTC_FLOAT 0x82D3
#endif
extern void (APIENTRYP gglGetInternalformati64v)(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint64 *params);

#ifndef GL_ARB_invalidate_subdata
#define GL_ARB_invalidate_subdata
#endif
extern void (APIENTRYP gglInvalidateTexSubImage)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth);
extern void (APIENTRYP gglInvalidateTexImage)(GLuint texture, GLint level);
extern void (APIENTRYP gglInvalidateBufferSubData)(GLuint buffer, GLintptr offset, GLsizeiptr length);
extern void (APIENTRYP gglInvalidateBufferData)(GLuint buffer);
extern void (APIENTRYP gglInvalidateFramebuffer)(GLenum target, GLsizei numAttachments, const GLenum *attachments);
extern void (APIENTRYP gglInvalidateSubFramebuffer)(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height);

#ifndef GL_ARB_map_buffer_alignment
#define GL_ARB_map_buffer_alignment
#define GL_MIN_MAP_BUFFER_ALIGNMENT 0x90BC
#endif

#ifndef GL_ARB_map_buffer_range
#define GL_ARB_map_buffer_range
/* reuse GL_MAP_READ_BIT */
/* reuse GL_MAP_WRITE_BIT */
/* reuse GL_MAP_INVALIDATE_RANGE_BIT */
/* reuse GL_MAP_INVALIDATE_BUFFER_BIT */
/* reuse GL_MAP_FLUSH_EXPLICIT_BIT */
/* reuse GL_MAP_UNSYNCHRONIZED_BIT */
#endif
/* reuse void * (APIENTRYP gglMapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) */
/* reuse void (APIENTRYP gglFlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length) */

#ifndef GL_ARB_matrix_palette
#define GL_ARB_matrix_palette
#define GL_MATRIX_PALETTE_ARB 0x8840
#define GL_MAX_MATRIX_PALETTE_STACK_DEPTH_ARB 0x8841
#define GL_MAX_PALETTE_MATRICES_ARB 0x8842
#define GL_CURRENT_PALETTE_MATRIX_ARB 0x8843
#define GL_MATRIX_INDEX_ARRAY_ARB 0x8844
#define GL_CURRENT_MATRIX_INDEX_ARB 0x8845
#define GL_MATRIX_INDEX_ARRAY_SIZE_ARB 0x8846
#define GL_MATRIX_INDEX_ARRAY_TYPE_ARB 0x8847
#define GL_MATRIX_INDEX_ARRAY_STRIDE_ARB 0x8848
#define GL_MATRIX_INDEX_ARRAY_POINTER_ARB 0x8849
#endif
extern void (APIENTRYP gglCurrentPaletteMatrixARB)(GLint index);
extern void (APIENTRYP gglMatrixIndexubvARB)(GLint size, const GLubyte *indices);
extern void (APIENTRYP gglMatrixIndexusvARB)(GLint size, const GLushort *indices);
extern void (APIENTRYP gglMatrixIndexuivARB)(GLint size, const GLuint *indices);
extern void (APIENTRYP gglMatrixIndexPointerARB)(GLint size, GLenum type, GLsizei stride, const void *pointer);

#ifndef GL_ARB_multi_bind
#define GL_ARB_multi_bind
#endif
extern void (APIENTRYP gglBindBuffersBase)(GLenum target, GLuint first, GLsizei count, const GLuint *buffers);
extern void (APIENTRYP gglBindBuffersRange)(GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes);
extern void (APIENTRYP gglBindTextures)(GLuint first, GLsizei count, const GLuint *textures);
extern void (APIENTRYP gglBindSamplers)(GLuint first, GLsizei count, const GLuint *samplers);
extern void (APIENTRYP gglBindImageTextures)(GLuint first, GLsizei count, const GLuint *textures);
extern void (APIENTRYP gglBindVertexBuffers)(GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides);

#ifndef GL_ARB_multi_draw_indirect
#define GL_ARB_multi_draw_indirect
#endif
extern void (APIENTRYP gglMultiDrawArraysIndirect)(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride);
extern void (APIENTRYP gglMultiDrawElementsIndirect)(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride);

#ifndef GL_ARB_multisample
#define GL_ARB_multisample
#define GL_MULTISAMPLE_ARB 0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE_ARB 0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_ARB 0x809F
#define GL_SAMPLE_COVERAGE_ARB 0x80A0
#define GL_SAMPLE_BUFFERS_ARB 0x80A8
#define GL_SAMPLES_ARB 0x80A9
#define GL_SAMPLE_COVERAGE_VALUE_ARB 0x80AA
#define GL_SAMPLE_COVERAGE_INVERT_ARB 0x80AB
#define GL_MULTISAMPLE_BIT_ARB 0x20000000
#endif
extern void (APIENTRYP gglSampleCoverageARB)(GLfloat value, GLboolean invert);

#ifndef GL_ARB_multitexture
#define GL_ARB_multitexture
#define GL_TEXTURE0_ARB 0x84C0
#define GL_TEXTURE1_ARB 0x84C1
#define GL_TEXTURE2_ARB 0x84C2
#define GL_TEXTURE3_ARB 0x84C3
#define GL_TEXTURE4_ARB 0x84C4
#define GL_TEXTURE5_ARB 0x84C5
#define GL_TEXTURE6_ARB 0x84C6
#define GL_TEXTURE7_ARB 0x84C7
#define GL_TEXTURE8_ARB 0x84C8
#define GL_TEXTURE9_ARB 0x84C9
#define GL_TEXTURE10_ARB 0x84CA
#define GL_TEXTURE11_ARB 0x84CB
#define GL_TEXTURE12_ARB 0x84CC
#define GL_TEXTURE13_ARB 0x84CD
#define GL_TEXTURE14_ARB 0x84CE
#define GL_TEXTURE15_ARB 0x84CF
#define GL_TEXTURE16_ARB 0x84D0
#define GL_TEXTURE17_ARB 0x84D1
#define GL_TEXTURE18_ARB 0x84D2
#define GL_TEXTURE19_ARB 0x84D3
#define GL_TEXTURE20_ARB 0x84D4
#define GL_TEXTURE21_ARB 0x84D5
#define GL_TEXTURE22_ARB 0x84D6
#define GL_TEXTURE23_ARB 0x84D7
#define GL_TEXTURE24_ARB 0x84D8
#define GL_TEXTURE25_ARB 0x84D9
#define GL_TEXTURE26_ARB 0x84DA
#define GL_TEXTURE27_ARB 0x84DB
#define GL_TEXTURE28_ARB 0x84DC
#define GL_TEXTURE29_ARB 0x84DD
#define GL_TEXTURE30_ARB 0x84DE
#define GL_TEXTURE31_ARB 0x84DF
#define GL_ACTIVE_TEXTURE_ARB 0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB 0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB 0x84E2
#endif
extern void (APIENTRYP gglActiveTextureARB)(GLenum texture);
extern void (APIENTRYP gglClientActiveTextureARB)(GLenum texture);
extern void (APIENTRYP gglMultiTexCoord1dARB)(GLenum target, GLdouble s);
extern void (APIENTRYP gglMultiTexCoord1dvARB)(GLenum target, const GLdouble *v);
extern void (APIENTRYP gglMultiTexCoord1fARB)(GLenum target, GLfloat s);
extern void (APIENTRYP gglMultiTexCoord1fvARB)(GLenum target, const GLfloat *v);
extern void (APIENTRYP gglMultiTexCoord1iARB)(GLenum target, GLint s);
extern void (APIENTRYP gglMultiTexCoord1ivARB)(GLenum target, const GLint *v);
extern void (APIENTRYP gglMultiTexCoord1sARB)(GLenum target, GLshort s);
extern void (APIENTRYP gglMultiTexCoord1svARB)(GLenum target, const GLshort *v);
extern void (APIENTRYP gglMultiTexCoord2dARB)(GLenum target, GLdouble s, GLdouble t);
extern void (APIENTRYP gglMultiTexCoord2dvARB)(GLenum target, const GLdouble *v);
extern void (APIENTRYP gglMultiTexCoord2fARB)(GLenum target, GLfloat s, GLfloat t);
extern void (APIENTRYP gglMultiTexCoord2fvARB)(GLenum target, const GLfloat *v);
extern void (APIENTRYP gglMultiTexCoord2iARB)(GLenum target, GLint s, GLint t);
extern void (APIENTRYP gglMultiTexCoord2ivARB)(GLenum target, const GLint *v);
extern void (APIENTRYP gglMultiTexCoord2sARB)(GLenum target, GLshort s, GLshort t);
extern void (APIENTRYP gglMultiTexCoord2svARB)(GLenum target, const GLshort *v);
extern void (APIENTRYP gglMultiTexCoord3dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r);
extern void (APIENTRYP gglMultiTexCoord3dvARB)(GLenum target, const GLdouble *v);
extern void (APIENTRYP gglMultiTexCoord3fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r);
extern void (APIENTRYP gglMultiTexCoord3fvARB)(GLenum target, const GLfloat *v);
extern void (APIENTRYP gglMultiTexCoord3iARB)(GLenum target, GLint s, GLint t, GLint r);
extern void (APIENTRYP gglMultiTexCoord3ivARB)(GLenum target, const GLint *v);
extern void (APIENTRYP gglMultiTexCoord3sARB)(GLenum target, GLshort s, GLshort t, GLshort r);
extern void (APIENTRYP gglMultiTexCoord3svARB)(GLenum target, const GLshort *v);
extern void (APIENTRYP gglMultiTexCoord4dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
extern void (APIENTRYP gglMultiTexCoord4dvARB)(GLenum target, const GLdouble *v);
extern void (APIENTRYP gglMultiTexCoord4fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern void (APIENTRYP gglMultiTexCoord4fvARB)(GLenum target, const GLfloat *v);
extern void (APIENTRYP gglMultiTexCoord4iARB)(GLenum target, GLint s, GLint t, GLint r, GLint q);
extern void (APIENTRYP gglMultiTexCoord4ivARB)(GLenum target, const GLint *v);
extern void (APIENTRYP gglMultiTexCoord4sARB)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
extern void (APIENTRYP gglMultiTexCoord4svARB)(GLenum target, const GLshort *v);

#ifndef GL_ARB_occlusion_query
#define GL_ARB_occlusion_query
#define GL_QUERY_COUNTER_BITS_ARB 0x8864
#define GL_CURRENT_QUERY_ARB 0x8865
#define GL_QUERY_RESULT_ARB 0x8866
#define GL_QUERY_RESULT_AVAILABLE_ARB 0x8867
#define GL_SAMPLES_PASSED_ARB 0x8914
#endif
extern void (APIENTRYP gglGenQueriesARB)(GLsizei n, GLuint *ids);
extern void (APIENTRYP gglDeleteQueriesARB)(GLsizei n, const GLuint *ids);
extern GLboolean (APIENTRYP gglIsQueryARB)(GLuint id);
extern void (APIENTRYP gglBeginQueryARB)(GLenum target, GLuint id);
extern void (APIENTRYP gglEndQueryARB)(GLenum target);
extern void (APIENTRYP gglGetQueryivARB)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetQueryObjectivARB)(GLuint id, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetQueryObjectuivARB)(GLuint id, GLenum pname, GLuint *params);

#ifndef GL_ARB_occlusion_query2
#define GL_ARB_occlusion_query2
#define GL_ANY_SAMPLES_PASSED 0x8C2F
#endif

#ifndef GL_ARB_parallel_shader_compile
#define GL_ARB_parallel_shader_compile
#define GL_MAX_SHADER_COMPILER_THREADS_ARB 0x91B0
#define GL_COMPLETION_STATUS_ARB 0x91B1
#endif
extern void (APIENTRYP gglMaxShaderCompilerThreadsARB)(GLuint count);

#ifndef GL_ARB_pipeline_statistics_query
#define GL_ARB_pipeline_statistics_query
#define GL_VERTICES_SUBMITTED_ARB 0x82EE
#define GL_PRIMITIVES_SUBMITTED_ARB 0x82EF
#define GL_VERTEX_SHADER_INVOCATIONS_ARB 0x82F0
#define GL_TESS_CONTROL_SHADER_PATCHES_ARB 0x82F1
#define GL_TESS_EVALUATION_SHADER_INVOCATIONS_ARB 0x82F2
/* reuse GL_GEOMETRY_SHADER_INVOCATIONS */
#define GL_GEOMETRY_SHADER_PRIMITIVES_EMITTED_ARB 0x82F3
#define GL_FRAGMENT_SHADER_INVOCATIONS_ARB 0x82F4
#define GL_COMPUTE_SHADER_INVOCATIONS_ARB 0x82F5
#define GL_CLIPPING_INPUT_PRIMITIVES_ARB 0x82F6
#define GL_CLIPPING_OUTPUT_PRIMITIVES_ARB 0x82F7
#endif

#ifndef GL_ARB_pixel_buffer_object
#define GL_ARB_pixel_buffer_object
#define GL_PIXEL_PACK_BUFFER_ARB 0x88EB
#define GL_PIXEL_UNPACK_BUFFER_ARB 0x88EC
#define GL_PIXEL_PACK_BUFFER_BINDING_ARB 0x88ED
#define GL_PIXEL_UNPACK_BUFFER_BINDING_ARB 0x88EF
#endif

#ifndef GL_ARB_point_parameters
#define GL_ARB_point_parameters
#define GL_POINT_SIZE_MIN_ARB 0x8126
#define GL_POINT_SIZE_MAX_ARB 0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_ARB 0x8128
#define GL_POINT_DISTANCE_ATTENUATION_ARB 0x8129
#endif
extern void (APIENTRYP gglPointParameterfARB)(GLenum pname, GLfloat param);
extern void (APIENTRYP gglPointParameterfvARB)(GLenum pname, const GLfloat *params);

#ifndef GL_ARB_point_sprite
#define GL_ARB_point_sprite
#define GL_POINT_SPRITE_ARB 0x8861
#define GL_COORD_REPLACE_ARB 0x8862
#endif

#ifndef GL_ARB_post_depth_coverage
#define GL_ARB_post_depth_coverage
#endif

#ifndef GL_ARB_program_interface_query
#define GL_ARB_program_interface_query
#define GL_UNIFORM 0x92E1
#define GL_UNIFORM_BLOCK 0x92E2
#define GL_PROGRAM_INPUT 0x92E3
#define GL_PROGRAM_OUTPUT 0x92E4
#define GL_BUFFER_VARIABLE 0x92E5
#define GL_SHADER_STORAGE_BLOCK 0x92E6
#define GL_ATOMIC_COUNTER_BUFFER 0x92C0
#define GL_VERTEX_SUBROUTINE 0x92E8
#define GL_TESS_CONTROL_SUBROUTINE 0x92E9
#define GL_TESS_EVALUATION_SUBROUTINE 0x92EA
#define GL_GEOMETRY_SUBROUTINE 0x92EB
#define GL_FRAGMENT_SUBROUTINE 0x92EC
#define GL_COMPUTE_SUBROUTINE 0x92ED
#define GL_VERTEX_SUBROUTINE_UNIFORM 0x92EE
#define GL_TESS_CONTROL_SUBROUTINE_UNIFORM 0x92EF
#define GL_TESS_EVALUATION_SUBROUTINE_UNIFORM 0x92F0
#define GL_GEOMETRY_SUBROUTINE_UNIFORM 0x92F1
#define GL_FRAGMENT_SUBROUTINE_UNIFORM 0x92F2
#define GL_COMPUTE_SUBROUTINE_UNIFORM 0x92F3
#define GL_TRANSFORM_FEEDBACK_VARYING 0x92F4
#define GL_ACTIVE_RESOURCES 0x92F5
#define GL_MAX_NAME_LENGTH 0x92F6
#define GL_MAX_NUM_ACTIVE_VARIABLES 0x92F7
#define GL_MAX_NUM_COMPATIBLE_SUBROUTINES 0x92F8
#define GL_NAME_LENGTH 0x92F9
#define GL_TYPE 0x92FA
#define GL_ARRAY_SIZE 0x92FB
#define GL_OFFSET 0x92FC
#define GL_BLOCK_INDEX 0x92FD
#define GL_ARRAY_STRIDE 0x92FE
#define GL_MATRIX_STRIDE 0x92FF
#define GL_IS_ROW_MAJOR 0x9300
#define GL_ATOMIC_COUNTER_BUFFER_INDEX 0x9301
#define GL_BUFFER_BINDING 0x9302
#define GL_BUFFER_DATA_SIZE 0x9303
#define GL_NUM_ACTIVE_VARIABLES 0x9304
#define GL_ACTIVE_VARIABLES 0x9305
#define GL_REFERENCED_BY_VERTEX_SHADER 0x9306
#define GL_REFERENCED_BY_TESS_CONTROL_SHADER 0x9307
#define GL_REFERENCED_BY_TESS_EVALUATION_SHADER 0x9308
#define GL_REFERENCED_BY_GEOMETRY_SHADER 0x9309
#define GL_REFERENCED_BY_FRAGMENT_SHADER 0x930A
#define GL_REFERENCED_BY_COMPUTE_SHADER 0x930B
#define GL_TOP_LEVEL_ARRAY_SIZE 0x930C
#define GL_TOP_LEVEL_ARRAY_STRIDE 0x930D
#define GL_LOCATION 0x930E
#define GL_LOCATION_INDEX 0x930F
#define GL_IS_PER_PATCH 0x92E7
#define GL_NUM_COMPATIBLE_SUBROUTINES 0x8E4A
#define GL_COMPATIBLE_SUBROUTINES 0x8E4B
#endif
extern void (APIENTRYP gglGetProgramInterfaceiv)(GLuint program, GLenum programInterface, GLenum pname, GLint *params);
extern GLuint (APIENTRYP gglGetProgramResourceIndex)(GLuint program, GLenum programInterface, const GLchar *name);
extern void (APIENTRYP gglGetProgramResourceName)(GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name);
extern void (APIENTRYP gglGetProgramResourceiv)(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLint *params);
extern GLint (APIENTRYP gglGetProgramResourceLocation)(GLuint program, GLenum programInterface, const GLchar *name);
extern GLint (APIENTRYP gglGetProgramResourceLocationIndex)(GLuint program, GLenum programInterface, const GLchar *name);

#ifndef GL_ARB_provoking_vertex
#define GL_ARB_provoking_vertex
/* reuse GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION */
/* reuse GL_FIRST_VERTEX_CONVENTION */
/* reuse GL_LAST_VERTEX_CONVENTION */
/* reuse GL_PROVOKING_VERTEX */
#endif
/* reuse void (APIENTRYP gglProvokingVertex)(GLenum mode) */

#ifndef GL_ARB_query_buffer_object
#define GL_ARB_query_buffer_object
#define GL_QUERY_BUFFER 0x9192
#define GL_QUERY_BUFFER_BARRIER_BIT 0x00008000
#define GL_QUERY_BUFFER_BINDING 0x9193
#define GL_QUERY_RESULT_NO_WAIT 0x9194
#endif

#ifndef GL_ARB_robust_buffer_access_behavior
#define GL_ARB_robust_buffer_access_behavior
#endif

#ifndef GL_ARB_robustness
#define GL_ARB_robustness
/* reuse GL_NO_ERROR */
#define GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB 0x00000004
#define GL_LOSE_CONTEXT_ON_RESET_ARB 0x8252
#define GL_GUILTY_CONTEXT_RESET_ARB 0x8253
#define GL_INNOCENT_CONTEXT_RESET_ARB 0x8254
#define GL_UNKNOWN_CONTEXT_RESET_ARB 0x8255
#define GL_RESET_NOTIFICATION_STRATEGY_ARB 0x8256
#define GL_NO_RESET_NOTIFICATION_ARB 0x8261
#endif
extern GLenum (APIENTRYP gglGetGraphicsResetStatusARB)();
extern void (APIENTRYP gglGetnTexImageARB)(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *img);
extern void (APIENTRYP gglReadnPixelsARB)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data);
extern void (APIENTRYP gglGetnCompressedTexImageARB)(GLenum target, GLint lod, GLsizei bufSize, void *img);
extern void (APIENTRYP gglGetnUniformfvARB)(GLuint program, GLint location, GLsizei bufSize, GLfloat *params);
extern void (APIENTRYP gglGetnUniformivARB)(GLuint program, GLint location, GLsizei bufSize, GLint *params);
extern void (APIENTRYP gglGetnUniformuivARB)(GLuint program, GLint location, GLsizei bufSize, GLuint *params);
extern void (APIENTRYP gglGetnUniformdvARB)(GLuint program, GLint location, GLsizei bufSize, GLdouble *params);
extern void (APIENTRYP gglGetnMapdvARB)(GLenum target, GLenum query, GLsizei bufSize, GLdouble *v);
extern void (APIENTRYP gglGetnMapfvARB)(GLenum target, GLenum query, GLsizei bufSize, GLfloat *v);
extern void (APIENTRYP gglGetnMapivARB)(GLenum target, GLenum query, GLsizei bufSize, GLint *v);
extern void (APIENTRYP gglGetnPixelMapfvARB)(GLenum map, GLsizei bufSize, GLfloat *values);
extern void (APIENTRYP gglGetnPixelMapuivARB)(GLenum map, GLsizei bufSize, GLuint *values);
extern void (APIENTRYP gglGetnPixelMapusvARB)(GLenum map, GLsizei bufSize, GLushort *values);
extern void (APIENTRYP gglGetnPolygonStippleARB)(GLsizei bufSize, GLubyte *pattern);
extern void (APIENTRYP gglGetnColorTableARB)(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table);
extern void (APIENTRYP gglGetnConvolutionFilterARB)(GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image);
extern void (APIENTRYP gglGetnSeparableFilterARB)(GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span);
extern void (APIENTRYP gglGetnHistogramARB)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values);
extern void (APIENTRYP gglGetnMinmaxARB)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values);

#ifndef GL_ARB_robustness_isolation
#define GL_ARB_robustness_isolation
#endif

#ifndef GL_ARB_sample_locations
#define GL_ARB_sample_locations
#define GL_SAMPLE_LOCATION_SUBPIXEL_BITS_ARB 0x933D
#define GL_SAMPLE_LOCATION_PIXEL_GRID_WIDTH_ARB 0x933E
#define GL_SAMPLE_LOCATION_PIXEL_GRID_HEIGHT_ARB 0x933F
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_TABLE_SIZE_ARB 0x9340
#define GL_SAMPLE_LOCATION_ARB 0x8E50
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_ARB 0x9341
#define GL_FRAMEBUFFER_PROGRAMMABLE_SAMPLE_LOCATIONS_ARB 0x9342
#define GL_FRAMEBUFFER_SAMPLE_LOCATION_PIXEL_GRID_ARB 0x9343
#endif
extern void (APIENTRYP gglFramebufferSampleLocationsfvARB)(GLenum target, GLuint start, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglNamedFramebufferSampleLocationsfvARB)(GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglEvaluateDepthValuesARB)();

#ifndef GL_ARB_sample_shading
#define GL_ARB_sample_shading
#define GL_SAMPLE_SHADING_ARB 0x8C36
#define GL_MIN_SAMPLE_SHADING_VALUE_ARB 0x8C37
#endif
extern void (APIENTRYP gglMinSampleShadingARB)(GLfloat value);

#ifndef GL_ARB_sampler_objects
#define GL_ARB_sampler_objects
#define GL_SAMPLER_BINDING 0x8919
#endif
extern void (APIENTRYP gglGenSamplers)(GLsizei count, GLuint *samplers);
extern void (APIENTRYP gglDeleteSamplers)(GLsizei count, const GLuint *samplers);
extern GLboolean (APIENTRYP gglIsSampler)(GLuint sampler);
extern void (APIENTRYP gglBindSampler)(GLuint unit, GLuint sampler);
extern void (APIENTRYP gglSamplerParameteri)(GLuint sampler, GLenum pname, GLint param);
extern void (APIENTRYP gglSamplerParameteriv)(GLuint sampler, GLenum pname, const GLint *param);
extern void (APIENTRYP gglSamplerParameterf)(GLuint sampler, GLenum pname, GLfloat param);
extern void (APIENTRYP gglSamplerParameterfv)(GLuint sampler, GLenum pname, const GLfloat *param);
extern void (APIENTRYP gglSamplerParameterIiv)(GLuint sampler, GLenum pname, const GLint *param);
extern void (APIENTRYP gglSamplerParameterIuiv)(GLuint sampler, GLenum pname, const GLuint *param);
extern void (APIENTRYP gglGetSamplerParameteriv)(GLuint sampler, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetSamplerParameterIiv)(GLuint sampler, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetSamplerParameterfv)(GLuint sampler, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetSamplerParameterIuiv)(GLuint sampler, GLenum pname, GLuint *params);

#ifndef GL_ARB_seamless_cube_map
#define GL_ARB_seamless_cube_map
/* reuse GL_TEXTURE_CUBE_MAP_SEAMLESS */
#endif

#ifndef GL_ARB_seamless_cubemap_per_texture
#define GL_ARB_seamless_cubemap_per_texture
/* reuse GL_TEXTURE_CUBE_MAP_SEAMLESS */
#endif

#ifndef GL_ARB_separate_shader_objects
#define GL_ARB_separate_shader_objects
#define GL_VERTEX_SHADER_BIT 0x00000001
#define GL_FRAGMENT_SHADER_BIT 0x00000002
#define GL_GEOMETRY_SHADER_BIT 0x00000004
#define GL_TESS_CONTROL_SHADER_BIT 0x00000008
#define GL_TESS_EVALUATION_SHADER_BIT 0x00000010
#define GL_ALL_SHADER_BITS 0xFFFFFFFF
#define GL_PROGRAM_SEPARABLE 0x8258
#define GL_ACTIVE_PROGRAM 0x8259
#define GL_PROGRAM_PIPELINE_BINDING 0x825A
#endif
extern void (APIENTRYP gglUseProgramStages)(GLuint pipeline, GLbitfield stages, GLuint program);
extern void (APIENTRYP gglActiveShaderProgram)(GLuint pipeline, GLuint program);
extern GLuint (APIENTRYP gglCreateShaderProgramv)(GLenum type, GLsizei count, const GLchar *const*strings);
extern void (APIENTRYP gglBindProgramPipeline)(GLuint pipeline);
extern void (APIENTRYP gglDeleteProgramPipelines)(GLsizei n, const GLuint *pipelines);
extern void (APIENTRYP gglGenProgramPipelines)(GLsizei n, GLuint *pipelines);
extern GLboolean (APIENTRYP gglIsProgramPipeline)(GLuint pipeline);
extern void (APIENTRYP gglGetProgramPipelineiv)(GLuint pipeline, GLenum pname, GLint *params);
extern void (APIENTRYP gglProgramUniform1i)(GLuint program, GLint location, GLint v0);
extern void (APIENTRYP gglProgramUniform1iv)(GLuint program, GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglProgramUniform1f)(GLuint program, GLint location, GLfloat v0);
extern void (APIENTRYP gglProgramUniform1fv)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglProgramUniform1d)(GLuint program, GLint location, GLdouble v0);
extern void (APIENTRYP gglProgramUniform1dv)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
extern void (APIENTRYP gglProgramUniform1ui)(GLuint program, GLint location, GLuint v0);
extern void (APIENTRYP gglProgramUniform1uiv)(GLuint program, GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglProgramUniform2i)(GLuint program, GLint location, GLint v0, GLint v1);
extern void (APIENTRYP gglProgramUniform2iv)(GLuint program, GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglProgramUniform2f)(GLuint program, GLint location, GLfloat v0, GLfloat v1);
extern void (APIENTRYP gglProgramUniform2fv)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglProgramUniform2d)(GLuint program, GLint location, GLdouble v0, GLdouble v1);
extern void (APIENTRYP gglProgramUniform2dv)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
extern void (APIENTRYP gglProgramUniform2ui)(GLuint program, GLint location, GLuint v0, GLuint v1);
extern void (APIENTRYP gglProgramUniform2uiv)(GLuint program, GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglProgramUniform3i)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
extern void (APIENTRYP gglProgramUniform3iv)(GLuint program, GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglProgramUniform3f)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern void (APIENTRYP gglProgramUniform3fv)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglProgramUniform3d)(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2);
extern void (APIENTRYP gglProgramUniform3dv)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
extern void (APIENTRYP gglProgramUniform3ui)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2);
extern void (APIENTRYP gglProgramUniform3uiv)(GLuint program, GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglProgramUniform4i)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
extern void (APIENTRYP gglProgramUniform4iv)(GLuint program, GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglProgramUniform4f)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void (APIENTRYP gglProgramUniform4fv)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglProgramUniform4d)(GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3);
extern void (APIENTRYP gglProgramUniform4dv)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
extern void (APIENTRYP gglProgramUniform4ui)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
extern void (APIENTRYP gglProgramUniform4uiv)(GLuint program, GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglProgramUniformMatrix2fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix3fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix4fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix2dv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix3dv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix4dv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix2x3fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix3x2fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix2x4fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix4x2fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix3x4fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix4x3fv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix2x3dv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix3x2dv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix2x4dv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix4x2dv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix3x4dv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix4x3dv)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglValidateProgramPipeline)(GLuint pipeline);
extern void (APIENTRYP gglGetProgramPipelineInfoLog)(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog);

#ifndef GL_ARB_shader_atomic_counter_ops
#define GL_ARB_shader_atomic_counter_ops
#endif

#ifndef GL_ARB_shader_atomic_counters
#define GL_ARB_shader_atomic_counters
/* reuse GL_ATOMIC_COUNTER_BUFFER */
#define GL_ATOMIC_COUNTER_BUFFER_BINDING 0x92C1
#define GL_ATOMIC_COUNTER_BUFFER_START 0x92C2
#define GL_ATOMIC_COUNTER_BUFFER_SIZE 0x92C3
#define GL_ATOMIC_COUNTER_BUFFER_DATA_SIZE 0x92C4
#define GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTERS 0x92C5
#define GL_ATOMIC_COUNTER_BUFFER_ACTIVE_ATOMIC_COUNTER_INDICES 0x92C6
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_VERTEX_SHADER 0x92C7
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TESS_CONTROL_SHADER 0x92C8
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_TESS_EVALUATION_SHADER 0x92C9
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_GEOMETRY_SHADER 0x92CA
#define GL_ATOMIC_COUNTER_BUFFER_REFERENCED_BY_FRAGMENT_SHADER 0x92CB
#define GL_MAX_VERTEX_ATOMIC_COUNTER_BUFFERS 0x92CC
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS 0x92CD
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS 0x92CE
#define GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS 0x92CF
#define GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS 0x92D0
#define GL_MAX_COMBINED_ATOMIC_COUNTER_BUFFERS 0x92D1
#define GL_MAX_VERTEX_ATOMIC_COUNTERS 0x92D2
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS 0x92D3
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS 0x92D4
#define GL_MAX_GEOMETRY_ATOMIC_COUNTERS 0x92D5
#define GL_MAX_FRAGMENT_ATOMIC_COUNTERS 0x92D6
#define GL_MAX_COMBINED_ATOMIC_COUNTERS 0x92D7
#define GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE 0x92D8
#define GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS 0x92DC
#define GL_ACTIVE_ATOMIC_COUNTER_BUFFERS 0x92D9
#define GL_UNIFORM_ATOMIC_COUNTER_BUFFER_INDEX 0x92DA
#define GL_UNSIGNED_INT_ATOMIC_COUNTER 0x92DB
#endif
extern void (APIENTRYP gglGetActiveAtomicCounterBufferiv)(GLuint program, GLuint bufferIndex, GLenum pname, GLint *params);

#ifndef GL_ARB_shader_ballot
#define GL_ARB_shader_ballot
#endif

#ifndef GL_ARB_shader_bit_encoding
#define GL_ARB_shader_bit_encoding
#endif

#ifndef GL_ARB_shader_clock
#define GL_ARB_shader_clock
#endif

#ifndef GL_ARB_shader_draw_parameters
#define GL_ARB_shader_draw_parameters
#endif

#ifndef GL_ARB_shader_group_vote
#define GL_ARB_shader_group_vote
#endif

#ifndef GL_ARB_shader_image_load_store
#define GL_ARB_shader_image_load_store
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT 0x00000001
#define GL_ELEMENT_ARRAY_BARRIER_BIT 0x00000002
#define GL_UNIFORM_BARRIER_BIT 0x00000004
#define GL_TEXTURE_FETCH_BARRIER_BIT 0x00000008
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT 0x00000020
#define GL_COMMAND_BARRIER_BIT 0x00000040
#define GL_PIXEL_BUFFER_BARRIER_BIT 0x00000080
#define GL_TEXTURE_UPDATE_BARRIER_BIT 0x00000100
#define GL_BUFFER_UPDATE_BARRIER_BIT 0x00000200
#define GL_FRAMEBUFFER_BARRIER_BIT 0x00000400
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT 0x00000800
#define GL_ATOMIC_COUNTER_BARRIER_BIT 0x00001000
#define GL_ALL_BARRIER_BITS 0xFFFFFFFF
#define GL_MAX_IMAGE_UNITS 0x8F38
#define GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS 0x8F39
#define GL_IMAGE_BINDING_NAME 0x8F3A
#define GL_IMAGE_BINDING_LEVEL 0x8F3B
#define GL_IMAGE_BINDING_LAYERED 0x8F3C
#define GL_IMAGE_BINDING_LAYER 0x8F3D
#define GL_IMAGE_BINDING_ACCESS 0x8F3E
#define GL_IMAGE_1D 0x904C
#define GL_IMAGE_2D 0x904D
#define GL_IMAGE_3D 0x904E
#define GL_IMAGE_2D_RECT 0x904F
#define GL_IMAGE_CUBE 0x9050
#define GL_IMAGE_BUFFER 0x9051
#define GL_IMAGE_1D_ARRAY 0x9052
#define GL_IMAGE_2D_ARRAY 0x9053
#define GL_IMAGE_CUBE_MAP_ARRAY 0x9054
#define GL_IMAGE_2D_MULTISAMPLE 0x9055
#define GL_IMAGE_2D_MULTISAMPLE_ARRAY 0x9056
#define GL_INT_IMAGE_1D 0x9057
#define GL_INT_IMAGE_2D 0x9058
#define GL_INT_IMAGE_3D 0x9059
#define GL_INT_IMAGE_2D_RECT 0x905A
#define GL_INT_IMAGE_CUBE 0x905B
#define GL_INT_IMAGE_BUFFER 0x905C
#define GL_INT_IMAGE_1D_ARRAY 0x905D
#define GL_INT_IMAGE_2D_ARRAY 0x905E
#define GL_INT_IMAGE_CUBE_MAP_ARRAY 0x905F
#define GL_INT_IMAGE_2D_MULTISAMPLE 0x9060
#define GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY 0x9061
#define GL_UNSIGNED_INT_IMAGE_1D 0x9062
#define GL_UNSIGNED_INT_IMAGE_2D 0x9063
#define GL_UNSIGNED_INT_IMAGE_3D 0x9064
#define GL_UNSIGNED_INT_IMAGE_2D_RECT 0x9065
#define GL_UNSIGNED_INT_IMAGE_CUBE 0x9066
#define GL_UNSIGNED_INT_IMAGE_BUFFER 0x9067
#define GL_UNSIGNED_INT_IMAGE_1D_ARRAY 0x9068
#define GL_UNSIGNED_INT_IMAGE_2D_ARRAY 0x9069
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY 0x906A
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE 0x906B
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY 0x906C
#define GL_MAX_IMAGE_SAMPLES 0x906D
#define GL_IMAGE_BINDING_FORMAT 0x906E
/* reuse GL_IMAGE_FORMAT_COMPATIBILITY_TYPE */
#define GL_IMAGE_FORMAT_COMPATIBILITY_BY_SIZE 0x90C8
#define GL_IMAGE_FORMAT_COMPATIBILITY_BY_CLASS 0x90C9
#define GL_MAX_VERTEX_IMAGE_UNIFORMS 0x90CA
#define GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS 0x90CB
#define GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS 0x90CC
#define GL_MAX_GEOMETRY_IMAGE_UNIFORMS 0x90CD
#define GL_MAX_FRAGMENT_IMAGE_UNIFORMS 0x90CE
#define GL_MAX_COMBINED_IMAGE_UNIFORMS 0x90CF
#endif
extern void (APIENTRYP gglBindImageTexture)(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
extern void (APIENTRYP gglMemoryBarrier)(GLbitfield barriers);

#ifndef GL_ARB_shader_image_size
#define GL_ARB_shader_image_size
#endif

#ifndef GL_ARB_shader_objects
#define GL_ARB_shader_objects
#define GL_PROGRAM_OBJECT_ARB 0x8B40
#define GL_SHADER_OBJECT_ARB 0x8B48
#define GL_OBJECT_TYPE_ARB 0x8B4E
#define GL_OBJECT_SUBTYPE_ARB 0x8B4F
#define GL_FLOAT_VEC2_ARB 0x8B50
#define GL_FLOAT_VEC3_ARB 0x8B51
#define GL_FLOAT_VEC4_ARB 0x8B52
#define GL_INT_VEC2_ARB 0x8B53
#define GL_INT_VEC3_ARB 0x8B54
#define GL_INT_VEC4_ARB 0x8B55
#define GL_BOOL_ARB 0x8B56
#define GL_BOOL_VEC2_ARB 0x8B57
#define GL_BOOL_VEC3_ARB 0x8B58
#define GL_BOOL_VEC4_ARB 0x8B59
#define GL_FLOAT_MAT2_ARB 0x8B5A
#define GL_FLOAT_MAT3_ARB 0x8B5B
#define GL_FLOAT_MAT4_ARB 0x8B5C
#define GL_SAMPLER_1D_ARB 0x8B5D
#define GL_SAMPLER_2D_ARB 0x8B5E
#define GL_SAMPLER_3D_ARB 0x8B5F
#define GL_SAMPLER_CUBE_ARB 0x8B60
#define GL_SAMPLER_1D_SHADOW_ARB 0x8B61
#define GL_SAMPLER_2D_SHADOW_ARB 0x8B62
#define GL_SAMPLER_2D_RECT_ARB 0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW_ARB 0x8B64
#define GL_OBJECT_DELETE_STATUS_ARB 0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB 0x8B81
#define GL_OBJECT_LINK_STATUS_ARB 0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB 0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB 0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB 0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB 0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB 0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB 0x8B88
#endif
extern void (APIENTRYP gglDeleteObjectARB)(GLhandleARB obj);
extern GLhandleARB (APIENTRYP gglGetHandleARB)(GLenum pname);
extern void (APIENTRYP gglDetachObjectARB)(GLhandleARB containerObj, GLhandleARB attachedObj);
extern GLhandleARB (APIENTRYP gglCreateShaderObjectARB)(GLenum shaderType);
extern void (APIENTRYP gglShaderSourceARB)(GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length);
extern void (APIENTRYP gglCompileShaderARB)(GLhandleARB shaderObj);
extern GLhandleARB (APIENTRYP gglCreateProgramObjectARB)();
extern void (APIENTRYP gglAttachObjectARB)(GLhandleARB containerObj, GLhandleARB obj);
extern void (APIENTRYP gglLinkProgramARB)(GLhandleARB programObj);
extern void (APIENTRYP gglUseProgramObjectARB)(GLhandleARB programObj);
extern void (APIENTRYP gglValidateProgramARB)(GLhandleARB programObj);
extern void (APIENTRYP gglUniform1fARB)(GLint location, GLfloat v0);
extern void (APIENTRYP gglUniform2fARB)(GLint location, GLfloat v0, GLfloat v1);
extern void (APIENTRYP gglUniform3fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern void (APIENTRYP gglUniform4fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void (APIENTRYP gglUniform1iARB)(GLint location, GLint v0);
extern void (APIENTRYP gglUniform2iARB)(GLint location, GLint v0, GLint v1);
extern void (APIENTRYP gglUniform3iARB)(GLint location, GLint v0, GLint v1, GLint v2);
extern void (APIENTRYP gglUniform4iARB)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
extern void (APIENTRYP gglUniform1fvARB)(GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglUniform2fvARB)(GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglUniform3fvARB)(GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglUniform4fvARB)(GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglUniform1ivARB)(GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglUniform2ivARB)(GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglUniform3ivARB)(GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglUniform4ivARB)(GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglUniformMatrix2fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix3fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix4fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglGetObjectParameterfvARB)(GLhandleARB obj, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetObjectParameterivARB)(GLhandleARB obj, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetInfoLogARB)(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
extern void (APIENTRYP gglGetAttachedObjectsARB)(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
extern GLint (APIENTRYP gglGetUniformLocationARB)(GLhandleARB programObj, const GLcharARB *name);
extern void (APIENTRYP gglGetActiveUniformARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
extern void (APIENTRYP gglGetUniformfvARB)(GLhandleARB programObj, GLint location, GLfloat *params);
extern void (APIENTRYP gglGetUniformivARB)(GLhandleARB programObj, GLint location, GLint *params);
extern void (APIENTRYP gglGetShaderSourceARB)(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);

#ifndef GL_ARB_shader_precision
#define GL_ARB_shader_precision
#endif

#ifndef GL_ARB_shader_stencil_export
#define GL_ARB_shader_stencil_export
#endif

#ifndef GL_ARB_shader_storage_buffer_object
#define GL_ARB_shader_storage_buffer_object
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#define GL_SHADER_STORAGE_BUFFER_BINDING 0x90D3
#define GL_SHADER_STORAGE_BUFFER_START 0x90D4
#define GL_SHADER_STORAGE_BUFFER_SIZE 0x90D5
#define GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS 0x90D6
#define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS 0x90D7
#define GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS 0x90D8
#define GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS 0x90D9
#define GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS 0x90DA
#define GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS 0x90DB
#define GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS 0x90DC
#define GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS 0x90DD
#define GL_MAX_SHADER_STORAGE_BLOCK_SIZE 0x90DE
#define GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT 0x90DF
#define GL_SHADER_STORAGE_BARRIER_BIT 0x00002000
#define GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES 0x8F39
/* reuse GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS */
#endif
extern void (APIENTRYP gglShaderStorageBlockBinding)(GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding);

#ifndef GL_ARB_shader_subroutine
#define GL_ARB_shader_subroutine
#define GL_ACTIVE_SUBROUTINES 0x8DE5
#define GL_ACTIVE_SUBROUTINE_UNIFORMS 0x8DE6
#define GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS 0x8E47
#define GL_ACTIVE_SUBROUTINE_MAX_LENGTH 0x8E48
#define GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH 0x8E49
#define GL_MAX_SUBROUTINES 0x8DE7
#define GL_MAX_SUBROUTINE_UNIFORM_LOCATIONS 0x8DE8
/* reuse GL_NUM_COMPATIBLE_SUBROUTINES */
/* reuse GL_COMPATIBLE_SUBROUTINES */
/* reuse GL_UNIFORM_SIZE */
/* reuse GL_UNIFORM_NAME_LENGTH */
#endif
extern GLint (APIENTRYP gglGetSubroutineUniformLocation)(GLuint program, GLenum shadertype, const GLchar *name);
extern GLuint (APIENTRYP gglGetSubroutineIndex)(GLuint program, GLenum shadertype, const GLchar *name);
extern void (APIENTRYP gglGetActiveSubroutineUniformiv)(GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values);
extern void (APIENTRYP gglGetActiveSubroutineUniformName)(GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei *length, GLchar *name);
extern void (APIENTRYP gglGetActiveSubroutineName)(GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei *length, GLchar *name);
extern void (APIENTRYP gglUniformSubroutinesuiv)(GLenum shadertype, GLsizei count, const GLuint *indices);
extern void (APIENTRYP gglGetUniformSubroutineuiv)(GLenum shadertype, GLint location, GLuint *params);
extern void (APIENTRYP gglGetProgramStageiv)(GLuint program, GLenum shadertype, GLenum pname, GLint *values);

#ifndef GL_ARB_shader_texture_image_samples
#define GL_ARB_shader_texture_image_samples
#endif

#ifndef GL_ARB_shader_texture_lod
#define GL_ARB_shader_texture_lod
#endif

#ifndef GL_ARB_shader_viewport_layer_array
#define GL_ARB_shader_viewport_layer_array
#endif

#ifndef GL_ARB_shading_language_100
#define GL_ARB_shading_language_100
#define GL_SHADING_LANGUAGE_VERSION_ARB 0x8B8C
#endif

#ifndef GL_ARB_shading_language_420pack
#define GL_ARB_shading_language_420pack
#endif

#ifndef GL_ARB_shading_language_include
#define GL_ARB_shading_language_include
#define GL_SHADER_INCLUDE_ARB 0x8DAE
#define GL_NAMED_STRING_LENGTH_ARB 0x8DE9
#define GL_NAMED_STRING_TYPE_ARB 0x8DEA
#endif
extern void (APIENTRYP gglNamedStringARB)(GLenum type, GLint namelen, const GLchar *name, GLint stringlen, const GLchar *string);
extern void (APIENTRYP gglDeleteNamedStringARB)(GLint namelen, const GLchar *name);
extern void (APIENTRYP gglCompileShaderIncludeARB)(GLuint shader, GLsizei count, const GLchar *const*path, const GLint *length);
extern GLboolean (APIENTRYP gglIsNamedStringARB)(GLint namelen, const GLchar *name);
extern void (APIENTRYP gglGetNamedStringARB)(GLint namelen, const GLchar *name, GLsizei bufSize, GLint *stringlen, GLchar *string);
extern void (APIENTRYP gglGetNamedStringivARB)(GLint namelen, const GLchar *name, GLenum pname, GLint *params);

#ifndef GL_ARB_shading_language_packing
#define GL_ARB_shading_language_packing
#endif

#ifndef GL_ARB_shadow
#define GL_ARB_shadow
#define GL_TEXTURE_COMPARE_MODE_ARB 0x884C
#define GL_TEXTURE_COMPARE_FUNC_ARB 0x884D
#define GL_COMPARE_R_TO_TEXTURE_ARB 0x884E
#endif

#ifndef GL_ARB_shadow_ambient
#define GL_ARB_shadow_ambient
#define GL_TEXTURE_COMPARE_FAIL_VALUE_ARB 0x80BF
#endif

#ifndef GL_ARB_sparse_buffer
#define GL_ARB_sparse_buffer
#define GL_SPARSE_STORAGE_BIT_ARB 0x0400
#define GL_SPARSE_BUFFER_PAGE_SIZE_ARB 0x82F8
#endif
extern void (APIENTRYP gglBufferPageCommitmentARB)(GLenum target, GLintptr offset, GLsizeiptr size, GLboolean commit);
extern void (APIENTRYP gglNamedBufferPageCommitmentEXT)(GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit);
extern void (APIENTRYP gglNamedBufferPageCommitmentARB)(GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit);

#ifndef GL_ARB_sparse_texture
#define GL_ARB_sparse_texture
#define GL_TEXTURE_SPARSE_ARB 0x91A6
#define GL_VIRTUAL_PAGE_SIZE_INDEX_ARB 0x91A7
#define GL_NUM_SPARSE_LEVELS_ARB 0x91AA
#define GL_NUM_VIRTUAL_PAGE_SIZES_ARB 0x91A8
#define GL_VIRTUAL_PAGE_SIZE_X_ARB 0x9195
#define GL_VIRTUAL_PAGE_SIZE_Y_ARB 0x9196
#define GL_VIRTUAL_PAGE_SIZE_Z_ARB 0x9197
#define GL_MAX_SPARSE_TEXTURE_SIZE_ARB 0x9198
#define GL_MAX_SPARSE_3D_TEXTURE_SIZE_ARB 0x9199
#define GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS_ARB 0x919A
#define GL_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_ARB 0x91A9
#endif
extern void (APIENTRYP gglTexPageCommitmentARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit);

#ifndef GL_ARB_sparse_texture2
#define GL_ARB_sparse_texture2
#endif

#ifndef GL_ARB_sparse_texture_clamp
#define GL_ARB_sparse_texture_clamp
#endif

#ifndef GL_ARB_stencil_texturing
#define GL_ARB_stencil_texturing
#define GL_DEPTH_STENCIL_TEXTURE_MODE 0x90EA
#endif

#ifndef GL_ARB_sync
#define GL_ARB_sync
/* reuse GL_MAX_SERVER_WAIT_TIMEOUT */
/* reuse GL_OBJECT_TYPE */
/* reuse GL_SYNC_CONDITION */
/* reuse GL_SYNC_STATUS */
/* reuse GL_SYNC_FLAGS */
/* reuse GL_SYNC_FENCE */
/* reuse GL_SYNC_GPU_COMMANDS_COMPLETE */
/* reuse GL_UNSIGNALED */
/* reuse GL_SIGNALED */
/* reuse GL_ALREADY_SIGNALED */
/* reuse GL_TIMEOUT_EXPIRED */
/* reuse GL_CONDITION_SATISFIED */
/* reuse GL_WAIT_FAILED */
/* reuse GL_SYNC_FLUSH_COMMANDS_BIT */
/* reuse GL_TIMEOUT_IGNORED */
#endif
/* reuse GLsync (APIENTRYP gglFenceSync)(GLenum condition, GLbitfield flags) */
/* reuse GLboolean (APIENTRYP gglIsSync)(GLsync sync) */
/* reuse void (APIENTRYP gglDeleteSync)(GLsync sync) */
/* reuse GLenum (APIENTRYP gglClientWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout) */
/* reuse void (APIENTRYP gglWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout) */
/* reuse void (APIENTRYP gglGetInteger64v)(GLenum pname, GLint64 *data) */
/* reuse void (APIENTRYP gglGetSynciv)(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values) */

#ifndef GL_ARB_tessellation_shader
#define GL_ARB_tessellation_shader
#define GL_PATCHES 0x000E
#define GL_PATCH_VERTICES 0x8E72
#define GL_PATCH_DEFAULT_INNER_LEVEL 0x8E73
#define GL_PATCH_DEFAULT_OUTER_LEVEL 0x8E74
#define GL_TESS_CONTROL_OUTPUT_VERTICES 0x8E75
#define GL_TESS_GEN_MODE 0x8E76
#define GL_TESS_GEN_SPACING 0x8E77
#define GL_TESS_GEN_VERTEX_ORDER 0x8E78
#define GL_TESS_GEN_POINT_MODE 0x8E79
/* reuse GL_TRIANGLES */
#define GL_ISOLINES 0x8E7A
#define GL_QUADS 0x0007
/* reuse GL_EQUAL */
#define GL_FRACTIONAL_ODD 0x8E7B
#define GL_FRACTIONAL_EVEN 0x8E7C
/* reuse GL_CCW */
/* reuse GL_CW */
#define GL_MAX_PATCH_VERTICES 0x8E7D
#define GL_MAX_TESS_GEN_LEVEL 0x8E7E
#define GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS 0x8E7F
#define GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS 0x8E80
#define GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS 0x8E81
#define GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS 0x8E82
#define GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS 0x8E83
#define GL_MAX_TESS_PATCH_COMPONENTS 0x8E84
#define GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS 0x8E85
#define GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS 0x8E86
#define GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS 0x8E89
#define GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS 0x8E8A
#define GL_MAX_TESS_CONTROL_INPUT_COMPONENTS 0x886C
#define GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS 0x886D
#define GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS 0x8E1E
#define GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS 0x8E1F
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_CONTROL_SHADER 0x84F0
#define GL_UNIFORM_BLOCK_REFERENCED_BY_TESS_EVALUATION_SHADER 0x84F1
#define GL_TESS_EVALUATION_SHADER 0x8E87
#define GL_TESS_CONTROL_SHADER 0x8E88
#endif
extern void (APIENTRYP gglPatchParameteri)(GLenum pname, GLint value);
extern void (APIENTRYP gglPatchParameterfv)(GLenum pname, const GLfloat *values);

#ifndef GL_ARB_texture_barrier
#define GL_ARB_texture_barrier
#endif
extern void (APIENTRYP gglTextureBarrier)();

#ifndef GL_ARB_texture_border_clamp
#define GL_ARB_texture_border_clamp
#define GL_CLAMP_TO_BORDER_ARB 0x812D
#endif

#ifndef GL_ARB_texture_buffer_object
#define GL_ARB_texture_buffer_object
#define GL_TEXTURE_BUFFER_ARB 0x8C2A
#define GL_MAX_TEXTURE_BUFFER_SIZE_ARB 0x8C2B
#define GL_TEXTURE_BINDING_BUFFER_ARB 0x8C2C
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING_ARB 0x8C2D
#define GL_TEXTURE_BUFFER_FORMAT_ARB 0x8C2E
#endif
extern void (APIENTRYP gglTexBufferARB)(GLenum target, GLenum internalformat, GLuint buffer);

#ifndef GL_ARB_texture_buffer_object_rgb32
#define GL_ARB_texture_buffer_object_rgb32
/* reuse GL_RGB32F */
/* reuse GL_RGB32UI */
/* reuse GL_RGB32I */
#endif

#ifndef GL_ARB_texture_buffer_range
#define GL_ARB_texture_buffer_range
#define GL_TEXTURE_BUFFER_OFFSET 0x919D
#define GL_TEXTURE_BUFFER_SIZE 0x919E
#define GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT 0x919F
#endif
extern void (APIENTRYP gglTexBufferRange)(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);

#ifndef GL_ARB_texture_compression
#define GL_ARB_texture_compression
#define GL_COMPRESSED_ALPHA_ARB 0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB 0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB 0x84EB
#define GL_COMPRESSED_INTENSITY_ARB 0x84EC
#define GL_COMPRESSED_RGB_ARB 0x84ED
#define GL_COMPRESSED_RGBA_ARB 0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB 0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB 0x86A0
#define GL_TEXTURE_COMPRESSED_ARB 0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB 0x86A3
#endif
extern void (APIENTRYP gglCompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglGetCompressedTexImageARB)(GLenum target, GLint level, void *img);

#ifndef GL_ARB_texture_compression_bptc
#define GL_ARB_texture_compression_bptc
#define GL_COMPRESSED_RGBA_BPTC_UNORM_ARB 0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB 0x8E8D
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB 0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB 0x8E8F
#endif

#ifndef GL_ARB_texture_compression_rgtc
#define GL_ARB_texture_compression_rgtc
/* reuse GL_COMPRESSED_RED_RGTC1 */
/* reuse GL_COMPRESSED_SIGNED_RED_RGTC1 */
/* reuse GL_COMPRESSED_RG_RGTC2 */
/* reuse GL_COMPRESSED_SIGNED_RG_RGTC2 */
#endif

#ifndef GL_ARB_texture_cube_map
#define GL_ARB_texture_cube_map
#define GL_NORMAL_MAP_ARB 0x8511
#define GL_REFLECTION_MAP_ARB 0x8512
#define GL_TEXTURE_CUBE_MAP_ARB 0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB 0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB 0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB 0x851C
#endif

#ifndef GL_ARB_texture_cube_map_array
#define GL_ARB_texture_cube_map_array
#define GL_TEXTURE_CUBE_MAP_ARRAY_ARB 0x9009
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_ARB 0x900A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARRAY_ARB 0x900B
#define GL_SAMPLER_CUBE_MAP_ARRAY_ARB 0x900C
#define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW_ARB 0x900D
#define GL_INT_SAMPLER_CUBE_MAP_ARRAY_ARB 0x900E
#define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY_ARB 0x900F
#endif

#ifndef GL_ARB_texture_env_add
#define GL_ARB_texture_env_add
#endif

#ifndef GL_ARB_texture_env_combine
#define GL_ARB_texture_env_combine
#define GL_COMBINE_ARB 0x8570
#define GL_COMBINE_RGB_ARB 0x8571
#define GL_COMBINE_ALPHA_ARB 0x8572
#define GL_SOURCE0_RGB_ARB 0x8580
#define GL_SOURCE1_RGB_ARB 0x8581
#define GL_SOURCE2_RGB_ARB 0x8582
#define GL_SOURCE0_ALPHA_ARB 0x8588
#define GL_SOURCE1_ALPHA_ARB 0x8589
#define GL_SOURCE2_ALPHA_ARB 0x858A
#define GL_OPERAND0_RGB_ARB 0x8590
#define GL_OPERAND1_RGB_ARB 0x8591
#define GL_OPERAND2_RGB_ARB 0x8592
#define GL_OPERAND0_ALPHA_ARB 0x8598
#define GL_OPERAND1_ALPHA_ARB 0x8599
#define GL_OPERAND2_ALPHA_ARB 0x859A
#define GL_RGB_SCALE_ARB 0x8573
#define GL_ADD_SIGNED_ARB 0x8574
#define GL_INTERPOLATE_ARB 0x8575
#define GL_SUBTRACT_ARB 0x84E7
#define GL_CONSTANT_ARB 0x8576
#define GL_PRIMARY_COLOR_ARB 0x8577
#define GL_PREVIOUS_ARB 0x8578
#endif

#ifndef GL_ARB_texture_env_crossbar
#define GL_ARB_texture_env_crossbar
#endif

#ifndef GL_ARB_texture_env_dot3
#define GL_ARB_texture_env_dot3
#define GL_DOT3_RGB_ARB 0x86AE
#define GL_DOT3_RGBA_ARB 0x86AF
#endif

#ifndef GL_ARB_texture_filter_minmax
#define GL_ARB_texture_filter_minmax
#define GL_TEXTURE_REDUCTION_MODE_ARB 0x9366
#define GL_WEIGHTED_AVERAGE_ARB 0x9367
#endif

#ifndef GL_ARB_texture_float
#define GL_ARB_texture_float
#define GL_TEXTURE_RED_TYPE_ARB 0x8C10
#define GL_TEXTURE_GREEN_TYPE_ARB 0x8C11
#define GL_TEXTURE_BLUE_TYPE_ARB 0x8C12
#define GL_TEXTURE_ALPHA_TYPE_ARB 0x8C13
#define GL_TEXTURE_LUMINANCE_TYPE_ARB 0x8C14
#define GL_TEXTURE_INTENSITY_TYPE_ARB 0x8C15
#define GL_TEXTURE_DEPTH_TYPE_ARB 0x8C16
#define GL_UNSIGNED_NORMALIZED_ARB 0x8C17
#define GL_RGBA32F_ARB 0x8814
#define GL_RGB32F_ARB 0x8815
#define GL_ALPHA32F_ARB 0x8816
#define GL_INTENSITY32F_ARB 0x8817
#define GL_LUMINANCE32F_ARB 0x8818
#define GL_LUMINANCE_ALPHA32F_ARB 0x8819
#define GL_RGBA16F_ARB 0x881A
#define GL_RGB16F_ARB 0x881B
#define GL_ALPHA16F_ARB 0x881C
#define GL_INTENSITY16F_ARB 0x881D
#define GL_LUMINANCE16F_ARB 0x881E
#define GL_LUMINANCE_ALPHA16F_ARB 0x881F
#endif

#ifndef GL_ARB_texture_gather
#define GL_ARB_texture_gather
#define GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET_ARB 0x8E5E
#define GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET_ARB 0x8E5F
#define GL_MAX_PROGRAM_TEXTURE_GATHER_COMPONENTS_ARB 0x8F9F
#endif

#ifndef GL_ARB_texture_mirror_clamp_to_edge
#define GL_ARB_texture_mirror_clamp_to_edge
#define GL_MIRROR_CLAMP_TO_EDGE 0x8743
#endif

#ifndef GL_ARB_texture_mirrored_repeat
#define GL_ARB_texture_mirrored_repeat
#define GL_MIRRORED_REPEAT_ARB 0x8370
#endif

#ifndef GL_ARB_texture_multisample
#define GL_ARB_texture_multisample
/* reuse GL_SAMPLE_POSITION */
/* reuse GL_SAMPLE_MASK */
/* reuse GL_SAMPLE_MASK_VALUE */
/* reuse GL_MAX_SAMPLE_MASK_WORDS */
/* reuse GL_TEXTURE_2D_MULTISAMPLE */
/* reuse GL_PROXY_TEXTURE_2D_MULTISAMPLE */
/* reuse GL_TEXTURE_2D_MULTISAMPLE_ARRAY */
/* reuse GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY */
/* reuse GL_TEXTURE_BINDING_2D_MULTISAMPLE */
/* reuse GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY */
/* reuse GL_TEXTURE_SAMPLES */
/* reuse GL_TEXTURE_FIXED_SAMPLE_LOCATIONS */
/* reuse GL_SAMPLER_2D_MULTISAMPLE */
/* reuse GL_INT_SAMPLER_2D_MULTISAMPLE */
/* reuse GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE */
/* reuse GL_SAMPLER_2D_MULTISAMPLE_ARRAY */
/* reuse GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY */
/* reuse GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY */
/* reuse GL_MAX_COLOR_TEXTURE_SAMPLES */
/* reuse GL_MAX_DEPTH_TEXTURE_SAMPLES */
/* reuse GL_MAX_INTEGER_SAMPLES */
#endif
/* reuse void (APIENTRYP gglTexImage2DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) */
/* reuse void (APIENTRYP gglTexImage3DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) */
/* reuse void (APIENTRYP gglGetMultisamplefv)(GLenum pname, GLuint index, GLfloat *val) */
/* reuse void (APIENTRYP gglSampleMaski)(GLuint maskNumber, GLbitfield mask) */

#ifndef GL_ARB_texture_non_power_of_two
#define GL_ARB_texture_non_power_of_two
#endif

#ifndef GL_ARB_texture_query_levels
#define GL_ARB_texture_query_levels
#endif

#ifndef GL_ARB_texture_query_lod
#define GL_ARB_texture_query_lod
#endif

#ifndef GL_ARB_texture_rectangle
#define GL_ARB_texture_rectangle
#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE_ARB 0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE_ARB 0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB 0x84F8
#endif

#ifndef GL_ARB_texture_rg
#define GL_ARB_texture_rg
/* reuse GL_RG */
/* reuse GL_RG_INTEGER */
/* reuse GL_R8 */
/* reuse GL_R16 */
/* reuse GL_RG8 */
/* reuse GL_RG16 */
/* reuse GL_R16F */
/* reuse GL_R32F */
/* reuse GL_RG16F */
/* reuse GL_RG32F */
/* reuse GL_R8I */
/* reuse GL_R8UI */
/* reuse GL_R16I */
/* reuse GL_R16UI */
/* reuse GL_R32I */
/* reuse GL_R32UI */
/* reuse GL_RG8I */
/* reuse GL_RG8UI */
/* reuse GL_RG16I */
/* reuse GL_RG16UI */
/* reuse GL_RG32I */
/* reuse GL_RG32UI */
#endif

#ifndef GL_ARB_texture_rgb10_a2ui
#define GL_ARB_texture_rgb10_a2ui
#define GL_RGB10_A2UI 0x906F
#endif

#ifndef GL_ARB_texture_stencil8
#define GL_ARB_texture_stencil8
/* reuse GL_STENCIL_INDEX */
/* reuse GL_STENCIL_INDEX8 */
#endif

#ifndef GL_ARB_texture_storage
#define GL_ARB_texture_storage
#define GL_TEXTURE_IMMUTABLE_FORMAT 0x912F
#endif
extern void (APIENTRYP gglTexStorage1D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width);
extern void (APIENTRYP gglTexStorage2D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglTexStorage3D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

#ifndef GL_ARB_texture_storage_multisample
#define GL_ARB_texture_storage_multisample
#endif
extern void (APIENTRYP gglTexStorage2DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
extern void (APIENTRYP gglTexStorage3DMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);

#ifndef GL_ARB_texture_swizzle
#define GL_ARB_texture_swizzle
#define GL_TEXTURE_SWIZZLE_R 0x8E42
#define GL_TEXTURE_SWIZZLE_G 0x8E43
#define GL_TEXTURE_SWIZZLE_B 0x8E44
#define GL_TEXTURE_SWIZZLE_A 0x8E45
#define GL_TEXTURE_SWIZZLE_RGBA 0x8E46
#endif

#ifndef GL_ARB_texture_view
#define GL_ARB_texture_view
#define GL_TEXTURE_VIEW_MIN_LEVEL 0x82DB
#define GL_TEXTURE_VIEW_NUM_LEVELS 0x82DC
#define GL_TEXTURE_VIEW_MIN_LAYER 0x82DD
#define GL_TEXTURE_VIEW_NUM_LAYERS 0x82DE
#define GL_TEXTURE_IMMUTABLE_LEVELS 0x82DF
#endif
extern void (APIENTRYP gglTextureView)(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers);

#ifndef GL_ARB_timer_query
#define GL_ARB_timer_query
#define GL_TIME_ELAPSED 0x88BF
#define GL_TIMESTAMP 0x8E28
#endif
extern void (APIENTRYP gglQueryCounter)(GLuint id, GLenum target);
extern void (APIENTRYP gglGetQueryObjecti64v)(GLuint id, GLenum pname, GLint64 *params);
extern void (APIENTRYP gglGetQueryObjectui64v)(GLuint id, GLenum pname, GLuint64 *params);

#ifndef GL_ARB_transform_feedback2
#define GL_ARB_transform_feedback2
#define GL_TRANSFORM_FEEDBACK 0x8E22
#define GL_TRANSFORM_FEEDBACK_BUFFER_PAUSED 0x8E23
#define GL_TRANSFORM_FEEDBACK_BUFFER_ACTIVE 0x8E24
#define GL_TRANSFORM_FEEDBACK_BINDING 0x8E25
#endif
extern void (APIENTRYP gglBindTransformFeedback)(GLenum target, GLuint id);
extern void (APIENTRYP gglDeleteTransformFeedbacks)(GLsizei n, const GLuint *ids);
extern void (APIENTRYP gglGenTransformFeedbacks)(GLsizei n, GLuint *ids);
extern GLboolean (APIENTRYP gglIsTransformFeedback)(GLuint id);
extern void (APIENTRYP gglPauseTransformFeedback)();
extern void (APIENTRYP gglResumeTransformFeedback)();
extern void (APIENTRYP gglDrawTransformFeedback)(GLenum mode, GLuint id);

#ifndef GL_ARB_transform_feedback3
#define GL_ARB_transform_feedback3
#define GL_MAX_TRANSFORM_FEEDBACK_BUFFERS 0x8E70
/* reuse GL_MAX_VERTEX_STREAMS */
#endif
extern void (APIENTRYP gglDrawTransformFeedbackStream)(GLenum mode, GLuint id, GLuint stream);
extern void (APIENTRYP gglBeginQueryIndexed)(GLenum target, GLuint index, GLuint id);
extern void (APIENTRYP gglEndQueryIndexed)(GLenum target, GLuint index);
extern void (APIENTRYP gglGetQueryIndexediv)(GLenum target, GLuint index, GLenum pname, GLint *params);

#ifndef GL_ARB_transform_feedback_instanced
#define GL_ARB_transform_feedback_instanced
#endif
extern void (APIENTRYP gglDrawTransformFeedbackInstanced)(GLenum mode, GLuint id, GLsizei instancecount);
extern void (APIENTRYP gglDrawTransformFeedbackStreamInstanced)(GLenum mode, GLuint id, GLuint stream, GLsizei instancecount);

#ifndef GL_ARB_transform_feedback_overflow_query
#define GL_ARB_transform_feedback_overflow_query
#define GL_TRANSFORM_FEEDBACK_OVERFLOW_ARB 0x82EC
#define GL_TRANSFORM_FEEDBACK_STREAM_OVERFLOW_ARB 0x82ED
#endif

#ifndef GL_ARB_transpose_matrix
#define GL_ARB_transpose_matrix
#define GL_TRANSPOSE_MODELVIEW_MATRIX_ARB 0x84E3
#define GL_TRANSPOSE_PROJECTION_MATRIX_ARB 0x84E4
#define GL_TRANSPOSE_TEXTURE_MATRIX_ARB 0x84E5
#define GL_TRANSPOSE_COLOR_MATRIX_ARB 0x84E6
#endif
extern void (APIENTRYP gglLoadTransposeMatrixfARB)(const GLfloat *m);
extern void (APIENTRYP gglLoadTransposeMatrixdARB)(const GLdouble *m);
extern void (APIENTRYP gglMultTransposeMatrixfARB)(const GLfloat *m);
extern void (APIENTRYP gglMultTransposeMatrixdARB)(const GLdouble *m);

#ifndef GL_ARB_uniform_buffer_object
#define GL_ARB_uniform_buffer_object
/* reuse GL_UNIFORM_BUFFER */
/* reuse GL_UNIFORM_BUFFER_BINDING */
/* reuse GL_UNIFORM_BUFFER_START */
/* reuse GL_UNIFORM_BUFFER_SIZE */
/* reuse GL_MAX_VERTEX_UNIFORM_BLOCKS */
/* reuse GL_MAX_GEOMETRY_UNIFORM_BLOCKS */
/* reuse GL_MAX_FRAGMENT_UNIFORM_BLOCKS */
/* reuse GL_MAX_COMBINED_UNIFORM_BLOCKS */
/* reuse GL_MAX_UNIFORM_BUFFER_BINDINGS */
/* reuse GL_MAX_UNIFORM_BLOCK_SIZE */
/* reuse GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS */
/* reuse GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS */
/* reuse GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS */
/* reuse GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT */
/* reuse GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH */
/* reuse GL_ACTIVE_UNIFORM_BLOCKS */
/* reuse GL_UNIFORM_TYPE */
/* reuse GL_UNIFORM_SIZE */
/* reuse GL_UNIFORM_NAME_LENGTH */
/* reuse GL_UNIFORM_BLOCK_INDEX */
/* reuse GL_UNIFORM_OFFSET */
/* reuse GL_UNIFORM_ARRAY_STRIDE */
/* reuse GL_UNIFORM_MATRIX_STRIDE */
/* reuse GL_UNIFORM_IS_ROW_MAJOR */
/* reuse GL_UNIFORM_BLOCK_BINDING */
/* reuse GL_UNIFORM_BLOCK_DATA_SIZE */
/* reuse GL_UNIFORM_BLOCK_NAME_LENGTH */
/* reuse GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS */
/* reuse GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES */
/* reuse GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER */
/* reuse GL_UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER */
/* reuse GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER */
/* reuse GL_INVALID_INDEX */
#endif
/* reuse void (APIENTRYP gglGetUniformIndices)(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices) */
/* reuse void (APIENTRYP gglGetActiveUniformsiv)(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params) */
/* reuse void (APIENTRYP gglGetActiveUniformName)(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName) */
/* reuse GLuint (APIENTRYP gglGetUniformBlockIndex)(GLuint program, const GLchar *uniformBlockName) */
/* reuse void (APIENTRYP gglGetActiveUniformBlockiv)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params) */
/* reuse void (APIENTRYP gglGetActiveUniformBlockName)(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName) */
/* reuse void (APIENTRYP gglUniformBlockBinding)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) */
/* reuse void (APIENTRYP gglBindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) */
/* reuse void (APIENTRYP gglBindBufferBase)(GLenum target, GLuint index, GLuint buffer) */
/* reuse void (APIENTRYP gglGetIntegeri_v)(GLenum target, GLuint index, GLint *data) */

#ifndef GL_ARB_vertex_array_bgra
#define GL_ARB_vertex_array_bgra
/* reuse GL_BGRA */
#endif

#ifndef GL_ARB_vertex_array_object
#define GL_ARB_vertex_array_object
/* reuse GL_VERTEX_ARRAY_BINDING */
#endif
/* reuse void (APIENTRYP gglBindVertexArray)(GLuint array) */
/* reuse void (APIENTRYP gglDeleteVertexArrays)(GLsizei n, const GLuint *arrays) */
/* reuse void (APIENTRYP gglGenVertexArrays)(GLsizei n, GLuint *arrays) */
/* reuse GLboolean (APIENTRYP gglIsVertexArray)(GLuint array) */

#ifndef GL_ARB_vertex_attrib_64bit
#define GL_ARB_vertex_attrib_64bit
/* reuse GL_RGB32I */
/* reuse GL_DOUBLE_VEC2 */
/* reuse GL_DOUBLE_VEC3 */
/* reuse GL_DOUBLE_VEC4 */
/* reuse GL_DOUBLE_MAT2 */
/* reuse GL_DOUBLE_MAT3 */
/* reuse GL_DOUBLE_MAT4 */
/* reuse GL_DOUBLE_MAT2x3 */
/* reuse GL_DOUBLE_MAT2x4 */
/* reuse GL_DOUBLE_MAT3x2 */
/* reuse GL_DOUBLE_MAT3x4 */
/* reuse GL_DOUBLE_MAT4x2 */
/* reuse GL_DOUBLE_MAT4x3 */
#endif
extern void (APIENTRYP gglVertexAttribL1d)(GLuint index, GLdouble x);
extern void (APIENTRYP gglVertexAttribL2d)(GLuint index, GLdouble x, GLdouble y);
extern void (APIENTRYP gglVertexAttribL3d)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglVertexAttribL4d)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglVertexAttribL1dv)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttribL2dv)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttribL3dv)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttribL4dv)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttribLPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
extern void (APIENTRYP gglGetVertexAttribLdv)(GLuint index, GLenum pname, GLdouble *params);

#ifndef GL_ARB_vertex_attrib_binding
#define GL_ARB_vertex_attrib_binding
#define GL_VERTEX_ATTRIB_BINDING 0x82D4
#define GL_VERTEX_ATTRIB_RELATIVE_OFFSET 0x82D5
#define GL_VERTEX_BINDING_DIVISOR 0x82D6
#define GL_VERTEX_BINDING_OFFSET 0x82D7
#define GL_VERTEX_BINDING_STRIDE 0x82D8
#define GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET 0x82D9
#define GL_MAX_VERTEX_ATTRIB_BINDINGS 0x82DA
#endif
extern void (APIENTRYP gglBindVertexBuffer)(GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
extern void (APIENTRYP gglVertexAttribFormat)(GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
extern void (APIENTRYP gglVertexAttribIFormat)(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
extern void (APIENTRYP gglVertexAttribLFormat)(GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
extern void (APIENTRYP gglVertexAttribBinding)(GLuint attribindex, GLuint bindingindex);
extern void (APIENTRYP gglVertexBindingDivisor)(GLuint bindingindex, GLuint divisor);

#ifndef GL_ARB_vertex_blend
#define GL_ARB_vertex_blend
#define GL_MAX_VERTEX_UNITS_ARB 0x86A4
#define GL_ACTIVE_VERTEX_UNITS_ARB 0x86A5
#define GL_WEIGHT_SUM_UNITY_ARB 0x86A6
#define GL_VERTEX_BLEND_ARB 0x86A7
#define GL_CURRENT_WEIGHT_ARB 0x86A8
#define GL_WEIGHT_ARRAY_TYPE_ARB 0x86A9
#define GL_WEIGHT_ARRAY_STRIDE_ARB 0x86AA
#define GL_WEIGHT_ARRAY_SIZE_ARB 0x86AB
#define GL_WEIGHT_ARRAY_POINTER_ARB 0x86AC
#define GL_WEIGHT_ARRAY_ARB 0x86AD
#define GL_MODELVIEW0_ARB 0x1700
#define GL_MODELVIEW1_ARB 0x850A
#define GL_MODELVIEW2_ARB 0x8722
#define GL_MODELVIEW3_ARB 0x8723
#define GL_MODELVIEW4_ARB 0x8724
#define GL_MODELVIEW5_ARB 0x8725
#define GL_MODELVIEW6_ARB 0x8726
#define GL_MODELVIEW7_ARB 0x8727
#define GL_MODELVIEW8_ARB 0x8728
#define GL_MODELVIEW9_ARB 0x8729
#define GL_MODELVIEW10_ARB 0x872A
#define GL_MODELVIEW11_ARB 0x872B
#define GL_MODELVIEW12_ARB 0x872C
#define GL_MODELVIEW13_ARB 0x872D
#define GL_MODELVIEW14_ARB 0x872E
#define GL_MODELVIEW15_ARB 0x872F
#define GL_MODELVIEW16_ARB 0x8730
#define GL_MODELVIEW17_ARB 0x8731
#define GL_MODELVIEW18_ARB 0x8732
#define GL_MODELVIEW19_ARB 0x8733
#define GL_MODELVIEW20_ARB 0x8734
#define GL_MODELVIEW21_ARB 0x8735
#define GL_MODELVIEW22_ARB 0x8736
#define GL_MODELVIEW23_ARB 0x8737
#define GL_MODELVIEW24_ARB 0x8738
#define GL_MODELVIEW25_ARB 0x8739
#define GL_MODELVIEW26_ARB 0x873A
#define GL_MODELVIEW27_ARB 0x873B
#define GL_MODELVIEW28_ARB 0x873C
#define GL_MODELVIEW29_ARB 0x873D
#define GL_MODELVIEW30_ARB 0x873E
#define GL_MODELVIEW31_ARB 0x873F
#endif
extern void (APIENTRYP gglWeightbvARB)(GLint size, const GLbyte *weights);
extern void (APIENTRYP gglWeightsvARB)(GLint size, const GLshort *weights);
extern void (APIENTRYP gglWeightivARB)(GLint size, const GLint *weights);
extern void (APIENTRYP gglWeightfvARB)(GLint size, const GLfloat *weights);
extern void (APIENTRYP gglWeightdvARB)(GLint size, const GLdouble *weights);
extern void (APIENTRYP gglWeightubvARB)(GLint size, const GLubyte *weights);
extern void (APIENTRYP gglWeightusvARB)(GLint size, const GLushort *weights);
extern void (APIENTRYP gglWeightuivARB)(GLint size, const GLuint *weights);
extern void (APIENTRYP gglWeightPointerARB)(GLint size, GLenum type, GLsizei stride, const void *pointer);
extern void (APIENTRYP gglVertexBlendARB)(GLint count);

#ifndef GL_ARB_vertex_buffer_object
#define GL_ARB_vertex_buffer_object
#define GL_BUFFER_SIZE_ARB 0x8764
#define GL_BUFFER_USAGE_ARB 0x8765
#define GL_ARRAY_BUFFER_ARB 0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB 0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB 0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB 0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB 0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB 0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB 0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB 0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB 0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB 0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB 0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB 0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB 0x889F
#define GL_READ_ONLY_ARB 0x88B8
#define GL_WRITE_ONLY_ARB 0x88B9
#define GL_READ_WRITE_ARB 0x88BA
#define GL_BUFFER_ACCESS_ARB 0x88BB
#define GL_BUFFER_MAPPED_ARB 0x88BC
#define GL_BUFFER_MAP_POINTER_ARB 0x88BD
#define GL_STREAM_DRAW_ARB 0x88E0
#define GL_STREAM_READ_ARB 0x88E1
#define GL_STREAM_COPY_ARB 0x88E2
#define GL_STATIC_DRAW_ARB 0x88E4
#define GL_STATIC_READ_ARB 0x88E5
#define GL_STATIC_COPY_ARB 0x88E6
#define GL_DYNAMIC_DRAW_ARB 0x88E8
#define GL_DYNAMIC_READ_ARB 0x88E9
#define GL_DYNAMIC_COPY_ARB 0x88EA
#endif
extern void (APIENTRYP gglBindBufferARB)(GLenum target, GLuint buffer);
extern void (APIENTRYP gglDeleteBuffersARB)(GLsizei n, const GLuint *buffers);
extern void (APIENTRYP gglGenBuffersARB)(GLsizei n, GLuint *buffers);
extern GLboolean (APIENTRYP gglIsBufferARB)(GLuint buffer);
extern void (APIENTRYP gglBufferDataARB)(GLenum target, GLsizeiptrARB size, const void *data, GLenum usage);
extern void (APIENTRYP gglBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void *data);
extern void (APIENTRYP gglGetBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, void *data);
extern void * (APIENTRYP gglMapBufferARB)(GLenum target, GLenum access);
extern GLboolean (APIENTRYP gglUnmapBufferARB)(GLenum target);
extern void (APIENTRYP gglGetBufferParameterivARB)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetBufferPointervARB)(GLenum target, GLenum pname, void **params);

#ifndef GL_ARB_vertex_program
#define GL_ARB_vertex_program
#define GL_COLOR_SUM_ARB 0x8458
#define GL_VERTEX_PROGRAM_ARB 0x8620
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB 0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB 0x8625
#define GL_CURRENT_VERTEX_ATTRIB_ARB 0x8626
/* reuse GL_PROGRAM_LENGTH_ARB */
/* reuse GL_PROGRAM_STRING_ARB */
/* reuse GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB */
/* reuse GL_MAX_PROGRAM_MATRICES_ARB */
/* reuse GL_CURRENT_MATRIX_STACK_DEPTH_ARB */
/* reuse GL_CURRENT_MATRIX_ARB */
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB 0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB 0x8643
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB 0x8645
/* reuse GL_PROGRAM_ERROR_POSITION_ARB */
/* reuse GL_PROGRAM_BINDING_ARB */
#define GL_MAX_VERTEX_ATTRIBS_ARB 0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB 0x886A
/* reuse GL_PROGRAM_ERROR_STRING_ARB */
/* reuse GL_PROGRAM_FORMAT_ASCII_ARB */
/* reuse GL_PROGRAM_FORMAT_ARB */
/* reuse GL_PROGRAM_INSTRUCTIONS_ARB */
/* reuse GL_MAX_PROGRAM_INSTRUCTIONS_ARB */
/* reuse GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB */
/* reuse GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB */
/* reuse GL_PROGRAM_TEMPORARIES_ARB */
/* reuse GL_MAX_PROGRAM_TEMPORARIES_ARB */
/* reuse GL_PROGRAM_NATIVE_TEMPORARIES_ARB */
/* reuse GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB */
/* reuse GL_PROGRAM_PARAMETERS_ARB */
/* reuse GL_MAX_PROGRAM_PARAMETERS_ARB */
/* reuse GL_PROGRAM_NATIVE_PARAMETERS_ARB */
/* reuse GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB */
/* reuse GL_PROGRAM_ATTRIBS_ARB */
/* reuse GL_MAX_PROGRAM_ATTRIBS_ARB */
/* reuse GL_PROGRAM_NATIVE_ATTRIBS_ARB */
/* reuse GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB */
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB 0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB 0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B3
/* reuse GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB */
/* reuse GL_MAX_PROGRAM_ENV_PARAMETERS_ARB */
/* reuse GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB */
/* reuse GL_TRANSPOSE_CURRENT_MATRIX_ARB */
/* reuse GL_MATRIX0_ARB */
/* reuse GL_MATRIX1_ARB */
/* reuse GL_MATRIX2_ARB */
/* reuse GL_MATRIX3_ARB */
/* reuse GL_MATRIX4_ARB */
/* reuse GL_MATRIX5_ARB */
/* reuse GL_MATRIX6_ARB */
/* reuse GL_MATRIX7_ARB */
/* reuse GL_MATRIX8_ARB */
/* reuse GL_MATRIX9_ARB */
/* reuse GL_MATRIX10_ARB */
/* reuse GL_MATRIX11_ARB */
/* reuse GL_MATRIX12_ARB */
/* reuse GL_MATRIX13_ARB */
/* reuse GL_MATRIX14_ARB */
/* reuse GL_MATRIX15_ARB */
/* reuse GL_MATRIX16_ARB */
/* reuse GL_MATRIX17_ARB */
/* reuse GL_MATRIX18_ARB */
/* reuse GL_MATRIX19_ARB */
/* reuse GL_MATRIX20_ARB */
/* reuse GL_MATRIX21_ARB */
/* reuse GL_MATRIX22_ARB */
/* reuse GL_MATRIX23_ARB */
/* reuse GL_MATRIX24_ARB */
/* reuse GL_MATRIX25_ARB */
/* reuse GL_MATRIX26_ARB */
/* reuse GL_MATRIX27_ARB */
/* reuse GL_MATRIX28_ARB */
/* reuse GL_MATRIX29_ARB */
/* reuse GL_MATRIX30_ARB */
/* reuse GL_MATRIX31_ARB */
#endif
extern void (APIENTRYP gglVertexAttrib1dARB)(GLuint index, GLdouble x);
extern void (APIENTRYP gglVertexAttrib1dvARB)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttrib1fARB)(GLuint index, GLfloat x);
extern void (APIENTRYP gglVertexAttrib1fvARB)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib1sARB)(GLuint index, GLshort x);
extern void (APIENTRYP gglVertexAttrib1svARB)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib2dARB)(GLuint index, GLdouble x, GLdouble y);
extern void (APIENTRYP gglVertexAttrib2dvARB)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttrib2fARB)(GLuint index, GLfloat x, GLfloat y);
extern void (APIENTRYP gglVertexAttrib2fvARB)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib2sARB)(GLuint index, GLshort x, GLshort y);
extern void (APIENTRYP gglVertexAttrib2svARB)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib3dARB)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglVertexAttrib3dvARB)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttrib3fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglVertexAttrib3fvARB)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib3sARB)(GLuint index, GLshort x, GLshort y, GLshort z);
extern void (APIENTRYP gglVertexAttrib3svARB)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib4NbvARB)(GLuint index, const GLbyte *v);
extern void (APIENTRYP gglVertexAttrib4NivARB)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttrib4NsvARB)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib4NubARB)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
extern void (APIENTRYP gglVertexAttrib4NubvARB)(GLuint index, const GLubyte *v);
extern void (APIENTRYP gglVertexAttrib4NuivARB)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglVertexAttrib4NusvARB)(GLuint index, const GLushort *v);
extern void (APIENTRYP gglVertexAttrib4bvARB)(GLuint index, const GLbyte *v);
extern void (APIENTRYP gglVertexAttrib4dARB)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglVertexAttrib4dvARB)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttrib4fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglVertexAttrib4fvARB)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib4ivARB)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttrib4sARB)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
extern void (APIENTRYP gglVertexAttrib4svARB)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib4ubvARB)(GLuint index, const GLubyte *v);
extern void (APIENTRYP gglVertexAttrib4uivARB)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglVertexAttrib4usvARB)(GLuint index, const GLushort *v);
extern void (APIENTRYP gglVertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
extern void (APIENTRYP gglEnableVertexAttribArrayARB)(GLuint index);
extern void (APIENTRYP gglDisableVertexAttribArrayARB)(GLuint index);
/* reuse void (APIENTRYP gglProgramStringARB)(GLenum target, GLenum format, GLsizei len, const void *string) */
/* reuse void (APIENTRYP gglBindProgramARB)(GLenum target, GLuint program) */
/* reuse void (APIENTRYP gglDeleteProgramsARB)(GLsizei n, const GLuint *programs) */
/* reuse void (APIENTRYP gglGenProgramsARB)(GLsizei n, GLuint *programs) */
/* reuse void (APIENTRYP gglProgramEnvParameter4dARB)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) */
/* reuse void (APIENTRYP gglProgramEnvParameter4dvARB)(GLenum target, GLuint index, const GLdouble *params) */
/* reuse void (APIENTRYP gglProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) */
/* reuse void (APIENTRYP gglProgramEnvParameter4fvARB)(GLenum target, GLuint index, const GLfloat *params) */
/* reuse void (APIENTRYP gglProgramLocalParameter4dARB)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) */
/* reuse void (APIENTRYP gglProgramLocalParameter4dvARB)(GLenum target, GLuint index, const GLdouble *params) */
/* reuse void (APIENTRYP gglProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) */
/* reuse void (APIENTRYP gglProgramLocalParameter4fvARB)(GLenum target, GLuint index, const GLfloat *params) */
/* reuse void (APIENTRYP gglGetProgramEnvParameterdvARB)(GLenum target, GLuint index, GLdouble *params) */
/* reuse void (APIENTRYP gglGetProgramEnvParameterfvARB)(GLenum target, GLuint index, GLfloat *params) */
/* reuse void (APIENTRYP gglGetProgramLocalParameterdvARB)(GLenum target, GLuint index, GLdouble *params) */
/* reuse void (APIENTRYP gglGetProgramLocalParameterfvARB)(GLenum target, GLuint index, GLfloat *params) */
/* reuse void (APIENTRYP gglGetProgramivARB)(GLenum target, GLenum pname, GLint *params) */
/* reuse void (APIENTRYP gglGetProgramStringARB)(GLenum target, GLenum pname, void *string) */
extern void (APIENTRYP gglGetVertexAttribdvARB)(GLuint index, GLenum pname, GLdouble *params);
extern void (APIENTRYP gglGetVertexAttribfvARB)(GLuint index, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetVertexAttribivARB)(GLuint index, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetVertexAttribPointervARB)(GLuint index, GLenum pname, void **pointer);
/* reuse GLboolean (APIENTRYP gglIsProgramARB)(GLuint program) */

#ifndef GL_ARB_vertex_shader
#define GL_ARB_vertex_shader
#define GL_VERTEX_SHADER_ARB 0x8B31
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB 0x8B4A
#define GL_MAX_VARYING_FLOATS_ARB 0x8B4B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB 0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB 0x8B4D
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB 0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB 0x8B8A
/* reuse GL_MAX_VERTEX_ATTRIBS_ARB */
/* reuse GL_MAX_TEXTURE_IMAGE_UNITS_ARB */
/* reuse GL_MAX_TEXTURE_COORDS_ARB */
/* reuse GL_VERTEX_PROGRAM_POINT_SIZE_ARB */
/* reuse GL_VERTEX_PROGRAM_TWO_SIDE_ARB */
/* reuse GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB */
/* reuse GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB */
/* reuse GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB */
/* reuse GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB */
/* reuse GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB */
/* reuse GL_CURRENT_VERTEX_ATTRIB_ARB */
/* reuse GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB */
/* reuse GL_FLOAT */
/* reuse GL_FLOAT_VEC2_ARB */
/* reuse GL_FLOAT_VEC3_ARB */
/* reuse GL_FLOAT_VEC4_ARB */
/* reuse GL_FLOAT_MAT2_ARB */
/* reuse GL_FLOAT_MAT3_ARB */
/* reuse GL_FLOAT_MAT4_ARB */
#endif
/* reuse void (APIENTRYP gglVertexAttrib1fARB)(GLuint index, GLfloat x) */
/* reuse void (APIENTRYP gglVertexAttrib1sARB)(GLuint index, GLshort x) */
/* reuse void (APIENTRYP gglVertexAttrib1dARB)(GLuint index, GLdouble x) */
/* reuse void (APIENTRYP gglVertexAttrib2fARB)(GLuint index, GLfloat x, GLfloat y) */
/* reuse void (APIENTRYP gglVertexAttrib2sARB)(GLuint index, GLshort x, GLshort y) */
/* reuse void (APIENTRYP gglVertexAttrib2dARB)(GLuint index, GLdouble x, GLdouble y) */
/* reuse void (APIENTRYP gglVertexAttrib3fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z) */
/* reuse void (APIENTRYP gglVertexAttrib3sARB)(GLuint index, GLshort x, GLshort y, GLshort z) */
/* reuse void (APIENTRYP gglVertexAttrib3dARB)(GLuint index, GLdouble x, GLdouble y, GLdouble z) */
/* reuse void (APIENTRYP gglVertexAttrib4fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) */
/* reuse void (APIENTRYP gglVertexAttrib4sARB)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w) */
/* reuse void (APIENTRYP gglVertexAttrib4dARB)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w) */
/* reuse void (APIENTRYP gglVertexAttrib4NubARB)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w) */
/* reuse void (APIENTRYP gglVertexAttrib1fvARB)(GLuint index, const GLfloat *v) */
/* reuse void (APIENTRYP gglVertexAttrib1svARB)(GLuint index, const GLshort *v) */
/* reuse void (APIENTRYP gglVertexAttrib1dvARB)(GLuint index, const GLdouble *v) */
/* reuse void (APIENTRYP gglVertexAttrib2fvARB)(GLuint index, const GLfloat *v) */
/* reuse void (APIENTRYP gglVertexAttrib2svARB)(GLuint index, const GLshort *v) */
/* reuse void (APIENTRYP gglVertexAttrib2dvARB)(GLuint index, const GLdouble *v) */
/* reuse void (APIENTRYP gglVertexAttrib3fvARB)(GLuint index, const GLfloat *v) */
/* reuse void (APIENTRYP gglVertexAttrib3svARB)(GLuint index, const GLshort *v) */
/* reuse void (APIENTRYP gglVertexAttrib3dvARB)(GLuint index, const GLdouble *v) */
/* reuse void (APIENTRYP gglVertexAttrib4fvARB)(GLuint index, const GLfloat *v) */
/* reuse void (APIENTRYP gglVertexAttrib4svARB)(GLuint index, const GLshort *v) */
/* reuse void (APIENTRYP gglVertexAttrib4dvARB)(GLuint index, const GLdouble *v) */
/* reuse void (APIENTRYP gglVertexAttrib4ivARB)(GLuint index, const GLint *v) */
/* reuse void (APIENTRYP gglVertexAttrib4bvARB)(GLuint index, const GLbyte *v) */
/* reuse void (APIENTRYP gglVertexAttrib4ubvARB)(GLuint index, const GLubyte *v) */
/* reuse void (APIENTRYP gglVertexAttrib4usvARB)(GLuint index, const GLushort *v) */
/* reuse void (APIENTRYP gglVertexAttrib4uivARB)(GLuint index, const GLuint *v) */
/* reuse void (APIENTRYP gglVertexAttrib4NbvARB)(GLuint index, const GLbyte *v) */
/* reuse void (APIENTRYP gglVertexAttrib4NsvARB)(GLuint index, const GLshort *v) */
/* reuse void (APIENTRYP gglVertexAttrib4NivARB)(GLuint index, const GLint *v) */
/* reuse void (APIENTRYP gglVertexAttrib4NubvARB)(GLuint index, const GLubyte *v) */
/* reuse void (APIENTRYP gglVertexAttrib4NusvARB)(GLuint index, const GLushort *v) */
/* reuse void (APIENTRYP gglVertexAttrib4NuivARB)(GLuint index, const GLuint *v) */
/* reuse void (APIENTRYP gglVertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) */
/* reuse void (APIENTRYP gglEnableVertexAttribArrayARB)(GLuint index) */
/* reuse void (APIENTRYP gglDisableVertexAttribArrayARB)(GLuint index) */
extern void (APIENTRYP gglBindAttribLocationARB)(GLhandleARB programObj, GLuint index, const GLcharARB *name);
extern void (APIENTRYP gglGetActiveAttribARB)(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
extern GLint (APIENTRYP gglGetAttribLocationARB)(GLhandleARB programObj, const GLcharARB *name);
/* reuse void (APIENTRYP gglGetVertexAttribdvARB)(GLuint index, GLenum pname, GLdouble *params) */
/* reuse void (APIENTRYP gglGetVertexAttribfvARB)(GLuint index, GLenum pname, GLfloat *params) */
/* reuse void (APIENTRYP gglGetVertexAttribivARB)(GLuint index, GLenum pname, GLint *params) */
/* reuse void (APIENTRYP gglGetVertexAttribPointervARB)(GLuint index, GLenum pname, void **pointer) */

#ifndef GL_ARB_vertex_type_10f_11f_11f_rev
#define GL_ARB_vertex_type_10f_11f_11f_rev
/* reuse GL_UNSIGNED_INT_10F_11F_11F_REV */
#endif

#ifndef GL_ARB_vertex_type_2_10_10_10_rev
#define GL_ARB_vertex_type_2_10_10_10_rev
/* reuse GL_UNSIGNED_INT_2_10_10_10_REV */
#define GL_INT_2_10_10_10_REV 0x8D9F
#endif
extern void (APIENTRYP gglVertexAttribP1ui)(GLuint index, GLenum type, GLboolean normalized, GLuint value);
extern void (APIENTRYP gglVertexAttribP1uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint *value);
extern void (APIENTRYP gglVertexAttribP2ui)(GLuint index, GLenum type, GLboolean normalized, GLuint value);
extern void (APIENTRYP gglVertexAttribP2uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint *value);
extern void (APIENTRYP gglVertexAttribP3ui)(GLuint index, GLenum type, GLboolean normalized, GLuint value);
extern void (APIENTRYP gglVertexAttribP3uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint *value);
extern void (APIENTRYP gglVertexAttribP4ui)(GLuint index, GLenum type, GLboolean normalized, GLuint value);
extern void (APIENTRYP gglVertexAttribP4uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint *value);
extern void (APIENTRYP gglVertexP2ui)(GLenum type, GLuint value);
extern void (APIENTRYP gglVertexP2uiv)(GLenum type, const GLuint *value);
extern void (APIENTRYP gglVertexP3ui)(GLenum type, GLuint value);
extern void (APIENTRYP gglVertexP3uiv)(GLenum type, const GLuint *value);
extern void (APIENTRYP gglVertexP4ui)(GLenum type, GLuint value);
extern void (APIENTRYP gglVertexP4uiv)(GLenum type, const GLuint *value);
extern void (APIENTRYP gglTexCoordP1ui)(GLenum type, GLuint coords);
extern void (APIENTRYP gglTexCoordP1uiv)(GLenum type, const GLuint *coords);
extern void (APIENTRYP gglTexCoordP2ui)(GLenum type, GLuint coords);
extern void (APIENTRYP gglTexCoordP2uiv)(GLenum type, const GLuint *coords);
extern void (APIENTRYP gglTexCoordP3ui)(GLenum type, GLuint coords);
extern void (APIENTRYP gglTexCoordP3uiv)(GLenum type, const GLuint *coords);
extern void (APIENTRYP gglTexCoordP4ui)(GLenum type, GLuint coords);
extern void (APIENTRYP gglTexCoordP4uiv)(GLenum type, const GLuint *coords);
extern void (APIENTRYP gglMultiTexCoordP1ui)(GLenum texture, GLenum type, GLuint coords);
extern void (APIENTRYP gglMultiTexCoordP1uiv)(GLenum texture, GLenum type, const GLuint *coords);
extern void (APIENTRYP gglMultiTexCoordP2ui)(GLenum texture, GLenum type, GLuint coords);
extern void (APIENTRYP gglMultiTexCoordP2uiv)(GLenum texture, GLenum type, const GLuint *coords);
extern void (APIENTRYP gglMultiTexCoordP3ui)(GLenum texture, GLenum type, GLuint coords);
extern void (APIENTRYP gglMultiTexCoordP3uiv)(GLenum texture, GLenum type, const GLuint *coords);
extern void (APIENTRYP gglMultiTexCoordP4ui)(GLenum texture, GLenum type, GLuint coords);
extern void (APIENTRYP gglMultiTexCoordP4uiv)(GLenum texture, GLenum type, const GLuint *coords);
extern void (APIENTRYP gglNormalP3ui)(GLenum type, GLuint coords);
extern void (APIENTRYP gglNormalP3uiv)(GLenum type, const GLuint *coords);
extern void (APIENTRYP gglColorP3ui)(GLenum type, GLuint color);
extern void (APIENTRYP gglColorP3uiv)(GLenum type, const GLuint *color);
extern void (APIENTRYP gglColorP4ui)(GLenum type, GLuint color);
extern void (APIENTRYP gglColorP4uiv)(GLenum type, const GLuint *color);
extern void (APIENTRYP gglSecondaryColorP3ui)(GLenum type, GLuint color);
extern void (APIENTRYP gglSecondaryColorP3uiv)(GLenum type, const GLuint *color);

#ifndef GL_ARB_viewport_array
#define GL_ARB_viewport_array
/* reuse GL_SCISSOR_BOX */
/* reuse GL_VIEWPORT */
/* reuse GL_DEPTH_RANGE */
/* reuse GL_SCISSOR_TEST */
#define GL_MAX_VIEWPORTS 0x825B
#define GL_VIEWPORT_SUBPIXEL_BITS 0x825C
#define GL_VIEWPORT_BOUNDS_RANGE 0x825D
#define GL_LAYER_PROVOKING_VERTEX 0x825E
#define GL_VIEWPORT_INDEX_PROVOKING_VERTEX 0x825F
#define GL_UNDEFINED_VERTEX 0x8260
/* reuse GL_FIRST_VERTEX_CONVENTION */
/* reuse GL_LAST_VERTEX_CONVENTION */
/* reuse GL_PROVOKING_VERTEX */
#endif
extern void (APIENTRYP gglViewportArrayv)(GLuint first, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglViewportIndexedf)(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h);
extern void (APIENTRYP gglViewportIndexedfv)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglScissorArrayv)(GLuint first, GLsizei count, const GLint *v);
extern void (APIENTRYP gglScissorIndexed)(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height);
extern void (APIENTRYP gglScissorIndexedv)(GLuint index, const GLint *v);
extern void (APIENTRYP gglDepthRangeArrayv)(GLuint first, GLsizei count, const GLdouble *v);
extern void (APIENTRYP gglDepthRangeIndexed)(GLuint index, GLdouble n, GLdouble f);
extern void (APIENTRYP gglGetFloati_v)(GLenum target, GLuint index, GLfloat *data);
extern void (APIENTRYP gglGetDoublei_v)(GLenum target, GLuint index, GLdouble *data);

#ifndef GL_ARB_window_pos
#define GL_ARB_window_pos
#endif
extern void (APIENTRYP gglWindowPos2dARB)(GLdouble x, GLdouble y);
extern void (APIENTRYP gglWindowPos2dvARB)(const GLdouble *v);
extern void (APIENTRYP gglWindowPos2fARB)(GLfloat x, GLfloat y);
extern void (APIENTRYP gglWindowPos2fvARB)(const GLfloat *v);
extern void (APIENTRYP gglWindowPos2iARB)(GLint x, GLint y);
extern void (APIENTRYP gglWindowPos2ivARB)(const GLint *v);
extern void (APIENTRYP gglWindowPos2sARB)(GLshort x, GLshort y);
extern void (APIENTRYP gglWindowPos2svARB)(const GLshort *v);
extern void (APIENTRYP gglWindowPos3dARB)(GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglWindowPos3dvARB)(const GLdouble *v);
extern void (APIENTRYP gglWindowPos3fARB)(GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglWindowPos3fvARB)(const GLfloat *v);
extern void (APIENTRYP gglWindowPos3iARB)(GLint x, GLint y, GLint z);
extern void (APIENTRYP gglWindowPos3ivARB)(const GLint *v);
extern void (APIENTRYP gglWindowPos3sARB)(GLshort x, GLshort y, GLshort z);
extern void (APIENTRYP gglWindowPos3svARB)(const GLshort *v);

#ifndef GL_ATI_draw_buffers
#define GL_ATI_draw_buffers
#define GL_MAX_DRAW_BUFFERS_ATI 0x8824
#define GL_DRAW_BUFFER0_ATI 0x8825
#define GL_DRAW_BUFFER1_ATI 0x8826
#define GL_DRAW_BUFFER2_ATI 0x8827
#define GL_DRAW_BUFFER3_ATI 0x8828
#define GL_DRAW_BUFFER4_ATI 0x8829
#define GL_DRAW_BUFFER5_ATI 0x882A
#define GL_DRAW_BUFFER6_ATI 0x882B
#define GL_DRAW_BUFFER7_ATI 0x882C
#define GL_DRAW_BUFFER8_ATI 0x882D
#define GL_DRAW_BUFFER9_ATI 0x882E
#define GL_DRAW_BUFFER10_ATI 0x882F
#define GL_DRAW_BUFFER11_ATI 0x8830
#define GL_DRAW_BUFFER12_ATI 0x8831
#define GL_DRAW_BUFFER13_ATI 0x8832
#define GL_DRAW_BUFFER14_ATI 0x8833
#define GL_DRAW_BUFFER15_ATI 0x8834
#endif
extern void (APIENTRYP gglDrawBuffersATI)(GLsizei n, const GLenum *bufs);

#ifndef GL_ATI_element_array
#define GL_ATI_element_array
#define GL_ELEMENT_ARRAY_ATI 0x8768
#define GL_ELEMENT_ARRAY_TYPE_ATI 0x8769
#define GL_ELEMENT_ARRAY_POINTER_ATI 0x876A
#endif
extern void (APIENTRYP gglElementPointerATI)(GLenum type, const void *pointer);
extern void (APIENTRYP gglDrawElementArrayATI)(GLenum mode, GLsizei count);
extern void (APIENTRYP gglDrawRangeElementArrayATI)(GLenum mode, GLuint start, GLuint end, GLsizei count);

#ifndef GL_ATI_envmap_bumpmap
#define GL_ATI_envmap_bumpmap
#define GL_BUMP_ROT_MATRIX_ATI 0x8775
#define GL_BUMP_ROT_MATRIX_SIZE_ATI 0x8776
#define GL_BUMP_NUM_TEX_UNITS_ATI 0x8777
#define GL_BUMP_TEX_UNITS_ATI 0x8778
#define GL_DUDV_ATI 0x8779
#define GL_DU8DV8_ATI 0x877A
#define GL_BUMP_ENVMAP_ATI 0x877B
#define GL_BUMP_TARGET_ATI 0x877C
#endif
extern void (APIENTRYP gglTexBumpParameterivATI)(GLenum pname, const GLint *param);
extern void (APIENTRYP gglTexBumpParameterfvATI)(GLenum pname, const GLfloat *param);
extern void (APIENTRYP gglGetTexBumpParameterivATI)(GLenum pname, GLint *param);
extern void (APIENTRYP gglGetTexBumpParameterfvATI)(GLenum pname, GLfloat *param);

#ifndef GL_ATI_fragment_shader
#define GL_ATI_fragment_shader
#define GL_FRAGMENT_SHADER_ATI 0x8920
#define GL_REG_0_ATI 0x8921
#define GL_REG_1_ATI 0x8922
#define GL_REG_2_ATI 0x8923
#define GL_REG_3_ATI 0x8924
#define GL_REG_4_ATI 0x8925
#define GL_REG_5_ATI 0x8926
#define GL_REG_6_ATI 0x8927
#define GL_REG_7_ATI 0x8928
#define GL_REG_8_ATI 0x8929
#define GL_REG_9_ATI 0x892A
#define GL_REG_10_ATI 0x892B
#define GL_REG_11_ATI 0x892C
#define GL_REG_12_ATI 0x892D
#define GL_REG_13_ATI 0x892E
#define GL_REG_14_ATI 0x892F
#define GL_REG_15_ATI 0x8930
#define GL_REG_16_ATI 0x8931
#define GL_REG_17_ATI 0x8932
#define GL_REG_18_ATI 0x8933
#define GL_REG_19_ATI 0x8934
#define GL_REG_20_ATI 0x8935
#define GL_REG_21_ATI 0x8936
#define GL_REG_22_ATI 0x8937
#define GL_REG_23_ATI 0x8938
#define GL_REG_24_ATI 0x8939
#define GL_REG_25_ATI 0x893A
#define GL_REG_26_ATI 0x893B
#define GL_REG_27_ATI 0x893C
#define GL_REG_28_ATI 0x893D
#define GL_REG_29_ATI 0x893E
#define GL_REG_30_ATI 0x893F
#define GL_REG_31_ATI 0x8940
#define GL_CON_0_ATI 0x8941
#define GL_CON_1_ATI 0x8942
#define GL_CON_2_ATI 0x8943
#define GL_CON_3_ATI 0x8944
#define GL_CON_4_ATI 0x8945
#define GL_CON_5_ATI 0x8946
#define GL_CON_6_ATI 0x8947
#define GL_CON_7_ATI 0x8948
#define GL_CON_8_ATI 0x8949
#define GL_CON_9_ATI 0x894A
#define GL_CON_10_ATI 0x894B
#define GL_CON_11_ATI 0x894C
#define GL_CON_12_ATI 0x894D
#define GL_CON_13_ATI 0x894E
#define GL_CON_14_ATI 0x894F
#define GL_CON_15_ATI 0x8950
#define GL_CON_16_ATI 0x8951
#define GL_CON_17_ATI 0x8952
#define GL_CON_18_ATI 0x8953
#define GL_CON_19_ATI 0x8954
#define GL_CON_20_ATI 0x8955
#define GL_CON_21_ATI 0x8956
#define GL_CON_22_ATI 0x8957
#define GL_CON_23_ATI 0x8958
#define GL_CON_24_ATI 0x8959
#define GL_CON_25_ATI 0x895A
#define GL_CON_26_ATI 0x895B
#define GL_CON_27_ATI 0x895C
#define GL_CON_28_ATI 0x895D
#define GL_CON_29_ATI 0x895E
#define GL_CON_30_ATI 0x895F
#define GL_CON_31_ATI 0x8960
#define GL_MOV_ATI 0x8961
#define GL_ADD_ATI 0x8963
#define GL_MUL_ATI 0x8964
#define GL_SUB_ATI 0x8965
#define GL_DOT3_ATI 0x8966
#define GL_DOT4_ATI 0x8967
#define GL_MAD_ATI 0x8968
#define GL_LERP_ATI 0x8969
#define GL_CND_ATI 0x896A
#define GL_CND0_ATI 0x896B
#define GL_DOT2_ADD_ATI 0x896C
#define GL_SECONDARY_INTERPOLATOR_ATI 0x896D
#define GL_NUM_FRAGMENT_REGISTERS_ATI 0x896E
#define GL_NUM_FRAGMENT_CONSTANTS_ATI 0x896F
#define GL_NUM_PASSES_ATI 0x8970
#define GL_NUM_INSTRUCTIONS_PER_PASS_ATI 0x8971
#define GL_NUM_INSTRUCTIONS_TOTAL_ATI 0x8972
#define GL_NUM_INPUT_INTERPOLATOR_COMPONENTS_ATI 0x8973
#define GL_NUM_LOOPBACK_COMPONENTS_ATI 0x8974
#define GL_COLOR_ALPHA_PAIRING_ATI 0x8975
#define GL_SWIZZLE_STR_ATI 0x8976
#define GL_SWIZZLE_STQ_ATI 0x8977
#define GL_SWIZZLE_STR_DR_ATI 0x8978
#define GL_SWIZZLE_STQ_DQ_ATI 0x8979
#define GL_SWIZZLE_STRQ_ATI 0x897A
#define GL_SWIZZLE_STRQ_DQ_ATI 0x897B
#define GL_RED_BIT_ATI 0x00000001
#define GL_GREEN_BIT_ATI 0x00000002
#define GL_BLUE_BIT_ATI 0x00000004
#define GL_2X_BIT_ATI 0x00000001
#define GL_4X_BIT_ATI 0x00000002
#define GL_8X_BIT_ATI 0x00000004
#define GL_HALF_BIT_ATI 0x00000008
#define GL_QUARTER_BIT_ATI 0x00000010
#define GL_EIGHTH_BIT_ATI 0x00000020
#define GL_SATURATE_BIT_ATI 0x00000040
#define GL_COMP_BIT_ATI 0x00000002
#define GL_NEGATE_BIT_ATI 0x00000004
#define GL_BIAS_BIT_ATI 0x00000008
#endif
extern GLuint (APIENTRYP gglGenFragmentShadersATI)(GLuint range);
extern void (APIENTRYP gglBindFragmentShaderATI)(GLuint id);
extern void (APIENTRYP gglDeleteFragmentShaderATI)(GLuint id);
extern void (APIENTRYP gglBeginFragmentShaderATI)();
extern void (APIENTRYP gglEndFragmentShaderATI)();
extern void (APIENTRYP gglPassTexCoordATI)(GLuint dst, GLuint coord, GLenum swizzle);
extern void (APIENTRYP gglSampleMapATI)(GLuint dst, GLuint interp, GLenum swizzle);
extern void (APIENTRYP gglColorFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
extern void (APIENTRYP gglColorFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
extern void (APIENTRYP gglColorFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
extern void (APIENTRYP gglAlphaFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
extern void (APIENTRYP gglAlphaFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
extern void (APIENTRYP gglAlphaFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
extern void (APIENTRYP gglSetFragmentShaderConstantATI)(GLuint dst, const GLfloat *value);

#ifndef GL_ATI_map_object_buffer
#define GL_ATI_map_object_buffer
#endif
extern void * (APIENTRYP gglMapObjectBufferATI)(GLuint buffer);
extern void (APIENTRYP gglUnmapObjectBufferATI)(GLuint buffer);

#ifndef GL_ATI_meminfo
#define GL_ATI_meminfo
#define GL_VBO_FREE_MEMORY_ATI 0x87FB
#define GL_TEXTURE_FREE_MEMORY_ATI 0x87FC
#define GL_RENDERBUFFER_FREE_MEMORY_ATI 0x87FD
#endif

#ifndef GL_ATI_pixel_format_float
#define GL_ATI_pixel_format_float
#define GL_RGBA_FLOAT_MODE_ATI 0x8820
#define GL_COLOR_CLEAR_UNCLAMPED_VALUE_ATI 0x8835
#endif

#ifndef GL_ATI_pn_triangles
#define GL_ATI_pn_triangles
#define GL_PN_TRIANGLES_ATI 0x87F0
#define GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI 0x87F1
#define GL_PN_TRIANGLES_POINT_MODE_ATI 0x87F2
#define GL_PN_TRIANGLES_NORMAL_MODE_ATI 0x87F3
#define GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI 0x87F4
#define GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATI 0x87F5
#define GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI 0x87F6
#define GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI 0x87F7
#define GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI 0x87F8
#endif
extern void (APIENTRYP gglPNTrianglesiATI)(GLenum pname, GLint param);
extern void (APIENTRYP gglPNTrianglesfATI)(GLenum pname, GLfloat param);

#ifndef GL_ATI_separate_stencil
#define GL_ATI_separate_stencil
#define GL_STENCIL_BACK_FUNC_ATI 0x8800
#define GL_STENCIL_BACK_FAIL_ATI 0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL_ATI 0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS_ATI 0x8803
#endif
extern void (APIENTRYP gglStencilOpSeparateATI)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
extern void (APIENTRYP gglStencilFuncSeparateATI)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);

#ifndef GL_ATI_text_fragment_shader
#define GL_ATI_text_fragment_shader
#define GL_TEXT_FRAGMENT_SHADER_ATI 0x8200
#endif

#ifndef GL_ATI_texture_env_combine3
#define GL_ATI_texture_env_combine3
#define GL_MODULATE_ADD_ATI 0x8744
#define GL_MODULATE_SIGNED_ADD_ATI 0x8745
#define GL_MODULATE_SUBTRACT_ATI 0x8746
#endif

#ifndef GL_ATI_texture_float
#define GL_ATI_texture_float
#define GL_RGBA_FLOAT32_ATI 0x8814
#define GL_RGB_FLOAT32_ATI 0x8815
#define GL_ALPHA_FLOAT32_ATI 0x8816
#define GL_INTENSITY_FLOAT32_ATI 0x8817
#define GL_LUMINANCE_FLOAT32_ATI 0x8818
#define GL_LUMINANCE_ALPHA_FLOAT32_ATI 0x8819
#define GL_RGBA_FLOAT16_ATI 0x881A
#define GL_RGB_FLOAT16_ATI 0x881B
#define GL_ALPHA_FLOAT16_ATI 0x881C
#define GL_INTENSITY_FLOAT16_ATI 0x881D
#define GL_LUMINANCE_FLOAT16_ATI 0x881E
#define GL_LUMINANCE_ALPHA_FLOAT16_ATI 0x881F
#endif

#ifndef GL_ATI_texture_mirror_once
#define GL_ATI_texture_mirror_once
#define GL_MIRROR_CLAMP_ATI 0x8742
#define GL_MIRROR_CLAMP_TO_EDGE_ATI 0x8743
#endif

#ifndef GL_ATI_vertex_array_object
#define GL_ATI_vertex_array_object
#define GL_STATIC_ATI 0x8760
#define GL_DYNAMIC_ATI 0x8761
#define GL_PRESERVE_ATI 0x8762
#define GL_DISCARD_ATI 0x8763
#define GL_OBJECT_BUFFER_SIZE_ATI 0x8764
#define GL_OBJECT_BUFFER_USAGE_ATI 0x8765
#define GL_ARRAY_OBJECT_BUFFER_ATI 0x8766
#define GL_ARRAY_OBJECT_OFFSET_ATI 0x8767
#endif
extern GLuint (APIENTRYP gglNewObjectBufferATI)(GLsizei size, const void *pointer, GLenum usage);
extern GLboolean (APIENTRYP gglIsObjectBufferATI)(GLuint buffer);
extern void (APIENTRYP gglUpdateObjectBufferATI)(GLuint buffer, GLuint offset, GLsizei size, const void *pointer, GLenum preserve);
extern void (APIENTRYP gglGetObjectBufferfvATI)(GLuint buffer, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetObjectBufferivATI)(GLuint buffer, GLenum pname, GLint *params);
extern void (APIENTRYP gglFreeObjectBufferATI)(GLuint buffer);
extern void (APIENTRYP gglArrayObjectATI)(GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset);
extern void (APIENTRYP gglGetArrayObjectfvATI)(GLenum array, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetArrayObjectivATI)(GLenum array, GLenum pname, GLint *params);
extern void (APIENTRYP gglVariantArrayObjectATI)(GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset);
extern void (APIENTRYP gglGetVariantArrayObjectfvATI)(GLuint id, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetVariantArrayObjectivATI)(GLuint id, GLenum pname, GLint *params);

#ifndef GL_ATI_vertex_attrib_array_object
#define GL_ATI_vertex_attrib_array_object
#endif
extern void (APIENTRYP gglVertexAttribArrayObjectATI)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint buffer, GLuint offset);
extern void (APIENTRYP gglGetVertexAttribArrayObjectfvATI)(GLuint index, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetVertexAttribArrayObjectivATI)(GLuint index, GLenum pname, GLint *params);

#ifndef GL_ATI_vertex_streams
#define GL_ATI_vertex_streams
#define GL_MAX_VERTEX_STREAMS_ATI 0x876B
#define GL_VERTEX_STREAM0_ATI 0x876C
#define GL_VERTEX_STREAM1_ATI 0x876D
#define GL_VERTEX_STREAM2_ATI 0x876E
#define GL_VERTEX_STREAM3_ATI 0x876F
#define GL_VERTEX_STREAM4_ATI 0x8770
#define GL_VERTEX_STREAM5_ATI 0x8771
#define GL_VERTEX_STREAM6_ATI 0x8772
#define GL_VERTEX_STREAM7_ATI 0x8773
#define GL_VERTEX_SOURCE_ATI 0x8774
#endif
extern void (APIENTRYP gglVertexStream1sATI)(GLenum stream, GLshort x);
extern void (APIENTRYP gglVertexStream1svATI)(GLenum stream, const GLshort *coords);
extern void (APIENTRYP gglVertexStream1iATI)(GLenum stream, GLint x);
extern void (APIENTRYP gglVertexStream1ivATI)(GLenum stream, const GLint *coords);
extern void (APIENTRYP gglVertexStream1fATI)(GLenum stream, GLfloat x);
extern void (APIENTRYP gglVertexStream1fvATI)(GLenum stream, const GLfloat *coords);
extern void (APIENTRYP gglVertexStream1dATI)(GLenum stream, GLdouble x);
extern void (APIENTRYP gglVertexStream1dvATI)(GLenum stream, const GLdouble *coords);
extern void (APIENTRYP gglVertexStream2sATI)(GLenum stream, GLshort x, GLshort y);
extern void (APIENTRYP gglVertexStream2svATI)(GLenum stream, const GLshort *coords);
extern void (APIENTRYP gglVertexStream2iATI)(GLenum stream, GLint x, GLint y);
extern void (APIENTRYP gglVertexStream2ivATI)(GLenum stream, const GLint *coords);
extern void (APIENTRYP gglVertexStream2fATI)(GLenum stream, GLfloat x, GLfloat y);
extern void (APIENTRYP gglVertexStream2fvATI)(GLenum stream, const GLfloat *coords);
extern void (APIENTRYP gglVertexStream2dATI)(GLenum stream, GLdouble x, GLdouble y);
extern void (APIENTRYP gglVertexStream2dvATI)(GLenum stream, const GLdouble *coords);
extern void (APIENTRYP gglVertexStream3sATI)(GLenum stream, GLshort x, GLshort y, GLshort z);
extern void (APIENTRYP gglVertexStream3svATI)(GLenum stream, const GLshort *coords);
extern void (APIENTRYP gglVertexStream3iATI)(GLenum stream, GLint x, GLint y, GLint z);
extern void (APIENTRYP gglVertexStream3ivATI)(GLenum stream, const GLint *coords);
extern void (APIENTRYP gglVertexStream3fATI)(GLenum stream, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglVertexStream3fvATI)(GLenum stream, const GLfloat *coords);
extern void (APIENTRYP gglVertexStream3dATI)(GLenum stream, GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglVertexStream3dvATI)(GLenum stream, const GLdouble *coords);
extern void (APIENTRYP gglVertexStream4sATI)(GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w);
extern void (APIENTRYP gglVertexStream4svATI)(GLenum stream, const GLshort *coords);
extern void (APIENTRYP gglVertexStream4iATI)(GLenum stream, GLint x, GLint y, GLint z, GLint w);
extern void (APIENTRYP gglVertexStream4ivATI)(GLenum stream, const GLint *coords);
extern void (APIENTRYP gglVertexStream4fATI)(GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglVertexStream4fvATI)(GLenum stream, const GLfloat *coords);
extern void (APIENTRYP gglVertexStream4dATI)(GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglVertexStream4dvATI)(GLenum stream, const GLdouble *coords);
extern void (APIENTRYP gglNormalStream3bATI)(GLenum stream, GLbyte nx, GLbyte ny, GLbyte nz);
extern void (APIENTRYP gglNormalStream3bvATI)(GLenum stream, const GLbyte *coords);
extern void (APIENTRYP gglNormalStream3sATI)(GLenum stream, GLshort nx, GLshort ny, GLshort nz);
extern void (APIENTRYP gglNormalStream3svATI)(GLenum stream, const GLshort *coords);
extern void (APIENTRYP gglNormalStream3iATI)(GLenum stream, GLint nx, GLint ny, GLint nz);
extern void (APIENTRYP gglNormalStream3ivATI)(GLenum stream, const GLint *coords);
extern void (APIENTRYP gglNormalStream3fATI)(GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz);
extern void (APIENTRYP gglNormalStream3fvATI)(GLenum stream, const GLfloat *coords);
extern void (APIENTRYP gglNormalStream3dATI)(GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz);
extern void (APIENTRYP gglNormalStream3dvATI)(GLenum stream, const GLdouble *coords);
extern void (APIENTRYP gglClientActiveVertexStreamATI)(GLenum stream);
extern void (APIENTRYP gglVertexBlendEnviATI)(GLenum pname, GLint param);
extern void (APIENTRYP gglVertexBlendEnvfATI)(GLenum pname, GLfloat param);

#ifndef GL_EXT_422_pixels
#define GL_EXT_422_pixels
#define GL_422_EXT 0x80CC
#define GL_422_REV_EXT 0x80CD
#define GL_422_AVERAGE_EXT 0x80CE
#define GL_422_REV_AVERAGE_EXT 0x80CF
#endif

#ifndef GL_EXT_abgr
#define GL_EXT_abgr
#define GL_ABGR_EXT 0x8000
#endif

#ifndef GL_EXT_bgra
#define GL_EXT_bgra
#define GL_BGR_EXT 0x80E0
#define GL_BGRA_EXT 0x80E1
#endif

#ifndef GL_EXT_bindable_uniform
#define GL_EXT_bindable_uniform
#define GL_MAX_VERTEX_BINDABLE_UNIFORMS_EXT 0x8DE2
#define GL_MAX_FRAGMENT_BINDABLE_UNIFORMS_EXT 0x8DE3
#define GL_MAX_GEOMETRY_BINDABLE_UNIFORMS_EXT 0x8DE4
#define GL_MAX_BINDABLE_UNIFORM_SIZE_EXT 0x8DED
#define GL_UNIFORM_BUFFER_EXT 0x8DEE
#define GL_UNIFORM_BUFFER_BINDING_EXT 0x8DEF
#endif
extern void (APIENTRYP gglUniformBufferEXT)(GLuint program, GLint location, GLuint buffer);
extern GLint (APIENTRYP gglGetUniformBufferSizeEXT)(GLuint program, GLint location);
extern GLintptr (APIENTRYP gglGetUniformOffsetEXT)(GLuint program, GLint location);

#ifndef GL_EXT_blend_color
#define GL_EXT_blend_color
#define GL_CONSTANT_COLOR_EXT 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR_EXT 0x8002
#define GL_CONSTANT_ALPHA_EXT 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA_EXT 0x8004
#define GL_BLEND_COLOR_EXT 0x8005
#endif
extern void (APIENTRYP gglBlendColorEXT)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);

#ifndef GL_EXT_blend_equation_separate
#define GL_EXT_blend_equation_separate
#define GL_BLEND_EQUATION_RGB_EXT 0x8009
#define GL_BLEND_EQUATION_ALPHA_EXT 0x883D
#endif
extern void (APIENTRYP gglBlendEquationSeparateEXT)(GLenum modeRGB, GLenum modeAlpha);

#ifndef GL_EXT_blend_func_separate
#define GL_EXT_blend_func_separate
#define GL_BLEND_DST_RGB_EXT 0x80C8
#define GL_BLEND_SRC_RGB_EXT 0x80C9
#define GL_BLEND_DST_ALPHA_EXT 0x80CA
#define GL_BLEND_SRC_ALPHA_EXT 0x80CB
#endif
extern void (APIENTRYP gglBlendFuncSeparateEXT)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

#ifndef GL_EXT_blend_logic_op
#define GL_EXT_blend_logic_op
#endif

#ifndef GL_EXT_blend_minmax
#define GL_EXT_blend_minmax
#define GL_MIN_EXT 0x8007
#define GL_MAX_EXT 0x8008
#define GL_FUNC_ADD_EXT 0x8006
#define GL_BLEND_EQUATION_EXT 0x8009
#endif
extern void (APIENTRYP gglBlendEquationEXT)(GLenum mode);

#ifndef GL_EXT_blend_subtract
#define GL_EXT_blend_subtract
#define GL_FUNC_SUBTRACT_EXT 0x800A
#define GL_FUNC_REVERSE_SUBTRACT_EXT 0x800B
#endif

#ifndef GL_EXT_clip_volume_hint
#define GL_EXT_clip_volume_hint
#define GL_CLIP_VOLUME_CLIPPING_HINT_EXT 0x80F0
#endif

#ifndef GL_EXT_cmyka
#define GL_EXT_cmyka
#define GL_CMYK_EXT 0x800C
#define GL_CMYKA_EXT 0x800D
#define GL_PACK_CMYK_HINT_EXT 0x800E
#define GL_UNPACK_CMYK_HINT_EXT 0x800F
#endif

#ifndef GL_EXT_color_subtable
#define GL_EXT_color_subtable
#endif
extern void (APIENTRYP gglColorSubTableEXT)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data);
extern void (APIENTRYP gglCopyColorSubTableEXT)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);

#ifndef GL_EXT_compiled_vertex_array
#define GL_EXT_compiled_vertex_array
#define GL_ARRAY_ELEMENT_LOCK_FIRST_EXT 0x81A8
#define GL_ARRAY_ELEMENT_LOCK_COUNT_EXT 0x81A9
#endif
extern void (APIENTRYP gglLockArraysEXT)(GLint first, GLsizei count);
extern void (APIENTRYP gglUnlockArraysEXT)();

#ifndef GL_EXT_convolution
#define GL_EXT_convolution
#define GL_CONVOLUTION_1D_EXT 0x8010
#define GL_CONVOLUTION_2D_EXT 0x8011
#define GL_SEPARABLE_2D_EXT 0x8012
#define GL_CONVOLUTION_BORDER_MODE_EXT 0x8013
#define GL_CONVOLUTION_FILTER_SCALE_EXT 0x8014
#define GL_CONVOLUTION_FILTER_BIAS_EXT 0x8015
#define GL_REDUCE_EXT 0x8016
#define GL_CONVOLUTION_FORMAT_EXT 0x8017
#define GL_CONVOLUTION_WIDTH_EXT 0x8018
#define GL_CONVOLUTION_HEIGHT_EXT 0x8019
#define GL_MAX_CONVOLUTION_WIDTH_EXT 0x801A
#define GL_MAX_CONVOLUTION_HEIGHT_EXT 0x801B
#define GL_POST_CONVOLUTION_RED_SCALE_EXT 0x801C
#define GL_POST_CONVOLUTION_GREEN_SCALE_EXT 0x801D
#define GL_POST_CONVOLUTION_BLUE_SCALE_EXT 0x801E
#define GL_POST_CONVOLUTION_ALPHA_SCALE_EXT 0x801F
#define GL_POST_CONVOLUTION_RED_BIAS_EXT 0x8020
#define GL_POST_CONVOLUTION_GREEN_BIAS_EXT 0x8021
#define GL_POST_CONVOLUTION_BLUE_BIAS_EXT 0x8022
#define GL_POST_CONVOLUTION_ALPHA_BIAS_EXT 0x8023
#endif
extern void (APIENTRYP gglConvolutionFilter1DEXT)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image);
extern void (APIENTRYP gglConvolutionFilter2DEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image);
extern void (APIENTRYP gglConvolutionParameterfEXT)(GLenum target, GLenum pname, GLfloat params);
extern void (APIENTRYP gglConvolutionParameterfvEXT)(GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglConvolutionParameteriEXT)(GLenum target, GLenum pname, GLint params);
extern void (APIENTRYP gglConvolutionParameterivEXT)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglCopyConvolutionFilter1DEXT)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
extern void (APIENTRYP gglCopyConvolutionFilter2DEXT)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglGetConvolutionFilterEXT)(GLenum target, GLenum format, GLenum type, void *image);
extern void (APIENTRYP gglGetConvolutionParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetConvolutionParameterivEXT)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetSeparableFilterEXT)(GLenum target, GLenum format, GLenum type, void *row, void *column, void *span);
extern void (APIENTRYP gglSeparableFilter2DEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column);

#ifndef GL_EXT_coordinate_frame
#define GL_EXT_coordinate_frame
#define GL_TANGENT_ARRAY_EXT 0x8439
#define GL_BINORMAL_ARRAY_EXT 0x843A
#define GL_CURRENT_TANGENT_EXT 0x843B
#define GL_CURRENT_BINORMAL_EXT 0x843C
#define GL_TANGENT_ARRAY_TYPE_EXT 0x843E
#define GL_TANGENT_ARRAY_STRIDE_EXT 0x843F
#define GL_BINORMAL_ARRAY_TYPE_EXT 0x8440
#define GL_BINORMAL_ARRAY_STRIDE_EXT 0x8441
#define GL_TANGENT_ARRAY_POINTER_EXT 0x8442
#define GL_BINORMAL_ARRAY_POINTER_EXT 0x8443
#define GL_MAP1_TANGENT_EXT 0x8444
#define GL_MAP2_TANGENT_EXT 0x8445
#define GL_MAP1_BINORMAL_EXT 0x8446
#define GL_MAP2_BINORMAL_EXT 0x8447
#endif
extern void (APIENTRYP gglTangent3bEXT)(GLbyte tx, GLbyte ty, GLbyte tz);
extern void (APIENTRYP gglTangent3bvEXT)(const GLbyte *v);
extern void (APIENTRYP gglTangent3dEXT)(GLdouble tx, GLdouble ty, GLdouble tz);
extern void (APIENTRYP gglTangent3dvEXT)(const GLdouble *v);
extern void (APIENTRYP gglTangent3fEXT)(GLfloat tx, GLfloat ty, GLfloat tz);
extern void (APIENTRYP gglTangent3fvEXT)(const GLfloat *v);
extern void (APIENTRYP gglTangent3iEXT)(GLint tx, GLint ty, GLint tz);
extern void (APIENTRYP gglTangent3ivEXT)(const GLint *v);
extern void (APIENTRYP gglTangent3sEXT)(GLshort tx, GLshort ty, GLshort tz);
extern void (APIENTRYP gglTangent3svEXT)(const GLshort *v);
extern void (APIENTRYP gglBinormal3bEXT)(GLbyte bx, GLbyte by, GLbyte bz);
extern void (APIENTRYP gglBinormal3bvEXT)(const GLbyte *v);
extern void (APIENTRYP gglBinormal3dEXT)(GLdouble bx, GLdouble by, GLdouble bz);
extern void (APIENTRYP gglBinormal3dvEXT)(const GLdouble *v);
extern void (APIENTRYP gglBinormal3fEXT)(GLfloat bx, GLfloat by, GLfloat bz);
extern void (APIENTRYP gglBinormal3fvEXT)(const GLfloat *v);
extern void (APIENTRYP gglBinormal3iEXT)(GLint bx, GLint by, GLint bz);
extern void (APIENTRYP gglBinormal3ivEXT)(const GLint *v);
extern void (APIENTRYP gglBinormal3sEXT)(GLshort bx, GLshort by, GLshort bz);
extern void (APIENTRYP gglBinormal3svEXT)(const GLshort *v);
extern void (APIENTRYP gglTangentPointerEXT)(GLenum type, GLsizei stride, const void *pointer);
extern void (APIENTRYP gglBinormalPointerEXT)(GLenum type, GLsizei stride, const void *pointer);

#ifndef GL_EXT_copy_texture
#define GL_EXT_copy_texture
#endif
extern void (APIENTRYP gglCopyTexImage1DEXT)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
extern void (APIENTRYP gglCopyTexImage2DEXT)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern void (APIENTRYP gglCopyTexSubImage1DEXT)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern void (APIENTRYP gglCopyTexSubImage2DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglCopyTexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);

#ifndef GL_EXT_cull_vertex
#define GL_EXT_cull_vertex
#define GL_CULL_VERTEX_EXT 0x81AA
#define GL_CULL_VERTEX_EYE_POSITION_EXT 0x81AB
#define GL_CULL_VERTEX_OBJECT_POSITION_EXT 0x81AC
#endif
extern void (APIENTRYP gglCullParameterdvEXT)(GLenum pname, GLdouble *params);
extern void (APIENTRYP gglCullParameterfvEXT)(GLenum pname, GLfloat *params);

#ifndef GL_EXT_debug_label
#define GL_EXT_debug_label
#define GL_PROGRAM_PIPELINE_OBJECT_EXT 0x8A4F
#define GL_PROGRAM_OBJECT_EXT 0x8B40
#define GL_SHADER_OBJECT_EXT 0x8B48
#define GL_BUFFER_OBJECT_EXT 0x9151
#define GL_QUERY_OBJECT_EXT 0x9153
#define GL_VERTEX_ARRAY_OBJECT_EXT 0x9154
#define GL_SAMPLER 0x82E6
/* reuse GL_TRANSFORM_FEEDBACK */
#endif
extern void (APIENTRYP gglLabelObjectEXT)(GLenum type, GLuint object, GLsizei length, const GLchar *label);
extern void (APIENTRYP gglGetObjectLabelEXT)(GLenum type, GLuint object, GLsizei bufSize, GLsizei *length, GLchar *label);

#ifndef GL_EXT_debug_marker
#define GL_EXT_debug_marker
#endif
extern void (APIENTRYP gglInsertEventMarkerEXT)(GLsizei length, const GLchar *marker);
extern void (APIENTRYP gglPushGroupMarkerEXT)(GLsizei length, const GLchar *marker);
extern void (APIENTRYP gglPopGroupMarkerEXT)();

#ifndef GL_EXT_depth_bounds_test
#define GL_EXT_depth_bounds_test
#define GL_DEPTH_BOUNDS_TEST_EXT 0x8890
#define GL_DEPTH_BOUNDS_EXT 0x8891
#endif
extern void (APIENTRYP gglDepthBoundsEXT)(GLclampd zmin, GLclampd zmax);

#ifndef GL_EXT_direct_state_access
#define GL_EXT_direct_state_access
#define GL_PROGRAM_MATRIX_EXT 0x8E2D
#define GL_TRANSPOSE_PROGRAM_MATRIX_EXT 0x8E2E
#define GL_PROGRAM_MATRIX_STACK_DEPTH_EXT 0x8E2F
#endif
extern void (APIENTRYP gglMatrixLoadfEXT)(GLenum mode, const GLfloat *m);
extern void (APIENTRYP gglMatrixLoaddEXT)(GLenum mode, const GLdouble *m);
extern void (APIENTRYP gglMatrixMultfEXT)(GLenum mode, const GLfloat *m);
extern void (APIENTRYP gglMatrixMultdEXT)(GLenum mode, const GLdouble *m);
extern void (APIENTRYP gglMatrixLoadIdentityEXT)(GLenum mode);
extern void (APIENTRYP gglMatrixRotatefEXT)(GLenum mode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglMatrixRotatedEXT)(GLenum mode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglMatrixScalefEXT)(GLenum mode, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglMatrixScaledEXT)(GLenum mode, GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglMatrixTranslatefEXT)(GLenum mode, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglMatrixTranslatedEXT)(GLenum mode, GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglMatrixFrustumEXT)(GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void (APIENTRYP gglMatrixOrthoEXT)(GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
extern void (APIENTRYP gglMatrixPopEXT)(GLenum mode);
extern void (APIENTRYP gglMatrixPushEXT)(GLenum mode);
extern void (APIENTRYP gglClientAttribDefaultEXT)(GLbitfield mask);
extern void (APIENTRYP gglPushClientAttribDefaultEXT)(GLbitfield mask);
extern void (APIENTRYP gglTextureParameterfEXT)(GLuint texture, GLenum target, GLenum pname, GLfloat param);
extern void (APIENTRYP gglTextureParameterfvEXT)(GLuint texture, GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglTextureParameteriEXT)(GLuint texture, GLenum target, GLenum pname, GLint param);
extern void (APIENTRYP gglTextureParameterivEXT)(GLuint texture, GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglTextureImage1DEXT)(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTextureImage2DEXT)(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTextureSubImage1DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTextureSubImage2DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglCopyTextureImage1DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
extern void (APIENTRYP gglCopyTextureImage2DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern void (APIENTRYP gglCopyTextureSubImage1DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern void (APIENTRYP gglCopyTextureSubImage2DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglGetTextureImageEXT)(GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void *pixels);
extern void (APIENTRYP gglGetTextureParameterfvEXT)(GLuint texture, GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetTextureParameterivEXT)(GLuint texture, GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetTextureLevelParameterfvEXT)(GLuint texture, GLenum target, GLint level, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetTextureLevelParameterivEXT)(GLuint texture, GLenum target, GLint level, GLenum pname, GLint *params);
extern void (APIENTRYP gglTextureImage3DEXT)(GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTextureSubImage3DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglCopyTextureSubImage3DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglBindMultiTextureEXT)(GLenum texunit, GLenum target, GLuint texture);
extern void (APIENTRYP gglMultiTexCoordPointerEXT)(GLenum texunit, GLint size, GLenum type, GLsizei stride, const void *pointer);
extern void (APIENTRYP gglMultiTexEnvfEXT)(GLenum texunit, GLenum target, GLenum pname, GLfloat param);
extern void (APIENTRYP gglMultiTexEnvfvEXT)(GLenum texunit, GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglMultiTexEnviEXT)(GLenum texunit, GLenum target, GLenum pname, GLint param);
extern void (APIENTRYP gglMultiTexEnvivEXT)(GLenum texunit, GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglMultiTexGendEXT)(GLenum texunit, GLenum coord, GLenum pname, GLdouble param);
extern void (APIENTRYP gglMultiTexGendvEXT)(GLenum texunit, GLenum coord, GLenum pname, const GLdouble *params);
extern void (APIENTRYP gglMultiTexGenfEXT)(GLenum texunit, GLenum coord, GLenum pname, GLfloat param);
extern void (APIENTRYP gglMultiTexGenfvEXT)(GLenum texunit, GLenum coord, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglMultiTexGeniEXT)(GLenum texunit, GLenum coord, GLenum pname, GLint param);
extern void (APIENTRYP gglMultiTexGenivEXT)(GLenum texunit, GLenum coord, GLenum pname, const GLint *params);
extern void (APIENTRYP gglGetMultiTexEnvfvEXT)(GLenum texunit, GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetMultiTexEnvivEXT)(GLenum texunit, GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetMultiTexGendvEXT)(GLenum texunit, GLenum coord, GLenum pname, GLdouble *params);
extern void (APIENTRYP gglGetMultiTexGenfvEXT)(GLenum texunit, GLenum coord, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetMultiTexGenivEXT)(GLenum texunit, GLenum coord, GLenum pname, GLint *params);
extern void (APIENTRYP gglMultiTexParameteriEXT)(GLenum texunit, GLenum target, GLenum pname, GLint param);
extern void (APIENTRYP gglMultiTexParameterivEXT)(GLenum texunit, GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglMultiTexParameterfEXT)(GLenum texunit, GLenum target, GLenum pname, GLfloat param);
extern void (APIENTRYP gglMultiTexParameterfvEXT)(GLenum texunit, GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglMultiTexImage1DEXT)(GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglMultiTexImage2DEXT)(GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglMultiTexSubImage1DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglMultiTexSubImage2DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglCopyMultiTexImage1DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
extern void (APIENTRYP gglCopyMultiTexImage2DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern void (APIENTRYP gglCopyMultiTexSubImage1DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern void (APIENTRYP gglCopyMultiTexSubImage2DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglGetMultiTexImageEXT)(GLenum texunit, GLenum target, GLint level, GLenum format, GLenum type, void *pixels);
extern void (APIENTRYP gglGetMultiTexParameterfvEXT)(GLenum texunit, GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetMultiTexParameterivEXT)(GLenum texunit, GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetMultiTexLevelParameterfvEXT)(GLenum texunit, GLenum target, GLint level, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetMultiTexLevelParameterivEXT)(GLenum texunit, GLenum target, GLint level, GLenum pname, GLint *params);
extern void (APIENTRYP gglMultiTexImage3DEXT)(GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglMultiTexSubImage3DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglCopyMultiTexSubImage3DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglEnableClientStateIndexedEXT)(GLenum array, GLuint index);
extern void (APIENTRYP gglDisableClientStateIndexedEXT)(GLenum array, GLuint index);
extern void (APIENTRYP gglGetFloatIndexedvEXT)(GLenum target, GLuint index, GLfloat *data);
extern void (APIENTRYP gglGetDoubleIndexedvEXT)(GLenum target, GLuint index, GLdouble *data);
extern void (APIENTRYP gglGetPointerIndexedvEXT)(GLenum target, GLuint index, void **data);
extern void (APIENTRYP gglEnableIndexedEXT)(GLenum target, GLuint index);
extern void (APIENTRYP gglDisableIndexedEXT)(GLenum target, GLuint index);
extern GLboolean (APIENTRYP gglIsEnabledIndexedEXT)(GLenum target, GLuint index);
extern void (APIENTRYP gglGetIntegerIndexedvEXT)(GLenum target, GLuint index, GLint *data);
extern void (APIENTRYP gglGetBooleanIndexedvEXT)(GLenum target, GLuint index, GLboolean *data);
extern void (APIENTRYP gglCompressedTextureImage3DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits);
extern void (APIENTRYP gglCompressedTextureImage2DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits);
extern void (APIENTRYP gglCompressedTextureImage1DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits);
extern void (APIENTRYP gglCompressedTextureSubImage3DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits);
extern void (APIENTRYP gglCompressedTextureSubImage2DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits);
extern void (APIENTRYP gglCompressedTextureSubImage1DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits);
extern void (APIENTRYP gglGetCompressedTextureImageEXT)(GLuint texture, GLenum target, GLint lod, void *img);
extern void (APIENTRYP gglCompressedMultiTexImage3DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits);
extern void (APIENTRYP gglCompressedMultiTexImage2DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits);
extern void (APIENTRYP gglCompressedMultiTexImage1DEXT)(GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits);
extern void (APIENTRYP gglCompressedMultiTexSubImage3DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits);
extern void (APIENTRYP gglCompressedMultiTexSubImage2DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits);
extern void (APIENTRYP gglCompressedMultiTexSubImage1DEXT)(GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits);
extern void (APIENTRYP gglGetCompressedMultiTexImageEXT)(GLenum texunit, GLenum target, GLint lod, void *img);
extern void (APIENTRYP gglMatrixLoadTransposefEXT)(GLenum mode, const GLfloat *m);
extern void (APIENTRYP gglMatrixLoadTransposedEXT)(GLenum mode, const GLdouble *m);
extern void (APIENTRYP gglMatrixMultTransposefEXT)(GLenum mode, const GLfloat *m);
extern void (APIENTRYP gglMatrixMultTransposedEXT)(GLenum mode, const GLdouble *m);
extern void (APIENTRYP gglNamedBufferDataEXT)(GLuint buffer, GLsizeiptr size, const void *data, GLenum usage);
extern void (APIENTRYP gglNamedBufferSubDataEXT)(GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data);
extern void * (APIENTRYP gglMapNamedBufferEXT)(GLuint buffer, GLenum access);
extern GLboolean (APIENTRYP gglUnmapNamedBufferEXT)(GLuint buffer);
extern void (APIENTRYP gglGetNamedBufferParameterivEXT)(GLuint buffer, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetNamedBufferPointervEXT)(GLuint buffer, GLenum pname, void **params);
extern void (APIENTRYP gglGetNamedBufferSubDataEXT)(GLuint buffer, GLintptr offset, GLsizeiptr size, void *data);
extern void (APIENTRYP gglProgramUniform1fEXT)(GLuint program, GLint location, GLfloat v0);
extern void (APIENTRYP gglProgramUniform2fEXT)(GLuint program, GLint location, GLfloat v0, GLfloat v1);
extern void (APIENTRYP gglProgramUniform3fEXT)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern void (APIENTRYP gglProgramUniform4fEXT)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void (APIENTRYP gglProgramUniform1iEXT)(GLuint program, GLint location, GLint v0);
extern void (APIENTRYP gglProgramUniform2iEXT)(GLuint program, GLint location, GLint v0, GLint v1);
extern void (APIENTRYP gglProgramUniform3iEXT)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
extern void (APIENTRYP gglProgramUniform4iEXT)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
extern void (APIENTRYP gglProgramUniform1fvEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglProgramUniform2fvEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglProgramUniform3fvEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglProgramUniform4fvEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglProgramUniform1ivEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglProgramUniform2ivEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglProgramUniform3ivEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglProgramUniform4ivEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglProgramUniformMatrix2fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix3fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix4fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix2x3fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix3x2fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix2x4fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix4x2fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix3x4fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix4x3fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglTextureBufferEXT)(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer);
extern void (APIENTRYP gglMultiTexBufferEXT)(GLenum texunit, GLenum target, GLenum internalformat, GLuint buffer);
extern void (APIENTRYP gglTextureParameterIivEXT)(GLuint texture, GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglTextureParameterIuivEXT)(GLuint texture, GLenum target, GLenum pname, const GLuint *params);
extern void (APIENTRYP gglGetTextureParameterIivEXT)(GLuint texture, GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetTextureParameterIuivEXT)(GLuint texture, GLenum target, GLenum pname, GLuint *params);
extern void (APIENTRYP gglMultiTexParameterIivEXT)(GLenum texunit, GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglMultiTexParameterIuivEXT)(GLenum texunit, GLenum target, GLenum pname, const GLuint *params);
extern void (APIENTRYP gglGetMultiTexParameterIivEXT)(GLenum texunit, GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetMultiTexParameterIuivEXT)(GLenum texunit, GLenum target, GLenum pname, GLuint *params);
extern void (APIENTRYP gglProgramUniform1uiEXT)(GLuint program, GLint location, GLuint v0);
extern void (APIENTRYP gglProgramUniform2uiEXT)(GLuint program, GLint location, GLuint v0, GLuint v1);
extern void (APIENTRYP gglProgramUniform3uiEXT)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2);
extern void (APIENTRYP gglProgramUniform4uiEXT)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
extern void (APIENTRYP gglProgramUniform1uivEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglProgramUniform2uivEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglProgramUniform3uivEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglProgramUniform4uivEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglNamedProgramLocalParameters4fvEXT)(GLuint program, GLenum target, GLuint index, GLsizei count, const GLfloat *params);
extern void (APIENTRYP gglNamedProgramLocalParameterI4iEXT)(GLuint program, GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w);
extern void (APIENTRYP gglNamedProgramLocalParameterI4ivEXT)(GLuint program, GLenum target, GLuint index, const GLint *params);
extern void (APIENTRYP gglNamedProgramLocalParametersI4ivEXT)(GLuint program, GLenum target, GLuint index, GLsizei count, const GLint *params);
extern void (APIENTRYP gglNamedProgramLocalParameterI4uiEXT)(GLuint program, GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
extern void (APIENTRYP gglNamedProgramLocalParameterI4uivEXT)(GLuint program, GLenum target, GLuint index, const GLuint *params);
extern void (APIENTRYP gglNamedProgramLocalParametersI4uivEXT)(GLuint program, GLenum target, GLuint index, GLsizei count, const GLuint *params);
extern void (APIENTRYP gglGetNamedProgramLocalParameterIivEXT)(GLuint program, GLenum target, GLuint index, GLint *params);
extern void (APIENTRYP gglGetNamedProgramLocalParameterIuivEXT)(GLuint program, GLenum target, GLuint index, GLuint *params);
extern void (APIENTRYP gglEnableClientStateiEXT)(GLenum array, GLuint index);
extern void (APIENTRYP gglDisableClientStateiEXT)(GLenum array, GLuint index);
extern void (APIENTRYP gglGetFloati_vEXT)(GLenum pname, GLuint index, GLfloat *params);
extern void (APIENTRYP gglGetDoublei_vEXT)(GLenum pname, GLuint index, GLdouble *params);
extern void (APIENTRYP gglGetPointeri_vEXT)(GLenum pname, GLuint index, void **params);
extern void (APIENTRYP gglNamedProgramStringEXT)(GLuint program, GLenum target, GLenum format, GLsizei len, const void *string);
extern void (APIENTRYP gglNamedProgramLocalParameter4dEXT)(GLuint program, GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglNamedProgramLocalParameter4dvEXT)(GLuint program, GLenum target, GLuint index, const GLdouble *params);
extern void (APIENTRYP gglNamedProgramLocalParameter4fEXT)(GLuint program, GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglNamedProgramLocalParameter4fvEXT)(GLuint program, GLenum target, GLuint index, const GLfloat *params);
extern void (APIENTRYP gglGetNamedProgramLocalParameterdvEXT)(GLuint program, GLenum target, GLuint index, GLdouble *params);
extern void (APIENTRYP gglGetNamedProgramLocalParameterfvEXT)(GLuint program, GLenum target, GLuint index, GLfloat *params);
extern void (APIENTRYP gglGetNamedProgramivEXT)(GLuint program, GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetNamedProgramStringEXT)(GLuint program, GLenum target, GLenum pname, void *string);
extern void (APIENTRYP gglNamedRenderbufferStorageEXT)(GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglGetNamedRenderbufferParameterivEXT)(GLuint renderbuffer, GLenum pname, GLint *params);
extern void (APIENTRYP gglNamedRenderbufferStorageMultisampleEXT)(GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglNamedRenderbufferStorageMultisampleCoverageEXT)(GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height);
extern GLenum (APIENTRYP gglCheckNamedFramebufferStatusEXT)(GLuint framebuffer, GLenum target);
extern void (APIENTRYP gglNamedFramebufferTexture1DEXT)(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern void (APIENTRYP gglNamedFramebufferTexture2DEXT)(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern void (APIENTRYP gglNamedFramebufferTexture3DEXT)(GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
extern void (APIENTRYP gglNamedFramebufferRenderbufferEXT)(GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
extern void (APIENTRYP gglGetNamedFramebufferAttachmentParameterivEXT)(GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params);
extern void (APIENTRYP gglGenerateTextureMipmapEXT)(GLuint texture, GLenum target);
extern void (APIENTRYP gglGenerateMultiTexMipmapEXT)(GLenum texunit, GLenum target);
extern void (APIENTRYP gglFramebufferDrawBufferEXT)(GLuint framebuffer, GLenum mode);
extern void (APIENTRYP gglFramebufferDrawBuffersEXT)(GLuint framebuffer, GLsizei n, const GLenum *bufs);
extern void (APIENTRYP gglFramebufferReadBufferEXT)(GLuint framebuffer, GLenum mode);
extern void (APIENTRYP gglGetFramebufferParameterivEXT)(GLuint framebuffer, GLenum pname, GLint *params);
extern void (APIENTRYP gglNamedCopyBufferSubDataEXT)(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
extern void (APIENTRYP gglNamedFramebufferTextureEXT)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
extern void (APIENTRYP gglNamedFramebufferTextureLayerEXT)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer);
extern void (APIENTRYP gglNamedFramebufferTextureFaceEXT)(GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLenum face);
extern void (APIENTRYP gglTextureRenderbufferEXT)(GLuint texture, GLenum target, GLuint renderbuffer);
extern void (APIENTRYP gglMultiTexRenderbufferEXT)(GLenum texunit, GLenum target, GLuint renderbuffer);
extern void (APIENTRYP gglVertexArrayVertexOffsetEXT)(GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset);
extern void (APIENTRYP gglVertexArrayColorOffsetEXT)(GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset);
extern void (APIENTRYP gglVertexArrayEdgeFlagOffsetEXT)(GLuint vaobj, GLuint buffer, GLsizei stride, GLintptr offset);
extern void (APIENTRYP gglVertexArrayIndexOffsetEXT)(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset);
extern void (APIENTRYP gglVertexArrayNormalOffsetEXT)(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset);
extern void (APIENTRYP gglVertexArrayTexCoordOffsetEXT)(GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset);
extern void (APIENTRYP gglVertexArrayMultiTexCoordOffsetEXT)(GLuint vaobj, GLuint buffer, GLenum texunit, GLint size, GLenum type, GLsizei stride, GLintptr offset);
extern void (APIENTRYP gglVertexArrayFogCoordOffsetEXT)(GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset);
extern void (APIENTRYP gglVertexArraySecondaryColorOffsetEXT)(GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset);
extern void (APIENTRYP gglVertexArrayVertexAttribOffsetEXT)(GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLintptr offset);
extern void (APIENTRYP gglVertexArrayVertexAttribIOffsetEXT)(GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset);
extern void (APIENTRYP gglEnableVertexArrayEXT)(GLuint vaobj, GLenum array);
extern void (APIENTRYP gglDisableVertexArrayEXT)(GLuint vaobj, GLenum array);
extern void (APIENTRYP gglEnableVertexArrayAttribEXT)(GLuint vaobj, GLuint index);
extern void (APIENTRYP gglDisableVertexArrayAttribEXT)(GLuint vaobj, GLuint index);
extern void (APIENTRYP gglGetVertexArrayIntegervEXT)(GLuint vaobj, GLenum pname, GLint *param);
extern void (APIENTRYP gglGetVertexArrayPointervEXT)(GLuint vaobj, GLenum pname, void **param);
extern void (APIENTRYP gglGetVertexArrayIntegeri_vEXT)(GLuint vaobj, GLuint index, GLenum pname, GLint *param);
extern void (APIENTRYP gglGetVertexArrayPointeri_vEXT)(GLuint vaobj, GLuint index, GLenum pname, void **param);
extern void * (APIENTRYP gglMapNamedBufferRangeEXT)(GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access);
extern void (APIENTRYP gglFlushMappedNamedBufferRangeEXT)(GLuint buffer, GLintptr offset, GLsizeiptr length);
extern void (APIENTRYP gglNamedBufferStorageEXT)(GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags);
extern void (APIENTRYP gglClearNamedBufferDataEXT)(GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data);
extern void (APIENTRYP gglClearNamedBufferSubDataEXT)(GLuint buffer, GLenum internalformat, GLsizeiptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data);
extern void (APIENTRYP gglNamedFramebufferParameteriEXT)(GLuint framebuffer, GLenum pname, GLint param);
extern void (APIENTRYP gglGetNamedFramebufferParameterivEXT)(GLuint framebuffer, GLenum pname, GLint *params);
extern void (APIENTRYP gglProgramUniform1dEXT)(GLuint program, GLint location, GLdouble x);
extern void (APIENTRYP gglProgramUniform2dEXT)(GLuint program, GLint location, GLdouble x, GLdouble y);
extern void (APIENTRYP gglProgramUniform3dEXT)(GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglProgramUniform4dEXT)(GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglProgramUniform1dvEXT)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
extern void (APIENTRYP gglProgramUniform2dvEXT)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
extern void (APIENTRYP gglProgramUniform3dvEXT)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
extern void (APIENTRYP gglProgramUniform4dvEXT)(GLuint program, GLint location, GLsizei count, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix2dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix3dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix4dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix2x3dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix2x4dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix3x2dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix3x4dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix4x2dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglProgramUniformMatrix4x3dvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
extern void (APIENTRYP gglTextureBufferRangeEXT)(GLuint texture, GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
extern void (APIENTRYP gglTextureStorage1DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width);
extern void (APIENTRYP gglTextureStorage2DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglTextureStorage3DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
extern void (APIENTRYP gglTextureStorage2DMultisampleEXT)(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
extern void (APIENTRYP gglTextureStorage3DMultisampleEXT)(GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
extern void (APIENTRYP gglVertexArrayBindVertexBufferEXT)(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
extern void (APIENTRYP gglVertexArrayVertexAttribFormatEXT)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
extern void (APIENTRYP gglVertexArrayVertexAttribIFormatEXT)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
extern void (APIENTRYP gglVertexArrayVertexAttribLFormatEXT)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
extern void (APIENTRYP gglVertexArrayVertexAttribBindingEXT)(GLuint vaobj, GLuint attribindex, GLuint bindingindex);
extern void (APIENTRYP gglVertexArrayVertexBindingDivisorEXT)(GLuint vaobj, GLuint bindingindex, GLuint divisor);
extern void (APIENTRYP gglVertexArrayVertexAttribLOffsetEXT)(GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset);
extern void (APIENTRYP gglTexturePageCommitmentEXT)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit);
extern void (APIENTRYP gglVertexArrayVertexAttribDivisorEXT)(GLuint vaobj, GLuint index, GLuint divisor);

#ifndef GL_EXT_draw_buffers2
#define GL_EXT_draw_buffers2
#endif
extern void (APIENTRYP gglColorMaskIndexedEXT)(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a);
/* reuse void (APIENTRYP gglGetBooleanIndexedvEXT)(GLenum target, GLuint index, GLboolean *data) */
/* reuse void (APIENTRYP gglGetIntegerIndexedvEXT)(GLenum target, GLuint index, GLint *data) */
/* reuse void (APIENTRYP gglEnableIndexedEXT)(GLenum target, GLuint index) */
/* reuse void (APIENTRYP gglDisableIndexedEXT)(GLenum target, GLuint index) */
/* reuse GLboolean (APIENTRYP gglIsEnabledIndexedEXT)(GLenum target, GLuint index) */

#ifndef GL_EXT_draw_instanced
#define GL_EXT_draw_instanced
#endif
extern void (APIENTRYP gglDrawArraysInstancedEXT)(GLenum mode, GLint start, GLsizei count, GLsizei primcount);
extern void (APIENTRYP gglDrawElementsInstancedEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);

#ifndef GL_EXT_draw_range_elements
#define GL_EXT_draw_range_elements
#define GL_MAX_ELEMENTS_VERTICES_EXT 0x80E8
#define GL_MAX_ELEMENTS_INDICES_EXT 0x80E9
#endif
extern void (APIENTRYP gglDrawRangeElementsEXT)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);

#ifndef GL_EXT_fog_coord
#define GL_EXT_fog_coord
#define GL_FOG_COORDINATE_SOURCE_EXT 0x8450
#define GL_FOG_COORDINATE_EXT 0x8451
#define GL_FRAGMENT_DEPTH_EXT 0x8452
#define GL_CURRENT_FOG_COORDINATE_EXT 0x8453
#define GL_FOG_COORDINATE_ARRAY_TYPE_EXT 0x8454
#define GL_FOG_COORDINATE_ARRAY_STRIDE_EXT 0x8455
#define GL_FOG_COORDINATE_ARRAY_POINTER_EXT 0x8456
#define GL_FOG_COORDINATE_ARRAY_EXT 0x8457
#endif
extern void (APIENTRYP gglFogCoordfEXT)(GLfloat coord);
extern void (APIENTRYP gglFogCoordfvEXT)(const GLfloat *coord);
extern void (APIENTRYP gglFogCoorddEXT)(GLdouble coord);
extern void (APIENTRYP gglFogCoorddvEXT)(const GLdouble *coord);
extern void (APIENTRYP gglFogCoordPointerEXT)(GLenum type, GLsizei stride, const void *pointer);

#ifndef GL_EXT_framebuffer_blit
#define GL_EXT_framebuffer_blit
#define GL_READ_FRAMEBUFFER_EXT 0x8CA8
#define GL_DRAW_FRAMEBUFFER_EXT 0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_EXT 0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_EXT 0x8CAA
#endif
extern void (APIENTRYP gglBlitFramebufferEXT)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

#ifndef GL_EXT_framebuffer_multisample
#define GL_EXT_framebuffer_multisample
#define GL_RENDERBUFFER_SAMPLES_EXT 0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT 0x8D56
#define GL_MAX_SAMPLES_EXT 0x8D57
#endif
extern void (APIENTRYP gglRenderbufferStorageMultisampleEXT)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

#ifndef GL_EXT_framebuffer_multisample_blit_scaled
#define GL_EXT_framebuffer_multisample_blit_scaled
#define GL_SCALED_RESOLVE_FASTEST_EXT 0x90BA
#define GL_SCALED_RESOLVE_NICEST_EXT 0x90BB
#endif

#ifndef GL_EXT_framebuffer_object
#define GL_EXT_framebuffer_object
#define GL_INVALID_FRAMEBUFFER_OPERATION_EXT 0x0506
#define GL_MAX_RENDERBUFFER_SIZE_EXT 0x84E8
#define GL_FRAMEBUFFER_BINDING_EXT 0x8CA6
#define GL_RENDERBUFFER_BINDING_EXT 0x8CA7
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT 0x8CD4
#define GL_FRAMEBUFFER_COMPLETE_EXT 0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT 0x8CD9
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT 0x8CDA
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED_EXT 0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS_EXT 0x8CDF
#define GL_COLOR_ATTACHMENT0_EXT 0x8CE0
#define GL_COLOR_ATTACHMENT1_EXT 0x8CE1
#define GL_COLOR_ATTACHMENT2_EXT 0x8CE2
#define GL_COLOR_ATTACHMENT3_EXT 0x8CE3
#define GL_COLOR_ATTACHMENT4_EXT 0x8CE4
#define GL_COLOR_ATTACHMENT5_EXT 0x8CE5
#define GL_COLOR_ATTACHMENT6_EXT 0x8CE6
#define GL_COLOR_ATTACHMENT7_EXT 0x8CE7
#define GL_COLOR_ATTACHMENT8_EXT 0x8CE8
#define GL_COLOR_ATTACHMENT9_EXT 0x8CE9
#define GL_COLOR_ATTACHMENT10_EXT 0x8CEA
#define GL_COLOR_ATTACHMENT11_EXT 0x8CEB
#define GL_COLOR_ATTACHMENT12_EXT 0x8CEC
#define GL_COLOR_ATTACHMENT13_EXT 0x8CED
#define GL_COLOR_ATTACHMENT14_EXT 0x8CEE
#define GL_COLOR_ATTACHMENT15_EXT 0x8CEF
#define GL_DEPTH_ATTACHMENT_EXT 0x8D00
#define GL_STENCIL_ATTACHMENT_EXT 0x8D20
#define GL_FRAMEBUFFER_EXT 0x8D40
#define GL_RENDERBUFFER_EXT 0x8D41
#define GL_RENDERBUFFER_WIDTH_EXT 0x8D42
#define GL_RENDERBUFFER_HEIGHT_EXT 0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT_EXT 0x8D44
#define GL_STENCIL_INDEX1_EXT 0x8D46
#define GL_STENCIL_INDEX4_EXT 0x8D47
#define GL_STENCIL_INDEX8_EXT 0x8D48
#define GL_STENCIL_INDEX16_EXT 0x8D49
#define GL_RENDERBUFFER_RED_SIZE_EXT 0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE_EXT 0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE_EXT 0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE_EXT 0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE_EXT 0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE_EXT 0x8D55
#endif
extern GLboolean (APIENTRYP gglIsRenderbufferEXT)(GLuint renderbuffer);
extern void (APIENTRYP gglBindRenderbufferEXT)(GLenum target, GLuint renderbuffer);
extern void (APIENTRYP gglDeleteRenderbuffersEXT)(GLsizei n, const GLuint *renderbuffers);
extern void (APIENTRYP gglGenRenderbuffersEXT)(GLsizei n, GLuint *renderbuffers);
extern void (APIENTRYP gglRenderbufferStorageEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglGetRenderbufferParameterivEXT)(GLenum target, GLenum pname, GLint *params);
extern GLboolean (APIENTRYP gglIsFramebufferEXT)(GLuint framebuffer);
extern void (APIENTRYP gglBindFramebufferEXT)(GLenum target, GLuint framebuffer);
extern void (APIENTRYP gglDeleteFramebuffersEXT)(GLsizei n, const GLuint *framebuffers);
extern void (APIENTRYP gglGenFramebuffersEXT)(GLsizei n, GLuint *framebuffers);
extern GLenum (APIENTRYP gglCheckFramebufferStatusEXT)(GLenum target);
extern void (APIENTRYP gglFramebufferTexture1DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern void (APIENTRYP gglFramebufferTexture2DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern void (APIENTRYP gglFramebufferTexture3DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
extern void (APIENTRYP gglFramebufferRenderbufferEXT)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
extern void (APIENTRYP gglGetFramebufferAttachmentParameterivEXT)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
extern void (APIENTRYP gglGenerateMipmapEXT)(GLenum target);

#ifndef GL_EXT_framebuffer_sRGB
#define GL_EXT_framebuffer_sRGB
#define GL_FRAMEBUFFER_SRGB_EXT 0x8DB9
#define GL_FRAMEBUFFER_SRGB_CAPABLE_EXT 0x8DBA
#endif

#ifndef GL_EXT_geometry_shader4
#define GL_EXT_geometry_shader4
#define GL_GEOMETRY_SHADER_EXT 0x8DD9
#define GL_GEOMETRY_VERTICES_OUT_EXT 0x8DDA
#define GL_GEOMETRY_INPUT_TYPE_EXT 0x8DDB
#define GL_GEOMETRY_OUTPUT_TYPE_EXT 0x8DDC
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT 0x8C29
#define GL_MAX_GEOMETRY_VARYING_COMPONENTS_EXT 0x8DDD
#define GL_MAX_VERTEX_VARYING_COMPONENTS_EXT 0x8DDE
#define GL_MAX_VARYING_COMPONENTS_EXT 0x8B4B
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT 0x8DDF
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT 0x8DE0
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT 0x8DE1
#define GL_LINES_ADJACENCY_EXT 0x000A
#define GL_LINE_STRIP_ADJACENCY_EXT 0x000B
#define GL_TRIANGLES_ADJACENCY_EXT 0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY_EXT 0x000D
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT 0x8DA8
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_EXT 0x8DA9
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT 0x8DA7
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER_EXT 0x8CD4
#define GL_PROGRAM_POINT_SIZE_EXT 0x8642
#endif
extern void (APIENTRYP gglProgramParameteriEXT)(GLuint program, GLenum pname, GLint value);

#ifndef GL_EXT_gpu_program_parameters
#define GL_EXT_gpu_program_parameters
#endif
extern void (APIENTRYP gglProgramEnvParameters4fvEXT)(GLenum target, GLuint index, GLsizei count, const GLfloat *params);
extern void (APIENTRYP gglProgramLocalParameters4fvEXT)(GLenum target, GLuint index, GLsizei count, const GLfloat *params);

#ifndef GL_EXT_gpu_shader4
#define GL_EXT_gpu_shader4
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER_EXT 0x88FD
#define GL_SAMPLER_1D_ARRAY_EXT 0x8DC0
#define GL_SAMPLER_2D_ARRAY_EXT 0x8DC1
#define GL_SAMPLER_BUFFER_EXT 0x8DC2
#define GL_SAMPLER_1D_ARRAY_SHADOW_EXT 0x8DC3
#define GL_SAMPLER_2D_ARRAY_SHADOW_EXT 0x8DC4
#define GL_SAMPLER_CUBE_SHADOW_EXT 0x8DC5
#define GL_UNSIGNED_INT_VEC2_EXT 0x8DC6
#define GL_UNSIGNED_INT_VEC3_EXT 0x8DC7
#define GL_UNSIGNED_INT_VEC4_EXT 0x8DC8
#define GL_INT_SAMPLER_1D_EXT 0x8DC9
#define GL_INT_SAMPLER_2D_EXT 0x8DCA
#define GL_INT_SAMPLER_3D_EXT 0x8DCB
#define GL_INT_SAMPLER_CUBE_EXT 0x8DCC
#define GL_INT_SAMPLER_2D_RECT_EXT 0x8DCD
#define GL_INT_SAMPLER_1D_ARRAY_EXT 0x8DCE
#define GL_INT_SAMPLER_2D_ARRAY_EXT 0x8DCF
#define GL_INT_SAMPLER_BUFFER_EXT 0x8DD0
#define GL_UNSIGNED_INT_SAMPLER_1D_EXT 0x8DD1
#define GL_UNSIGNED_INT_SAMPLER_2D_EXT 0x8DD2
#define GL_UNSIGNED_INT_SAMPLER_3D_EXT 0x8DD3
#define GL_UNSIGNED_INT_SAMPLER_CUBE_EXT 0x8DD4
#define GL_UNSIGNED_INT_SAMPLER_2D_RECT_EXT 0x8DD5
#define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY_EXT 0x8DD6
#define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY_EXT 0x8DD7
#define GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT 0x8DD8
#define GL_MIN_PROGRAM_TEXEL_OFFSET_EXT 0x8904
#define GL_MAX_PROGRAM_TEXEL_OFFSET_EXT 0x8905
#endif
extern void (APIENTRYP gglGetUniformuivEXT)(GLuint program, GLint location, GLuint *params);
extern void (APIENTRYP gglBindFragDataLocationEXT)(GLuint program, GLuint color, const GLchar *name);
extern GLint (APIENTRYP gglGetFragDataLocationEXT)(GLuint program, const GLchar *name);
extern void (APIENTRYP gglUniform1uiEXT)(GLint location, GLuint v0);
extern void (APIENTRYP gglUniform2uiEXT)(GLint location, GLuint v0, GLuint v1);
extern void (APIENTRYP gglUniform3uiEXT)(GLint location, GLuint v0, GLuint v1, GLuint v2);
extern void (APIENTRYP gglUniform4uiEXT)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
extern void (APIENTRYP gglUniform1uivEXT)(GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglUniform2uivEXT)(GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglUniform3uivEXT)(GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglUniform4uivEXT)(GLint location, GLsizei count, const GLuint *value);

#ifndef GL_EXT_histogram
#define GL_EXT_histogram
#define GL_HISTOGRAM_EXT 0x8024
#define GL_PROXY_HISTOGRAM_EXT 0x8025
#define GL_HISTOGRAM_WIDTH_EXT 0x8026
#define GL_HISTOGRAM_FORMAT_EXT 0x8027
#define GL_HISTOGRAM_RED_SIZE_EXT 0x8028
#define GL_HISTOGRAM_GREEN_SIZE_EXT 0x8029
#define GL_HISTOGRAM_BLUE_SIZE_EXT 0x802A
#define GL_HISTOGRAM_ALPHA_SIZE_EXT 0x802B
#define GL_HISTOGRAM_LUMINANCE_SIZE_EXT 0x802C
#define GL_HISTOGRAM_SINK_EXT 0x802D
#define GL_MINMAX_EXT 0x802E
#define GL_MINMAX_FORMAT_EXT 0x802F
#define GL_MINMAX_SINK_EXT 0x8030
#define GL_TABLE_TOO_LARGE_EXT 0x8031
#endif
extern void (APIENTRYP gglGetHistogramEXT)(GLenum target, GLboolean reset, GLenum format, GLenum type, void *values);
extern void (APIENTRYP gglGetHistogramParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetHistogramParameterivEXT)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetMinmaxEXT)(GLenum target, GLboolean reset, GLenum format, GLenum type, void *values);
extern void (APIENTRYP gglGetMinmaxParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetMinmaxParameterivEXT)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglHistogramEXT)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
extern void (APIENTRYP gglMinmaxEXT)(GLenum target, GLenum internalformat, GLboolean sink);
extern void (APIENTRYP gglResetHistogramEXT)(GLenum target);
extern void (APIENTRYP gglResetMinmaxEXT)(GLenum target);

#ifndef GL_EXT_index_array_formats
#define GL_EXT_index_array_formats
#define GL_IUI_V2F_EXT 0x81AD
#define GL_IUI_V3F_EXT 0x81AE
#define GL_IUI_N3F_V2F_EXT 0x81AF
#define GL_IUI_N3F_V3F_EXT 0x81B0
#define GL_T2F_IUI_V2F_EXT 0x81B1
#define GL_T2F_IUI_V3F_EXT 0x81B2
#define GL_T2F_IUI_N3F_V2F_EXT 0x81B3
#define GL_T2F_IUI_N3F_V3F_EXT 0x81B4
#endif

#ifndef GL_EXT_index_func
#define GL_EXT_index_func
#define GL_INDEX_TEST_EXT 0x81B5
#define GL_INDEX_TEST_FUNC_EXT 0x81B6
#define GL_INDEX_TEST_REF_EXT 0x81B7
#endif
extern void (APIENTRYP gglIndexFuncEXT)(GLenum func, GLclampf ref);

#ifndef GL_EXT_index_material
#define GL_EXT_index_material
#define GL_INDEX_MATERIAL_EXT 0x81B8
#define GL_INDEX_MATERIAL_PARAMETER_EXT 0x81B9
#define GL_INDEX_MATERIAL_FACE_EXT 0x81BA
#endif
extern void (APIENTRYP gglIndexMaterialEXT)(GLenum face, GLenum mode);

#ifndef GL_EXT_index_texture
#define GL_EXT_index_texture
#endif

#ifndef GL_EXT_light_texture
#define GL_EXT_light_texture
#define GL_FRAGMENT_MATERIAL_EXT 0x8349
#define GL_FRAGMENT_NORMAL_EXT 0x834A
#define GL_FRAGMENT_COLOR_EXT 0x834C
#define GL_ATTENUATION_EXT 0x834D
#define GL_SHADOW_ATTENUATION_EXT 0x834E
#define GL_TEXTURE_APPLICATION_MODE_EXT 0x834F
#define GL_TEXTURE_LIGHT_EXT 0x8350
#define GL_TEXTURE_MATERIAL_FACE_EXT 0x8351
#define GL_TEXTURE_MATERIAL_PARAMETER_EXT 0x8352
/* reuse GL_FRAGMENT_DEPTH_EXT */
#endif
extern void (APIENTRYP gglApplyTextureEXT)(GLenum mode);
extern void (APIENTRYP gglTextureLightEXT)(GLenum pname);
extern void (APIENTRYP gglTextureMaterialEXT)(GLenum face, GLenum mode);

#ifndef GL_EXT_misc_attribute
#define GL_EXT_misc_attribute
#endif

#ifndef GL_EXT_multi_draw_arrays
#define GL_EXT_multi_draw_arrays
#endif
extern void (APIENTRYP gglMultiDrawArraysEXT)(GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount);
extern void (APIENTRYP gglMultiDrawElementsEXT)(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount);

#ifndef GL_EXT_multisample
#define GL_EXT_multisample
#define GL_MULTISAMPLE_EXT 0x809D
#define GL_SAMPLE_ALPHA_TO_MASK_EXT 0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_EXT 0x809F
#define GL_SAMPLE_MASK_EXT 0x80A0
#define GL_1PASS_EXT 0x80A1
#define GL_2PASS_0_EXT 0x80A2
#define GL_2PASS_1_EXT 0x80A3
#define GL_4PASS_0_EXT 0x80A4
#define GL_4PASS_1_EXT 0x80A5
#define GL_4PASS_2_EXT 0x80A6
#define GL_4PASS_3_EXT 0x80A7
#define GL_SAMPLE_BUFFERS_EXT 0x80A8
#define GL_SAMPLES_EXT 0x80A9
#define GL_SAMPLE_MASK_VALUE_EXT 0x80AA
#define GL_SAMPLE_MASK_INVERT_EXT 0x80AB
#define GL_SAMPLE_PATTERN_EXT 0x80AC
#define GL_MULTISAMPLE_BIT_EXT 0x20000000
#endif
extern void (APIENTRYP gglSampleMaskEXT)(GLclampf value, GLboolean invert);
extern void (APIENTRYP gglSamplePatternEXT)(GLenum pattern);

#ifndef GL_EXT_packed_depth_stencil
#define GL_EXT_packed_depth_stencil
#define GL_DEPTH_STENCIL_EXT 0x84F9
#define GL_UNSIGNED_INT_24_8_EXT 0x84FA
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#define GL_TEXTURE_STENCIL_SIZE_EXT 0x88F1
#endif

#ifndef GL_EXT_packed_float
#define GL_EXT_packed_float
#define GL_R11F_G11F_B10F_EXT 0x8C3A
#define GL_UNSIGNED_INT_10F_11F_11F_REV_EXT 0x8C3B
#define GL_RGBA_SIGNED_COMPONENTS_EXT 0x8C3C
#endif

#ifndef GL_EXT_packed_pixels
#define GL_EXT_packed_pixels
#define GL_UNSIGNED_BYTE_3_3_2_EXT 0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4_EXT 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1_EXT 0x8034
#define GL_UNSIGNED_INT_8_8_8_8_EXT 0x8035
#define GL_UNSIGNED_INT_10_10_10_2_EXT 0x8036
#endif

#ifndef GL_EXT_paletted_texture
#define GL_EXT_paletted_texture
#define GL_COLOR_INDEX1_EXT 0x80E2
#define GL_COLOR_INDEX2_EXT 0x80E3
#define GL_COLOR_INDEX4_EXT 0x80E4
#define GL_COLOR_INDEX8_EXT 0x80E5
#define GL_COLOR_INDEX12_EXT 0x80E6
#define GL_COLOR_INDEX16_EXT 0x80E7
#define GL_TEXTURE_INDEX_SIZE_EXT 0x80ED
#endif
extern void (APIENTRYP gglColorTableEXT)(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void *table);
extern void (APIENTRYP gglGetColorTableEXT)(GLenum target, GLenum format, GLenum type, void *data);
extern void (APIENTRYP gglGetColorTableParameterivEXT)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetColorTableParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params);

#ifndef GL_EXT_pixel_buffer_object
#define GL_EXT_pixel_buffer_object
#define GL_PIXEL_PACK_BUFFER_EXT 0x88EB
#define GL_PIXEL_UNPACK_BUFFER_EXT 0x88EC
#define GL_PIXEL_PACK_BUFFER_BINDING_EXT 0x88ED
#define GL_PIXEL_UNPACK_BUFFER_BINDING_EXT 0x88EF
#endif

#ifndef GL_EXT_pixel_transform
#define GL_EXT_pixel_transform
#define GL_PIXEL_TRANSFORM_2D_EXT 0x8330
#define GL_PIXEL_MAG_FILTER_EXT 0x8331
#define GL_PIXEL_MIN_FILTER_EXT 0x8332
#define GL_PIXEL_CUBIC_WEIGHT_EXT 0x8333
#define GL_CUBIC_EXT 0x8334
#define GL_AVERAGE_EXT 0x8335
#define GL_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT 0x8336
#define GL_MAX_PIXEL_TRANSFORM_2D_STACK_DEPTH_EXT 0x8337
#define GL_PIXEL_TRANSFORM_2D_MATRIX_EXT 0x8338
#endif
extern void (APIENTRYP gglPixelTransformParameteriEXT)(GLenum target, GLenum pname, GLint param);
extern void (APIENTRYP gglPixelTransformParameterfEXT)(GLenum target, GLenum pname, GLfloat param);
extern void (APIENTRYP gglPixelTransformParameterivEXT)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglPixelTransformParameterfvEXT)(GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglGetPixelTransformParameterivEXT)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetPixelTransformParameterfvEXT)(GLenum target, GLenum pname, GLfloat *params);

#ifndef GL_EXT_pixel_transform_color_table
#define GL_EXT_pixel_transform_color_table
#endif

#ifndef GL_EXT_point_parameters
#define GL_EXT_point_parameters
#define GL_POINT_SIZE_MIN_EXT 0x8126
#define GL_POINT_SIZE_MAX_EXT 0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT 0x8128
#define GL_DISTANCE_ATTENUATION_EXT 0x8129
#endif
extern void (APIENTRYP gglPointParameterfEXT)(GLenum pname, GLfloat param);
extern void (APIENTRYP gglPointParameterfvEXT)(GLenum pname, const GLfloat *params);

#ifndef GL_EXT_polygon_offset
#define GL_EXT_polygon_offset
#define GL_POLYGON_OFFSET_EXT 0x8037
#define GL_POLYGON_OFFSET_FACTOR_EXT 0x8038
#define GL_POLYGON_OFFSET_BIAS_EXT 0x8039
#endif
extern void (APIENTRYP gglPolygonOffsetEXT)(GLfloat factor, GLfloat bias);

#ifndef GL_EXT_polygon_offset_clamp
#define GL_EXT_polygon_offset_clamp
#define GL_POLYGON_OFFSET_CLAMP_EXT 0x8E1B
#endif
extern void (APIENTRYP gglPolygonOffsetClampEXT)(GLfloat factor, GLfloat units, GLfloat clamp);

#ifndef GL_EXT_post_depth_coverage
#define GL_EXT_post_depth_coverage
#endif

#ifndef GL_EXT_provoking_vertex
#define GL_EXT_provoking_vertex
#define GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION_EXT 0x8E4C
#define GL_FIRST_VERTEX_CONVENTION_EXT 0x8E4D
#define GL_LAST_VERTEX_CONVENTION_EXT 0x8E4E
#define GL_PROVOKING_VERTEX_EXT 0x8E4F
#endif
extern void (APIENTRYP gglProvokingVertexEXT)(GLenum mode);

#ifndef GL_EXT_raster_multisample
#define GL_EXT_raster_multisample
#define GL_RASTER_MULTISAMPLE_EXT 0x9327
#define GL_RASTER_SAMPLES_EXT 0x9328
#define GL_MAX_RASTER_SAMPLES_EXT 0x9329
#define GL_RASTER_FIXED_SAMPLE_LOCATIONS_EXT 0x932A
#define GL_MULTISAMPLE_RASTERIZATION_ALLOWED_EXT 0x932B
#define GL_EFFECTIVE_RASTER_SAMPLES_EXT 0x932C
#endif
extern void (APIENTRYP gglRasterSamplesEXT)(GLuint samples, GLboolean fixedsamplelocations);

#ifndef GL_EXT_rescale_normal
#define GL_EXT_rescale_normal
#define GL_RESCALE_NORMAL_EXT 0x803A
#endif

#ifndef GL_EXT_secondary_color
#define GL_EXT_secondary_color
#define GL_COLOR_SUM_EXT 0x8458
#define GL_CURRENT_SECONDARY_COLOR_EXT 0x8459
#define GL_SECONDARY_COLOR_ARRAY_SIZE_EXT 0x845A
#define GL_SECONDARY_COLOR_ARRAY_TYPE_EXT 0x845B
#define GL_SECONDARY_COLOR_ARRAY_STRIDE_EXT 0x845C
#define GL_SECONDARY_COLOR_ARRAY_POINTER_EXT 0x845D
#define GL_SECONDARY_COLOR_ARRAY_EXT 0x845E
#endif
extern void (APIENTRYP gglSecondaryColor3bEXT)(GLbyte red, GLbyte green, GLbyte blue);
extern void (APIENTRYP gglSecondaryColor3bvEXT)(const GLbyte *v);
extern void (APIENTRYP gglSecondaryColor3dEXT)(GLdouble red, GLdouble green, GLdouble blue);
extern void (APIENTRYP gglSecondaryColor3dvEXT)(const GLdouble *v);
extern void (APIENTRYP gglSecondaryColor3fEXT)(GLfloat red, GLfloat green, GLfloat blue);
extern void (APIENTRYP gglSecondaryColor3fvEXT)(const GLfloat *v);
extern void (APIENTRYP gglSecondaryColor3iEXT)(GLint red, GLint green, GLint blue);
extern void (APIENTRYP gglSecondaryColor3ivEXT)(const GLint *v);
extern void (APIENTRYP gglSecondaryColor3sEXT)(GLshort red, GLshort green, GLshort blue);
extern void (APIENTRYP gglSecondaryColor3svEXT)(const GLshort *v);
extern void (APIENTRYP gglSecondaryColor3ubEXT)(GLubyte red, GLubyte green, GLubyte blue);
extern void (APIENTRYP gglSecondaryColor3ubvEXT)(const GLubyte *v);
extern void (APIENTRYP gglSecondaryColor3uiEXT)(GLuint red, GLuint green, GLuint blue);
extern void (APIENTRYP gglSecondaryColor3uivEXT)(const GLuint *v);
extern void (APIENTRYP gglSecondaryColor3usEXT)(GLushort red, GLushort green, GLushort blue);
extern void (APIENTRYP gglSecondaryColor3usvEXT)(const GLushort *v);
extern void (APIENTRYP gglSecondaryColorPointerEXT)(GLint size, GLenum type, GLsizei stride, const void *pointer);

#ifndef GL_EXT_separate_shader_objects
#define GL_EXT_separate_shader_objects
#define GL_ACTIVE_PROGRAM_EXT 0x8259
#endif
extern void (APIENTRYP gglUseShaderProgramEXT)(GLenum type, GLuint program);
extern void (APIENTRYP gglActiveProgramEXT)(GLuint program);
extern GLuint (APIENTRYP gglCreateShaderProgramEXT)(GLenum type, const GLchar *string);

#ifndef GL_EXT_separate_specular_color
#define GL_EXT_separate_specular_color
#define GL_LIGHT_MODEL_COLOR_CONTROL_EXT 0x81F8
#define GL_SINGLE_COLOR_EXT 0x81F9
#define GL_SEPARATE_SPECULAR_COLOR_EXT 0x81FA
#endif

#ifndef GL_EXT_shader_image_load_formatted
#define GL_EXT_shader_image_load_formatted
#endif

#ifndef GL_EXT_shader_image_load_store
#define GL_EXT_shader_image_load_store
#define GL_MAX_IMAGE_UNITS_EXT 0x8F38
#define GL_MAX_COMBINED_IMAGE_UNITS_AND_FRAGMENT_OUTPUTS_EXT 0x8F39
#define GL_IMAGE_BINDING_NAME_EXT 0x8F3A
#define GL_IMAGE_BINDING_LEVEL_EXT 0x8F3B
#define GL_IMAGE_BINDING_LAYERED_EXT 0x8F3C
#define GL_IMAGE_BINDING_LAYER_EXT 0x8F3D
#define GL_IMAGE_BINDING_ACCESS_EXT 0x8F3E
#define GL_IMAGE_1D_EXT 0x904C
#define GL_IMAGE_2D_EXT 0x904D
#define GL_IMAGE_3D_EXT 0x904E
#define GL_IMAGE_2D_RECT_EXT 0x904F
#define GL_IMAGE_CUBE_EXT 0x9050
#define GL_IMAGE_BUFFER_EXT 0x9051
#define GL_IMAGE_1D_ARRAY_EXT 0x9052
#define GL_IMAGE_2D_ARRAY_EXT 0x9053
#define GL_IMAGE_CUBE_MAP_ARRAY_EXT 0x9054
#define GL_IMAGE_2D_MULTISAMPLE_EXT 0x9055
#define GL_IMAGE_2D_MULTISAMPLE_ARRAY_EXT 0x9056
#define GL_INT_IMAGE_1D_EXT 0x9057
#define GL_INT_IMAGE_2D_EXT 0x9058
#define GL_INT_IMAGE_3D_EXT 0x9059
#define GL_INT_IMAGE_2D_RECT_EXT 0x905A
#define GL_INT_IMAGE_CUBE_EXT 0x905B
#define GL_INT_IMAGE_BUFFER_EXT 0x905C
#define GL_INT_IMAGE_1D_ARRAY_EXT 0x905D
#define GL_INT_IMAGE_2D_ARRAY_EXT 0x905E
#define GL_INT_IMAGE_CUBE_MAP_ARRAY_EXT 0x905F
#define GL_INT_IMAGE_2D_MULTISAMPLE_EXT 0x9060
#define GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY_EXT 0x9061
#define GL_UNSIGNED_INT_IMAGE_1D_EXT 0x9062
#define GL_UNSIGNED_INT_IMAGE_2D_EXT 0x9063
#define GL_UNSIGNED_INT_IMAGE_3D_EXT 0x9064
#define GL_UNSIGNED_INT_IMAGE_2D_RECT_EXT 0x9065
#define GL_UNSIGNED_INT_IMAGE_CUBE_EXT 0x9066
#define GL_UNSIGNED_INT_IMAGE_BUFFER_EXT 0x9067
#define GL_UNSIGNED_INT_IMAGE_1D_ARRAY_EXT 0x9068
#define GL_UNSIGNED_INT_IMAGE_2D_ARRAY_EXT 0x9069
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY_EXT 0x906A
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_EXT 0x906B
#define GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY_EXT 0x906C
#define GL_MAX_IMAGE_SAMPLES_EXT 0x906D
#define GL_IMAGE_BINDING_FORMAT_EXT 0x906E
#define GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT_EXT 0x00000001
#define GL_ELEMENT_ARRAY_BARRIER_BIT_EXT 0x00000002
#define GL_UNIFORM_BARRIER_BIT_EXT 0x00000004
#define GL_TEXTURE_FETCH_BARRIER_BIT_EXT 0x00000008
#define GL_SHADER_IMAGE_ACCESS_BARRIER_BIT_EXT 0x00000020
#define GL_COMMAND_BARRIER_BIT_EXT 0x00000040
#define GL_PIXEL_BUFFER_BARRIER_BIT_EXT 0x00000080
#define GL_TEXTURE_UPDATE_BARRIER_BIT_EXT 0x00000100
#define GL_BUFFER_UPDATE_BARRIER_BIT_EXT 0x00000200
#define GL_FRAMEBUFFER_BARRIER_BIT_EXT 0x00000400
#define GL_TRANSFORM_FEEDBACK_BARRIER_BIT_EXT 0x00000800
#define GL_ATOMIC_COUNTER_BARRIER_BIT_EXT 0x00001000
#define GL_ALL_BARRIER_BITS_EXT 0xFFFFFFFF
#endif
extern void (APIENTRYP gglBindImageTextureEXT)(GLuint index, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLint format);
extern void (APIENTRYP gglMemoryBarrierEXT)(GLbitfield barriers);

#ifndef GL_EXT_shader_integer_mix
#define GL_EXT_shader_integer_mix
#endif

#ifndef GL_EXT_shadow_funcs
#define GL_EXT_shadow_funcs
#endif

#ifndef GL_EXT_shared_texture_palette
#define GL_EXT_shared_texture_palette
#define GL_SHARED_TEXTURE_PALETTE_EXT 0x81FB
#endif

#ifndef GL_EXT_sparse_texture2
#define GL_EXT_sparse_texture2
#endif

#ifndef GL_EXT_stencil_clear_tag
#define GL_EXT_stencil_clear_tag
#define GL_STENCIL_TAG_BITS_EXT 0x88F2
#define GL_STENCIL_CLEAR_TAG_VALUE_EXT 0x88F3
#endif
extern void (APIENTRYP gglStencilClearTagEXT)(GLsizei stencilTagBits, GLuint stencilClearTag);

#ifndef GL_EXT_stencil_two_side
#define GL_EXT_stencil_two_side
#define GL_STENCIL_TEST_TWO_SIDE_EXT 0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT 0x8911
#endif
extern void (APIENTRYP gglActiveStencilFaceEXT)(GLenum face);

#ifndef GL_EXT_stencil_wrap
#define GL_EXT_stencil_wrap
#define GL_INCR_WRAP_EXT 0x8507
#define GL_DECR_WRAP_EXT 0x8508
#endif

#ifndef GL_EXT_subtexture
#define GL_EXT_subtexture
#endif
extern void (APIENTRYP gglTexSubImage1DEXT)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTexSubImage2DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);

#ifndef GL_EXT_texture
#define GL_EXT_texture
#define GL_ALPHA4_EXT 0x803B
#define GL_ALPHA8_EXT 0x803C
#define GL_ALPHA12_EXT 0x803D
#define GL_ALPHA16_EXT 0x803E
#define GL_LUMINANCE4_EXT 0x803F
#define GL_LUMINANCE8_EXT 0x8040
#define GL_LUMINANCE12_EXT 0x8041
#define GL_LUMINANCE16_EXT 0x8042
#define GL_LUMINANCE4_ALPHA4_EXT 0x8043
#define GL_LUMINANCE6_ALPHA2_EXT 0x8044
#define GL_LUMINANCE8_ALPHA8_EXT 0x8045
#define GL_LUMINANCE12_ALPHA4_EXT 0x8046
#define GL_LUMINANCE12_ALPHA12_EXT 0x8047
#define GL_LUMINANCE16_ALPHA16_EXT 0x8048
#define GL_INTENSITY_EXT 0x8049
#define GL_INTENSITY4_EXT 0x804A
#define GL_INTENSITY8_EXT 0x804B
#define GL_INTENSITY12_EXT 0x804C
#define GL_INTENSITY16_EXT 0x804D
#define GL_RGB2_EXT 0x804E
#define GL_RGB4_EXT 0x804F
#define GL_RGB5_EXT 0x8050
#define GL_RGB8_EXT 0x8051
#define GL_RGB10_EXT 0x8052
#define GL_RGB12_EXT 0x8053
#define GL_RGB16_EXT 0x8054
#define GL_RGBA2_EXT 0x8055
#define GL_RGBA4_EXT 0x8056
#define GL_RGB5_A1_EXT 0x8057
#define GL_RGBA8_EXT 0x8058
#define GL_RGB10_A2_EXT 0x8059
#define GL_RGBA12_EXT 0x805A
#define GL_RGBA16_EXT 0x805B
#define GL_TEXTURE_RED_SIZE_EXT 0x805C
#define GL_TEXTURE_GREEN_SIZE_EXT 0x805D
#define GL_TEXTURE_BLUE_SIZE_EXT 0x805E
#define GL_TEXTURE_ALPHA_SIZE_EXT 0x805F
#define GL_TEXTURE_LUMINANCE_SIZE_EXT 0x8060
#define GL_TEXTURE_INTENSITY_SIZE_EXT 0x8061
#define GL_REPLACE_EXT 0x8062
#define GL_PROXY_TEXTURE_1D_EXT 0x8063
#define GL_PROXY_TEXTURE_2D_EXT 0x8064
#define GL_TEXTURE_TOO_LARGE_EXT 0x8065
#endif

#ifndef GL_EXT_texture3D
#define GL_EXT_texture3D
#define GL_PACK_SKIP_IMAGES_EXT 0x806B
#define GL_PACK_IMAGE_HEIGHT_EXT 0x806C
#define GL_UNPACK_SKIP_IMAGES_EXT 0x806D
#define GL_UNPACK_IMAGE_HEIGHT_EXT 0x806E
#define GL_TEXTURE_3D_EXT 0x806F
#define GL_PROXY_TEXTURE_3D_EXT 0x8070
#define GL_TEXTURE_DEPTH_EXT 0x8071
#define GL_TEXTURE_WRAP_R_EXT 0x8072
#define GL_MAX_3D_TEXTURE_SIZE_EXT 0x8073
#endif
extern void (APIENTRYP gglTexImage3DEXT)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);

#ifndef GL_EXT_texture_array
#define GL_EXT_texture_array
#define GL_TEXTURE_1D_ARRAY_EXT 0x8C18
#define GL_PROXY_TEXTURE_1D_ARRAY_EXT 0x8C19
#define GL_TEXTURE_2D_ARRAY_EXT 0x8C1A
#define GL_PROXY_TEXTURE_2D_ARRAY_EXT 0x8C1B
#define GL_TEXTURE_BINDING_1D_ARRAY_EXT 0x8C1C
#define GL_TEXTURE_BINDING_2D_ARRAY_EXT 0x8C1D
#define GL_MAX_ARRAY_TEXTURE_LAYERS_EXT 0x88FF
#define GL_COMPARE_REF_DEPTH_TO_TEXTURE_EXT 0x884E
/* reuse GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER_EXT */
#endif
extern void (APIENTRYP gglFramebufferTextureLayerEXT)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);

#ifndef GL_EXT_texture_buffer_object
#define GL_EXT_texture_buffer_object
#define GL_TEXTURE_BUFFER_EXT 0x8C2A
#define GL_MAX_TEXTURE_BUFFER_SIZE_EXT 0x8C2B
#define GL_TEXTURE_BINDING_BUFFER_EXT 0x8C2C
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING_EXT 0x8C2D
#define GL_TEXTURE_BUFFER_FORMAT_EXT 0x8C2E
#endif
extern void (APIENTRYP gglTexBufferEXT)(GLenum target, GLenum internalformat, GLuint buffer);

#ifndef GL_EXT_texture_compression_latc
#define GL_EXT_texture_compression_latc
#define GL_COMPRESSED_LUMINANCE_LATC1_EXT 0x8C70
#define GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT 0x8C71
#define GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT 0x8C72
#define GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT 0x8C73
#endif

#ifndef GL_EXT_texture_compression_rgtc
#define GL_EXT_texture_compression_rgtc
#define GL_COMPRESSED_RED_RGTC1_EXT 0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1_EXT 0x8DBC
#define GL_COMPRESSED_RED_GREEN_RGTC2_EXT 0x8DBD
#define GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT 0x8DBE
#endif

#ifndef GL_EXT_texture_compression_s3tc
#define GL_EXT_texture_compression_s3tc
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#ifndef GL_EXT_texture_cube_map
#define GL_EXT_texture_cube_map
#define GL_NORMAL_MAP_EXT 0x8511
#define GL_REFLECTION_MAP_EXT 0x8512
#define GL_TEXTURE_CUBE_MAP_EXT 0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_EXT 0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT 0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_EXT 0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_EXT 0x851C
#endif

#ifndef GL_EXT_texture_env_add
#define GL_EXT_texture_env_add
#endif

#ifndef GL_EXT_texture_env_combine
#define GL_EXT_texture_env_combine
#define GL_COMBINE_EXT 0x8570
#define GL_COMBINE_RGB_EXT 0x8571
#define GL_COMBINE_ALPHA_EXT 0x8572
#define GL_RGB_SCALE_EXT 0x8573
#define GL_ADD_SIGNED_EXT 0x8574
#define GL_INTERPOLATE_EXT 0x8575
#define GL_CONSTANT_EXT 0x8576
#define GL_PRIMARY_COLOR_EXT 0x8577
#define GL_PREVIOUS_EXT 0x8578
#define GL_SOURCE0_RGB_EXT 0x8580
#define GL_SOURCE1_RGB_EXT 0x8581
#define GL_SOURCE2_RGB_EXT 0x8582
#define GL_SOURCE0_ALPHA_EXT 0x8588
#define GL_SOURCE1_ALPHA_EXT 0x8589
#define GL_SOURCE2_ALPHA_EXT 0x858A
#define GL_OPERAND0_RGB_EXT 0x8590
#define GL_OPERAND1_RGB_EXT 0x8591
#define GL_OPERAND2_RGB_EXT 0x8592
#define GL_OPERAND0_ALPHA_EXT 0x8598
#define GL_OPERAND1_ALPHA_EXT 0x8599
#define GL_OPERAND2_ALPHA_EXT 0x859A
#endif

#ifndef GL_EXT_texture_env_dot3
#define GL_EXT_texture_env_dot3
#define GL_DOT3_RGB_EXT 0x8740
#define GL_DOT3_RGBA_EXT 0x8741
#endif

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_EXT_texture_filter_anisotropic
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#ifndef GL_EXT_texture_filter_minmax
#define GL_EXT_texture_filter_minmax
/* reuse GL_RASTER_MULTISAMPLE_EXT */
/* reuse GL_RASTER_SAMPLES_EXT */
/* reuse GL_MAX_RASTER_SAMPLES_EXT */
/* reuse GL_RASTER_FIXED_SAMPLE_LOCATIONS_EXT */
/* reuse GL_MULTISAMPLE_RASTERIZATION_ALLOWED_EXT */
/* reuse GL_EFFECTIVE_RASTER_SAMPLES_EXT */
#endif
/* reuse void (APIENTRYP gglRasterSamplesEXT)(GLuint samples, GLboolean fixedsamplelocations) */

#ifndef GL_EXT_texture_integer
#define GL_EXT_texture_integer
#define GL_RGBA32UI_EXT 0x8D70
#define GL_RGB32UI_EXT 0x8D71
#define GL_ALPHA32UI_EXT 0x8D72
#define GL_INTENSITY32UI_EXT 0x8D73
#define GL_LUMINANCE32UI_EXT 0x8D74
#define GL_LUMINANCE_ALPHA32UI_EXT 0x8D75
#define GL_RGBA16UI_EXT 0x8D76
#define GL_RGB16UI_EXT 0x8D77
#define GL_ALPHA16UI_EXT 0x8D78
#define GL_INTENSITY16UI_EXT 0x8D79
#define GL_LUMINANCE16UI_EXT 0x8D7A
#define GL_LUMINANCE_ALPHA16UI_EXT 0x8D7B
#define GL_RGBA8UI_EXT 0x8D7C
#define GL_RGB8UI_EXT 0x8D7D
#define GL_ALPHA8UI_EXT 0x8D7E
#define GL_INTENSITY8UI_EXT 0x8D7F
#define GL_LUMINANCE8UI_EXT 0x8D80
#define GL_LUMINANCE_ALPHA8UI_EXT 0x8D81
#define GL_RGBA32I_EXT 0x8D82
#define GL_RGB32I_EXT 0x8D83
#define GL_ALPHA32I_EXT 0x8D84
#define GL_INTENSITY32I_EXT 0x8D85
#define GL_LUMINANCE32I_EXT 0x8D86
#define GL_LUMINANCE_ALPHA32I_EXT 0x8D87
#define GL_RGBA16I_EXT 0x8D88
#define GL_RGB16I_EXT 0x8D89
#define GL_ALPHA16I_EXT 0x8D8A
#define GL_INTENSITY16I_EXT 0x8D8B
#define GL_LUMINANCE16I_EXT 0x8D8C
#define GL_LUMINANCE_ALPHA16I_EXT 0x8D8D
#define GL_RGBA8I_EXT 0x8D8E
#define GL_RGB8I_EXT 0x8D8F
#define GL_ALPHA8I_EXT 0x8D90
#define GL_INTENSITY8I_EXT 0x8D91
#define GL_LUMINANCE8I_EXT 0x8D92
#define GL_LUMINANCE_ALPHA8I_EXT 0x8D93
#define GL_RED_INTEGER_EXT 0x8D94
#define GL_GREEN_INTEGER_EXT 0x8D95
#define GL_BLUE_INTEGER_EXT 0x8D96
#define GL_ALPHA_INTEGER_EXT 0x8D97
#define GL_RGB_INTEGER_EXT 0x8D98
#define GL_RGBA_INTEGER_EXT 0x8D99
#define GL_BGR_INTEGER_EXT 0x8D9A
#define GL_BGRA_INTEGER_EXT 0x8D9B
#define GL_LUMINANCE_INTEGER_EXT 0x8D9C
#define GL_LUMINANCE_ALPHA_INTEGER_EXT 0x8D9D
#define GL_RGBA_INTEGER_MODE_EXT 0x8D9E
#endif
extern void (APIENTRYP gglTexParameterIivEXT)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglTexParameterIuivEXT)(GLenum target, GLenum pname, const GLuint *params);
extern void (APIENTRYP gglGetTexParameterIivEXT)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetTexParameterIuivEXT)(GLenum target, GLenum pname, GLuint *params);
extern void (APIENTRYP gglClearColorIiEXT)(GLint red, GLint green, GLint blue, GLint alpha);
extern void (APIENTRYP gglClearColorIuiEXT)(GLuint red, GLuint green, GLuint blue, GLuint alpha);

#ifndef GL_EXT_texture_lod_bias
#define GL_EXT_texture_lod_bias
#define GL_MAX_TEXTURE_LOD_BIAS_EXT 0x84FD
#define GL_TEXTURE_FILTER_CONTROL_EXT 0x8500
#define GL_TEXTURE_LOD_BIAS_EXT 0x8501
#endif

#ifndef GL_EXT_texture_mirror_clamp
#define GL_EXT_texture_mirror_clamp
#define GL_MIRROR_CLAMP_EXT 0x8742
#define GL_MIRROR_CLAMP_TO_EDGE_EXT 0x8743
#define GL_MIRROR_CLAMP_TO_BORDER_EXT 0x8912
#endif

#ifndef GL_EXT_texture_object
#define GL_EXT_texture_object
#define GL_TEXTURE_PRIORITY_EXT 0x8066
#define GL_TEXTURE_RESIDENT_EXT 0x8067
#define GL_TEXTURE_1D_BINDING_EXT 0x8068
#define GL_TEXTURE_2D_BINDING_EXT 0x8069
#define GL_TEXTURE_3D_BINDING_EXT 0x806A
#endif
extern GLboolean (APIENTRYP gglAreTexturesResidentEXT)(GLsizei n, const GLuint *textures, GLboolean *residences);
extern void (APIENTRYP gglBindTextureEXT)(GLenum target, GLuint texture);
extern void (APIENTRYP gglDeleteTexturesEXT)(GLsizei n, const GLuint *textures);
extern void (APIENTRYP gglGenTexturesEXT)(GLsizei n, GLuint *textures);
extern GLboolean (APIENTRYP gglIsTextureEXT)(GLuint texture);
extern void (APIENTRYP gglPrioritizeTexturesEXT)(GLsizei n, const GLuint *textures, const GLclampf *priorities);

#ifndef GL_EXT_texture_perturb_normal
#define GL_EXT_texture_perturb_normal
#define GL_PERTURB_EXT 0x85AE
#define GL_TEXTURE_NORMAL_EXT 0x85AF
#endif
extern void (APIENTRYP gglTextureNormalEXT)(GLenum mode);

#ifndef GL_EXT_texture_sRGB
#define GL_EXT_texture_sRGB
#define GL_SRGB_EXT 0x8C40
#define GL_SRGB8_EXT 0x8C41
#define GL_SRGB_ALPHA_EXT 0x8C42
#define GL_SRGB8_ALPHA8_EXT 0x8C43
#define GL_SLUMINANCE_ALPHA_EXT 0x8C44
#define GL_SLUMINANCE8_ALPHA8_EXT 0x8C45
#define GL_SLUMINANCE_EXT 0x8C46
#define GL_SLUMINANCE8_EXT 0x8C47
#define GL_COMPRESSED_SRGB_EXT 0x8C48
#define GL_COMPRESSED_SRGB_ALPHA_EXT 0x8C49
#define GL_COMPRESSED_SLUMINANCE_EXT 0x8C4A
#define GL_COMPRESSED_SLUMINANCE_ALPHA_EXT 0x8C4B
#define GL_COMPRESSED_SRGB_S3TC_DXT1_EXT 0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
#endif

#ifndef GL_EXT_texture_sRGB_decode
#define GL_EXT_texture_sRGB_decode
#define GL_TEXTURE_SRGB_DECODE_EXT 0x8A48
#define GL_DECODE_EXT 0x8A49
#define GL_SKIP_DECODE_EXT 0x8A4A
#endif

#ifndef GL_EXT_texture_shared_exponent
#define GL_EXT_texture_shared_exponent
#define GL_RGB9_E5_EXT 0x8C3D
#define GL_UNSIGNED_INT_5_9_9_9_REV_EXT 0x8C3E
#define GL_TEXTURE_SHARED_SIZE_EXT 0x8C3F
#endif

#ifndef GL_EXT_texture_snorm
#define GL_EXT_texture_snorm
#define GL_ALPHA_SNORM 0x9010
#define GL_LUMINANCE_SNORM 0x9011
#define GL_LUMINANCE_ALPHA_SNORM 0x9012
#define GL_INTENSITY_SNORM 0x9013
#define GL_ALPHA8_SNORM 0x9014
#define GL_LUMINANCE8_SNORM 0x9015
#define GL_LUMINANCE8_ALPHA8_SNORM 0x9016
#define GL_INTENSITY8_SNORM 0x9017
#define GL_ALPHA16_SNORM 0x9018
#define GL_LUMINANCE16_SNORM 0x9019
#define GL_LUMINANCE16_ALPHA16_SNORM 0x901A
#define GL_INTENSITY16_SNORM 0x901B
#define GL_RED_SNORM 0x8F90
#define GL_RG_SNORM 0x8F91
#define GL_RGB_SNORM 0x8F92
#define GL_RGBA_SNORM 0x8F93
/* reuse GL_R8_SNORM */
/* reuse GL_RG8_SNORM */
/* reuse GL_RGB8_SNORM */
/* reuse GL_RGBA8_SNORM */
/* reuse GL_R16_SNORM */
/* reuse GL_RG16_SNORM */
/* reuse GL_RGB16_SNORM */
/* reuse GL_RGBA16_SNORM */
/* reuse GL_SIGNED_NORMALIZED */
#endif

#ifndef GL_EXT_texture_swizzle
#define GL_EXT_texture_swizzle
#define GL_TEXTURE_SWIZZLE_R_EXT 0x8E42
#define GL_TEXTURE_SWIZZLE_G_EXT 0x8E43
#define GL_TEXTURE_SWIZZLE_B_EXT 0x8E44
#define GL_TEXTURE_SWIZZLE_A_EXT 0x8E45
#define GL_TEXTURE_SWIZZLE_RGBA_EXT 0x8E46
#endif

#ifndef GL_EXT_timer_query
#define GL_EXT_timer_query
#define GL_TIME_ELAPSED_EXT 0x88BF
#endif
extern void (APIENTRYP gglGetQueryObjecti64vEXT)(GLuint id, GLenum pname, GLint64 *params);
extern void (APIENTRYP gglGetQueryObjectui64vEXT)(GLuint id, GLenum pname, GLuint64 *params);

#ifndef GL_EXT_transform_feedback
#define GL_EXT_transform_feedback
#define GL_TRANSFORM_FEEDBACK_BUFFER_EXT 0x8C8E
#define GL_TRANSFORM_FEEDBACK_BUFFER_START_EXT 0x8C84
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE_EXT 0x8C85
#define GL_TRANSFORM_FEEDBACK_BUFFER_BINDING_EXT 0x8C8F
#define GL_INTERLEAVED_ATTRIBS_EXT 0x8C8C
#define GL_SEPARATE_ATTRIBS_EXT 0x8C8D
#define GL_PRIMITIVES_GENERATED_EXT 0x8C87
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_EXT 0x8C88
#define GL_RASTERIZER_DISCARD_EXT 0x8C89
#define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_EXT 0x8C8A
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS_EXT 0x8C8B
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_EXT 0x8C80
#define GL_TRANSFORM_FEEDBACK_VARYINGS_EXT 0x8C83
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE_EXT 0x8C7F
#define GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH_EXT 0x8C76
#endif
extern void (APIENTRYP gglBeginTransformFeedbackEXT)(GLenum primitiveMode);
extern void (APIENTRYP gglEndTransformFeedbackEXT)();
extern void (APIENTRYP gglBindBufferRangeEXT)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
extern void (APIENTRYP gglBindBufferOffsetEXT)(GLenum target, GLuint index, GLuint buffer, GLintptr offset);
extern void (APIENTRYP gglBindBufferBaseEXT)(GLenum target, GLuint index, GLuint buffer);
extern void (APIENTRYP gglTransformFeedbackVaryingsEXT)(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode);
extern void (APIENTRYP gglGetTransformFeedbackVaryingEXT)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);

#ifndef GL_EXT_vertex_array
#define GL_EXT_vertex_array
#define GL_VERTEX_ARRAY_EXT 0x8074
#define GL_NORMAL_ARRAY_EXT 0x8075
#define GL_COLOR_ARRAY_EXT 0x8076
#define GL_INDEX_ARRAY_EXT 0x8077
#define GL_TEXTURE_COORD_ARRAY_EXT 0x8078
#define GL_EDGE_FLAG_ARRAY_EXT 0x8079
#define GL_VERTEX_ARRAY_SIZE_EXT 0x807A
#define GL_VERTEX_ARRAY_TYPE_EXT 0x807B
#define GL_VERTEX_ARRAY_STRIDE_EXT 0x807C
#define GL_VERTEX_ARRAY_COUNT_EXT 0x807D
#define GL_NORMAL_ARRAY_TYPE_EXT 0x807E
#define GL_NORMAL_ARRAY_STRIDE_EXT 0x807F
#define GL_NORMAL_ARRAY_COUNT_EXT 0x8080
#define GL_COLOR_ARRAY_SIZE_EXT 0x8081
#define GL_COLOR_ARRAY_TYPE_EXT 0x8082
#define GL_COLOR_ARRAY_STRIDE_EXT 0x8083
#define GL_COLOR_ARRAY_COUNT_EXT 0x8084
#define GL_INDEX_ARRAY_TYPE_EXT 0x8085
#define GL_INDEX_ARRAY_STRIDE_EXT 0x8086
#define GL_INDEX_ARRAY_COUNT_EXT 0x8087
#define GL_TEXTURE_COORD_ARRAY_SIZE_EXT 0x8088
#define GL_TEXTURE_COORD_ARRAY_TYPE_EXT 0x8089
#define GL_TEXTURE_COORD_ARRAY_STRIDE_EXT 0x808A
#define GL_TEXTURE_COORD_ARRAY_COUNT_EXT 0x808B
#define GL_EDGE_FLAG_ARRAY_STRIDE_EXT 0x808C
#define GL_EDGE_FLAG_ARRAY_COUNT_EXT 0x808D
#define GL_VERTEX_ARRAY_POINTER_EXT 0x808E
#define GL_NORMAL_ARRAY_POINTER_EXT 0x808F
#define GL_COLOR_ARRAY_POINTER_EXT 0x8090
#define GL_INDEX_ARRAY_POINTER_EXT 0x8091
#define GL_TEXTURE_COORD_ARRAY_POINTER_EXT 0x8092
#define GL_EDGE_FLAG_ARRAY_POINTER_EXT 0x8093
#endif
extern void (APIENTRYP gglArrayElementEXT)(GLint i);
extern void (APIENTRYP gglColorPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer);
extern void (APIENTRYP gglDrawArraysEXT)(GLenum mode, GLint first, GLsizei count);
extern void (APIENTRYP gglEdgeFlagPointerEXT)(GLsizei stride, GLsizei count, const GLboolean *pointer);
extern void (APIENTRYP gglGetPointervEXT)(GLenum pname, void **params);
extern void (APIENTRYP gglIndexPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const void *pointer);
extern void (APIENTRYP gglNormalPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const void *pointer);
extern void (APIENTRYP gglTexCoordPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer);
extern void (APIENTRYP gglVertexPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer);

#ifndef GL_EXT_vertex_array_bgra
#define GL_EXT_vertex_array_bgra
/* reuse GL_BGRA */
#endif

#ifndef GL_EXT_vertex_attrib_64bit
#define GL_EXT_vertex_attrib_64bit
/* reuse GL_DOUBLE */
#define GL_DOUBLE_VEC2_EXT 0x8FFC
#define GL_DOUBLE_VEC3_EXT 0x8FFD
#define GL_DOUBLE_VEC4_EXT 0x8FFE
#define GL_DOUBLE_MAT2_EXT 0x8F46
#define GL_DOUBLE_MAT3_EXT 0x8F47
#define GL_DOUBLE_MAT4_EXT 0x8F48
#define GL_DOUBLE_MAT2x3_EXT 0x8F49
#define GL_DOUBLE_MAT2x4_EXT 0x8F4A
#define GL_DOUBLE_MAT3x2_EXT 0x8F4B
#define GL_DOUBLE_MAT3x4_EXT 0x8F4C
#define GL_DOUBLE_MAT4x2_EXT 0x8F4D
#define GL_DOUBLE_MAT4x3_EXT 0x8F4E
#endif
extern void (APIENTRYP gglVertexAttribL1dEXT)(GLuint index, GLdouble x);
extern void (APIENTRYP gglVertexAttribL2dEXT)(GLuint index, GLdouble x, GLdouble y);
extern void (APIENTRYP gglVertexAttribL3dEXT)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglVertexAttribL4dEXT)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglVertexAttribL1dvEXT)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttribL2dvEXT)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttribL3dvEXT)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttribL4dvEXT)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttribLPointerEXT)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
extern void (APIENTRYP gglGetVertexAttribLdvEXT)(GLuint index, GLenum pname, GLdouble *params);

#ifndef GL_EXT_vertex_shader
#define GL_EXT_vertex_shader
#define GL_VERTEX_SHADER_EXT 0x8780
#define GL_VERTEX_SHADER_BINDING_EXT 0x8781
#define GL_OP_INDEX_EXT 0x8782
#define GL_OP_NEGATE_EXT 0x8783
#define GL_OP_DOT3_EXT 0x8784
#define GL_OP_DOT4_EXT 0x8785
#define GL_OP_MUL_EXT 0x8786
#define GL_OP_ADD_EXT 0x8787
#define GL_OP_MADD_EXT 0x8788
#define GL_OP_FRAC_EXT 0x8789
#define GL_OP_MAX_EXT 0x878A
#define GL_OP_MIN_EXT 0x878B
#define GL_OP_SET_GE_EXT 0x878C
#define GL_OP_SET_LT_EXT 0x878D
#define GL_OP_CLAMP_EXT 0x878E
#define GL_OP_FLOOR_EXT 0x878F
#define GL_OP_ROUND_EXT 0x8790
#define GL_OP_EXP_BASE_2_EXT 0x8791
#define GL_OP_LOG_BASE_2_EXT 0x8792
#define GL_OP_POWER_EXT 0x8793
#define GL_OP_RECIP_EXT 0x8794
#define GL_OP_RECIP_SQRT_EXT 0x8795
#define GL_OP_SUB_EXT 0x8796
#define GL_OP_CROSS_PRODUCT_EXT 0x8797
#define GL_OP_MULTIPLY_MATRIX_EXT 0x8798
#define GL_OP_MOV_EXT 0x8799
#define GL_OUTPUT_VERTEX_EXT 0x879A
#define GL_OUTPUT_COLOR0_EXT 0x879B
#define GL_OUTPUT_COLOR1_EXT 0x879C
#define GL_OUTPUT_TEXTURE_COORD0_EXT 0x879D
#define GL_OUTPUT_TEXTURE_COORD1_EXT 0x879E
#define GL_OUTPUT_TEXTURE_COORD2_EXT 0x879F
#define GL_OUTPUT_TEXTURE_COORD3_EXT 0x87A0
#define GL_OUTPUT_TEXTURE_COORD4_EXT 0x87A1
#define GL_OUTPUT_TEXTURE_COORD5_EXT 0x87A2
#define GL_OUTPUT_TEXTURE_COORD6_EXT 0x87A3
#define GL_OUTPUT_TEXTURE_COORD7_EXT 0x87A4
#define GL_OUTPUT_TEXTURE_COORD8_EXT 0x87A5
#define GL_OUTPUT_TEXTURE_COORD9_EXT 0x87A6
#define GL_OUTPUT_TEXTURE_COORD10_EXT 0x87A7
#define GL_OUTPUT_TEXTURE_COORD11_EXT 0x87A8
#define GL_OUTPUT_TEXTURE_COORD12_EXT 0x87A9
#define GL_OUTPUT_TEXTURE_COORD13_EXT 0x87AA
#define GL_OUTPUT_TEXTURE_COORD14_EXT 0x87AB
#define GL_OUTPUT_TEXTURE_COORD15_EXT 0x87AC
#define GL_OUTPUT_TEXTURE_COORD16_EXT 0x87AD
#define GL_OUTPUT_TEXTURE_COORD17_EXT 0x87AE
#define GL_OUTPUT_TEXTURE_COORD18_EXT 0x87AF
#define GL_OUTPUT_TEXTURE_COORD19_EXT 0x87B0
#define GL_OUTPUT_TEXTURE_COORD20_EXT 0x87B1
#define GL_OUTPUT_TEXTURE_COORD21_EXT 0x87B2
#define GL_OUTPUT_TEXTURE_COORD22_EXT 0x87B3
#define GL_OUTPUT_TEXTURE_COORD23_EXT 0x87B4
#define GL_OUTPUT_TEXTURE_COORD24_EXT 0x87B5
#define GL_OUTPUT_TEXTURE_COORD25_EXT 0x87B6
#define GL_OUTPUT_TEXTURE_COORD26_EXT 0x87B7
#define GL_OUTPUT_TEXTURE_COORD27_EXT 0x87B8
#define GL_OUTPUT_TEXTURE_COORD28_EXT 0x87B9
#define GL_OUTPUT_TEXTURE_COORD29_EXT 0x87BA
#define GL_OUTPUT_TEXTURE_COORD30_EXT 0x87BB
#define GL_OUTPUT_TEXTURE_COORD31_EXT 0x87BC
#define GL_OUTPUT_FOG_EXT 0x87BD
#define GL_SCALAR_EXT 0x87BE
#define GL_VECTOR_EXT 0x87BF
#define GL_MATRIX_EXT 0x87C0
#define GL_VARIANT_EXT 0x87C1
#define GL_INVARIANT_EXT 0x87C2
#define GL_LOCAL_CONSTANT_EXT 0x87C3
#define GL_LOCAL_EXT 0x87C4
#define GL_MAX_VERTEX_SHADER_INSTRUCTIONS_EXT 0x87C5
#define GL_MAX_VERTEX_SHADER_VARIANTS_EXT 0x87C6
#define GL_MAX_VERTEX_SHADER_INVARIANTS_EXT 0x87C7
#define GL_MAX_VERTEX_SHADER_LOCAL_CONSTANTS_EXT 0x87C8
#define GL_MAX_VERTEX_SHADER_LOCALS_EXT 0x87C9
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_INSTRUCTIONS_EXT 0x87CA
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_VARIANTS_EXT 0x87CB
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_LOCAL_CONSTANTS_EXT 0x87CC
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_INVARIANTS_EXT 0x87CD
#define GL_MAX_OPTIMIZED_VERTEX_SHADER_LOCALS_EXT 0x87CE
#define GL_VERTEX_SHADER_INSTRUCTIONS_EXT 0x87CF
#define GL_VERTEX_SHADER_VARIANTS_EXT 0x87D0
#define GL_VERTEX_SHADER_INVARIANTS_EXT 0x87D1
#define GL_VERTEX_SHADER_LOCAL_CONSTANTS_EXT 0x87D2
#define GL_VERTEX_SHADER_LOCALS_EXT 0x87D3
#define GL_VERTEX_SHADER_OPTIMIZED_EXT 0x87D4
#define GL_X_EXT 0x87D5
#define GL_Y_EXT 0x87D6
#define GL_Z_EXT 0x87D7
#define GL_W_EXT 0x87D8
#define GL_NEGATIVE_X_EXT 0x87D9
#define GL_NEGATIVE_Y_EXT 0x87DA
#define GL_NEGATIVE_Z_EXT 0x87DB
#define GL_NEGATIVE_W_EXT 0x87DC
#define GL_ZERO_EXT 0x87DD
#define GL_ONE_EXT 0x87DE
#define GL_NEGATIVE_ONE_EXT 0x87DF
#define GL_NORMALIZED_RANGE_EXT 0x87E0
#define GL_FULL_RANGE_EXT 0x87E1
#define GL_CURRENT_VERTEX_EXT 0x87E2
#define GL_MVP_MATRIX_EXT 0x87E3
#define GL_VARIANT_VALUE_EXT 0x87E4
#define GL_VARIANT_DATATYPE_EXT 0x87E5
#define GL_VARIANT_ARRAY_STRIDE_EXT 0x87E6
#define GL_VARIANT_ARRAY_TYPE_EXT 0x87E7
#define GL_VARIANT_ARRAY_EXT 0x87E8
#define GL_VARIANT_ARRAY_POINTER_EXT 0x87E9
#define GL_INVARIANT_VALUE_EXT 0x87EA
#define GL_INVARIANT_DATATYPE_EXT 0x87EB
#define GL_LOCAL_CONSTANT_VALUE_EXT 0x87EC
#define GL_LOCAL_CONSTANT_DATATYPE_EXT 0x87ED
#endif
extern void (APIENTRYP gglBeginVertexShaderEXT)();
extern void (APIENTRYP gglEndVertexShaderEXT)();
extern void (APIENTRYP gglBindVertexShaderEXT)(GLuint id);
extern GLuint (APIENTRYP gglGenVertexShadersEXT)(GLuint range);
extern void (APIENTRYP gglDeleteVertexShaderEXT)(GLuint id);
extern void (APIENTRYP gglShaderOp1EXT)(GLenum op, GLuint res, GLuint arg1);
extern void (APIENTRYP gglShaderOp2EXT)(GLenum op, GLuint res, GLuint arg1, GLuint arg2);
extern void (APIENTRYP gglShaderOp3EXT)(GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3);
extern void (APIENTRYP gglSwizzleEXT)(GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW);
extern void (APIENTRYP gglWriteMaskEXT)(GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW);
extern void (APIENTRYP gglInsertComponentEXT)(GLuint res, GLuint src, GLuint num);
extern void (APIENTRYP gglExtractComponentEXT)(GLuint res, GLuint src, GLuint num);
extern GLuint (APIENTRYP gglGenSymbolsEXT)(GLenum datatype, GLenum storagetype, GLenum range, GLuint components);
extern void (APIENTRYP gglSetInvariantEXT)(GLuint id, GLenum type, const void *addr);
extern void (APIENTRYP gglSetLocalConstantEXT)(GLuint id, GLenum type, const void *addr);
extern void (APIENTRYP gglVariantbvEXT)(GLuint id, const GLbyte *addr);
extern void (APIENTRYP gglVariantsvEXT)(GLuint id, const GLshort *addr);
extern void (APIENTRYP gglVariantivEXT)(GLuint id, const GLint *addr);
extern void (APIENTRYP gglVariantfvEXT)(GLuint id, const GLfloat *addr);
extern void (APIENTRYP gglVariantdvEXT)(GLuint id, const GLdouble *addr);
extern void (APIENTRYP gglVariantubvEXT)(GLuint id, const GLubyte *addr);
extern void (APIENTRYP gglVariantusvEXT)(GLuint id, const GLushort *addr);
extern void (APIENTRYP gglVariantuivEXT)(GLuint id, const GLuint *addr);
extern void (APIENTRYP gglVariantPointerEXT)(GLuint id, GLenum type, GLuint stride, const void *addr);
extern void (APIENTRYP gglEnableVariantClientStateEXT)(GLuint id);
extern void (APIENTRYP gglDisableVariantClientStateEXT)(GLuint id);
extern GLuint (APIENTRYP gglBindLightParameterEXT)(GLenum light, GLenum value);
extern GLuint (APIENTRYP gglBindMaterialParameterEXT)(GLenum face, GLenum value);
extern GLuint (APIENTRYP gglBindTexGenParameterEXT)(GLenum unit, GLenum coord, GLenum value);
extern GLuint (APIENTRYP gglBindTextureUnitParameterEXT)(GLenum unit, GLenum value);
extern GLuint (APIENTRYP gglBindParameterEXT)(GLenum value);
extern GLboolean (APIENTRYP gglIsVariantEnabledEXT)(GLuint id, GLenum cap);
extern void (APIENTRYP gglGetVariantBooleanvEXT)(GLuint id, GLenum value, GLboolean *data);
extern void (APIENTRYP gglGetVariantIntegervEXT)(GLuint id, GLenum value, GLint *data);
extern void (APIENTRYP gglGetVariantFloatvEXT)(GLuint id, GLenum value, GLfloat *data);
extern void (APIENTRYP gglGetVariantPointervEXT)(GLuint id, GLenum value, void **data);
extern void (APIENTRYP gglGetInvariantBooleanvEXT)(GLuint id, GLenum value, GLboolean *data);
extern void (APIENTRYP gglGetInvariantIntegervEXT)(GLuint id, GLenum value, GLint *data);
extern void (APIENTRYP gglGetInvariantFloatvEXT)(GLuint id, GLenum value, GLfloat *data);
extern void (APIENTRYP gglGetLocalConstantBooleanvEXT)(GLuint id, GLenum value, GLboolean *data);
extern void (APIENTRYP gglGetLocalConstantIntegervEXT)(GLuint id, GLenum value, GLint *data);
extern void (APIENTRYP gglGetLocalConstantFloatvEXT)(GLuint id, GLenum value, GLfloat *data);

#ifndef GL_EXT_vertex_weighting
#define GL_EXT_vertex_weighting
#define GL_MODELVIEW0_STACK_DEPTH_EXT 0x0BA3
#define GL_MODELVIEW1_STACK_DEPTH_EXT 0x8502
#define GL_MODELVIEW0_MATRIX_EXT 0x0BA6
#define GL_MODELVIEW1_MATRIX_EXT 0x8506
#define GL_VERTEX_WEIGHTING_EXT 0x8509
#define GL_MODELVIEW0_EXT 0x1700
#define GL_MODELVIEW1_EXT 0x850A
#define GL_CURRENT_VERTEX_WEIGHT_EXT 0x850B
#define GL_VERTEX_WEIGHT_ARRAY_EXT 0x850C
#define GL_VERTEX_WEIGHT_ARRAY_SIZE_EXT 0x850D
#define GL_VERTEX_WEIGHT_ARRAY_TYPE_EXT 0x850E
#define GL_VERTEX_WEIGHT_ARRAY_STRIDE_EXT 0x850F
#define GL_VERTEX_WEIGHT_ARRAY_POINTER_EXT 0x8510
#endif
extern void (APIENTRYP gglVertexWeightfEXT)(GLfloat weight);
extern void (APIENTRYP gglVertexWeightfvEXT)(const GLfloat *weight);
extern void (APIENTRYP gglVertexWeightPointerEXT)(GLint size, GLenum type, GLsizei stride, const void *pointer);

#ifndef GL_EXT_window_rectangles
#define GL_EXT_window_rectangles
#define GL_INCLUSIVE_EXT 0x8F10
#define GL_EXCLUSIVE_EXT 0x8F11
#define GL_WINDOW_RECTANGLE_EXT 0x8F12
#define GL_WINDOW_RECTANGLE_MODE_EXT 0x8F13
#define GL_MAX_WINDOW_RECTANGLES_EXT 0x8F14
#define GL_NUM_WINDOW_RECTANGLES_EXT 0x8F15
#endif
extern void (APIENTRYP gglWindowRectanglesEXT)(GLenum mode, GLsizei count, const GLint *box);

#ifndef GL_EXT_x11_sync_object
#define GL_EXT_x11_sync_object
#define GL_SYNC_X11_FENCE_EXT 0x90E1
#endif
extern GLsync (APIENTRYP gglImportSyncEXT)(GLenum external_sync_type, GLintptr external_sync, GLbitfield flags);

#ifndef GL_GREMEDY_frame_terminator
#define GL_GREMEDY_frame_terminator
#endif
extern void (APIENTRYP gglFrameTerminatorGREMEDY)();

#ifndef GL_GREMEDY_string_marker
#define GL_GREMEDY_string_marker
#endif
extern void (APIENTRYP gglStringMarkerGREMEDY)(GLsizei len, const void *string);

#ifndef GL_HP_convolution_border_modes
#define GL_HP_convolution_border_modes
#define GL_IGNORE_BORDER_HP 0x8150
#define GL_CONSTANT_BORDER_HP 0x8151
#define GL_REPLICATE_BORDER_HP 0x8153
#define GL_CONVOLUTION_BORDER_COLOR_HP 0x8154
#endif

#ifndef GL_HP_image_transform
#define GL_HP_image_transform
#define GL_IMAGE_SCALE_X_HP 0x8155
#define GL_IMAGE_SCALE_Y_HP 0x8156
#define GL_IMAGE_TRANSLATE_X_HP 0x8157
#define GL_IMAGE_TRANSLATE_Y_HP 0x8158
#define GL_IMAGE_ROTATE_ANGLE_HP 0x8159
#define GL_IMAGE_ROTATE_ORIGIN_X_HP 0x815A
#define GL_IMAGE_ROTATE_ORIGIN_Y_HP 0x815B
#define GL_IMAGE_MAG_FILTER_HP 0x815C
#define GL_IMAGE_MIN_FILTER_HP 0x815D
#define GL_IMAGE_CUBIC_WEIGHT_HP 0x815E
#define GL_CUBIC_HP 0x815F
#define GL_AVERAGE_HP 0x8160
#define GL_IMAGE_TRANSFORM_2D_HP 0x8161
#define GL_POST_IMAGE_TRANSFORM_COLOR_TABLE_HP 0x8162
#define GL_PROXY_POST_IMAGE_TRANSFORM_COLOR_TABLE_HP 0x8163
#endif
extern void (APIENTRYP gglImageTransformParameteriHP)(GLenum target, GLenum pname, GLint param);
extern void (APIENTRYP gglImageTransformParameterfHP)(GLenum target, GLenum pname, GLfloat param);
extern void (APIENTRYP gglImageTransformParameterivHP)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglImageTransformParameterfvHP)(GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglGetImageTransformParameterivHP)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetImageTransformParameterfvHP)(GLenum target, GLenum pname, GLfloat *params);

#ifndef GL_HP_occlusion_test
#define GL_HP_occlusion_test
#define GL_OCCLUSION_TEST_HP 0x8165
#define GL_OCCLUSION_TEST_RESULT_HP 0x8166
#endif

#ifndef GL_HP_texture_lighting
#define GL_HP_texture_lighting
#define GL_TEXTURE_LIGHTING_MODE_HP 0x8167
#define GL_TEXTURE_POST_SPECULAR_HP 0x8168
#define GL_TEXTURE_PRE_SPECULAR_HP 0x8169
#endif

#ifndef GL_IBM_cull_vertex
#define GL_IBM_cull_vertex
#define GL_CULL_VERTEX_IBM 103050
#endif

#ifndef GL_IBM_multimode_draw_arrays
#define GL_IBM_multimode_draw_arrays
#endif
extern void (APIENTRYP gglMultiModeDrawArraysIBM)(const GLenum *mode, const GLint *first, const GLsizei *count, GLsizei primcount, GLint modestride);
extern void (APIENTRYP gglMultiModeDrawElementsIBM)(const GLenum *mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount, GLint modestride);

#ifndef GL_IBM_rasterpos_clip
#define GL_IBM_rasterpos_clip
#define GL_RASTER_POSITION_UNCLIPPED_IBM 0x19262
#endif

#ifndef GL_IBM_static_data
#define GL_IBM_static_data
#define GL_ALL_STATIC_DATA_IBM 103060
#define GL_STATIC_VERTEX_ARRAY_IBM 103061
#endif
extern void (APIENTRYP gglFlushStaticDataIBM)(GLenum target);

#ifndef GL_IBM_texture_mirrored_repeat
#define GL_IBM_texture_mirrored_repeat
#define GL_MIRRORED_REPEAT_IBM 0x8370
#endif

#ifndef GL_IBM_vertex_array_lists
#define GL_IBM_vertex_array_lists
#define GL_VERTEX_ARRAY_LIST_IBM 103070
#define GL_NORMAL_ARRAY_LIST_IBM 103071
#define GL_COLOR_ARRAY_LIST_IBM 103072
#define GL_INDEX_ARRAY_LIST_IBM 103073
#define GL_TEXTURE_COORD_ARRAY_LIST_IBM 103074
#define GL_EDGE_FLAG_ARRAY_LIST_IBM 103075
#define GL_FOG_COORDINATE_ARRAY_LIST_IBM 103076
#define GL_SECONDARY_COLOR_ARRAY_LIST_IBM 103077
#define GL_VERTEX_ARRAY_LIST_STRIDE_IBM 103080
#define GL_NORMAL_ARRAY_LIST_STRIDE_IBM 103081
#define GL_COLOR_ARRAY_LIST_STRIDE_IBM 103082
#define GL_INDEX_ARRAY_LIST_STRIDE_IBM 103083
#define GL_TEXTURE_COORD_ARRAY_LIST_STRIDE_IBM 103084
#define GL_EDGE_FLAG_ARRAY_LIST_STRIDE_IBM 103085
#define GL_FOG_COORDINATE_ARRAY_LIST_STRIDE_IBM 103086
#define GL_SECONDARY_COLOR_ARRAY_LIST_STRIDE_IBM 103087
#endif
extern void (APIENTRYP gglColorPointerListIBM)(GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride);
extern void (APIENTRYP gglSecondaryColorPointerListIBM)(GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride);
extern void (APIENTRYP gglEdgeFlagPointerListIBM)(GLint stride, const GLboolean **pointer, GLint ptrstride);
extern void (APIENTRYP gglFogCoordPointerListIBM)(GLenum type, GLint stride, const void **pointer, GLint ptrstride);
extern void (APIENTRYP gglIndexPointerListIBM)(GLenum type, GLint stride, const void **pointer, GLint ptrstride);
extern void (APIENTRYP gglNormalPointerListIBM)(GLenum type, GLint stride, const void **pointer, GLint ptrstride);
extern void (APIENTRYP gglTexCoordPointerListIBM)(GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride);
extern void (APIENTRYP gglVertexPointerListIBM)(GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride);

#ifndef GL_INGR_blend_func_separate
#define GL_INGR_blend_func_separate
#endif
extern void (APIENTRYP gglBlendFuncSeparateINGR)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

#ifndef GL_INGR_color_clamp
#define GL_INGR_color_clamp
#define GL_RED_MIN_CLAMP_INGR 0x8560
#define GL_GREEN_MIN_CLAMP_INGR 0x8561
#define GL_BLUE_MIN_CLAMP_INGR 0x8562
#define GL_ALPHA_MIN_CLAMP_INGR 0x8563
#define GL_RED_MAX_CLAMP_INGR 0x8564
#define GL_GREEN_MAX_CLAMP_INGR 0x8565
#define GL_BLUE_MAX_CLAMP_INGR 0x8566
#define GL_ALPHA_MAX_CLAMP_INGR 0x8567
#endif

#ifndef GL_INGR_interlace_read
#define GL_INGR_interlace_read
#define GL_INTERLACE_READ_INGR 0x8568
#endif

#ifndef GL_INTEL_conservative_rasterization
#define GL_INTEL_conservative_rasterization
#define GL_CONSERVATIVE_RASTERIZATION_INTEL 0x83FE
#endif

#ifndef GL_INTEL_fragment_shader_ordering
#define GL_INTEL_fragment_shader_ordering
#endif

#ifndef GL_INTEL_framebuffer_CMAA
#define GL_INTEL_framebuffer_CMAA
#endif
extern void (APIENTRYP gglApplyFramebufferAttachmentCMAAINTEL)();

#ifndef GL_INTEL_map_texture
#define GL_INTEL_map_texture
#define GL_TEXTURE_MEMORY_LAYOUT_INTEL 0x83FF
#define GL_LAYOUT_DEFAULT_INTEL 0
#define GL_LAYOUT_LINEAR_INTEL 1
#define GL_LAYOUT_LINEAR_CPU_CACHED_INTEL 2
#endif
extern void (APIENTRYP gglSyncTextureINTEL)(GLuint texture);
extern void (APIENTRYP gglUnmapTexture2DINTEL)(GLuint texture, GLint level);
extern void * (APIENTRYP gglMapTexture2DINTEL)(GLuint texture, GLint level, GLbitfield access, GLint *stride, GLenum *layout);

#ifndef GL_INTEL_parallel_arrays
#define GL_INTEL_parallel_arrays
#define GL_PARALLEL_ARRAYS_INTEL 0x83F4
#define GL_VERTEX_ARRAY_PARALLEL_POINTERS_INTEL 0x83F5
#define GL_NORMAL_ARRAY_PARALLEL_POINTERS_INTEL 0x83F6
#define GL_COLOR_ARRAY_PARALLEL_POINTERS_INTEL 0x83F7
#define GL_TEXTURE_COORD_ARRAY_PARALLEL_POINTERS_INTEL 0x83F8
#endif
extern void (APIENTRYP gglVertexPointervINTEL)(GLint size, GLenum type, const void **pointer);
extern void (APIENTRYP gglNormalPointervINTEL)(GLenum type, const void **pointer);
extern void (APIENTRYP gglColorPointervINTEL)(GLint size, GLenum type, const void **pointer);
extern void (APIENTRYP gglTexCoordPointervINTEL)(GLint size, GLenum type, const void **pointer);

#ifndef GL_INTEL_performance_query
#define GL_INTEL_performance_query
#define GL_PERFQUERY_SINGLE_CONTEXT_INTEL 0x00000000
#define GL_PERFQUERY_GLOBAL_CONTEXT_INTEL 0x00000001
#define GL_PERFQUERY_WAIT_INTEL 0x83FB
#define GL_PERFQUERY_FLUSH_INTEL 0x83FA
#define GL_PERFQUERY_DONOT_FLUSH_INTEL 0x83F9
#define GL_PERFQUERY_COUNTER_EVENT_INTEL 0x94F0
#define GL_PERFQUERY_COUNTER_DURATION_NORM_INTEL 0x94F1
#define GL_PERFQUERY_COUNTER_DURATION_RAW_INTEL 0x94F2
#define GL_PERFQUERY_COUNTER_THROUGHPUT_INTEL 0x94F3
#define GL_PERFQUERY_COUNTER_RAW_INTEL 0x94F4
#define GL_PERFQUERY_COUNTER_TIMESTAMP_INTEL 0x94F5
#define GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL 0x94F8
#define GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL 0x94F9
#define GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL 0x94FA
#define GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL 0x94FB
#define GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL 0x94FC
#define GL_PERFQUERY_QUERY_NAME_LENGTH_MAX_INTEL 0x94FD
#define GL_PERFQUERY_COUNTER_NAME_LENGTH_MAX_INTEL 0x94FE
#define GL_PERFQUERY_COUNTER_DESC_LENGTH_MAX_INTEL 0x94FF
#define GL_PERFQUERY_GPA_EXTENDED_COUNTERS_INTEL 0x9500
#endif
extern void (APIENTRYP gglBeginPerfQueryINTEL)(GLuint queryHandle);
extern void (APIENTRYP gglCreatePerfQueryINTEL)(GLuint queryId, GLuint *queryHandle);
extern void (APIENTRYP gglDeletePerfQueryINTEL)(GLuint queryHandle);
extern void (APIENTRYP gglEndPerfQueryINTEL)(GLuint queryHandle);
extern void (APIENTRYP gglGetFirstPerfQueryIdINTEL)(GLuint *queryId);
extern void (APIENTRYP gglGetNextPerfQueryIdINTEL)(GLuint queryId, GLuint *nextQueryId);
extern void (APIENTRYP gglGetPerfCounterInfoINTEL)(GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar *counterName, GLuint counterDescLength, GLchar *counterDesc, GLuint *counterOffset, GLuint *counterDataSize, GLuint *counterTypeEnum, GLuint *counterDataTypeEnum, GLuint64 *rawCounterMaxValue);
extern void (APIENTRYP gglGetPerfQueryDataINTEL)(GLuint queryHandle, GLuint flags, GLsizei dataSize, GLvoid *data, GLuint *bytesWritten);
extern void (APIENTRYP gglGetPerfQueryIdByNameINTEL)(GLchar *queryName, GLuint *queryId);
extern void (APIENTRYP gglGetPerfQueryInfoINTEL)(GLuint queryId, GLuint queryNameLength, GLchar *queryName, GLuint *dataSize, GLuint *noCounters, GLuint *noInstances, GLuint *capsMask);

#ifndef GL_KHR_blend_equation_advanced
#define GL_KHR_blend_equation_advanced
#define GL_MULTIPLY_KHR 0x9294
#define GL_SCREEN_KHR 0x9295
#define GL_OVERLAY_KHR 0x9296
#define GL_DARKEN_KHR 0x9297
#define GL_LIGHTEN_KHR 0x9298
#define GL_COLORDODGE_KHR 0x9299
#define GL_COLORBURN_KHR 0x929A
#define GL_HARDLIGHT_KHR 0x929B
#define GL_SOFTLIGHT_KHR 0x929C
#define GL_DIFFERENCE_KHR 0x929E
#define GL_EXCLUSION_KHR 0x92A0
#define GL_HSL_HUE_KHR 0x92AD
#define GL_HSL_SATURATION_KHR 0x92AE
#define GL_HSL_COLOR_KHR 0x92AF
#define GL_HSL_LUMINOSITY_KHR 0x92B0
#endif
extern void (APIENTRYP gglBlendBarrierKHR)();

#ifndef GL_KHR_blend_equation_advanced_coherent
#define GL_KHR_blend_equation_advanced_coherent
#define GL_BLEND_ADVANCED_COHERENT_KHR 0x9285
#endif

#ifndef GL_KHR_context_flush_control
#define GL_KHR_context_flush_control
#define GL_CONTEXT_RELEASE_BEHAVIOR 0x82FB
#define GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH 0x82FC
/* reuse GL_NONE */
#endif

#ifndef GL_KHR_debug
#define GL_KHR_debug
#define GL_DEBUG_OUTPUT_SYNCHRONOUS 0x8242
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH 0x8243
#define GL_DEBUG_CALLBACK_FUNCTION 0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM 0x8245
#define GL_DEBUG_SOURCE_API 0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM 0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY 0x8249
#define GL_DEBUG_SOURCE_APPLICATION 0x824A
#define GL_DEBUG_SOURCE_OTHER 0x824B
#define GL_DEBUG_TYPE_ERROR 0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 0x824E
#define GL_DEBUG_TYPE_PORTABILITY 0x824F
#define GL_DEBUG_TYPE_PERFORMANCE 0x8250
#define GL_DEBUG_TYPE_OTHER 0x8251
#define GL_DEBUG_TYPE_MARKER 0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP 0x8269
#define GL_DEBUG_TYPE_POP_GROUP 0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH 0x826C
#define GL_DEBUG_GROUP_STACK_DEPTH 0x826D
#define GL_BUFFER 0x82E0
#define GL_SHADER 0x82E1
#define GL_PROGRAM 0x82E2
#define GL_VERTEX_ARRAY 0x8074
#define GL_QUERY 0x82E3
#define GL_PROGRAM_PIPELINE 0x82E4
/* reuse GL_SAMPLER */
#define GL_MAX_LABEL_LENGTH 0x82E8
#define GL_MAX_DEBUG_MESSAGE_LENGTH 0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES 0x9144
#define GL_DEBUG_LOGGED_MESSAGES 0x9145
#define GL_DEBUG_SEVERITY_HIGH 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM 0x9147
#define GL_DEBUG_SEVERITY_LOW 0x9148
#define GL_DEBUG_OUTPUT 0x92E0
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_DISPLAY_LIST 0x82E7
#endif
extern void (APIENTRYP gglDebugMessageControl)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
extern void (APIENTRYP gglDebugMessageInsert)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf);
extern void (APIENTRYP gglDebugMessageCallback)(GLDEBUGPROC callback, const void *userParam);
extern GLuint (APIENTRYP gglGetDebugMessageLog)(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog);
extern void (APIENTRYP gglPushDebugGroup)(GLenum source, GLuint id, GLsizei length, const GLchar *message);
extern void (APIENTRYP gglPopDebugGroup)();
extern void (APIENTRYP gglObjectLabel)(GLenum identifier, GLuint name, GLsizei length, const GLchar *label);
extern void (APIENTRYP gglGetObjectLabel)(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label);
extern void (APIENTRYP gglObjectPtrLabel)(const void *ptr, GLsizei length, const GLchar *label);
extern void (APIENTRYP gglGetObjectPtrLabel)(const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label);
extern void (APIENTRYP gglGetPointerv)(GLenum pname, void **params);

#ifndef GL_KHR_no_error
#define GL_KHR_no_error
#define GL_CONTEXT_FLAG_NO_ERROR_BIT_KHR 0x00000008
#endif

#ifndef GL_KHR_robust_buffer_access_behavior
#define GL_KHR_robust_buffer_access_behavior
#endif

#ifndef GL_KHR_robustness
#define GL_KHR_robustness
/* reuse GL_NO_ERROR */
#define GL_CONTEXT_ROBUST_ACCESS 0x90F3
#define GL_LOSE_CONTEXT_ON_RESET 0x8252
#define GL_GUILTY_CONTEXT_RESET 0x8253
#define GL_INNOCENT_CONTEXT_RESET 0x8254
#define GL_UNKNOWN_CONTEXT_RESET 0x8255
#define GL_RESET_NOTIFICATION_STRATEGY 0x8256
#define GL_NO_RESET_NOTIFICATION 0x8261
#define GL_CONTEXT_LOST 0x0507
#endif
extern GLenum (APIENTRYP gglGetGraphicsResetStatus)();
extern void (APIENTRYP gglReadnPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data);
extern void (APIENTRYP gglGetnUniformfv)(GLuint program, GLint location, GLsizei bufSize, GLfloat *params);
extern void (APIENTRYP gglGetnUniformiv)(GLuint program, GLint location, GLsizei bufSize, GLint *params);
extern void (APIENTRYP gglGetnUniformuiv)(GLuint program, GLint location, GLsizei bufSize, GLuint *params);

#ifndef GL_KHR_texture_compression_astc_hdr
#define GL_KHR_texture_compression_astc_hdr
#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR 0x93B0
#define GL_COMPRESSED_RGBA_ASTC_5x4_KHR 0x93B1
#define GL_COMPRESSED_RGBA_ASTC_5x5_KHR 0x93B2
#define GL_COMPRESSED_RGBA_ASTC_6x5_KHR 0x93B3
#define GL_COMPRESSED_RGBA_ASTC_6x6_KHR 0x93B4
#define GL_COMPRESSED_RGBA_ASTC_8x5_KHR 0x93B5
#define GL_COMPRESSED_RGBA_ASTC_8x6_KHR 0x93B6
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR 0x93B7
#define GL_COMPRESSED_RGBA_ASTC_10x5_KHR 0x93B8
#define GL_COMPRESSED_RGBA_ASTC_10x6_KHR 0x93B9
#define GL_COMPRESSED_RGBA_ASTC_10x8_KHR 0x93BA
#define GL_COMPRESSED_RGBA_ASTC_10x10_KHR 0x93BB
#define GL_COMPRESSED_RGBA_ASTC_12x10_KHR 0x93BC
#define GL_COMPRESSED_RGBA_ASTC_12x12_KHR 0x93BD
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR 0x93D0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR 0x93D1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR 0x93D2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR 0x93D3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR 0x93D4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR 0x93D5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR 0x93D6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR 0x93D7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR 0x93D8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR 0x93D9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR 0x93DA
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR 0x93DB
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR 0x93DC
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR 0x93DD
#endif

#ifndef GL_KHR_texture_compression_astc_ldr
#define GL_KHR_texture_compression_astc_ldr
/* reuse GL_COMPRESSED_RGBA_ASTC_4x4_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_5x4_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_5x5_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_6x5_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_6x6_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_8x5_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_8x6_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_8x8_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_10x5_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_10x6_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_10x8_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_10x10_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_12x10_KHR */
/* reuse GL_COMPRESSED_RGBA_ASTC_12x12_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR */
/* reuse GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR */
#endif

#ifndef GL_KHR_texture_compression_astc_sliced_3d
#define GL_KHR_texture_compression_astc_sliced_3d
#endif

#ifndef GL_MESAX_texture_stack
#define GL_MESAX_texture_stack
#define GL_TEXTURE_1D_STACK_MESAX 0x8759
#define GL_TEXTURE_2D_STACK_MESAX 0x875A
#define GL_PROXY_TEXTURE_1D_STACK_MESAX 0x875B
#define GL_PROXY_TEXTURE_2D_STACK_MESAX 0x875C
#define GL_TEXTURE_1D_STACK_BINDING_MESAX 0x875D
#define GL_TEXTURE_2D_STACK_BINDING_MESAX 0x875E
#endif

#ifndef GL_MESA_pack_invert
#define GL_MESA_pack_invert
#define GL_PACK_INVERT_MESA 0x8758
#endif

#ifndef GL_MESA_resize_buffers
#define GL_MESA_resize_buffers
#endif
extern void (APIENTRYP gglResizeBuffersMESA)();

#ifndef GL_MESA_window_pos
#define GL_MESA_window_pos
#endif
extern void (APIENTRYP gglWindowPos2dMESA)(GLdouble x, GLdouble y);
extern void (APIENTRYP gglWindowPos2dvMESA)(const GLdouble *v);
extern void (APIENTRYP gglWindowPos2fMESA)(GLfloat x, GLfloat y);
extern void (APIENTRYP gglWindowPos2fvMESA)(const GLfloat *v);
extern void (APIENTRYP gglWindowPos2iMESA)(GLint x, GLint y);
extern void (APIENTRYP gglWindowPos2ivMESA)(const GLint *v);
extern void (APIENTRYP gglWindowPos2sMESA)(GLshort x, GLshort y);
extern void (APIENTRYP gglWindowPos2svMESA)(const GLshort *v);
extern void (APIENTRYP gglWindowPos3dMESA)(GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglWindowPos3dvMESA)(const GLdouble *v);
extern void (APIENTRYP gglWindowPos3fMESA)(GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglWindowPos3fvMESA)(const GLfloat *v);
extern void (APIENTRYP gglWindowPos3iMESA)(GLint x, GLint y, GLint z);
extern void (APIENTRYP gglWindowPos3ivMESA)(const GLint *v);
extern void (APIENTRYP gglWindowPos3sMESA)(GLshort x, GLshort y, GLshort z);
extern void (APIENTRYP gglWindowPos3svMESA)(const GLshort *v);
extern void (APIENTRYP gglWindowPos4dMESA)(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglWindowPos4dvMESA)(const GLdouble *v);
extern void (APIENTRYP gglWindowPos4fMESA)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglWindowPos4fvMESA)(const GLfloat *v);
extern void (APIENTRYP gglWindowPos4iMESA)(GLint x, GLint y, GLint z, GLint w);
extern void (APIENTRYP gglWindowPos4ivMESA)(const GLint *v);
extern void (APIENTRYP gglWindowPos4sMESA)(GLshort x, GLshort y, GLshort z, GLshort w);
extern void (APIENTRYP gglWindowPos4svMESA)(const GLshort *v);

#ifndef GL_MESA_ycbcr_texture
#define GL_MESA_ycbcr_texture
#define GL_UNSIGNED_SHORT_8_8_MESA 0x85BA
#define GL_UNSIGNED_SHORT_8_8_REV_MESA 0x85BB
#define GL_YCBCR_MESA 0x8757
#endif

#ifndef GL_NVX_conditional_render
#define GL_NVX_conditional_render
#endif
extern void (APIENTRYP gglBeginConditionalRenderNVX)(GLuint id);
extern void (APIENTRYP gglEndConditionalRenderNVX)();

#ifndef GL_NVX_gpu_memory_info
#define GL_NVX_gpu_memory_info
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX 0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX 0x904A
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX 0x904B
#endif

#ifndef GL_NV_bindless_multi_draw_indirect
#define GL_NV_bindless_multi_draw_indirect
#endif
extern void (APIENTRYP gglMultiDrawArraysIndirectBindlessNV)(GLenum mode, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount);
extern void (APIENTRYP gglMultiDrawElementsIndirectBindlessNV)(GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount);

#ifndef GL_NV_bindless_multi_draw_indirect_count
#define GL_NV_bindless_multi_draw_indirect_count
#endif
extern void (APIENTRYP gglMultiDrawArraysIndirectBindlessCountNV)(GLenum mode, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount);
extern void (APIENTRYP gglMultiDrawElementsIndirectBindlessCountNV)(GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount);

#ifndef GL_NV_bindless_texture
#define GL_NV_bindless_texture
#endif
extern GLuint64 (APIENTRYP gglGetTextureHandleNV)(GLuint texture);
extern GLuint64 (APIENTRYP gglGetTextureSamplerHandleNV)(GLuint texture, GLuint sampler);
extern void (APIENTRYP gglMakeTextureHandleResidentNV)(GLuint64 handle);
extern void (APIENTRYP gglMakeTextureHandleNonResidentNV)(GLuint64 handle);
extern GLuint64 (APIENTRYP gglGetImageHandleNV)(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format);
extern void (APIENTRYP gglMakeImageHandleResidentNV)(GLuint64 handle, GLenum access);
extern void (APIENTRYP gglMakeImageHandleNonResidentNV)(GLuint64 handle);
extern void (APIENTRYP gglUniformHandleui64NV)(GLint location, GLuint64 value);
extern void (APIENTRYP gglUniformHandleui64vNV)(GLint location, GLsizei count, const GLuint64 *value);
extern void (APIENTRYP gglProgramUniformHandleui64NV)(GLuint program, GLint location, GLuint64 value);
extern void (APIENTRYP gglProgramUniformHandleui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64 *values);
extern GLboolean (APIENTRYP gglIsTextureHandleResidentNV)(GLuint64 handle);
extern GLboolean (APIENTRYP gglIsImageHandleResidentNV)(GLuint64 handle);

#ifndef GL_NV_blend_equation_advanced
#define GL_NV_blend_equation_advanced
#define GL_BLEND_OVERLAP_NV 0x9281
#define GL_BLEND_PREMULTIPLIED_SRC_NV 0x9280
#define GL_BLUE_NV 0x1905
#define GL_COLORBURN_NV 0x929A
#define GL_COLORDODGE_NV 0x9299
#define GL_CONJOINT_NV 0x9284
#define GL_CONTRAST_NV 0x92A1
#define GL_DARKEN_NV 0x9297
#define GL_DIFFERENCE_NV 0x929E
#define GL_DISJOINT_NV 0x9283
#define GL_DST_ATOP_NV 0x928F
#define GL_DST_IN_NV 0x928B
#define GL_DST_NV 0x9287
#define GL_DST_OUT_NV 0x928D
#define GL_DST_OVER_NV 0x9289
#define GL_EXCLUSION_NV 0x92A0
#define GL_GREEN_NV 0x1904
#define GL_HARDLIGHT_NV 0x929B
#define GL_HARDMIX_NV 0x92A9
#define GL_HSL_COLOR_NV 0x92AF
#define GL_HSL_HUE_NV 0x92AD
#define GL_HSL_LUMINOSITY_NV 0x92B0
#define GL_HSL_SATURATION_NV 0x92AE
/* reuse GL_INVERT */
#define GL_INVERT_OVG_NV 0x92B4
#define GL_INVERT_RGB_NV 0x92A3
#define GL_LIGHTEN_NV 0x9298
#define GL_LINEARBURN_NV 0x92A5
#define GL_LINEARDODGE_NV 0x92A4
#define GL_LINEARLIGHT_NV 0x92A7
#define GL_MINUS_CLAMPED_NV 0x92B3
#define GL_MINUS_NV 0x929F
#define GL_MULTIPLY_NV 0x9294
#define GL_OVERLAY_NV 0x9296
#define GL_PINLIGHT_NV 0x92A8
#define GL_PLUS_CLAMPED_ALPHA_NV 0x92B2
#define GL_PLUS_CLAMPED_NV 0x92B1
#define GL_PLUS_DARKER_NV 0x9292
#define GL_PLUS_NV 0x9291
#define GL_RED_NV 0x1903
#define GL_SCREEN_NV 0x9295
#define GL_SOFTLIGHT_NV 0x929C
#define GL_SRC_ATOP_NV 0x928E
#define GL_SRC_IN_NV 0x928A
#define GL_SRC_NV 0x9286
#define GL_SRC_OUT_NV 0x928C
#define GL_SRC_OVER_NV 0x9288
#define GL_UNCORRELATED_NV 0x9282
#define GL_VIVIDLIGHT_NV 0x92A6
#define GL_XOR_NV 0x1506
/* reuse GL_ZERO */
#endif
extern void (APIENTRYP gglBlendParameteriNV)(GLenum pname, GLint value);
extern void (APIENTRYP gglBlendBarrierNV)();

#ifndef GL_NV_blend_equation_advanced_coherent
#define GL_NV_blend_equation_advanced_coherent
#define GL_BLEND_ADVANCED_COHERENT_NV 0x9285
#endif

#ifndef GL_NV_blend_square
#define GL_NV_blend_square
#endif

#ifndef GL_NV_clip_space_w_scaling
#define GL_NV_clip_space_w_scaling
#define GL_VIEWPORT_POSITION_W_SCALE_NV 0x937C
#define GL_VIEWPORT_POSITION_W_SCALE_X_COEFF_NV 0x937D
#define GL_VIEWPORT_POSITION_W_SCALE_Y_COEFF_NV 0x937E
#endif
extern void (APIENTRYP gglViewportPositionWScaleNV)(GLuint index, GLfloat xcoeff, GLfloat ycoeff);

#ifndef GL_NV_command_list
#define GL_NV_command_list
#define GL_TERMINATE_SEQUENCE_COMMAND_NV 0x0000
#define GL_NOP_COMMAND_NV 0x0001
#define GL_DRAW_ELEMENTS_COMMAND_NV 0x0002
#define GL_DRAW_ARRAYS_COMMAND_NV 0x0003
#define GL_DRAW_ELEMENTS_STRIP_COMMAND_NV 0x0004
#define GL_DRAW_ARRAYS_STRIP_COMMAND_NV 0x0005
#define GL_DRAW_ELEMENTS_INSTANCED_COMMAND_NV 0x0006
#define GL_DRAW_ARRAYS_INSTANCED_COMMAND_NV 0x0007
#define GL_ELEMENT_ADDRESS_COMMAND_NV 0x0008
#define GL_ATTRIBUTE_ADDRESS_COMMAND_NV 0x0009
#define GL_UNIFORM_ADDRESS_COMMAND_NV 0x000A
#define GL_BLEND_COLOR_COMMAND_NV 0x000B
#define GL_STENCIL_REF_COMMAND_NV 0x000C
#define GL_LINE_WIDTH_COMMAND_NV 0x000D
#define GL_POLYGON_OFFSET_COMMAND_NV 0x000E
#define GL_ALPHA_REF_COMMAND_NV 0x000F
#define GL_VIEWPORT_COMMAND_NV 0x0010
#define GL_SCISSOR_COMMAND_NV 0x0011
#define GL_FRONT_FACE_COMMAND_NV 0x0012
#endif
extern void (APIENTRYP gglCreateStatesNV)(GLsizei n, GLuint *states);
extern void (APIENTRYP gglDeleteStatesNV)(GLsizei n, const GLuint *states);
extern GLboolean (APIENTRYP gglIsStateNV)(GLuint state);
extern void (APIENTRYP gglStateCaptureNV)(GLuint state, GLenum mode);
extern GLuint (APIENTRYP gglGetCommandHeaderNV)(GLenum tokenID, GLuint size);
extern GLushort (APIENTRYP gglGetStageIndexNV)(GLenum shadertype);
extern void (APIENTRYP gglDrawCommandsNV)(GLenum primitiveMode, GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, GLuint count);
extern void (APIENTRYP gglDrawCommandsAddressNV)(GLenum primitiveMode, const GLuint64 *indirects, const GLsizei *sizes, GLuint count);
extern void (APIENTRYP gglDrawCommandsStatesNV)(GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count);
extern void (APIENTRYP gglDrawCommandsStatesAddressNV)(const GLuint64 *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count);
extern void (APIENTRYP gglCreateCommandListsNV)(GLsizei n, GLuint *lists);
extern void (APIENTRYP gglDeleteCommandListsNV)(GLsizei n, const GLuint *lists);
extern GLboolean (APIENTRYP gglIsCommandListNV)(GLuint list);
extern void (APIENTRYP gglListDrawCommandsStatesClientNV)(GLuint list, GLuint segment, const void **indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count);
extern void (APIENTRYP gglCommandListSegmentsNV)(GLuint list, GLuint segments);
extern void (APIENTRYP gglCompileCommandListNV)(GLuint list);
extern void (APIENTRYP gglCallCommandListNV)(GLuint list);

#ifndef GL_NV_compute_program5
#define GL_NV_compute_program5
#define GL_COMPUTE_PROGRAM_NV 0x90FB
#define GL_COMPUTE_PROGRAM_PARAMETER_BUFFER_NV 0x90FC
#endif

#ifndef GL_NV_conditional_render
#define GL_NV_conditional_render
#define GL_QUERY_WAIT_NV 0x8E13
#define GL_QUERY_NO_WAIT_NV 0x8E14
#define GL_QUERY_BY_REGION_WAIT_NV 0x8E15
#define GL_QUERY_BY_REGION_NO_WAIT_NV 0x8E16
#endif
extern void (APIENTRYP gglBeginConditionalRenderNV)(GLuint id, GLenum mode);
extern void (APIENTRYP gglEndConditionalRenderNV)();

#ifndef GL_NV_conservative_raster
#define GL_NV_conservative_raster
#define GL_CONSERVATIVE_RASTERIZATION_NV 0x9346
#define GL_SUBPIXEL_PRECISION_BIAS_X_BITS_NV 0x9347
#define GL_SUBPIXEL_PRECISION_BIAS_Y_BITS_NV 0x9348
#define GL_MAX_SUBPIXEL_PRECISION_BIAS_BITS_NV 0x9349
#endif
extern void (APIENTRYP gglSubpixelPrecisionBiasNV)(GLuint xbits, GLuint ybits);

#ifndef GL_NV_conservative_raster_dilate
#define GL_NV_conservative_raster_dilate
#define GL_CONSERVATIVE_RASTER_DILATE_NV 0x9379
#define GL_CONSERVATIVE_RASTER_DILATE_RANGE_NV 0x937A
#define GL_CONSERVATIVE_RASTER_DILATE_GRANULARITY_NV 0x937B
#endif
extern void (APIENTRYP gglConservativeRasterParameterfNV)(GLenum pname, GLfloat value);

#ifndef GL_NV_conservative_raster_pre_snap_triangles
#define GL_NV_conservative_raster_pre_snap_triangles
#define GL_CONSERVATIVE_RASTER_MODE_NV 0x954D
#define GL_CONSERVATIVE_RASTER_MODE_POST_SNAP_NV 0x954E
#define GL_CONSERVATIVE_RASTER_MODE_PRE_SNAP_TRIANGLES_NV 0x954F
/* reuse GL_CONSERVATIVE_RASTER_MODE_NV */
#endif
extern void (APIENTRYP gglConservativeRasterParameteriNV)(GLenum pname, GLint param);

#ifndef GL_NV_copy_depth_to_color
#define GL_NV_copy_depth_to_color
#define GL_DEPTH_STENCIL_TO_RGBA_NV 0x886E
#define GL_DEPTH_STENCIL_TO_BGRA_NV 0x886F
#endif

#ifndef GL_NV_copy_image
#define GL_NV_copy_image
#endif
extern void (APIENTRYP gglCopyImageSubDataNV)(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth);

#ifndef GL_NV_deep_texture3D
#define GL_NV_deep_texture3D
#define GL_MAX_DEEP_3D_TEXTURE_WIDTH_HEIGHT_NV 0x90D0
#define GL_MAX_DEEP_3D_TEXTURE_DEPTH_NV 0x90D1
#endif

#ifndef GL_NV_depth_buffer_float
#define GL_NV_depth_buffer_float
#define GL_DEPTH_COMPONENT32F_NV 0x8DAB
#define GL_DEPTH32F_STENCIL8_NV 0x8DAC
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV_NV 0x8DAD
#define GL_DEPTH_BUFFER_FLOAT_MODE_NV 0x8DAF
#endif
extern void (APIENTRYP gglDepthRangedNV)(GLdouble zNear, GLdouble zFar);
extern void (APIENTRYP gglClearDepthdNV)(GLdouble depth);
extern void (APIENTRYP gglDepthBoundsdNV)(GLdouble zmin, GLdouble zmax);

#ifndef GL_NV_depth_clamp
#define GL_NV_depth_clamp
#define GL_DEPTH_CLAMP_NV 0x864F
#endif

#ifndef GL_NV_draw_texture
#define GL_NV_draw_texture
#endif
extern void (APIENTRYP gglDrawTextureNV)(GLuint texture, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1);

#ifndef GL_NV_evaluators
#define GL_NV_evaluators
#define GL_EVAL_2D_NV 0x86C0
#define GL_EVAL_TRIANGULAR_2D_NV 0x86C1
#define GL_MAP_TESSELLATION_NV 0x86C2
#define GL_MAP_ATTRIB_U_ORDER_NV 0x86C3
#define GL_MAP_ATTRIB_V_ORDER_NV 0x86C4
#define GL_EVAL_FRACTIONAL_TESSELLATION_NV 0x86C5
#define GL_EVAL_VERTEX_ATTRIB0_NV 0x86C6
#define GL_EVAL_VERTEX_ATTRIB1_NV 0x86C7
#define GL_EVAL_VERTEX_ATTRIB2_NV 0x86C8
#define GL_EVAL_VERTEX_ATTRIB3_NV 0x86C9
#define GL_EVAL_VERTEX_ATTRIB4_NV 0x86CA
#define GL_EVAL_VERTEX_ATTRIB5_NV 0x86CB
#define GL_EVAL_VERTEX_ATTRIB6_NV 0x86CC
#define GL_EVAL_VERTEX_ATTRIB7_NV 0x86CD
#define GL_EVAL_VERTEX_ATTRIB8_NV 0x86CE
#define GL_EVAL_VERTEX_ATTRIB9_NV 0x86CF
#define GL_EVAL_VERTEX_ATTRIB10_NV 0x86D0
#define GL_EVAL_VERTEX_ATTRIB11_NV 0x86D1
#define GL_EVAL_VERTEX_ATTRIB12_NV 0x86D2
#define GL_EVAL_VERTEX_ATTRIB13_NV 0x86D3
#define GL_EVAL_VERTEX_ATTRIB14_NV 0x86D4
#define GL_EVAL_VERTEX_ATTRIB15_NV 0x86D5
#define GL_MAX_MAP_TESSELLATION_NV 0x86D6
#define GL_MAX_RATIONAL_EVAL_ORDER_NV 0x86D7
#endif
extern void (APIENTRYP gglMapControlPointsNV)(GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, const void *points);
extern void (APIENTRYP gglMapParameterivNV)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglMapParameterfvNV)(GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglGetMapControlPointsNV)(GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, void *points);
extern void (APIENTRYP gglGetMapParameterivNV)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetMapParameterfvNV)(GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetMapAttribParameterivNV)(GLenum target, GLuint index, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetMapAttribParameterfvNV)(GLenum target, GLuint index, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglEvalMapsNV)(GLenum target, GLenum mode);

#ifndef GL_NV_explicit_multisample
#define GL_NV_explicit_multisample
#define GL_SAMPLE_POSITION_NV 0x8E50
#define GL_SAMPLE_MASK_NV 0x8E51
#define GL_SAMPLE_MASK_VALUE_NV 0x8E52
#define GL_TEXTURE_BINDING_RENDERBUFFER_NV 0x8E53
#define GL_TEXTURE_RENDERBUFFER_DATA_STORE_BINDING_NV 0x8E54
#define GL_TEXTURE_RENDERBUFFER_NV 0x8E55
#define GL_SAMPLER_RENDERBUFFER_NV 0x8E56
#define GL_INT_SAMPLER_RENDERBUFFER_NV 0x8E57
#define GL_UNSIGNED_INT_SAMPLER_RENDERBUFFER_NV 0x8E58
#define GL_MAX_SAMPLE_MASK_WORDS_NV 0x8E59
#endif
extern void (APIENTRYP gglGetMultisamplefvNV)(GLenum pname, GLuint index, GLfloat *val);
extern void (APIENTRYP gglSampleMaskIndexedNV)(GLuint index, GLbitfield mask);
extern void (APIENTRYP gglTexRenderbufferNV)(GLenum target, GLuint renderbuffer);

#ifndef GL_NV_fence
#define GL_NV_fence
#define GL_ALL_COMPLETED_NV 0x84F2
#define GL_FENCE_STATUS_NV 0x84F3
#define GL_FENCE_CONDITION_NV 0x84F4
#endif
extern void (APIENTRYP gglDeleteFencesNV)(GLsizei n, const GLuint *fences);
extern void (APIENTRYP gglGenFencesNV)(GLsizei n, GLuint *fences);
extern GLboolean (APIENTRYP gglIsFenceNV)(GLuint fence);
extern GLboolean (APIENTRYP gglTestFenceNV)(GLuint fence);
extern void (APIENTRYP gglGetFenceivNV)(GLuint fence, GLenum pname, GLint *params);
extern void (APIENTRYP gglFinishFenceNV)(GLuint fence);
extern void (APIENTRYP gglSetFenceNV)(GLuint fence, GLenum condition);

#ifndef GL_NV_fill_rectangle
#define GL_NV_fill_rectangle
#define GL_FILL_RECTANGLE_NV 0x933C
#endif

#ifndef GL_NV_float_buffer
#define GL_NV_float_buffer
#define GL_FLOAT_R_NV 0x8880
#define GL_FLOAT_RG_NV 0x8881
#define GL_FLOAT_RGB_NV 0x8882
#define GL_FLOAT_RGBA_NV 0x8883
#define GL_FLOAT_R16_NV 0x8884
#define GL_FLOAT_R32_NV 0x8885
#define GL_FLOAT_RG16_NV 0x8886
#define GL_FLOAT_RG32_NV 0x8887
#define GL_FLOAT_RGB16_NV 0x8888
#define GL_FLOAT_RGB32_NV 0x8889
#define GL_FLOAT_RGBA16_NV 0x888A
#define GL_FLOAT_RGBA32_NV 0x888B
#define GL_TEXTURE_FLOAT_COMPONENTS_NV 0x888C
#define GL_FLOAT_CLEAR_COLOR_VALUE_NV 0x888D
#define GL_FLOAT_RGBA_MODE_NV 0x888E
#endif

#ifndef GL_NV_fog_distance
#define GL_NV_fog_distance
#define GL_FOG_DISTANCE_MODE_NV 0x855A
#define GL_EYE_RADIAL_NV 0x855B
#define GL_EYE_PLANE_ABSOLUTE_NV 0x855C
#define GL_EYE_PLANE 0x2502
#endif

#ifndef GL_NV_fragment_coverage_to_color
#define GL_NV_fragment_coverage_to_color
#define GL_FRAGMENT_COVERAGE_TO_COLOR_NV 0x92DD
#define GL_FRAGMENT_COVERAGE_COLOR_NV 0x92DE
#endif
extern void (APIENTRYP gglFragmentCoverageColorNV)(GLuint color);

#ifndef GL_NV_fragment_program
#define GL_NV_fragment_program
#define GL_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMETERS_NV 0x8868
#define GL_FRAGMENT_PROGRAM_NV 0x8870
#define GL_MAX_TEXTURE_COORDS_NV 0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_NV 0x8872
#define GL_FRAGMENT_PROGRAM_BINDING_NV 0x8873
#define GL_PROGRAM_ERROR_STRING_NV 0x8874
#endif
extern void (APIENTRYP gglProgramNamedParameter4fNV)(GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglProgramNamedParameter4fvNV)(GLuint id, GLsizei len, const GLubyte *name, const GLfloat *v);
extern void (APIENTRYP gglProgramNamedParameter4dNV)(GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglProgramNamedParameter4dvNV)(GLuint id, GLsizei len, const GLubyte *name, const GLdouble *v);
extern void (APIENTRYP gglGetProgramNamedParameterfvNV)(GLuint id, GLsizei len, const GLubyte *name, GLfloat *params);
extern void (APIENTRYP gglGetProgramNamedParameterdvNV)(GLuint id, GLsizei len, const GLubyte *name, GLdouble *params);

#ifndef GL_NV_fragment_program2
#define GL_NV_fragment_program2
#define GL_MAX_PROGRAM_EXEC_INSTRUCTIONS_NV 0x88F4
#define GL_MAX_PROGRAM_CALL_DEPTH_NV 0x88F5
#define GL_MAX_PROGRAM_IF_DEPTH_NV 0x88F6
#define GL_MAX_PROGRAM_LOOP_DEPTH_NV 0x88F7
#define GL_MAX_PROGRAM_LOOP_COUNT_NV 0x88F8
#endif

#ifndef GL_NV_fragment_program4
#define GL_NV_fragment_program4
#endif

#ifndef GL_NV_fragment_program_option
#define GL_NV_fragment_program_option
#endif

#ifndef GL_NV_fragment_shader_interlock
#define GL_NV_fragment_shader_interlock
#endif

#ifndef GL_NV_framebuffer_mixed_samples
#define GL_NV_framebuffer_mixed_samples
/* reuse GL_RASTER_MULTISAMPLE_EXT */
#define GL_COVERAGE_MODULATION_TABLE_NV 0x9331
/* reuse GL_RASTER_SAMPLES_EXT */
/* reuse GL_MAX_RASTER_SAMPLES_EXT */
/* reuse GL_RASTER_FIXED_SAMPLE_LOCATIONS_EXT */
/* reuse GL_MULTISAMPLE_RASTERIZATION_ALLOWED_EXT */
/* reuse GL_EFFECTIVE_RASTER_SAMPLES_EXT */
#define GL_COLOR_SAMPLES_NV 0x8E20
#define GL_DEPTH_SAMPLES_NV 0x932D
#define GL_STENCIL_SAMPLES_NV 0x932E
#define GL_MIXED_DEPTH_SAMPLES_SUPPORTED_NV 0x932F
#define GL_MIXED_STENCIL_SAMPLES_SUPPORTED_NV 0x9330
#define GL_COVERAGE_MODULATION_NV 0x9332
#define GL_COVERAGE_MODULATION_TABLE_SIZE_NV 0x9333
#endif
/* reuse void (APIENTRYP gglRasterSamplesEXT)(GLuint samples, GLboolean fixedsamplelocations) */
extern void (APIENTRYP gglCoverageModulationTableNV)(GLsizei n, const GLfloat *v);
extern void (APIENTRYP gglGetCoverageModulationTableNV)(GLsizei bufsize, GLfloat *v);
extern void (APIENTRYP gglCoverageModulationNV)(GLenum components);

#ifndef GL_NV_framebuffer_multisample_coverage
#define GL_NV_framebuffer_multisample_coverage
#define GL_RENDERBUFFER_COVERAGE_SAMPLES_NV 0x8CAB
#define GL_RENDERBUFFER_COLOR_SAMPLES_NV 0x8E10
#define GL_MAX_MULTISAMPLE_COVERAGE_MODES_NV 0x8E11
#define GL_MULTISAMPLE_COVERAGE_MODES_NV 0x8E12
#endif
extern void (APIENTRYP gglRenderbufferStorageMultisampleCoverageNV)(GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height);

#ifndef GL_NV_geometry_program4
#define GL_NV_geometry_program4
/* reuse GL_LINES_ADJACENCY_EXT */
/* reuse GL_LINE_STRIP_ADJACENCY_EXT */
/* reuse GL_TRIANGLES_ADJACENCY_EXT */
/* reuse GL_TRIANGLE_STRIP_ADJACENCY_EXT */
#define GL_GEOMETRY_PROGRAM_NV 0x8C26
#define GL_MAX_PROGRAM_OUTPUT_VERTICES_NV 0x8C27
#define GL_MAX_PROGRAM_TOTAL_OUTPUT_COMPONENTS_NV 0x8C28
/* reuse GL_GEOMETRY_VERTICES_OUT_EXT */
/* reuse GL_GEOMETRY_INPUT_TYPE_EXT */
/* reuse GL_GEOMETRY_OUTPUT_TYPE_EXT */
/* reuse GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT */
/* reuse GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT */
/* reuse GL_FRAMEBUFFER_INCOMPLETE_LAYER_COUNT_EXT */
/* reuse GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER_EXT */
/* reuse GL_PROGRAM_POINT_SIZE_EXT */
#endif
extern void (APIENTRYP gglProgramVertexLimitNV)(GLenum target, GLint limit);
extern void (APIENTRYP gglFramebufferTextureEXT)(GLenum target, GLenum attachment, GLuint texture, GLint level);
/* reuse void (APIENTRYP gglFramebufferTextureLayerEXT)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) */
extern void (APIENTRYP gglFramebufferTextureFaceEXT)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face);

#ifndef GL_NV_geometry_shader4
#define GL_NV_geometry_shader4
#endif

#ifndef GL_NV_geometry_shader_passthrough
#define GL_NV_geometry_shader_passthrough
#endif

#ifndef GL_NV_gpu_program4
#define GL_NV_gpu_program4
#define GL_MIN_PROGRAM_TEXEL_OFFSET_NV 0x8904
#define GL_MAX_PROGRAM_TEXEL_OFFSET_NV 0x8905
#define GL_PROGRAM_ATTRIB_COMPONENTS_NV 0x8906
#define GL_PROGRAM_RESULT_COMPONENTS_NV 0x8907
#define GL_MAX_PROGRAM_ATTRIB_COMPONENTS_NV 0x8908
#define GL_MAX_PROGRAM_RESULT_COMPONENTS_NV 0x8909
#define GL_MAX_PROGRAM_GENERIC_ATTRIBS_NV 0x8DA5
#define GL_MAX_PROGRAM_GENERIC_RESULTS_NV 0x8DA6
#endif
extern void (APIENTRYP gglProgramLocalParameterI4iNV)(GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w);
extern void (APIENTRYP gglProgramLocalParameterI4ivNV)(GLenum target, GLuint index, const GLint *params);
extern void (APIENTRYP gglProgramLocalParametersI4ivNV)(GLenum target, GLuint index, GLsizei count, const GLint *params);
extern void (APIENTRYP gglProgramLocalParameterI4uiNV)(GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
extern void (APIENTRYP gglProgramLocalParameterI4uivNV)(GLenum target, GLuint index, const GLuint *params);
extern void (APIENTRYP gglProgramLocalParametersI4uivNV)(GLenum target, GLuint index, GLsizei count, const GLuint *params);
extern void (APIENTRYP gglProgramEnvParameterI4iNV)(GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w);
extern void (APIENTRYP gglProgramEnvParameterI4ivNV)(GLenum target, GLuint index, const GLint *params);
extern void (APIENTRYP gglProgramEnvParametersI4ivNV)(GLenum target, GLuint index, GLsizei count, const GLint *params);
extern void (APIENTRYP gglProgramEnvParameterI4uiNV)(GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
extern void (APIENTRYP gglProgramEnvParameterI4uivNV)(GLenum target, GLuint index, const GLuint *params);
extern void (APIENTRYP gglProgramEnvParametersI4uivNV)(GLenum target, GLuint index, GLsizei count, const GLuint *params);
extern void (APIENTRYP gglGetProgramLocalParameterIivNV)(GLenum target, GLuint index, GLint *params);
extern void (APIENTRYP gglGetProgramLocalParameterIuivNV)(GLenum target, GLuint index, GLuint *params);
extern void (APIENTRYP gglGetProgramEnvParameterIivNV)(GLenum target, GLuint index, GLint *params);
extern void (APIENTRYP gglGetProgramEnvParameterIuivNV)(GLenum target, GLuint index, GLuint *params);

#ifndef GL_NV_gpu_program5
#define GL_NV_gpu_program5
#define GL_MAX_GEOMETRY_PROGRAM_INVOCATIONS_NV 0x8E5A
#define GL_MIN_FRAGMENT_INTERPOLATION_OFFSET_NV 0x8E5B
#define GL_MAX_FRAGMENT_INTERPOLATION_OFFSET_NV 0x8E5C
#define GL_FRAGMENT_PROGRAM_INTERPOLATION_OFFSET_BITS_NV 0x8E5D
#define GL_MIN_PROGRAM_TEXTURE_GATHER_OFFSET_NV 0x8E5E
#define GL_MAX_PROGRAM_TEXTURE_GATHER_OFFSET_NV 0x8E5F
#define GL_MAX_PROGRAM_SUBROUTINE_PARAMETERS_NV 0x8F44
#define GL_MAX_PROGRAM_SUBROUTINE_NUM_NV 0x8F45
#endif
extern void (APIENTRYP gglProgramSubroutineParametersuivNV)(GLenum target, GLsizei count, const GLuint *params);
extern void (APIENTRYP gglGetProgramSubroutineParameteruivNV)(GLenum target, GLuint index, GLuint *param);

#ifndef GL_NV_gpu_program5_mem_extended
#define GL_NV_gpu_program5_mem_extended
#endif

#ifndef GL_NV_gpu_shader5
#define GL_NV_gpu_shader5
/* reuse GL_INT64_NV */
/* reuse GL_UNSIGNED_INT64_NV */
/* reuse GL_INT8_NV */
/* reuse GL_INT8_VEC2_NV */
/* reuse GL_INT8_VEC3_NV */
/* reuse GL_INT8_VEC4_NV */
/* reuse GL_INT16_NV */
/* reuse GL_INT16_VEC2_NV */
/* reuse GL_INT16_VEC3_NV */
/* reuse GL_INT16_VEC4_NV */
/* reuse GL_INT64_VEC2_NV */
/* reuse GL_INT64_VEC3_NV */
/* reuse GL_INT64_VEC4_NV */
/* reuse GL_UNSIGNED_INT8_NV */
/* reuse GL_UNSIGNED_INT8_VEC2_NV */
/* reuse GL_UNSIGNED_INT8_VEC3_NV */
/* reuse GL_UNSIGNED_INT8_VEC4_NV */
/* reuse GL_UNSIGNED_INT16_NV */
/* reuse GL_UNSIGNED_INT16_VEC2_NV */
/* reuse GL_UNSIGNED_INT16_VEC3_NV */
/* reuse GL_UNSIGNED_INT16_VEC4_NV */
/* reuse GL_UNSIGNED_INT64_VEC2_NV */
/* reuse GL_UNSIGNED_INT64_VEC3_NV */
/* reuse GL_UNSIGNED_INT64_VEC4_NV */
/* reuse GL_FLOAT16_NV */
/* reuse GL_FLOAT16_VEC2_NV */
/* reuse GL_FLOAT16_VEC3_NV */
/* reuse GL_FLOAT16_VEC4_NV */
/* reuse GL_PATCHES */
#endif
/* reuse void (APIENTRYP gglUniform1i64NV)(GLint location, GLint64EXT x) */
/* reuse void (APIENTRYP gglUniform2i64NV)(GLint location, GLint64EXT x, GLint64EXT y) */
/* reuse void (APIENTRYP gglUniform3i64NV)(GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z) */
/* reuse void (APIENTRYP gglUniform4i64NV)(GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w) */
/* reuse void (APIENTRYP gglUniform1i64vNV)(GLint location, GLsizei count, const GLint64EXT *value) */
/* reuse void (APIENTRYP gglUniform2i64vNV)(GLint location, GLsizei count, const GLint64EXT *value) */
/* reuse void (APIENTRYP gglUniform3i64vNV)(GLint location, GLsizei count, const GLint64EXT *value) */
/* reuse void (APIENTRYP gglUniform4i64vNV)(GLint location, GLsizei count, const GLint64EXT *value) */
/* reuse void (APIENTRYP gglUniform1ui64NV)(GLint location, GLuint64EXT x) */
/* reuse void (APIENTRYP gglUniform2ui64NV)(GLint location, GLuint64EXT x, GLuint64EXT y) */
/* reuse void (APIENTRYP gglUniform3ui64NV)(GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z) */
/* reuse void (APIENTRYP gglUniform4ui64NV)(GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w) */
/* reuse void (APIENTRYP gglUniform1ui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value) */
/* reuse void (APIENTRYP gglUniform2ui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value) */
/* reuse void (APIENTRYP gglUniform3ui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value) */
/* reuse void (APIENTRYP gglUniform4ui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value) */
/* reuse void (APIENTRYP gglGetUniformi64vNV)(GLuint program, GLint location, GLint64EXT *params) */
/* reuse void (APIENTRYP gglProgramUniform1i64NV)(GLuint program, GLint location, GLint64EXT x) */
/* reuse void (APIENTRYP gglProgramUniform2i64NV)(GLuint program, GLint location, GLint64EXT x, GLint64EXT y) */
/* reuse void (APIENTRYP gglProgramUniform3i64NV)(GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z) */
/* reuse void (APIENTRYP gglProgramUniform4i64NV)(GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w) */
/* reuse void (APIENTRYP gglProgramUniform1i64vNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value) */
/* reuse void (APIENTRYP gglProgramUniform2i64vNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value) */
/* reuse void (APIENTRYP gglProgramUniform3i64vNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value) */
/* reuse void (APIENTRYP gglProgramUniform4i64vNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value) */
/* reuse void (APIENTRYP gglProgramUniform1ui64NV)(GLuint program, GLint location, GLuint64EXT x) */
/* reuse void (APIENTRYP gglProgramUniform2ui64NV)(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y) */
/* reuse void (APIENTRYP gglProgramUniform3ui64NV)(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z) */
/* reuse void (APIENTRYP gglProgramUniform4ui64NV)(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w) */
/* reuse void (APIENTRYP gglProgramUniform1ui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) */
/* reuse void (APIENTRYP gglProgramUniform2ui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) */
/* reuse void (APIENTRYP gglProgramUniform3ui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) */
/* reuse void (APIENTRYP gglProgramUniform4ui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) */

#ifndef GL_NV_half_float
#define GL_NV_half_float
#define GL_HALF_FLOAT_NV 0x140B
#endif
extern void (APIENTRYP gglVertex2hNV)(GLhalfNV x, GLhalfNV y);
extern void (APIENTRYP gglVertex2hvNV)(const GLhalfNV *v);
extern void (APIENTRYP gglVertex3hNV)(GLhalfNV x, GLhalfNV y, GLhalfNV z);
extern void (APIENTRYP gglVertex3hvNV)(const GLhalfNV *v);
extern void (APIENTRYP gglVertex4hNV)(GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w);
extern void (APIENTRYP gglVertex4hvNV)(const GLhalfNV *v);
extern void (APIENTRYP gglNormal3hNV)(GLhalfNV nx, GLhalfNV ny, GLhalfNV nz);
extern void (APIENTRYP gglNormal3hvNV)(const GLhalfNV *v);
extern void (APIENTRYP gglColor3hNV)(GLhalfNV red, GLhalfNV green, GLhalfNV blue);
extern void (APIENTRYP gglColor3hvNV)(const GLhalfNV *v);
extern void (APIENTRYP gglColor4hNV)(GLhalfNV red, GLhalfNV green, GLhalfNV blue, GLhalfNV alpha);
extern void (APIENTRYP gglColor4hvNV)(const GLhalfNV *v);
extern void (APIENTRYP gglTexCoord1hNV)(GLhalfNV s);
extern void (APIENTRYP gglTexCoord1hvNV)(const GLhalfNV *v);
extern void (APIENTRYP gglTexCoord2hNV)(GLhalfNV s, GLhalfNV t);
extern void (APIENTRYP gglTexCoord2hvNV)(const GLhalfNV *v);
extern void (APIENTRYP gglTexCoord3hNV)(GLhalfNV s, GLhalfNV t, GLhalfNV r);
extern void (APIENTRYP gglTexCoord3hvNV)(const GLhalfNV *v);
extern void (APIENTRYP gglTexCoord4hNV)(GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q);
extern void (APIENTRYP gglTexCoord4hvNV)(const GLhalfNV *v);
extern void (APIENTRYP gglMultiTexCoord1hNV)(GLenum target, GLhalfNV s);
extern void (APIENTRYP gglMultiTexCoord1hvNV)(GLenum target, const GLhalfNV *v);
extern void (APIENTRYP gglMultiTexCoord2hNV)(GLenum target, GLhalfNV s, GLhalfNV t);
extern void (APIENTRYP gglMultiTexCoord2hvNV)(GLenum target, const GLhalfNV *v);
extern void (APIENTRYP gglMultiTexCoord3hNV)(GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r);
extern void (APIENTRYP gglMultiTexCoord3hvNV)(GLenum target, const GLhalfNV *v);
extern void (APIENTRYP gglMultiTexCoord4hNV)(GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q);
extern void (APIENTRYP gglMultiTexCoord4hvNV)(GLenum target, const GLhalfNV *v);
extern void (APIENTRYP gglFogCoordhNV)(GLhalfNV fog);
extern void (APIENTRYP gglFogCoordhvNV)(const GLhalfNV *fog);
extern void (APIENTRYP gglSecondaryColor3hNV)(GLhalfNV red, GLhalfNV green, GLhalfNV blue);
extern void (APIENTRYP gglSecondaryColor3hvNV)(const GLhalfNV *v);
extern void (APIENTRYP gglVertexWeighthNV)(GLhalfNV weight);
extern void (APIENTRYP gglVertexWeighthvNV)(const GLhalfNV *weight);
extern void (APIENTRYP gglVertexAttrib1hNV)(GLuint index, GLhalfNV x);
extern void (APIENTRYP gglVertexAttrib1hvNV)(GLuint index, const GLhalfNV *v);
extern void (APIENTRYP gglVertexAttrib2hNV)(GLuint index, GLhalfNV x, GLhalfNV y);
extern void (APIENTRYP gglVertexAttrib2hvNV)(GLuint index, const GLhalfNV *v);
extern void (APIENTRYP gglVertexAttrib3hNV)(GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z);
extern void (APIENTRYP gglVertexAttrib3hvNV)(GLuint index, const GLhalfNV *v);
extern void (APIENTRYP gglVertexAttrib4hNV)(GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w);
extern void (APIENTRYP gglVertexAttrib4hvNV)(GLuint index, const GLhalfNV *v);
extern void (APIENTRYP gglVertexAttribs1hvNV)(GLuint index, GLsizei n, const GLhalfNV *v);
extern void (APIENTRYP gglVertexAttribs2hvNV)(GLuint index, GLsizei n, const GLhalfNV *v);
extern void (APIENTRYP gglVertexAttribs3hvNV)(GLuint index, GLsizei n, const GLhalfNV *v);
extern void (APIENTRYP gglVertexAttribs4hvNV)(GLuint index, GLsizei n, const GLhalfNV *v);

#ifndef GL_NV_internalformat_sample_query
#define GL_NV_internalformat_sample_query
/* reuse GL_RENDERBUFFER */
/* reuse GL_TEXTURE_2D_MULTISAMPLE */
/* reuse GL_TEXTURE_2D_MULTISAMPLE_ARRAY */
#define GL_MULTISAMPLES_NV 0x9371
#define GL_SUPERSAMPLE_SCALE_X_NV 0x9372
#define GL_SUPERSAMPLE_SCALE_Y_NV 0x9373
#define GL_CONFORMANT_NV 0x9374
#endif
extern void (APIENTRYP gglGetInternalformatSampleivNV)(GLenum target, GLenum internalformat, GLsizei samples, GLenum pname, GLsizei bufSize, GLint *params);

#ifndef GL_NV_light_max_exponent
#define GL_NV_light_max_exponent
#define GL_MAX_SHININESS_NV 0x8504
#define GL_MAX_SPOT_EXPONENT_NV 0x8505
#endif

#ifndef GL_NV_multisample_coverage
#define GL_NV_multisample_coverage
/* reuse GL_SAMPLES_ARB */
/* reuse GL_COLOR_SAMPLES_NV */
#endif

#ifndef GL_NV_multisample_filter_hint
#define GL_NV_multisample_filter_hint
#define GL_MULTISAMPLE_FILTER_HINT_NV 0x8534
#endif

#ifndef GL_NV_occlusion_query
#define GL_NV_occlusion_query
#define GL_PIXEL_COUNTER_BITS_NV 0x8864
#define GL_CURRENT_OCCLUSION_QUERY_ID_NV 0x8865
#define GL_PIXEL_COUNT_NV 0x8866
#define GL_PIXEL_COUNT_AVAILABLE_NV 0x8867
#endif
extern void (APIENTRYP gglGenOcclusionQueriesNV)(GLsizei n, GLuint *ids);
extern void (APIENTRYP gglDeleteOcclusionQueriesNV)(GLsizei n, const GLuint *ids);
extern GLboolean (APIENTRYP gglIsOcclusionQueryNV)(GLuint id);
extern void (APIENTRYP gglBeginOcclusionQueryNV)(GLuint id);
extern void (APIENTRYP gglEndOcclusionQueryNV)();
extern void (APIENTRYP gglGetOcclusionQueryivNV)(GLuint id, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetOcclusionQueryuivNV)(GLuint id, GLenum pname, GLuint *params);

#ifndef GL_NV_packed_depth_stencil
#define GL_NV_packed_depth_stencil
#define GL_DEPTH_STENCIL_NV 0x84F9
#define GL_UNSIGNED_INT_24_8_NV 0x84FA
#endif

#ifndef GL_NV_parameter_buffer_object
#define GL_NV_parameter_buffer_object
#define GL_MAX_PROGRAM_PARAMETER_BUFFER_BINDINGS_NV 0x8DA0
#define GL_MAX_PROGRAM_PARAMETER_BUFFER_SIZE_NV 0x8DA1
#define GL_VERTEX_PROGRAM_PARAMETER_BUFFER_NV 0x8DA2
#define GL_GEOMETRY_PROGRAM_PARAMETER_BUFFER_NV 0x8DA3
#define GL_FRAGMENT_PROGRAM_PARAMETER_BUFFER_NV 0x8DA4
#endif
extern void (APIENTRYP gglProgramBufferParametersfvNV)(GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLfloat *params);
extern void (APIENTRYP gglProgramBufferParametersIivNV)(GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLint *params);
extern void (APIENTRYP gglProgramBufferParametersIuivNV)(GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLuint *params);

#ifndef GL_NV_parameter_buffer_object2
#define GL_NV_parameter_buffer_object2
#endif

#ifndef GL_NV_path_rendering
#define GL_NV_path_rendering
#define GL_PATH_FORMAT_SVG_NV 0x9070
#define GL_PATH_FORMAT_PS_NV 0x9071
#define GL_STANDARD_FONT_NAME_NV 0x9072
#define GL_SYSTEM_FONT_NAME_NV 0x9073
#define GL_FILE_NAME_NV 0x9074
#define GL_PATH_STROKE_WIDTH_NV 0x9075
#define GL_PATH_END_CAPS_NV 0x9076
#define GL_PATH_INITIAL_END_CAP_NV 0x9077
#define GL_PATH_TERMINAL_END_CAP_NV 0x9078
#define GL_PATH_JOIN_STYLE_NV 0x9079
#define GL_PATH_MITER_LIMIT_NV 0x907A
#define GL_PATH_DASH_CAPS_NV 0x907B
#define GL_PATH_INITIAL_DASH_CAP_NV 0x907C
#define GL_PATH_TERMINAL_DASH_CAP_NV 0x907D
#define GL_PATH_DASH_OFFSET_NV 0x907E
#define GL_PATH_CLIENT_LENGTH_NV 0x907F
#define GL_PATH_FILL_MODE_NV 0x9080
#define GL_PATH_FILL_MASK_NV 0x9081
#define GL_PATH_FILL_COVER_MODE_NV 0x9082
#define GL_PATH_STROKE_COVER_MODE_NV 0x9083
#define GL_PATH_STROKE_MASK_NV 0x9084
#define GL_COUNT_UP_NV 0x9088
#define GL_COUNT_DOWN_NV 0x9089
#define GL_PATH_OBJECT_BOUNDING_BOX_NV 0x908A
#define GL_CONVEX_HULL_NV 0x908B
#define GL_BOUNDING_BOX_NV 0x908D
#define GL_TRANSLATE_X_NV 0x908E
#define GL_TRANSLATE_Y_NV 0x908F
#define GL_TRANSLATE_2D_NV 0x9090
#define GL_TRANSLATE_3D_NV 0x9091
#define GL_AFFINE_2D_NV 0x9092
#define GL_AFFINE_3D_NV 0x9094
#define GL_TRANSPOSE_AFFINE_2D_NV 0x9096
#define GL_TRANSPOSE_AFFINE_3D_NV 0x9098
#define GL_UTF8_NV 0x909A
#define GL_UTF16_NV 0x909B
#define GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV 0x909C
#define GL_PATH_COMMAND_COUNT_NV 0x909D
#define GL_PATH_COORD_COUNT_NV 0x909E
#define GL_PATH_DASH_ARRAY_COUNT_NV 0x909F
#define GL_PATH_COMPUTED_LENGTH_NV 0x90A0
#define GL_PATH_FILL_BOUNDING_BOX_NV 0x90A1
#define GL_PATH_STROKE_BOUNDING_BOX_NV 0x90A2
#define GL_SQUARE_NV 0x90A3
#define GL_ROUND_NV 0x90A4
#define GL_TRIANGULAR_NV 0x90A5
#define GL_BEVEL_NV 0x90A6
#define GL_MITER_REVERT_NV 0x90A7
#define GL_MITER_TRUNCATE_NV 0x90A8
#define GL_SKIP_MISSING_GLYPH_NV 0x90A9
#define GL_USE_MISSING_GLYPH_NV 0x90AA
#define GL_PATH_ERROR_POSITION_NV 0x90AB
#define GL_ACCUM_ADJACENT_PAIRS_NV 0x90AD
#define GL_ADJACENT_PAIRS_NV 0x90AE
#define GL_FIRST_TO_REST_NV 0x90AF
#define GL_PATH_GEN_MODE_NV 0x90B0
#define GL_PATH_GEN_COEFF_NV 0x90B1
#define GL_PATH_GEN_COMPONENTS_NV 0x90B3
#define GL_PATH_STENCIL_FUNC_NV 0x90B7
#define GL_PATH_STENCIL_REF_NV 0x90B8
#define GL_PATH_STENCIL_VALUE_MASK_NV 0x90B9
#define GL_PATH_STENCIL_DEPTH_OFFSET_FACTOR_NV 0x90BD
#define GL_PATH_STENCIL_DEPTH_OFFSET_UNITS_NV 0x90BE
#define GL_PATH_COVER_DEPTH_FUNC_NV 0x90BF
#define GL_PATH_DASH_OFFSET_RESET_NV 0x90B4
#define GL_MOVE_TO_RESETS_NV 0x90B5
#define GL_MOVE_TO_CONTINUES_NV 0x90B6
#define GL_CLOSE_PATH_NV 0x00
#define GL_MOVE_TO_NV 0x02
#define GL_RELATIVE_MOVE_TO_NV 0x03
#define GL_LINE_TO_NV 0x04
#define GL_RELATIVE_LINE_TO_NV 0x05
#define GL_HORIZONTAL_LINE_TO_NV 0x06
#define GL_RELATIVE_HORIZONTAL_LINE_TO_NV 0x07
#define GL_VERTICAL_LINE_TO_NV 0x08
#define GL_RELATIVE_VERTICAL_LINE_TO_NV 0x09
#define GL_QUADRATIC_CURVE_TO_NV 0x0A
#define GL_RELATIVE_QUADRATIC_CURVE_TO_NV 0x0B
#define GL_CUBIC_CURVE_TO_NV 0x0C
#define GL_RELATIVE_CUBIC_CURVE_TO_NV 0x0D
#define GL_SMOOTH_QUADRATIC_CURVE_TO_NV 0x0E
#define GL_RELATIVE_SMOOTH_QUADRATIC_CURVE_TO_NV 0x0F
#define GL_SMOOTH_CUBIC_CURVE_TO_NV 0x10
#define GL_RELATIVE_SMOOTH_CUBIC_CURVE_TO_NV 0x11
#define GL_SMALL_CCW_ARC_TO_NV 0x12
#define GL_RELATIVE_SMALL_CCW_ARC_TO_NV 0x13
#define GL_SMALL_CW_ARC_TO_NV 0x14
#define GL_RELATIVE_SMALL_CW_ARC_TO_NV 0x15
#define GL_LARGE_CCW_ARC_TO_NV 0x16
#define GL_RELATIVE_LARGE_CCW_ARC_TO_NV 0x17
#define GL_LARGE_CW_ARC_TO_NV 0x18
#define GL_RELATIVE_LARGE_CW_ARC_TO_NV 0x19
#define GL_RESTART_PATH_NV 0xF0
#define GL_DUP_FIRST_CUBIC_CURVE_TO_NV 0xF2
#define GL_DUP_LAST_CUBIC_CURVE_TO_NV 0xF4
#define GL_RECT_NV 0xF6
#define GL_CIRCULAR_CCW_ARC_TO_NV 0xF8
#define GL_CIRCULAR_CW_ARC_TO_NV 0xFA
#define GL_CIRCULAR_TANGENT_ARC_TO_NV 0xFC
#define GL_ARC_TO_NV 0xFE
#define GL_RELATIVE_ARC_TO_NV 0xFF
#define GL_BOLD_BIT_NV 0x01
#define GL_ITALIC_BIT_NV 0x02
#define GL_GLYPH_WIDTH_BIT_NV 0x01
#define GL_GLYPH_HEIGHT_BIT_NV 0x02
#define GL_GLYPH_HORIZONTAL_BEARING_X_BIT_NV 0x04
#define GL_GLYPH_HORIZONTAL_BEARING_Y_BIT_NV 0x08
#define GL_GLYPH_HORIZONTAL_BEARING_ADVANCE_BIT_NV 0x10
#define GL_GLYPH_VERTICAL_BEARING_X_BIT_NV 0x20
#define GL_GLYPH_VERTICAL_BEARING_Y_BIT_NV 0x40
#define GL_GLYPH_VERTICAL_BEARING_ADVANCE_BIT_NV 0x80
#define GL_GLYPH_HAS_KERNING_BIT_NV 0x100
#define GL_FONT_X_MIN_BOUNDS_BIT_NV 0x00010000
#define GL_FONT_Y_MIN_BOUNDS_BIT_NV 0x00020000
#define GL_FONT_X_MAX_BOUNDS_BIT_NV 0x00040000
#define GL_FONT_Y_MAX_BOUNDS_BIT_NV 0x00080000
#define GL_FONT_UNITS_PER_EM_BIT_NV 0x00100000
#define GL_FONT_ASCENDER_BIT_NV 0x00200000
#define GL_FONT_DESCENDER_BIT_NV 0x00400000
#define GL_FONT_HEIGHT_BIT_NV 0x00800000
#define GL_FONT_MAX_ADVANCE_WIDTH_BIT_NV 0x01000000
#define GL_FONT_MAX_ADVANCE_HEIGHT_BIT_NV 0x02000000
#define GL_FONT_UNDERLINE_POSITION_BIT_NV 0x04000000
#define GL_FONT_UNDERLINE_THICKNESS_BIT_NV 0x08000000
#define GL_FONT_HAS_KERNING_BIT_NV 0x10000000
#define GL_ROUNDED_RECT_NV 0xE8
#define GL_RELATIVE_ROUNDED_RECT_NV 0xE9
#define GL_ROUNDED_RECT2_NV 0xEA
#define GL_RELATIVE_ROUNDED_RECT2_NV 0xEB
#define GL_ROUNDED_RECT4_NV 0xEC
#define GL_RELATIVE_ROUNDED_RECT4_NV 0xED
#define GL_ROUNDED_RECT8_NV 0xEE
#define GL_RELATIVE_ROUNDED_RECT8_NV 0xEF
#define GL_RELATIVE_RECT_NV 0xF7
#define GL_FONT_GLYPHS_AVAILABLE_NV 0x9368
#define GL_FONT_TARGET_UNAVAILABLE_NV 0x9369
#define GL_FONT_UNAVAILABLE_NV 0x936A
#define GL_FONT_UNINTELLIGIBLE_NV 0x936B
#define GL_CONIC_CURVE_TO_NV 0x1A
#define GL_RELATIVE_CONIC_CURVE_TO_NV 0x1B
#define GL_FONT_NUM_GLYPH_INDICES_BIT_NV 0x20000000
#define GL_STANDARD_FONT_FORMAT_NV 0x936C
#define GL_2_BYTES_NV 0x1407
#define GL_3_BYTES_NV 0x1408
#define GL_4_BYTES_NV 0x1409
#define GL_EYE_LINEAR_NV 0x2400
#define GL_OBJECT_LINEAR_NV 0x2401
#define GL_CONSTANT_NV 0x8576
#define GL_PATH_FOG_GEN_MODE_NV 0x90AC
#define GL_PRIMARY_COLOR 0x8577
#define GL_PRIMARY_COLOR_NV 0x852C
#define GL_SECONDARY_COLOR_NV 0x852D
#define GL_PATH_GEN_COLOR_FORMAT_NV 0x90B2
#define GL_PATH_PROJECTION_NV 0x1701
#define GL_PATH_MODELVIEW_NV 0x1700
#define GL_PATH_MODELVIEW_STACK_DEPTH_NV 0x0BA3
#define GL_PATH_MODELVIEW_MATRIX_NV 0x0BA6
#define GL_PATH_MAX_MODELVIEW_STACK_DEPTH_NV 0x0D36
#define GL_PATH_TRANSPOSE_MODELVIEW_MATRIX_NV 0x84E3
#define GL_PATH_PROJECTION_STACK_DEPTH_NV 0x0BA4
#define GL_PATH_PROJECTION_MATRIX_NV 0x0BA7
#define GL_PATH_MAX_PROJECTION_STACK_DEPTH_NV 0x0D38
#define GL_PATH_TRANSPOSE_PROJECTION_MATRIX_NV 0x84E4
#define GL_FRAGMENT_INPUT_NV 0x936D
#endif
extern GLuint (APIENTRYP gglGenPathsNV)(GLsizei range);
extern void (APIENTRYP gglDeletePathsNV)(GLuint path, GLsizei range);
extern GLboolean (APIENTRYP gglIsPathNV)(GLuint path);
extern void (APIENTRYP gglPathCommandsNV)(GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords);
extern void (APIENTRYP gglPathCoordsNV)(GLuint path, GLsizei numCoords, GLenum coordType, const void *coords);
extern void (APIENTRYP gglPathSubCommandsNV)(GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords);
extern void (APIENTRYP gglPathSubCoordsNV)(GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void *coords);
extern void (APIENTRYP gglPathStringNV)(GLuint path, GLenum format, GLsizei length, const void *pathString);
extern void (APIENTRYP gglPathGlyphsNV)(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void *charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
extern void (APIENTRYP gglPathGlyphRangeNV)(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
extern void (APIENTRYP gglWeightPathsNV)(GLuint resultPath, GLsizei numPaths, const GLuint *paths, const GLfloat *weights);
extern void (APIENTRYP gglCopyPathNV)(GLuint resultPath, GLuint srcPath);
extern void (APIENTRYP gglInterpolatePathsNV)(GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight);
extern void (APIENTRYP gglTransformPathNV)(GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat *transformValues);
extern void (APIENTRYP gglPathParameterivNV)(GLuint path, GLenum pname, const GLint *value);
extern void (APIENTRYP gglPathParameteriNV)(GLuint path, GLenum pname, GLint value);
extern void (APIENTRYP gglPathParameterfvNV)(GLuint path, GLenum pname, const GLfloat *value);
extern void (APIENTRYP gglPathParameterfNV)(GLuint path, GLenum pname, GLfloat value);
extern void (APIENTRYP gglPathDashArrayNV)(GLuint path, GLsizei dashCount, const GLfloat *dashArray);
extern void (APIENTRYP gglPathStencilFuncNV)(GLenum func, GLint ref, GLuint mask);
extern void (APIENTRYP gglPathStencilDepthOffsetNV)(GLfloat factor, GLfloat units);
extern void (APIENTRYP gglStencilFillPathNV)(GLuint path, GLenum fillMode, GLuint mask);
extern void (APIENTRYP gglStencilStrokePathNV)(GLuint path, GLint reference, GLuint mask);
extern void (APIENTRYP gglStencilFillPathInstancedNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues);
extern void (APIENTRYP gglStencilStrokePathInstancedNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat *transformValues);
extern void (APIENTRYP gglPathCoverDepthFuncNV)(GLenum func);
extern void (APIENTRYP gglCoverFillPathNV)(GLuint path, GLenum coverMode);
extern void (APIENTRYP gglCoverStrokePathNV)(GLuint path, GLenum coverMode);
extern void (APIENTRYP gglCoverFillPathInstancedNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
extern void (APIENTRYP gglCoverStrokePathInstancedNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
extern void (APIENTRYP gglGetPathParameterivNV)(GLuint path, GLenum pname, GLint *value);
extern void (APIENTRYP gglGetPathParameterfvNV)(GLuint path, GLenum pname, GLfloat *value);
extern void (APIENTRYP gglGetPathCommandsNV)(GLuint path, GLubyte *commands);
extern void (APIENTRYP gglGetPathCoordsNV)(GLuint path, GLfloat *coords);
extern void (APIENTRYP gglGetPathDashArrayNV)(GLuint path, GLfloat *dashArray);
extern void (APIENTRYP gglGetPathMetricsNV)(GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics);
extern void (APIENTRYP gglGetPathMetricRangeNV)(GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics);
extern void (APIENTRYP gglGetPathSpacingNV)(GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat *returnedSpacing);
extern GLboolean (APIENTRYP gglIsPointInFillPathNV)(GLuint path, GLuint mask, GLfloat x, GLfloat y);
extern GLboolean (APIENTRYP gglIsPointInStrokePathNV)(GLuint path, GLfloat x, GLfloat y);
extern GLfloat (APIENTRYP gglGetPathLengthNV)(GLuint path, GLsizei startSegment, GLsizei numSegments);
extern GLboolean (APIENTRYP gglPointAlongPathNV)(GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat *x, GLfloat *y, GLfloat *tangentX, GLfloat *tangentY);
extern void (APIENTRYP gglMatrixLoad3x2fNV)(GLenum matrixMode, const GLfloat *m);
extern void (APIENTRYP gglMatrixLoad3x3fNV)(GLenum matrixMode, const GLfloat *m);
extern void (APIENTRYP gglMatrixLoadTranspose3x3fNV)(GLenum matrixMode, const GLfloat *m);
extern void (APIENTRYP gglMatrixMult3x2fNV)(GLenum matrixMode, const GLfloat *m);
extern void (APIENTRYP gglMatrixMult3x3fNV)(GLenum matrixMode, const GLfloat *m);
extern void (APIENTRYP gglMatrixMultTranspose3x3fNV)(GLenum matrixMode, const GLfloat *m);
extern void (APIENTRYP gglStencilThenCoverFillPathNV)(GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode);
extern void (APIENTRYP gglStencilThenCoverStrokePathNV)(GLuint path, GLint reference, GLuint mask, GLenum coverMode);
extern void (APIENTRYP gglStencilThenCoverFillPathInstancedNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
extern void (APIENTRYP gglStencilThenCoverStrokePathInstancedNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
extern GLenum (APIENTRYP gglPathGlyphIndexRangeNV)(GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint baseAndCount[2]);
extern GLenum (APIENTRYP gglPathGlyphIndexArrayNV)(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
extern GLenum (APIENTRYP gglPathMemoryGlyphIndexArrayNV)(GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void *fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
extern void (APIENTRYP gglProgramPathFragmentInputGenNV)(GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat *coeffs);
extern void (APIENTRYP gglGetProgramResourcefvNV)(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLfloat *params);
extern void (APIENTRYP gglPathColorGenNV)(GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat *coeffs);
extern void (APIENTRYP gglPathTexGenNV)(GLenum texCoordSet, GLenum genMode, GLint components, const GLfloat *coeffs);
extern void (APIENTRYP gglPathFogGenNV)(GLenum genMode);
extern void (APIENTRYP gglGetPathColorGenivNV)(GLenum color, GLenum pname, GLint *value);
extern void (APIENTRYP gglGetPathColorGenfvNV)(GLenum color, GLenum pname, GLfloat *value);
extern void (APIENTRYP gglGetPathTexGenivNV)(GLenum texCoordSet, GLenum pname, GLint *value);
extern void (APIENTRYP gglGetPathTexGenfvNV)(GLenum texCoordSet, GLenum pname, GLfloat *value);

#ifndef GL_NV_path_rendering_shared_edge
#define GL_NV_path_rendering_shared_edge
#define GL_SHARED_EDGE_NV 0xC0
#endif

#ifndef GL_NV_pixel_data_range
#define GL_NV_pixel_data_range
#define GL_WRITE_PIXEL_DATA_RANGE_NV 0x8878
#define GL_READ_PIXEL_DATA_RANGE_NV 0x8879
#define GL_WRITE_PIXEL_DATA_RANGE_LENGTH_NV 0x887A
#define GL_READ_PIXEL_DATA_RANGE_LENGTH_NV 0x887B
#define GL_WRITE_PIXEL_DATA_RANGE_POINTER_NV 0x887C
#define GL_READ_PIXEL_DATA_RANGE_POINTER_NV 0x887D
#endif
extern void (APIENTRYP gglPixelDataRangeNV)(GLenum target, GLsizei length, const void *pointer);
extern void (APIENTRYP gglFlushPixelDataRangeNV)(GLenum target);

#ifndef GL_NV_point_sprite
#define GL_NV_point_sprite
#define GL_POINT_SPRITE_NV 0x8861
#define GL_COORD_REPLACE_NV 0x8862
#define GL_POINT_SPRITE_R_MODE_NV 0x8863
#endif
extern void (APIENTRYP gglPointParameteriNV)(GLenum pname, GLint param);
extern void (APIENTRYP gglPointParameterivNV)(GLenum pname, const GLint *params);

#ifndef GL_NV_present_video
#define GL_NV_present_video
#define GL_FRAME_NV 0x8E26
#define GL_FIELDS_NV 0x8E27
#define GL_CURRENT_TIME_NV 0x8E28
#define GL_NUM_FILL_STREAMS_NV 0x8E29
#define GL_PRESENT_TIME_NV 0x8E2A
#define GL_PRESENT_DURATION_NV 0x8E2B
#endif
extern void (APIENTRYP gglPresentFrameKeyedNV)(GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLuint key0, GLenum target1, GLuint fill1, GLuint key1);
extern void (APIENTRYP gglPresentFrameDualFillNV)(GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLenum target1, GLuint fill1, GLenum target2, GLuint fill2, GLenum target3, GLuint fill3);
extern void (APIENTRYP gglGetVideoivNV)(GLuint video_slot, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetVideouivNV)(GLuint video_slot, GLenum pname, GLuint *params);
extern void (APIENTRYP gglGetVideoi64vNV)(GLuint video_slot, GLenum pname, GLint64EXT *params);
extern void (APIENTRYP gglGetVideoui64vNV)(GLuint video_slot, GLenum pname, GLuint64EXT *params);

#ifndef GL_NV_primitive_restart
#define GL_NV_primitive_restart
#define GL_PRIMITIVE_RESTART_NV 0x8558
#define GL_PRIMITIVE_RESTART_INDEX_NV 0x8559
#endif
extern void (APIENTRYP gglPrimitiveRestartNV)();
extern void (APIENTRYP gglPrimitiveRestartIndexNV)(GLuint index);

#ifndef GL_NV_register_combiners
#define GL_NV_register_combiners
#define GL_REGISTER_COMBINERS_NV 0x8522
#define GL_VARIABLE_A_NV 0x8523
#define GL_VARIABLE_B_NV 0x8524
#define GL_VARIABLE_C_NV 0x8525
#define GL_VARIABLE_D_NV 0x8526
#define GL_VARIABLE_E_NV 0x8527
#define GL_VARIABLE_F_NV 0x8528
#define GL_VARIABLE_G_NV 0x8529
#define GL_CONSTANT_COLOR0_NV 0x852A
#define GL_CONSTANT_COLOR1_NV 0x852B
/* reuse GL_PRIMARY_COLOR_NV */
/* reuse GL_SECONDARY_COLOR_NV */
#define GL_SPARE0_NV 0x852E
#define GL_SPARE1_NV 0x852F
#define GL_DISCARD_NV 0x8530
#define GL_E_TIMES_F_NV 0x8531
#define GL_SPARE0_PLUS_SECONDARY_COLOR_NV 0x8532
#define GL_UNSIGNED_IDENTITY_NV 0x8536
#define GL_UNSIGNED_INVERT_NV 0x8537
#define GL_EXPAND_NORMAL_NV 0x8538
#define GL_EXPAND_NEGATE_NV 0x8539
#define GL_HALF_BIAS_NORMAL_NV 0x853A
#define GL_HALF_BIAS_NEGATE_NV 0x853B
#define GL_SIGNED_IDENTITY_NV 0x853C
#define GL_SIGNED_NEGATE_NV 0x853D
#define GL_SCALE_BY_TWO_NV 0x853E
#define GL_SCALE_BY_FOUR_NV 0x853F
#define GL_SCALE_BY_ONE_HALF_NV 0x8540
#define GL_BIAS_BY_NEGATIVE_ONE_HALF_NV 0x8541
#define GL_COMBINER_INPUT_NV 0x8542
#define GL_COMBINER_MAPPING_NV 0x8543
#define GL_COMBINER_COMPONENT_USAGE_NV 0x8544
#define GL_COMBINER_AB_DOT_PRODUCT_NV 0x8545
#define GL_COMBINER_CD_DOT_PRODUCT_NV 0x8546
#define GL_COMBINER_MUX_SUM_NV 0x8547
#define GL_COMBINER_SCALE_NV 0x8548
#define GL_COMBINER_BIAS_NV 0x8549
#define GL_COMBINER_AB_OUTPUT_NV 0x854A
#define GL_COMBINER_CD_OUTPUT_NV 0x854B
#define GL_COMBINER_SUM_OUTPUT_NV 0x854C
#define GL_MAX_GENERAL_COMBINERS_NV 0x854D
#define GL_NUM_GENERAL_COMBINERS_NV 0x854E
#define GL_COLOR_SUM_CLAMP_NV 0x854F
#define GL_COMBINER0_NV 0x8550
#define GL_COMBINER1_NV 0x8551
#define GL_COMBINER2_NV 0x8552
#define GL_COMBINER3_NV 0x8553
#define GL_COMBINER4_NV 0x8554
#define GL_COMBINER5_NV 0x8555
#define GL_COMBINER6_NV 0x8556
#define GL_COMBINER7_NV 0x8557
/* reuse GL_TEXTURE0_ARB */
/* reuse GL_TEXTURE1_ARB */
/* reuse GL_ZERO */
/* reuse GL_NONE */
#define GL_FOG 0x0B60
#endif
extern void (APIENTRYP gglCombinerParameterfvNV)(GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglCombinerParameterfNV)(GLenum pname, GLfloat param);
extern void (APIENTRYP gglCombinerParameterivNV)(GLenum pname, const GLint *params);
extern void (APIENTRYP gglCombinerParameteriNV)(GLenum pname, GLint param);
extern void (APIENTRYP gglCombinerInputNV)(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
extern void (APIENTRYP gglCombinerOutputNV)(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
extern void (APIENTRYP gglFinalCombinerInputNV)(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
extern void (APIENTRYP gglGetCombinerInputParameterfvNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetCombinerInputParameterivNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetCombinerOutputParameterfvNV)(GLenum stage, GLenum portion, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetCombinerOutputParameterivNV)(GLenum stage, GLenum portion, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetFinalCombinerInputParameterfvNV)(GLenum variable, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetFinalCombinerInputParameterivNV)(GLenum variable, GLenum pname, GLint *params);

#ifndef GL_NV_register_combiners2
#define GL_NV_register_combiners2
#define GL_PER_STAGE_CONSTANTS_NV 0x8535
#endif
extern void (APIENTRYP gglCombinerStageParameterfvNV)(GLenum stage, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglGetCombinerStageParameterfvNV)(GLenum stage, GLenum pname, GLfloat *params);

#ifndef GL_NV_robustness_video_memory_purge
#define GL_NV_robustness_video_memory_purge
#define GL_PURGED_CONTEXT_RESET_NV 0x92BB
#endif

#ifndef GL_NV_sample_locations
#define GL_NV_sample_locations
#define GL_SAMPLE_LOCATION_SUBPIXEL_BITS_NV 0x933D
#define GL_SAMPLE_LOCATION_PIXEL_GRID_WIDTH_NV 0x933E
#define GL_SAMPLE_LOCATION_PIXEL_GRID_HEIGHT_NV 0x933F
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_TABLE_SIZE_NV 0x9340
#define GL_SAMPLE_LOCATION_NV 0x8E50
#define GL_PROGRAMMABLE_SAMPLE_LOCATION_NV 0x9341
#define GL_FRAMEBUFFER_PROGRAMMABLE_SAMPLE_LOCATIONS_NV 0x9342
#define GL_FRAMEBUFFER_SAMPLE_LOCATION_PIXEL_GRID_NV 0x9343
#endif
extern void (APIENTRYP gglFramebufferSampleLocationsfvNV)(GLenum target, GLuint start, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglNamedFramebufferSampleLocationsfvNV)(GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglResolveDepthValuesNV)();

#ifndef GL_NV_sample_mask_override_coverage
#define GL_NV_sample_mask_override_coverage
#endif

#ifndef GL_NV_shader_atomic_counters
#define GL_NV_shader_atomic_counters
#endif

#ifndef GL_NV_shader_atomic_float
#define GL_NV_shader_atomic_float
#endif

#ifndef GL_NV_shader_atomic_float64
#define GL_NV_shader_atomic_float64
#endif

#ifndef GL_NV_shader_atomic_fp16_vector
#define GL_NV_shader_atomic_fp16_vector
#endif

#ifndef GL_NV_shader_atomic_int64
#define GL_NV_shader_atomic_int64
#endif

#ifndef GL_NV_shader_buffer_load
#define GL_NV_shader_buffer_load
#define GL_BUFFER_GPU_ADDRESS_NV 0x8F1D
#define GL_GPU_ADDRESS_NV 0x8F34
#define GL_MAX_SHADER_BUFFER_ADDRESS_NV 0x8F35
#endif
extern void (APIENTRYP gglMakeBufferResidentNV)(GLenum target, GLenum access);
extern void (APIENTRYP gglMakeBufferNonResidentNV)(GLenum target);
extern GLboolean (APIENTRYP gglIsBufferResidentNV)(GLenum target);
extern void (APIENTRYP gglMakeNamedBufferResidentNV)(GLuint buffer, GLenum access);
extern void (APIENTRYP gglMakeNamedBufferNonResidentNV)(GLuint buffer);
extern GLboolean (APIENTRYP gglIsNamedBufferResidentNV)(GLuint buffer);
extern void (APIENTRYP gglGetBufferParameterui64vNV)(GLenum target, GLenum pname, GLuint64EXT *params);
extern void (APIENTRYP gglGetNamedBufferParameterui64vNV)(GLuint buffer, GLenum pname, GLuint64EXT *params);
extern void (APIENTRYP gglGetIntegerui64vNV)(GLenum value, GLuint64EXT *result);
extern void (APIENTRYP gglUniformui64NV)(GLint location, GLuint64EXT value);
extern void (APIENTRYP gglUniformui64vNV)(GLint location, GLsizei count, const GLuint64EXT *value);
/* reuse void (APIENTRYP gglGetUniformui64vNV)(GLuint program, GLint location, GLuint64EXT *params) */
extern void (APIENTRYP gglProgramUniformui64NV)(GLuint program, GLint location, GLuint64EXT value);
extern void (APIENTRYP gglProgramUniformui64vNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);

#ifndef GL_NV_shader_buffer_store
#define GL_NV_shader_buffer_store
#define GL_SHADER_GLOBAL_ACCESS_BARRIER_BIT_NV 0x00000010
/* reuse GL_READ_WRITE */
/* reuse GL_WRITE_ONLY */
#endif

#ifndef GL_NV_shader_storage_buffer_object
#define GL_NV_shader_storage_buffer_object
#endif

#ifndef GL_NV_shader_thread_group
#define GL_NV_shader_thread_group
#define GL_WARP_SIZE_NV 0x9339
#define GL_WARPS_PER_SM_NV 0x933A
#define GL_SM_COUNT_NV 0x933B
#endif

#ifndef GL_NV_shader_thread_shuffle
#define GL_NV_shader_thread_shuffle
#endif

#ifndef GL_NV_stereo_view_rendering
#define GL_NV_stereo_view_rendering
#endif

#ifndef GL_NV_tessellation_program5
#define GL_NV_tessellation_program5
#define GL_MAX_PROGRAM_PATCH_ATTRIBS_NV 0x86D8
#define GL_TESS_CONTROL_PROGRAM_NV 0x891E
#define GL_TESS_EVALUATION_PROGRAM_NV 0x891F
#define GL_TESS_CONTROL_PROGRAM_PARAMETER_BUFFER_NV 0x8C74
#define GL_TESS_EVALUATION_PROGRAM_PARAMETER_BUFFER_NV 0x8C75
#endif

#ifndef GL_NV_texgen_emboss
#define GL_NV_texgen_emboss
#define GL_EMBOSS_LIGHT_NV 0x855D
#define GL_EMBOSS_CONSTANT_NV 0x855E
#define GL_EMBOSS_MAP_NV 0x855F
#endif

#ifndef GL_NV_texgen_reflection
#define GL_NV_texgen_reflection
#define GL_NORMAL_MAP_NV 0x8511
#define GL_REFLECTION_MAP_NV 0x8512
#endif

#ifndef GL_NV_texture_barrier
#define GL_NV_texture_barrier
#endif
extern void (APIENTRYP gglTextureBarrierNV)();

#ifndef GL_NV_texture_compression_vtc
#define GL_NV_texture_compression_vtc
#endif

#ifndef GL_NV_texture_env_combine4
#define GL_NV_texture_env_combine4
#define GL_COMBINE4_NV 0x8503
#define GL_SOURCE3_RGB_NV 0x8583
#define GL_SOURCE3_ALPHA_NV 0x858B
#define GL_OPERAND3_RGB_NV 0x8593
#define GL_OPERAND3_ALPHA_NV 0x859B
#endif

#ifndef GL_NV_texture_expand_normal
#define GL_NV_texture_expand_normal
#define GL_TEXTURE_UNSIGNED_REMAP_MODE_NV 0x888F
#endif

#ifndef GL_NV_texture_multisample
#define GL_NV_texture_multisample
#define GL_TEXTURE_COVERAGE_SAMPLES_NV 0x9045
#define GL_TEXTURE_COLOR_SAMPLES_NV 0x9046
#endif
extern void (APIENTRYP gglTexImage2DMultisampleCoverageNV)(GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations);
extern void (APIENTRYP gglTexImage3DMultisampleCoverageNV)(GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations);
extern void (APIENTRYP gglTextureImage2DMultisampleNV)(GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations);
extern void (APIENTRYP gglTextureImage3DMultisampleNV)(GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations);
extern void (APIENTRYP gglTextureImage2DMultisampleCoverageNV)(GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations);
extern void (APIENTRYP gglTextureImage3DMultisampleCoverageNV)(GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations);

#ifndef GL_NV_texture_rectangle
#define GL_NV_texture_rectangle
#define GL_TEXTURE_RECTANGLE_NV 0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE_NV 0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE_NV 0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE_NV 0x84F8
#endif

#ifndef GL_NV_texture_shader
#define GL_NV_texture_shader
#define GL_OFFSET_TEXTURE_RECTANGLE_NV 0x864C
#define GL_OFFSET_TEXTURE_RECTANGLE_SCALE_NV 0x864D
#define GL_DOT_PRODUCT_TEXTURE_RECTANGLE_NV 0x864E
#define GL_RGBA_UNSIGNED_DOT_PRODUCT_MAPPING_NV 0x86D9
#define GL_UNSIGNED_INT_S8_S8_8_8_NV 0x86DA
#define GL_UNSIGNED_INT_8_8_S8_S8_REV_NV 0x86DB
#define GL_DSDT_MAG_INTENSITY_NV 0x86DC
#define GL_SHADER_CONSISTENT_NV 0x86DD
#define GL_TEXTURE_SHADER_NV 0x86DE
#define GL_SHADER_OPERATION_NV 0x86DF
#define GL_CULL_MODES_NV 0x86E0
#define GL_OFFSET_TEXTURE_MATRIX_NV 0x86E1
#define GL_OFFSET_TEXTURE_SCALE_NV 0x86E2
#define GL_OFFSET_TEXTURE_BIAS_NV 0x86E3
#define GL_OFFSET_TEXTURE_2D_MATRIX_NV 0x86E1
#define GL_OFFSET_TEXTURE_2D_SCALE_NV 0x86E2
#define GL_OFFSET_TEXTURE_2D_BIAS_NV 0x86E3
#define GL_PREVIOUS_TEXTURE_INPUT_NV 0x86E4
#define GL_CONST_EYE_NV 0x86E5
#define GL_PASS_THROUGH_NV 0x86E6
#define GL_CULL_FRAGMENT_NV 0x86E7
#define GL_OFFSET_TEXTURE_2D_NV 0x86E8
#define GL_DEPENDENT_AR_TEXTURE_2D_NV 0x86E9
#define GL_DEPENDENT_GB_TEXTURE_2D_NV 0x86EA
#define GL_DOT_PRODUCT_NV 0x86EC
#define GL_DOT_PRODUCT_DEPTH_REPLACE_NV 0x86ED
#define GL_DOT_PRODUCT_TEXTURE_2D_NV 0x86EE
#define GL_DOT_PRODUCT_TEXTURE_CUBE_MAP_NV 0x86F0
#define GL_DOT_PRODUCT_DIFFUSE_CUBE_MAP_NV 0x86F1
#define GL_DOT_PRODUCT_REFLECT_CUBE_MAP_NV 0x86F2
#define GL_DOT_PRODUCT_CONST_EYE_REFLECT_CUBE_MAP_NV 0x86F3
#define GL_HILO_NV 0x86F4
#define GL_DSDT_NV 0x86F5
#define GL_DSDT_MAG_NV 0x86F6
#define GL_DSDT_MAG_VIB_NV 0x86F7
#define GL_HILO16_NV 0x86F8
#define GL_SIGNED_HILO_NV 0x86F9
#define GL_SIGNED_HILO16_NV 0x86FA
#define GL_SIGNED_RGBA_NV 0x86FB
#define GL_SIGNED_RGBA8_NV 0x86FC
#define GL_SIGNED_RGB_NV 0x86FE
#define GL_SIGNED_RGB8_NV 0x86FF
#define GL_SIGNED_LUMINANCE_NV 0x8701
#define GL_SIGNED_LUMINANCE8_NV 0x8702
#define GL_SIGNED_LUMINANCE_ALPHA_NV 0x8703
#define GL_SIGNED_LUMINANCE8_ALPHA8_NV 0x8704
#define GL_SIGNED_ALPHA_NV 0x8705
#define GL_SIGNED_ALPHA8_NV 0x8706
#define GL_SIGNED_INTENSITY_NV 0x8707
#define GL_SIGNED_INTENSITY8_NV 0x8708
#define GL_DSDT8_NV 0x8709
#define GL_DSDT8_MAG8_NV 0x870A
#define GL_DSDT8_MAG8_INTENSITY8_NV 0x870B
#define GL_SIGNED_RGB_UNSIGNED_ALPHA_NV 0x870C
#define GL_SIGNED_RGB8_UNSIGNED_ALPHA8_NV 0x870D
#define GL_HI_SCALE_NV 0x870E
#define GL_LO_SCALE_NV 0x870F
#define GL_DS_SCALE_NV 0x8710
#define GL_DT_SCALE_NV 0x8711
#define GL_MAGNITUDE_SCALE_NV 0x8712
#define GL_VIBRANCE_SCALE_NV 0x8713
#define GL_HI_BIAS_NV 0x8714
#define GL_LO_BIAS_NV 0x8715
#define GL_DS_BIAS_NV 0x8716
#define GL_DT_BIAS_NV 0x8717
#define GL_MAGNITUDE_BIAS_NV 0x8718
#define GL_VIBRANCE_BIAS_NV 0x8719
#define GL_TEXTURE_BORDER_VALUES_NV 0x871A
#define GL_TEXTURE_HI_SIZE_NV 0x871B
#define GL_TEXTURE_LO_SIZE_NV 0x871C
#define GL_TEXTURE_DS_SIZE_NV 0x871D
#define GL_TEXTURE_DT_SIZE_NV 0x871E
#define GL_TEXTURE_MAG_SIZE_NV 0x871F
#endif

#ifndef GL_NV_texture_shader2
#define GL_NV_texture_shader2
#define GL_DOT_PRODUCT_TEXTURE_3D_NV 0x86EF
#endif

#ifndef GL_NV_texture_shader3
#define GL_NV_texture_shader3
#define GL_OFFSET_PROJECTIVE_TEXTURE_2D_NV 0x8850
#define GL_OFFSET_PROJECTIVE_TEXTURE_2D_SCALE_NV 0x8851
#define GL_OFFSET_PROJECTIVE_TEXTURE_RECTANGLE_NV 0x8852
#define GL_OFFSET_PROJECTIVE_TEXTURE_RECTANGLE_SCALE_NV 0x8853
#define GL_OFFSET_HILO_TEXTURE_2D_NV 0x8854
#define GL_OFFSET_HILO_TEXTURE_RECTANGLE_NV 0x8855
#define GL_OFFSET_HILO_PROJECTIVE_TEXTURE_2D_NV 0x8856
#define GL_OFFSET_HILO_PROJECTIVE_TEXTURE_RECTANGLE_NV 0x8857
#define GL_DEPENDENT_HILO_TEXTURE_2D_NV 0x8858
#define GL_DEPENDENT_RGB_TEXTURE_3D_NV 0x8859
#define GL_DEPENDENT_RGB_TEXTURE_CUBE_MAP_NV 0x885A
#define GL_DOT_PRODUCT_PASS_THROUGH_NV 0x885B
#define GL_DOT_PRODUCT_TEXTURE_1D_NV 0x885C
#define GL_DOT_PRODUCT_AFFINE_DEPTH_REPLACE_NV 0x885D
#define GL_HILO8_NV 0x885E
#define GL_SIGNED_HILO8_NV 0x885F
#define GL_FORCE_BLUE_TO_ONE_NV 0x8860
#endif

#ifndef GL_NV_transform_feedback
#define GL_NV_transform_feedback
#define GL_BACK_PRIMARY_COLOR_NV 0x8C77
#define GL_BACK_SECONDARY_COLOR_NV 0x8C78
#define GL_TEXTURE_COORD_NV 0x8C79
#define GL_CLIP_DISTANCE_NV 0x8C7A
#define GL_VERTEX_ID_NV 0x8C7B
#define GL_PRIMITIVE_ID_NV 0x8C7C
#define GL_GENERIC_ATTRIB_NV 0x8C7D
#define GL_TRANSFORM_FEEDBACK_ATTRIBS_NV 0x8C7E
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE_NV 0x8C7F
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_NV 0x8C80
#define GL_ACTIVE_VARYINGS_NV 0x8C81
#define GL_ACTIVE_VARYING_MAX_LENGTH_NV 0x8C82
#define GL_TRANSFORM_FEEDBACK_VARYINGS_NV 0x8C83
#define GL_TRANSFORM_FEEDBACK_BUFFER_START_NV 0x8C84
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE_NV 0x8C85
#define GL_TRANSFORM_FEEDBACK_RECORD_NV 0x8C86
#define GL_PRIMITIVES_GENERATED_NV 0x8C87
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_NV 0x8C88
#define GL_RASTERIZER_DISCARD_NV 0x8C89
#define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_NV 0x8C8A
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS_NV 0x8C8B
#define GL_INTERLEAVED_ATTRIBS_NV 0x8C8C
#define GL_SEPARATE_ATTRIBS_NV 0x8C8D
#define GL_TRANSFORM_FEEDBACK_BUFFER_NV 0x8C8E
#define GL_TRANSFORM_FEEDBACK_BUFFER_BINDING_NV 0x8C8F
#define GL_LAYER_NV 0x8DAA
#define GL_NEXT_BUFFER_NV -2
#define GL_SKIP_COMPONENTS4_NV -3
#define GL_SKIP_COMPONENTS3_NV -4
#define GL_SKIP_COMPONENTS2_NV -5
#define GL_SKIP_COMPONENTS1_NV -6
#endif
extern void (APIENTRYP gglBeginTransformFeedbackNV)(GLenum primitiveMode);
extern void (APIENTRYP gglEndTransformFeedbackNV)();
extern void (APIENTRYP gglTransformFeedbackAttribsNV)(GLsizei count, const GLint *attribs, GLenum bufferMode);
extern void (APIENTRYP gglBindBufferRangeNV)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
extern void (APIENTRYP gglBindBufferOffsetNV)(GLenum target, GLuint index, GLuint buffer, GLintptr offset);
extern void (APIENTRYP gglBindBufferBaseNV)(GLenum target, GLuint index, GLuint buffer);
extern void (APIENTRYP gglTransformFeedbackVaryingsNV)(GLuint program, GLsizei count, const GLint *locations, GLenum bufferMode);
extern void (APIENTRYP gglActiveVaryingNV)(GLuint program, const GLchar *name);
extern GLint (APIENTRYP gglGetVaryingLocationNV)(GLuint program, const GLchar *name);
extern void (APIENTRYP gglGetActiveVaryingNV)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
extern void (APIENTRYP gglGetTransformFeedbackVaryingNV)(GLuint program, GLuint index, GLint *location);
extern void (APIENTRYP gglTransformFeedbackStreamAttribsNV)(GLsizei count, const GLint *attribs, GLsizei nbuffers, const GLint *bufstreams, GLenum bufferMode);

#ifndef GL_NV_transform_feedback2
#define GL_NV_transform_feedback2
#define GL_TRANSFORM_FEEDBACK_NV 0x8E22
#define GL_TRANSFORM_FEEDBACK_BUFFER_PAUSED_NV 0x8E23
#define GL_TRANSFORM_FEEDBACK_BUFFER_ACTIVE_NV 0x8E24
#define GL_TRANSFORM_FEEDBACK_BINDING_NV 0x8E25
#endif
extern void (APIENTRYP gglBindTransformFeedbackNV)(GLenum target, GLuint id);
extern void (APIENTRYP gglDeleteTransformFeedbacksNV)(GLsizei n, const GLuint *ids);
extern void (APIENTRYP gglGenTransformFeedbacksNV)(GLsizei n, GLuint *ids);
extern GLboolean (APIENTRYP gglIsTransformFeedbackNV)(GLuint id);
extern void (APIENTRYP gglPauseTransformFeedbackNV)();
extern void (APIENTRYP gglResumeTransformFeedbackNV)();
extern void (APIENTRYP gglDrawTransformFeedbackNV)(GLenum mode, GLuint id);

#ifndef GL_NV_uniform_buffer_unified_memory
#define GL_NV_uniform_buffer_unified_memory
#define GL_UNIFORM_BUFFER_UNIFIED_NV 0x936E
#define GL_UNIFORM_BUFFER_ADDRESS_NV 0x936F
#define GL_UNIFORM_BUFFER_LENGTH_NV 0x9370
#endif

#ifndef GL_NV_vdpau_interop
#define GL_NV_vdpau_interop
#define GL_SURFACE_STATE_NV 0x86EB
#define GL_SURFACE_REGISTERED_NV 0x86FD
#define GL_SURFACE_MAPPED_NV 0x8700
#define GL_WRITE_DISCARD_NV 0x88BE
#endif
extern void (APIENTRYP gglVDPAUInitNV)(const void *vdpDevice, const void *getProcAddress);
extern void (APIENTRYP gglVDPAUFiniNV)();
extern GLvdpauSurfaceNV (APIENTRYP gglVDPAURegisterVideoSurfaceNV)(const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames);
extern GLvdpauSurfaceNV (APIENTRYP gglVDPAURegisterOutputSurfaceNV)(const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames);
extern GLboolean (APIENTRYP gglVDPAUIsSurfaceNV)(GLvdpauSurfaceNV surface);
extern void (APIENTRYP gglVDPAUUnregisterSurfaceNV)(GLvdpauSurfaceNV surface);
extern void (APIENTRYP gglVDPAUGetSurfaceivNV)(GLvdpauSurfaceNV surface, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
extern void (APIENTRYP gglVDPAUSurfaceAccessNV)(GLvdpauSurfaceNV surface, GLenum access);
extern void (APIENTRYP gglVDPAUMapSurfacesNV)(GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces);
extern void (APIENTRYP gglVDPAUUnmapSurfacesNV)(GLsizei numSurface, const GLvdpauSurfaceNV *surfaces);

#ifndef GL_NV_vertex_array_range
#define GL_NV_vertex_array_range
#define GL_VERTEX_ARRAY_RANGE_NV 0x851D
#define GL_VERTEX_ARRAY_RANGE_LENGTH_NV 0x851E
#define GL_VERTEX_ARRAY_RANGE_VALID_NV 0x851F
#define GL_MAX_VERTEX_ARRAY_RANGE_ELEMENT_NV 0x8520
#define GL_VERTEX_ARRAY_RANGE_POINTER_NV 0x8521
#endif
extern void (APIENTRYP gglFlushVertexArrayRangeNV)();
extern void (APIENTRYP gglVertexArrayRangeNV)(GLsizei length, const void *pointer);

#ifndef GL_NV_vertex_array_range2
#define GL_NV_vertex_array_range2
#define GL_VERTEX_ARRAY_RANGE_WITHOUT_FLUSH_NV 0x8533
#endif

#ifndef GL_NV_vertex_attrib_integer_64bit
#define GL_NV_vertex_attrib_integer_64bit
/* reuse GL_INT64_NV */
/* reuse GL_UNSIGNED_INT64_NV */
#endif
extern void (APIENTRYP gglVertexAttribL1i64NV)(GLuint index, GLint64EXT x);
extern void (APIENTRYP gglVertexAttribL2i64NV)(GLuint index, GLint64EXT x, GLint64EXT y);
extern void (APIENTRYP gglVertexAttribL3i64NV)(GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z);
extern void (APIENTRYP gglVertexAttribL4i64NV)(GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w);
extern void (APIENTRYP gglVertexAttribL1i64vNV)(GLuint index, const GLint64EXT *v);
extern void (APIENTRYP gglVertexAttribL2i64vNV)(GLuint index, const GLint64EXT *v);
extern void (APIENTRYP gglVertexAttribL3i64vNV)(GLuint index, const GLint64EXT *v);
extern void (APIENTRYP gglVertexAttribL4i64vNV)(GLuint index, const GLint64EXT *v);
extern void (APIENTRYP gglVertexAttribL1ui64NV)(GLuint index, GLuint64EXT x);
extern void (APIENTRYP gglVertexAttribL2ui64NV)(GLuint index, GLuint64EXT x, GLuint64EXT y);
extern void (APIENTRYP gglVertexAttribL3ui64NV)(GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z);
extern void (APIENTRYP gglVertexAttribL4ui64NV)(GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w);
extern void (APIENTRYP gglVertexAttribL1ui64vNV)(GLuint index, const GLuint64EXT *v);
extern void (APIENTRYP gglVertexAttribL2ui64vNV)(GLuint index, const GLuint64EXT *v);
extern void (APIENTRYP gglVertexAttribL3ui64vNV)(GLuint index, const GLuint64EXT *v);
extern void (APIENTRYP gglVertexAttribL4ui64vNV)(GLuint index, const GLuint64EXT *v);
extern void (APIENTRYP gglGetVertexAttribLi64vNV)(GLuint index, GLenum pname, GLint64EXT *params);
extern void (APIENTRYP gglGetVertexAttribLui64vNV)(GLuint index, GLenum pname, GLuint64EXT *params);
extern void (APIENTRYP gglVertexAttribLFormatNV)(GLuint index, GLint size, GLenum type, GLsizei stride);

#ifndef GL_NV_vertex_buffer_unified_memory
#define GL_NV_vertex_buffer_unified_memory
#define GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV 0x8F1E
#define GL_ELEMENT_ARRAY_UNIFIED_NV 0x8F1F
#define GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV 0x8F20
#define GL_VERTEX_ARRAY_ADDRESS_NV 0x8F21
#define GL_NORMAL_ARRAY_ADDRESS_NV 0x8F22
#define GL_COLOR_ARRAY_ADDRESS_NV 0x8F23
#define GL_INDEX_ARRAY_ADDRESS_NV 0x8F24
#define GL_TEXTURE_COORD_ARRAY_ADDRESS_NV 0x8F25
#define GL_EDGE_FLAG_ARRAY_ADDRESS_NV 0x8F26
#define GL_SECONDARY_COLOR_ARRAY_ADDRESS_NV 0x8F27
#define GL_FOG_COORD_ARRAY_ADDRESS_NV 0x8F28
#define GL_ELEMENT_ARRAY_ADDRESS_NV 0x8F29
#define GL_VERTEX_ATTRIB_ARRAY_LENGTH_NV 0x8F2A
#define GL_VERTEX_ARRAY_LENGTH_NV 0x8F2B
#define GL_NORMAL_ARRAY_LENGTH_NV 0x8F2C
#define GL_COLOR_ARRAY_LENGTH_NV 0x8F2D
#define GL_INDEX_ARRAY_LENGTH_NV 0x8F2E
#define GL_TEXTURE_COORD_ARRAY_LENGTH_NV 0x8F2F
#define GL_EDGE_FLAG_ARRAY_LENGTH_NV 0x8F30
#define GL_SECONDARY_COLOR_ARRAY_LENGTH_NV 0x8F31
#define GL_FOG_COORD_ARRAY_LENGTH_NV 0x8F32
#define GL_ELEMENT_ARRAY_LENGTH_NV 0x8F33
#define GL_DRAW_INDIRECT_UNIFIED_NV 0x8F40
#define GL_DRAW_INDIRECT_ADDRESS_NV 0x8F41
#define GL_DRAW_INDIRECT_LENGTH_NV 0x8F42
#endif
extern void (APIENTRYP gglBufferAddressRangeNV)(GLenum pname, GLuint index, GLuint64EXT address, GLsizeiptr length);
extern void (APIENTRYP gglVertexFormatNV)(GLint size, GLenum type, GLsizei stride);
extern void (APIENTRYP gglNormalFormatNV)(GLenum type, GLsizei stride);
extern void (APIENTRYP gglColorFormatNV)(GLint size, GLenum type, GLsizei stride);
extern void (APIENTRYP gglIndexFormatNV)(GLenum type, GLsizei stride);
extern void (APIENTRYP gglTexCoordFormatNV)(GLint size, GLenum type, GLsizei stride);
extern void (APIENTRYP gglEdgeFlagFormatNV)(GLsizei stride);
extern void (APIENTRYP gglSecondaryColorFormatNV)(GLint size, GLenum type, GLsizei stride);
extern void (APIENTRYP gglFogCoordFormatNV)(GLenum type, GLsizei stride);
extern void (APIENTRYP gglVertexAttribFormatNV)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride);
extern void (APIENTRYP gglVertexAttribIFormatNV)(GLuint index, GLint size, GLenum type, GLsizei stride);
extern void (APIENTRYP gglGetIntegerui64i_vNV)(GLenum value, GLuint index, GLuint64EXT *result);

#ifndef GL_NV_vertex_program
#define GL_NV_vertex_program
#define GL_VERTEX_PROGRAM_NV 0x8620
#define GL_VERTEX_STATE_PROGRAM_NV 0x8621
#define GL_ATTRIB_ARRAY_SIZE_NV 0x8623
#define GL_ATTRIB_ARRAY_STRIDE_NV 0x8624
#define GL_ATTRIB_ARRAY_TYPE_NV 0x8625
#define GL_CURRENT_ATTRIB_NV 0x8626
#define GL_PROGRAM_LENGTH_NV 0x8627
#define GL_PROGRAM_STRING_NV 0x8628
#define GL_MODELVIEW_PROJECTION_NV 0x8629
#define GL_IDENTITY_NV 0x862A
#define GL_INVERSE_NV 0x862B
#define GL_TRANSPOSE_NV 0x862C
#define GL_INVERSE_TRANSPOSE_NV 0x862D
#define GL_MAX_TRACK_MATRIX_STACK_DEPTH_NV 0x862E
#define GL_MAX_TRACK_MATRICES_NV 0x862F
#define GL_MATRIX0_NV 0x8630
#define GL_MATRIX1_NV 0x8631
#define GL_MATRIX2_NV 0x8632
#define GL_MATRIX3_NV 0x8633
#define GL_MATRIX4_NV 0x8634
#define GL_MATRIX5_NV 0x8635
#define GL_MATRIX6_NV 0x8636
#define GL_MATRIX7_NV 0x8637
#define GL_CURRENT_MATRIX_STACK_DEPTH_NV 0x8640
#define GL_CURRENT_MATRIX_NV 0x8641
#define GL_VERTEX_PROGRAM_POINT_SIZE_NV 0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_NV 0x8643
#define GL_PROGRAM_PARAMETER_NV 0x8644
#define GL_ATTRIB_ARRAY_POINTER_NV 0x8645
#define GL_PROGRAM_TARGET_NV 0x8646
#define GL_PROGRAM_RESIDENT_NV 0x8647
#define GL_TRACK_MATRIX_NV 0x8648
#define GL_TRACK_MATRIX_TRANSFORM_NV 0x8649
#define GL_VERTEX_PROGRAM_BINDING_NV 0x864A
#define GL_PROGRAM_ERROR_POSITION_NV 0x864B
#define GL_VERTEX_ATTRIB_ARRAY0_NV 0x8650
#define GL_VERTEX_ATTRIB_ARRAY1_NV 0x8651
#define GL_VERTEX_ATTRIB_ARRAY2_NV 0x8652
#define GL_VERTEX_ATTRIB_ARRAY3_NV 0x8653
#define GL_VERTEX_ATTRIB_ARRAY4_NV 0x8654
#define GL_VERTEX_ATTRIB_ARRAY5_NV 0x8655
#define GL_VERTEX_ATTRIB_ARRAY6_NV 0x8656
#define GL_VERTEX_ATTRIB_ARRAY7_NV 0x8657
#define GL_VERTEX_ATTRIB_ARRAY8_NV 0x8658
#define GL_VERTEX_ATTRIB_ARRAY9_NV 0x8659
#define GL_VERTEX_ATTRIB_ARRAY10_NV 0x865A
#define GL_VERTEX_ATTRIB_ARRAY11_NV 0x865B
#define GL_VERTEX_ATTRIB_ARRAY12_NV 0x865C
#define GL_VERTEX_ATTRIB_ARRAY13_NV 0x865D
#define GL_VERTEX_ATTRIB_ARRAY14_NV 0x865E
#define GL_VERTEX_ATTRIB_ARRAY15_NV 0x865F
#define GL_MAP1_VERTEX_ATTRIB0_4_NV 0x8660
#define GL_MAP1_VERTEX_ATTRIB1_4_NV 0x8661
#define GL_MAP1_VERTEX_ATTRIB2_4_NV 0x8662
#define GL_MAP1_VERTEX_ATTRIB3_4_NV 0x8663
#define GL_MAP1_VERTEX_ATTRIB4_4_NV 0x8664
#define GL_MAP1_VERTEX_ATTRIB5_4_NV 0x8665
#define GL_MAP1_VERTEX_ATTRIB6_4_NV 0x8666
#define GL_MAP1_VERTEX_ATTRIB7_4_NV 0x8667
#define GL_MAP1_VERTEX_ATTRIB8_4_NV 0x8668
#define GL_MAP1_VERTEX_ATTRIB9_4_NV 0x8669
#define GL_MAP1_VERTEX_ATTRIB10_4_NV 0x866A
#define GL_MAP1_VERTEX_ATTRIB11_4_NV 0x866B
#define GL_MAP1_VERTEX_ATTRIB12_4_NV 0x866C
#define GL_MAP1_VERTEX_ATTRIB13_4_NV 0x866D
#define GL_MAP1_VERTEX_ATTRIB14_4_NV 0x866E
#define GL_MAP1_VERTEX_ATTRIB15_4_NV 0x866F
#define GL_MAP2_VERTEX_ATTRIB0_4_NV 0x8670
#define GL_MAP2_VERTEX_ATTRIB1_4_NV 0x8671
#define GL_MAP2_VERTEX_ATTRIB2_4_NV 0x8672
#define GL_MAP2_VERTEX_ATTRIB3_4_NV 0x8673
#define GL_MAP2_VERTEX_ATTRIB4_4_NV 0x8674
#define GL_MAP2_VERTEX_ATTRIB5_4_NV 0x8675
#define GL_MAP2_VERTEX_ATTRIB6_4_NV 0x8676
#define GL_MAP2_VERTEX_ATTRIB7_4_NV 0x8677
#define GL_MAP2_VERTEX_ATTRIB8_4_NV 0x8678
#define GL_MAP2_VERTEX_ATTRIB9_4_NV 0x8679
#define GL_MAP2_VERTEX_ATTRIB10_4_NV 0x867A
#define GL_MAP2_VERTEX_ATTRIB11_4_NV 0x867B
#define GL_MAP2_VERTEX_ATTRIB12_4_NV 0x867C
#define GL_MAP2_VERTEX_ATTRIB13_4_NV 0x867D
#define GL_MAP2_VERTEX_ATTRIB14_4_NV 0x867E
#define GL_MAP2_VERTEX_ATTRIB15_4_NV 0x867F
#endif
extern GLboolean (APIENTRYP gglAreProgramsResidentNV)(GLsizei n, const GLuint *programs, GLboolean *residences);
extern void (APIENTRYP gglBindProgramNV)(GLenum target, GLuint id);
extern void (APIENTRYP gglDeleteProgramsNV)(GLsizei n, const GLuint *programs);
extern void (APIENTRYP gglExecuteProgramNV)(GLenum target, GLuint id, const GLfloat *params);
extern void (APIENTRYP gglGenProgramsNV)(GLsizei n, GLuint *programs);
extern void (APIENTRYP gglGetProgramParameterdvNV)(GLenum target, GLuint index, GLenum pname, GLdouble *params);
extern void (APIENTRYP gglGetProgramParameterfvNV)(GLenum target, GLuint index, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetProgramivNV)(GLuint id, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetProgramStringNV)(GLuint id, GLenum pname, GLubyte *program);
extern void (APIENTRYP gglGetTrackMatrixivNV)(GLenum target, GLuint address, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetVertexAttribdvNV)(GLuint index, GLenum pname, GLdouble *params);
extern void (APIENTRYP gglGetVertexAttribfvNV)(GLuint index, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetVertexAttribivNV)(GLuint index, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetVertexAttribPointervNV)(GLuint index, GLenum pname, void **pointer);
extern GLboolean (APIENTRYP gglIsProgramNV)(GLuint id);
extern void (APIENTRYP gglLoadProgramNV)(GLenum target, GLuint id, GLsizei len, const GLubyte *program);
extern void (APIENTRYP gglProgramParameter4dNV)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglProgramParameter4dvNV)(GLenum target, GLuint index, const GLdouble *v);
extern void (APIENTRYP gglProgramParameter4fNV)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglProgramParameter4fvNV)(GLenum target, GLuint index, const GLfloat *v);
extern void (APIENTRYP gglProgramParameters4dvNV)(GLenum target, GLuint index, GLsizei count, const GLdouble *v);
extern void (APIENTRYP gglProgramParameters4fvNV)(GLenum target, GLuint index, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglRequestResidentProgramsNV)(GLsizei n, const GLuint *programs);
extern void (APIENTRYP gglTrackMatrixNV)(GLenum target, GLuint address, GLenum matrix, GLenum transform);
extern void (APIENTRYP gglVertexAttribPointerNV)(GLuint index, GLint fsize, GLenum type, GLsizei stride, const void *pointer);
extern void (APIENTRYP gglVertexAttrib1dNV)(GLuint index, GLdouble x);
extern void (APIENTRYP gglVertexAttrib1dvNV)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttrib1fNV)(GLuint index, GLfloat x);
extern void (APIENTRYP gglVertexAttrib1fvNV)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib1sNV)(GLuint index, GLshort x);
extern void (APIENTRYP gglVertexAttrib1svNV)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib2dNV)(GLuint index, GLdouble x, GLdouble y);
extern void (APIENTRYP gglVertexAttrib2dvNV)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttrib2fNV)(GLuint index, GLfloat x, GLfloat y);
extern void (APIENTRYP gglVertexAttrib2fvNV)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib2sNV)(GLuint index, GLshort x, GLshort y);
extern void (APIENTRYP gglVertexAttrib2svNV)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib3dNV)(GLuint index, GLdouble x, GLdouble y, GLdouble z);
extern void (APIENTRYP gglVertexAttrib3dvNV)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttrib3fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglVertexAttrib3fvNV)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib3sNV)(GLuint index, GLshort x, GLshort y, GLshort z);
extern void (APIENTRYP gglVertexAttrib3svNV)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib4dNV)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void (APIENTRYP gglVertexAttrib4dvNV)(GLuint index, const GLdouble *v);
extern void (APIENTRYP gglVertexAttrib4fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglVertexAttrib4fvNV)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib4sNV)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
extern void (APIENTRYP gglVertexAttrib4svNV)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttrib4ubNV)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
extern void (APIENTRYP gglVertexAttrib4ubvNV)(GLuint index, const GLubyte *v);
extern void (APIENTRYP gglVertexAttribs1dvNV)(GLuint index, GLsizei count, const GLdouble *v);
extern void (APIENTRYP gglVertexAttribs1fvNV)(GLuint index, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglVertexAttribs1svNV)(GLuint index, GLsizei count, const GLshort *v);
extern void (APIENTRYP gglVertexAttribs2dvNV)(GLuint index, GLsizei count, const GLdouble *v);
extern void (APIENTRYP gglVertexAttribs2fvNV)(GLuint index, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglVertexAttribs2svNV)(GLuint index, GLsizei count, const GLshort *v);
extern void (APIENTRYP gglVertexAttribs3dvNV)(GLuint index, GLsizei count, const GLdouble *v);
extern void (APIENTRYP gglVertexAttribs3fvNV)(GLuint index, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglVertexAttribs3svNV)(GLuint index, GLsizei count, const GLshort *v);
extern void (APIENTRYP gglVertexAttribs4dvNV)(GLuint index, GLsizei count, const GLdouble *v);
extern void (APIENTRYP gglVertexAttribs4fvNV)(GLuint index, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglVertexAttribs4svNV)(GLuint index, GLsizei count, const GLshort *v);
extern void (APIENTRYP gglVertexAttribs4ubvNV)(GLuint index, GLsizei count, const GLubyte *v);

#ifndef GL_NV_vertex_program1_1
#define GL_NV_vertex_program1_1
#endif

#ifndef GL_NV_vertex_program2
#define GL_NV_vertex_program2
#endif

#ifndef GL_NV_vertex_program2_option
#define GL_NV_vertex_program2_option
/* reuse GL_MAX_PROGRAM_EXEC_INSTRUCTIONS_NV */
/* reuse GL_MAX_PROGRAM_CALL_DEPTH_NV */
#endif

#ifndef GL_NV_vertex_program3
#define GL_NV_vertex_program3
/* reuse GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB */
#endif

#ifndef GL_NV_vertex_program4
#define GL_NV_vertex_program4
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER_NV 0x88FD
#endif
extern void (APIENTRYP gglVertexAttribI1iEXT)(GLuint index, GLint x);
extern void (APIENTRYP gglVertexAttribI2iEXT)(GLuint index, GLint x, GLint y);
extern void (APIENTRYP gglVertexAttribI3iEXT)(GLuint index, GLint x, GLint y, GLint z);
extern void (APIENTRYP gglVertexAttribI4iEXT)(GLuint index, GLint x, GLint y, GLint z, GLint w);
extern void (APIENTRYP gglVertexAttribI1uiEXT)(GLuint index, GLuint x);
extern void (APIENTRYP gglVertexAttribI2uiEXT)(GLuint index, GLuint x, GLuint y);
extern void (APIENTRYP gglVertexAttribI3uiEXT)(GLuint index, GLuint x, GLuint y, GLuint z);
extern void (APIENTRYP gglVertexAttribI4uiEXT)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
extern void (APIENTRYP gglVertexAttribI1ivEXT)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttribI2ivEXT)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttribI3ivEXT)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttribI4ivEXT)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttribI1uivEXT)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglVertexAttribI2uivEXT)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglVertexAttribI3uivEXT)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglVertexAttribI4uivEXT)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglVertexAttribI4bvEXT)(GLuint index, const GLbyte *v);
extern void (APIENTRYP gglVertexAttribI4svEXT)(GLuint index, const GLshort *v);
extern void (APIENTRYP gglVertexAttribI4ubvEXT)(GLuint index, const GLubyte *v);
extern void (APIENTRYP gglVertexAttribI4usvEXT)(GLuint index, const GLushort *v);
extern void (APIENTRYP gglVertexAttribIPointerEXT)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
extern void (APIENTRYP gglGetVertexAttribIivEXT)(GLuint index, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetVertexAttribIuivEXT)(GLuint index, GLenum pname, GLuint *params);

#ifndef GL_NV_video_capture
#define GL_NV_video_capture
#define GL_VIDEO_BUFFER_NV 0x9020
#define GL_VIDEO_BUFFER_BINDING_NV 0x9021
#define GL_FIELD_UPPER_NV 0x9022
#define GL_FIELD_LOWER_NV 0x9023
#define GL_NUM_VIDEO_CAPTURE_STREAMS_NV 0x9024
#define GL_NEXT_VIDEO_CAPTURE_BUFFER_STATUS_NV 0x9025
#define GL_VIDEO_CAPTURE_TO_422_SUPPORTED_NV 0x9026
#define GL_LAST_VIDEO_CAPTURE_STATUS_NV 0x9027
#define GL_VIDEO_BUFFER_PITCH_NV 0x9028
#define GL_VIDEO_COLOR_CONVERSION_MATRIX_NV 0x9029
#define GL_VIDEO_COLOR_CONVERSION_MAX_NV 0x902A
#define GL_VIDEO_COLOR_CONVERSION_MIN_NV 0x902B
#define GL_VIDEO_COLOR_CONVERSION_OFFSET_NV 0x902C
#define GL_VIDEO_BUFFER_INTERNAL_FORMAT_NV 0x902D
#define GL_PARTIAL_SUCCESS_NV 0x902E
#define GL_SUCCESS_NV 0x902F
#define GL_FAILURE_NV 0x9030
#define GL_YCBYCR8_422_NV 0x9031
#define GL_YCBAYCR8A_4224_NV 0x9032
#define GL_Z6Y10Z6CB10Z6Y10Z6CR10_422_NV 0x9033
#define GL_Z6Y10Z6CB10Z6A10Z6Y10Z6CR10Z6A10_4224_NV 0x9034
#define GL_Z4Y12Z4CB12Z4Y12Z4CR12_422_NV 0x9035
#define GL_Z4Y12Z4CB12Z4A12Z4Y12Z4CR12Z4A12_4224_NV 0x9036
#define GL_Z4Y12Z4CB12Z4CR12_444_NV 0x9037
#define GL_VIDEO_CAPTURE_FRAME_WIDTH_NV 0x9038
#define GL_VIDEO_CAPTURE_FRAME_HEIGHT_NV 0x9039
#define GL_VIDEO_CAPTURE_FIELD_UPPER_HEIGHT_NV 0x903A
#define GL_VIDEO_CAPTURE_FIELD_LOWER_HEIGHT_NV 0x903B
#define GL_VIDEO_CAPTURE_SURFACE_ORIGIN_NV 0x903C
#endif
extern void (APIENTRYP gglBeginVideoCaptureNV)(GLuint video_capture_slot);
extern void (APIENTRYP gglBindVideoCaptureStreamBufferNV)(GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLintptrARB offset);
extern void (APIENTRYP gglBindVideoCaptureStreamTextureNV)(GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLenum target, GLuint texture);
extern void (APIENTRYP gglEndVideoCaptureNV)(GLuint video_capture_slot);
extern void (APIENTRYP gglGetVideoCaptureivNV)(GLuint video_capture_slot, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetVideoCaptureStreamivNV)(GLuint video_capture_slot, GLuint stream, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetVideoCaptureStreamfvNV)(GLuint video_capture_slot, GLuint stream, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetVideoCaptureStreamdvNV)(GLuint video_capture_slot, GLuint stream, GLenum pname, GLdouble *params);
extern GLenum (APIENTRYP gglVideoCaptureNV)(GLuint video_capture_slot, GLuint *sequence_num, GLuint64EXT *capture_time);
extern void (APIENTRYP gglVideoCaptureStreamParameterivNV)(GLuint video_capture_slot, GLuint stream, GLenum pname, const GLint *params);
extern void (APIENTRYP gglVideoCaptureStreamParameterfvNV)(GLuint video_capture_slot, GLuint stream, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglVideoCaptureStreamParameterdvNV)(GLuint video_capture_slot, GLuint stream, GLenum pname, const GLdouble *params);

#ifndef GL_NV_viewport_array2
#define GL_NV_viewport_array2
#endif

#ifndef GL_NV_viewport_swizzle
#define GL_NV_viewport_swizzle
#define GL_VIEWPORT_SWIZZLE_POSITIVE_X_NV 0x9350
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_X_NV 0x9351
#define GL_VIEWPORT_SWIZZLE_POSITIVE_Y_NV 0x9352
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_Y_NV 0x9353
#define GL_VIEWPORT_SWIZZLE_POSITIVE_Z_NV 0x9354
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_Z_NV 0x9355
#define GL_VIEWPORT_SWIZZLE_POSITIVE_W_NV 0x9356
#define GL_VIEWPORT_SWIZZLE_NEGATIVE_W_NV 0x9357
#define GL_VIEWPORT_SWIZZLE_X_NV 0x9358
#define GL_VIEWPORT_SWIZZLE_Y_NV 0x9359
#define GL_VIEWPORT_SWIZZLE_Z_NV 0x935A
#define GL_VIEWPORT_SWIZZLE_W_NV 0x935B
#endif
extern void (APIENTRYP gglViewportSwizzleNV)(GLuint index, GLenum swizzlex, GLenum swizzley, GLenum swizzlez, GLenum swizzlew);

#ifndef GL_OES_byte_coordinates
#define GL_OES_byte_coordinates
/* reuse GL_BYTE */
#endif
extern void (APIENTRYP gglMultiTexCoord1bOES)(GLenum texture, GLbyte s);
extern void (APIENTRYP gglMultiTexCoord1bvOES)(GLenum texture, const GLbyte *coords);
extern void (APIENTRYP gglMultiTexCoord2bOES)(GLenum texture, GLbyte s, GLbyte t);
extern void (APIENTRYP gglMultiTexCoord2bvOES)(GLenum texture, const GLbyte *coords);
extern void (APIENTRYP gglMultiTexCoord3bOES)(GLenum texture, GLbyte s, GLbyte t, GLbyte r);
extern void (APIENTRYP gglMultiTexCoord3bvOES)(GLenum texture, const GLbyte *coords);
extern void (APIENTRYP gglMultiTexCoord4bOES)(GLenum texture, GLbyte s, GLbyte t, GLbyte r, GLbyte q);
extern void (APIENTRYP gglMultiTexCoord4bvOES)(GLenum texture, const GLbyte *coords);
extern void (APIENTRYP gglTexCoord1bOES)(GLbyte s);
extern void (APIENTRYP gglTexCoord1bvOES)(const GLbyte *coords);
extern void (APIENTRYP gglTexCoord2bOES)(GLbyte s, GLbyte t);
extern void (APIENTRYP gglTexCoord2bvOES)(const GLbyte *coords);
extern void (APIENTRYP gglTexCoord3bOES)(GLbyte s, GLbyte t, GLbyte r);
extern void (APIENTRYP gglTexCoord3bvOES)(const GLbyte *coords);
extern void (APIENTRYP gglTexCoord4bOES)(GLbyte s, GLbyte t, GLbyte r, GLbyte q);
extern void (APIENTRYP gglTexCoord4bvOES)(const GLbyte *coords);
extern void (APIENTRYP gglVertex2bOES)(GLbyte x, GLbyte y);
extern void (APIENTRYP gglVertex2bvOES)(const GLbyte *coords);
extern void (APIENTRYP gglVertex3bOES)(GLbyte x, GLbyte y, GLbyte z);
extern void (APIENTRYP gglVertex3bvOES)(const GLbyte *coords);
extern void (APIENTRYP gglVertex4bOES)(GLbyte x, GLbyte y, GLbyte z, GLbyte w);
extern void (APIENTRYP gglVertex4bvOES)(const GLbyte *coords);

#ifndef GL_OES_compressed_paletted_texture
#define GL_OES_compressed_paletted_texture
#define GL_PALETTE4_RGB8_OES 0x8B90
#define GL_PALETTE4_RGBA8_OES 0x8B91
#define GL_PALETTE4_R5_G6_B5_OES 0x8B92
#define GL_PALETTE4_RGBA4_OES 0x8B93
#define GL_PALETTE4_RGB5_A1_OES 0x8B94
#define GL_PALETTE8_RGB8_OES 0x8B95
#define GL_PALETTE8_RGBA8_OES 0x8B96
#define GL_PALETTE8_R5_G6_B5_OES 0x8B97
#define GL_PALETTE8_RGBA4_OES 0x8B98
#define GL_PALETTE8_RGB5_A1_OES 0x8B99
#endif

#ifndef GL_OES_fixed_point
#define GL_OES_fixed_point
#define GL_FIXED_OES 0x140C
#endif
extern void (APIENTRYP gglAlphaFuncxOES)(GLenum func, GLfixed ref);
extern void (APIENTRYP gglClearColorxOES)(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
extern void (APIENTRYP gglClearDepthxOES)(GLfixed depth);
extern void (APIENTRYP gglClipPlanexOES)(GLenum plane, const GLfixed *equation);
extern void (APIENTRYP gglColor4xOES)(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
extern void (APIENTRYP gglDepthRangexOES)(GLfixed n, GLfixed f);
extern void (APIENTRYP gglFogxOES)(GLenum pname, GLfixed param);
extern void (APIENTRYP gglFogxvOES)(GLenum pname, const GLfixed *param);
extern void (APIENTRYP gglFrustumxOES)(GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f);
extern void (APIENTRYP gglGetClipPlanexOES)(GLenum plane, GLfixed *equation);
extern void (APIENTRYP gglGetFixedvOES)(GLenum pname, GLfixed *params);
extern void (APIENTRYP gglGetTexEnvxvOES)(GLenum target, GLenum pname, GLfixed *params);
extern void (APIENTRYP gglGetTexParameterxvOES)(GLenum target, GLenum pname, GLfixed *params);
extern void (APIENTRYP gglLightModelxOES)(GLenum pname, GLfixed param);
extern void (APIENTRYP gglLightModelxvOES)(GLenum pname, const GLfixed *param);
extern void (APIENTRYP gglLightxOES)(GLenum light, GLenum pname, GLfixed param);
extern void (APIENTRYP gglLightxvOES)(GLenum light, GLenum pname, const GLfixed *params);
extern void (APIENTRYP gglLineWidthxOES)(GLfixed width);
extern void (APIENTRYP gglLoadMatrixxOES)(const GLfixed *m);
extern void (APIENTRYP gglMaterialxOES)(GLenum face, GLenum pname, GLfixed param);
extern void (APIENTRYP gglMaterialxvOES)(GLenum face, GLenum pname, const GLfixed *param);
extern void (APIENTRYP gglMultMatrixxOES)(const GLfixed *m);
extern void (APIENTRYP gglMultiTexCoord4xOES)(GLenum texture, GLfixed s, GLfixed t, GLfixed r, GLfixed q);
extern void (APIENTRYP gglNormal3xOES)(GLfixed nx, GLfixed ny, GLfixed nz);
extern void (APIENTRYP gglOrthoxOES)(GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f);
extern void (APIENTRYP gglPointParameterxvOES)(GLenum pname, const GLfixed *params);
extern void (APIENTRYP gglPointSizexOES)(GLfixed size);
extern void (APIENTRYP gglPolygonOffsetxOES)(GLfixed factor, GLfixed units);
extern void (APIENTRYP gglRotatexOES)(GLfixed angle, GLfixed x, GLfixed y, GLfixed z);
extern void (APIENTRYP gglScalexOES)(GLfixed x, GLfixed y, GLfixed z);
extern void (APIENTRYP gglTexEnvxOES)(GLenum target, GLenum pname, GLfixed param);
extern void (APIENTRYP gglTexEnvxvOES)(GLenum target, GLenum pname, const GLfixed *params);
extern void (APIENTRYP gglTexParameterxOES)(GLenum target, GLenum pname, GLfixed param);
extern void (APIENTRYP gglTexParameterxvOES)(GLenum target, GLenum pname, const GLfixed *params);
extern void (APIENTRYP gglTranslatexOES)(GLfixed x, GLfixed y, GLfixed z);
extern void (APIENTRYP gglAccumxOES)(GLenum op, GLfixed value);
extern void (APIENTRYP gglBitmapxOES)(GLsizei width, GLsizei height, GLfixed xorig, GLfixed yorig, GLfixed xmove, GLfixed ymove, const GLubyte *bitmap);
extern void (APIENTRYP gglBlendColorxOES)(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
extern void (APIENTRYP gglClearAccumxOES)(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha);
extern void (APIENTRYP gglColor3xOES)(GLfixed red, GLfixed green, GLfixed blue);
extern void (APIENTRYP gglColor3xvOES)(const GLfixed *components);
extern void (APIENTRYP gglColor4xvOES)(const GLfixed *components);
extern void (APIENTRYP gglConvolutionParameterxOES)(GLenum target, GLenum pname, GLfixed param);
extern void (APIENTRYP gglConvolutionParameterxvOES)(GLenum target, GLenum pname, const GLfixed *params);
extern void (APIENTRYP gglEvalCoord1xOES)(GLfixed u);
extern void (APIENTRYP gglEvalCoord1xvOES)(const GLfixed *coords);
extern void (APIENTRYP gglEvalCoord2xOES)(GLfixed u, GLfixed v);
extern void (APIENTRYP gglEvalCoord2xvOES)(const GLfixed *coords);
extern void (APIENTRYP gglFeedbackBufferxOES)(GLsizei n, GLenum type, const GLfixed *buffer);
extern void (APIENTRYP gglGetConvolutionParameterxvOES)(GLenum target, GLenum pname, GLfixed *params);
extern void (APIENTRYP gglGetHistogramParameterxvOES)(GLenum target, GLenum pname, GLfixed *params);
extern void (APIENTRYP gglGetLightxOES)(GLenum light, GLenum pname, GLfixed *params);
extern void (APIENTRYP gglGetMapxvOES)(GLenum target, GLenum query, GLfixed *v);
extern void (APIENTRYP gglGetMaterialxOES)(GLenum face, GLenum pname, GLfixed param);
extern void (APIENTRYP gglGetPixelMapxv)(GLenum map, GLint size, GLfixed *values);
extern void (APIENTRYP gglGetTexGenxvOES)(GLenum coord, GLenum pname, GLfixed *params);
extern void (APIENTRYP gglGetTexLevelParameterxvOES)(GLenum target, GLint level, GLenum pname, GLfixed *params);
extern void (APIENTRYP gglIndexxOES)(GLfixed component);
extern void (APIENTRYP gglIndexxvOES)(const GLfixed *component);
extern void (APIENTRYP gglLoadTransposeMatrixxOES)(const GLfixed *m);
extern void (APIENTRYP gglMap1xOES)(GLenum target, GLfixed u1, GLfixed u2, GLint stride, GLint order, GLfixed points);
extern void (APIENTRYP gglMap2xOES)(GLenum target, GLfixed u1, GLfixed u2, GLint ustride, GLint uorder, GLfixed v1, GLfixed v2, GLint vstride, GLint vorder, GLfixed points);
extern void (APIENTRYP gglMapGrid1xOES)(GLint n, GLfixed u1, GLfixed u2);
extern void (APIENTRYP gglMapGrid2xOES)(GLint n, GLfixed u1, GLfixed u2, GLfixed v1, GLfixed v2);
extern void (APIENTRYP gglMultTransposeMatrixxOES)(const GLfixed *m);
extern void (APIENTRYP gglMultiTexCoord1xOES)(GLenum texture, GLfixed s);
extern void (APIENTRYP gglMultiTexCoord1xvOES)(GLenum texture, const GLfixed *coords);
extern void (APIENTRYP gglMultiTexCoord2xOES)(GLenum texture, GLfixed s, GLfixed t);
extern void (APIENTRYP gglMultiTexCoord2xvOES)(GLenum texture, const GLfixed *coords);
extern void (APIENTRYP gglMultiTexCoord3xOES)(GLenum texture, GLfixed s, GLfixed t, GLfixed r);
extern void (APIENTRYP gglMultiTexCoord3xvOES)(GLenum texture, const GLfixed *coords);
extern void (APIENTRYP gglMultiTexCoord4xvOES)(GLenum texture, const GLfixed *coords);
extern void (APIENTRYP gglNormal3xvOES)(const GLfixed *coords);
extern void (APIENTRYP gglPassThroughxOES)(GLfixed token);
extern void (APIENTRYP gglPixelMapx)(GLenum map, GLint size, const GLfixed *values);
extern void (APIENTRYP gglPixelStorex)(GLenum pname, GLfixed param);
extern void (APIENTRYP gglPixelTransferxOES)(GLenum pname, GLfixed param);
extern void (APIENTRYP gglPixelZoomxOES)(GLfixed xfactor, GLfixed yfactor);
extern void (APIENTRYP gglPrioritizeTexturesxOES)(GLsizei n, const GLuint *textures, const GLfixed *priorities);
extern void (APIENTRYP gglRasterPos2xOES)(GLfixed x, GLfixed y);
extern void (APIENTRYP gglRasterPos2xvOES)(const GLfixed *coords);
extern void (APIENTRYP gglRasterPos3xOES)(GLfixed x, GLfixed y, GLfixed z);
extern void (APIENTRYP gglRasterPos3xvOES)(const GLfixed *coords);
extern void (APIENTRYP gglRasterPos4xOES)(GLfixed x, GLfixed y, GLfixed z, GLfixed w);
extern void (APIENTRYP gglRasterPos4xvOES)(const GLfixed *coords);
extern void (APIENTRYP gglRectxOES)(GLfixed x1, GLfixed y1, GLfixed x2, GLfixed y2);
extern void (APIENTRYP gglRectxvOES)(const GLfixed *v1, const GLfixed *v2);
extern void (APIENTRYP gglTexCoord1xOES)(GLfixed s);
extern void (APIENTRYP gglTexCoord1xvOES)(const GLfixed *coords);
extern void (APIENTRYP gglTexCoord2xOES)(GLfixed s, GLfixed t);
extern void (APIENTRYP gglTexCoord2xvOES)(const GLfixed *coords);
extern void (APIENTRYP gglTexCoord3xOES)(GLfixed s, GLfixed t, GLfixed r);
extern void (APIENTRYP gglTexCoord3xvOES)(const GLfixed *coords);
extern void (APIENTRYP gglTexCoord4xOES)(GLfixed s, GLfixed t, GLfixed r, GLfixed q);
extern void (APIENTRYP gglTexCoord4xvOES)(const GLfixed *coords);
extern void (APIENTRYP gglTexGenxOES)(GLenum coord, GLenum pname, GLfixed param);
extern void (APIENTRYP gglTexGenxvOES)(GLenum coord, GLenum pname, const GLfixed *params);
extern void (APIENTRYP gglVertex2xOES)(GLfixed x);
extern void (APIENTRYP gglVertex2xvOES)(const GLfixed *coords);
extern void (APIENTRYP gglVertex3xOES)(GLfixed x, GLfixed y);
extern void (APIENTRYP gglVertex3xvOES)(const GLfixed *coords);
extern void (APIENTRYP gglVertex4xOES)(GLfixed x, GLfixed y, GLfixed z);
extern void (APIENTRYP gglVertex4xvOES)(const GLfixed *coords);

#ifndef GL_OES_query_matrix
#define GL_OES_query_matrix
#endif
extern GLbitfield (APIENTRYP gglQueryMatrixxOES)(GLfixed *mantissa, GLint *exponent);

#ifndef GL_OES_read_format
#define GL_OES_read_format
#define GL_IMPLEMENTATION_COLOR_READ_TYPE_OES 0x8B9A
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES 0x8B9B
#endif

#ifndef GL_OES_single_precision
#define GL_OES_single_precision
#endif
extern void (APIENTRYP gglClearDepthfOES)(GLclampf depth);
extern void (APIENTRYP gglClipPlanefOES)(GLenum plane, const GLfloat *equation);
extern void (APIENTRYP gglDepthRangefOES)(GLclampf n, GLclampf f);
extern void (APIENTRYP gglFrustumfOES)(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);
extern void (APIENTRYP gglGetClipPlanefOES)(GLenum plane, GLfloat *equation);
extern void (APIENTRYP gglOrthofOES)(GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f);

#ifndef GL_OML_interlace
#define GL_OML_interlace
#define GL_INTERLACE_OML 0x8980
#define GL_INTERLACE_READ_OML 0x8981
#endif

#ifndef GL_OML_resample
#define GL_OML_resample
#define GL_PACK_RESAMPLE_OML 0x8984
#define GL_UNPACK_RESAMPLE_OML 0x8985
#define GL_RESAMPLE_REPLICATE_OML 0x8986
#define GL_RESAMPLE_ZERO_FILL_OML 0x8987
#define GL_RESAMPLE_AVERAGE_OML 0x8988
#define GL_RESAMPLE_DECIMATE_OML 0x8989
#endif

#ifndef GL_OML_subsample
#define GL_OML_subsample
#define GL_FORMAT_SUBSAMPLE_24_24_OML 0x8982
#define GL_FORMAT_SUBSAMPLE_244_244_OML 0x8983
#endif

#ifndef GL_OVR_multiview
#define GL_OVR_multiview
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_NUM_VIEWS_OVR 0x9630
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_BASE_VIEW_INDEX_OVR 0x9632
#define GL_MAX_VIEWS_OVR 0x9631
#define GL_FRAMEBUFFER_INCOMPLETE_VIEW_TARGETS_OVR 0x9633
#endif
extern void (APIENTRYP gglFramebufferTextureMultiviewOVR)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews);

#ifndef GL_OVR_multiview2
#define GL_OVR_multiview2
#endif

#ifndef GL_PGI_misc_hints
#define GL_PGI_misc_hints
#define GL_PREFER_DOUBLEBUFFER_HINT_PGI 0x1A1F8
#define GL_CONSERVE_MEMORY_HINT_PGI 0x1A1FD
#define GL_RECLAIM_MEMORY_HINT_PGI 0x1A1FE
#define GL_NATIVE_GRAPHICS_HANDLE_PGI 0x1A202
#define GL_NATIVE_GRAPHICS_BEGIN_HINT_PGI 0x1A203
#define GL_NATIVE_GRAPHICS_END_HINT_PGI 0x1A204
#define GL_ALWAYS_FAST_HINT_PGI 0x1A20C
#define GL_ALWAYS_SOFT_HINT_PGI 0x1A20D
#define GL_ALLOW_DRAW_OBJ_HINT_PGI 0x1A20E
#define GL_ALLOW_DRAW_WIN_HINT_PGI 0x1A20F
#define GL_ALLOW_DRAW_FRG_HINT_PGI 0x1A210
#define GL_ALLOW_DRAW_MEM_HINT_PGI 0x1A211
#define GL_STRICT_DEPTHFUNC_HINT_PGI 0x1A216
#define GL_STRICT_LIGHTING_HINT_PGI 0x1A217
#define GL_STRICT_SCISSOR_HINT_PGI 0x1A218
#define GL_FULL_STIPPLE_HINT_PGI 0x1A219
#define GL_CLIP_NEAR_HINT_PGI 0x1A220
#define GL_CLIP_FAR_HINT_PGI 0x1A221
#define GL_WIDE_LINE_HINT_PGI 0x1A222
#define GL_BACK_NORMALS_HINT_PGI 0x1A223
#endif
extern void (APIENTRYP gglHintPGI)(GLenum target, GLint mode);

#ifndef GL_PGI_vertex_hints
#define GL_PGI_vertex_hints
#define GL_VERTEX_DATA_HINT_PGI 0x1A22A
#define GL_VERTEX_CONSISTENT_HINT_PGI 0x1A22B
#define GL_MATERIAL_SIDE_HINT_PGI 0x1A22C
#define GL_MAX_VERTEX_HINT_PGI 0x1A22D
#define GL_COLOR3_BIT_PGI 0x00010000
#define GL_COLOR4_BIT_PGI 0x00020000
#define GL_EDGEFLAG_BIT_PGI 0x00040000
#define GL_INDEX_BIT_PGI 0x00080000
#define GL_MAT_AMBIENT_BIT_PGI 0x00100000
#define GL_MAT_AMBIENT_AND_DIFFUSE_BIT_PGI 0x00200000
#define GL_MAT_DIFFUSE_BIT_PGI 0x00400000
#define GL_MAT_EMISSION_BIT_PGI 0x00800000
#define GL_MAT_COLOR_INDEXES_BIT_PGI 0x01000000
#define GL_MAT_SHININESS_BIT_PGI 0x02000000
#define GL_MAT_SPECULAR_BIT_PGI 0x04000000
#define GL_NORMAL_BIT_PGI 0x08000000
#define GL_TEXCOORD1_BIT_PGI 0x10000000
#define GL_TEXCOORD2_BIT_PGI 0x20000000
#define GL_TEXCOORD3_BIT_PGI 0x40000000
#define GL_TEXCOORD4_BIT_PGI 0x80000000
#define GL_VERTEX23_BIT_PGI 0x00000004
#define GL_VERTEX4_BIT_PGI 0x00000008
#endif

#ifndef GL_REND_screen_coordinates
#define GL_REND_screen_coordinates
#define GL_SCREEN_COORDINATES_REND 0x8490
#define GL_INVERTED_SCREEN_W_REND 0x8491
#endif

#ifndef GL_S3_s3tc
#define GL_S3_s3tc
#define GL_RGB_S3TC 0x83A0
#define GL_RGB4_S3TC 0x83A1
#define GL_RGBA_S3TC 0x83A2
#define GL_RGBA4_S3TC 0x83A3
#define GL_RGBA_DXT5_S3TC 0x83A4
#define GL_RGBA4_DXT5_S3TC 0x83A5
#endif

#ifndef GL_SGIS_detail_texture
#define GL_SGIS_detail_texture
#define GL_DETAIL_TEXTURE_2D_SGIS 0x8095
#define GL_DETAIL_TEXTURE_2D_BINDING_SGIS 0x8096
#define GL_LINEAR_DETAIL_SGIS 0x8097
#define GL_LINEAR_DETAIL_ALPHA_SGIS 0x8098
#define GL_LINEAR_DETAIL_COLOR_SGIS 0x8099
#define GL_DETAIL_TEXTURE_LEVEL_SGIS 0x809A
#define GL_DETAIL_TEXTURE_MODE_SGIS 0x809B
#define GL_DETAIL_TEXTURE_FUNC_POINTS_SGIS 0x809C
#endif
extern void (APIENTRYP gglDetailTexFuncSGIS)(GLenum target, GLsizei n, const GLfloat *points);
extern void (APIENTRYP gglGetDetailTexFuncSGIS)(GLenum target, GLfloat *points);

#ifndef GL_SGIS_fog_function
#define GL_SGIS_fog_function
#define GL_FOG_FUNC_SGIS 0x812A
#define GL_FOG_FUNC_POINTS_SGIS 0x812B
#define GL_MAX_FOG_FUNC_POINTS_SGIS 0x812C
#endif
extern void (APIENTRYP gglFogFuncSGIS)(GLsizei n, const GLfloat *points);
extern void (APIENTRYP gglGetFogFuncSGIS)(GLfloat *points);

#ifndef GL_SGIS_generate_mipmap
#define GL_SGIS_generate_mipmap
#define GL_GENERATE_MIPMAP_SGIS 0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS 0x8192
#endif

#ifndef GL_SGIS_multisample
#define GL_SGIS_multisample
#define GL_MULTISAMPLE_SGIS 0x809D
#define GL_SAMPLE_ALPHA_TO_MASK_SGIS 0x809E
#define GL_SAMPLE_ALPHA_TO_ONE_SGIS 0x809F
#define GL_SAMPLE_MASK_SGIS 0x80A0
#define GL_1PASS_SGIS 0x80A1
#define GL_2PASS_0_SGIS 0x80A2
#define GL_2PASS_1_SGIS 0x80A3
#define GL_4PASS_0_SGIS 0x80A4
#define GL_4PASS_1_SGIS 0x80A5
#define GL_4PASS_2_SGIS 0x80A6
#define GL_4PASS_3_SGIS 0x80A7
#define GL_SAMPLE_BUFFERS_SGIS 0x80A8
#define GL_SAMPLES_SGIS 0x80A9
#define GL_SAMPLE_MASK_VALUE_SGIS 0x80AA
#define GL_SAMPLE_MASK_INVERT_SGIS 0x80AB
#define GL_SAMPLE_PATTERN_SGIS 0x80AC
#endif
extern void (APIENTRYP gglSampleMaskSGIS)(GLclampf value, GLboolean invert);
extern void (APIENTRYP gglSamplePatternSGIS)(GLenum pattern);

#ifndef GL_SGIS_pixel_texture
#define GL_SGIS_pixel_texture
#define GL_PIXEL_TEXTURE_SGIS 0x8353
#define GL_PIXEL_FRAGMENT_RGB_SOURCE_SGIS 0x8354
#define GL_PIXEL_FRAGMENT_ALPHA_SOURCE_SGIS 0x8355
#define GL_PIXEL_GROUP_COLOR_SGIS 0x8356
#endif
extern void (APIENTRYP gglPixelTexGenParameteriSGIS)(GLenum pname, GLint param);
extern void (APIENTRYP gglPixelTexGenParameterivSGIS)(GLenum pname, const GLint *params);
extern void (APIENTRYP gglPixelTexGenParameterfSGIS)(GLenum pname, GLfloat param);
extern void (APIENTRYP gglPixelTexGenParameterfvSGIS)(GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglGetPixelTexGenParameterivSGIS)(GLenum pname, GLint *params);
extern void (APIENTRYP gglGetPixelTexGenParameterfvSGIS)(GLenum pname, GLfloat *params);

#ifndef GL_SGIS_point_line_texgen
#define GL_SGIS_point_line_texgen
#define GL_EYE_DISTANCE_TO_POINT_SGIS 0x81F0
#define GL_OBJECT_DISTANCE_TO_POINT_SGIS 0x81F1
#define GL_EYE_DISTANCE_TO_LINE_SGIS 0x81F2
#define GL_OBJECT_DISTANCE_TO_LINE_SGIS 0x81F3
#define GL_EYE_POINT_SGIS 0x81F4
#define GL_OBJECT_POINT_SGIS 0x81F5
#define GL_EYE_LINE_SGIS 0x81F6
#define GL_OBJECT_LINE_SGIS 0x81F7
#endif

#ifndef GL_SGIS_point_parameters
#define GL_SGIS_point_parameters
#define GL_POINT_SIZE_MIN_SGIS 0x8126
#define GL_POINT_SIZE_MAX_SGIS 0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_SGIS 0x8128
#define GL_DISTANCE_ATTENUATION_SGIS 0x8129
#endif
extern void (APIENTRYP gglPointParameterfSGIS)(GLenum pname, GLfloat param);
extern void (APIENTRYP gglPointParameterfvSGIS)(GLenum pname, const GLfloat *params);

#ifndef GL_SGIS_sharpen_texture
#define GL_SGIS_sharpen_texture
#define GL_LINEAR_SHARPEN_SGIS 0x80AD
#define GL_LINEAR_SHARPEN_ALPHA_SGIS 0x80AE
#define GL_LINEAR_SHARPEN_COLOR_SGIS 0x80AF
#define GL_SHARPEN_TEXTURE_FUNC_POINTS_SGIS 0x80B0
#endif
extern void (APIENTRYP gglSharpenTexFuncSGIS)(GLenum target, GLsizei n, const GLfloat *points);
extern void (APIENTRYP gglGetSharpenTexFuncSGIS)(GLenum target, GLfloat *points);

#ifndef GL_SGIS_texture4D
#define GL_SGIS_texture4D
#define GL_PACK_SKIP_VOLUMES_SGIS 0x8130
#define GL_PACK_IMAGE_DEPTH_SGIS 0x8131
#define GL_UNPACK_SKIP_VOLUMES_SGIS 0x8132
#define GL_UNPACK_IMAGE_DEPTH_SGIS 0x8133
#define GL_TEXTURE_4D_SGIS 0x8134
#define GL_PROXY_TEXTURE_4D_SGIS 0x8135
#define GL_TEXTURE_4DSIZE_SGIS 0x8136
#define GL_TEXTURE_WRAP_Q_SGIS 0x8137
#define GL_MAX_4D_TEXTURE_SIZE_SGIS 0x8138
#define GL_TEXTURE_4D_BINDING_SGIS 0x814F
#endif
extern void (APIENTRYP gglTexImage4DSGIS)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTexSubImage4DSGIS)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const void *pixels);

#ifndef GL_SGIS_texture_border_clamp
#define GL_SGIS_texture_border_clamp
#define GL_CLAMP_TO_BORDER_SGIS 0x812D
#endif

#ifndef GL_SGIS_texture_color_mask
#define GL_SGIS_texture_color_mask
#define GL_TEXTURE_COLOR_WRITEMASK_SGIS 0x81EF
#endif
extern void (APIENTRYP gglTextureColorMaskSGIS)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);

#ifndef GL_SGIS_texture_edge_clamp
#define GL_SGIS_texture_edge_clamp
#define GL_CLAMP_TO_EDGE_SGIS 0x812F
#endif

#ifndef GL_SGIS_texture_filter4
#define GL_SGIS_texture_filter4
#define GL_FILTER4_SGIS 0x8146
#define GL_TEXTURE_FILTER4_SIZE_SGIS 0x8147
#endif
extern void (APIENTRYP gglGetTexFilterFuncSGIS)(GLenum target, GLenum filter, GLfloat *weights);
extern void (APIENTRYP gglTexFilterFuncSGIS)(GLenum target, GLenum filter, GLsizei n, const GLfloat *weights);

#ifndef GL_SGIS_texture_lod
#define GL_SGIS_texture_lod
#define GL_TEXTURE_MIN_LOD_SGIS 0x813A
#define GL_TEXTURE_MAX_LOD_SGIS 0x813B
#define GL_TEXTURE_BASE_LEVEL_SGIS 0x813C
#define GL_TEXTURE_MAX_LEVEL_SGIS 0x813D
#endif

#ifndef GL_SGIS_texture_select
#define GL_SGIS_texture_select
#define GL_DUAL_ALPHA4_SGIS 0x8110
#define GL_DUAL_ALPHA8_SGIS 0x8111
#define GL_DUAL_ALPHA12_SGIS 0x8112
#define GL_DUAL_ALPHA16_SGIS 0x8113
#define GL_DUAL_LUMINANCE4_SGIS 0x8114
#define GL_DUAL_LUMINANCE8_SGIS 0x8115
#define GL_DUAL_LUMINANCE12_SGIS 0x8116
#define GL_DUAL_LUMINANCE16_SGIS 0x8117
#define GL_DUAL_INTENSITY4_SGIS 0x8118
#define GL_DUAL_INTENSITY8_SGIS 0x8119
#define GL_DUAL_INTENSITY12_SGIS 0x811A
#define GL_DUAL_INTENSITY16_SGIS 0x811B
#define GL_DUAL_LUMINANCE_ALPHA4_SGIS 0x811C
#define GL_DUAL_LUMINANCE_ALPHA8_SGIS 0x811D
#define GL_QUAD_ALPHA4_SGIS 0x811E
#define GL_QUAD_ALPHA8_SGIS 0x811F
#define GL_QUAD_LUMINANCE4_SGIS 0x8120
#define GL_QUAD_LUMINANCE8_SGIS 0x8121
#define GL_QUAD_INTENSITY4_SGIS 0x8122
#define GL_QUAD_INTENSITY8_SGIS 0x8123
#define GL_DUAL_TEXTURE_SELECT_SGIS 0x8124
#define GL_QUAD_TEXTURE_SELECT_SGIS 0x8125
#endif

#ifndef GL_SGIX_async
#define GL_SGIX_async
#define GL_ASYNC_MARKER_SGIX 0x8329
#endif
extern void (APIENTRYP gglAsyncMarkerSGIX)(GLuint marker);
extern GLint (APIENTRYP gglFinishAsyncSGIX)(GLuint *markerp);
extern GLint (APIENTRYP gglPollAsyncSGIX)(GLuint *markerp);
extern GLuint (APIENTRYP gglGenAsyncMarkersSGIX)(GLsizei range);
extern void (APIENTRYP gglDeleteAsyncMarkersSGIX)(GLuint marker, GLsizei range);
extern GLboolean (APIENTRYP gglIsAsyncMarkerSGIX)(GLuint marker);

#ifndef GL_SGIX_async_histogram
#define GL_SGIX_async_histogram
#define GL_ASYNC_HISTOGRAM_SGIX 0x832C
#define GL_MAX_ASYNC_HISTOGRAM_SGIX 0x832D
#endif

#ifndef GL_SGIX_async_pixel
#define GL_SGIX_async_pixel
#define GL_ASYNC_TEX_IMAGE_SGIX 0x835C
#define GL_ASYNC_DRAW_PIXELS_SGIX 0x835D
#define GL_ASYNC_READ_PIXELS_SGIX 0x835E
#define GL_MAX_ASYNC_TEX_IMAGE_SGIX 0x835F
#define GL_MAX_ASYNC_DRAW_PIXELS_SGIX 0x8360
#define GL_MAX_ASYNC_READ_PIXELS_SGIX 0x8361
#endif

#ifndef GL_SGIX_blend_alpha_minmax
#define GL_SGIX_blend_alpha_minmax
#define GL_ALPHA_MIN_SGIX 0x8320
#define GL_ALPHA_MAX_SGIX 0x8321
#endif

#ifndef GL_SGIX_calligraphic_fragment
#define GL_SGIX_calligraphic_fragment
#define GL_CALLIGRAPHIC_FRAGMENT_SGIX 0x8183
#endif

#ifndef GL_SGIX_clipmap
#define GL_SGIX_clipmap
#define GL_LINEAR_CLIPMAP_LINEAR_SGIX 0x8170
#define GL_TEXTURE_CLIPMAP_CENTER_SGIX 0x8171
#define GL_TEXTURE_CLIPMAP_FRAME_SGIX 0x8172
#define GL_TEXTURE_CLIPMAP_OFFSET_SGIX 0x8173
#define GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX 0x8174
#define GL_TEXTURE_CLIPMAP_LOD_OFFSET_SGIX 0x8175
#define GL_TEXTURE_CLIPMAP_DEPTH_SGIX 0x8176
#define GL_MAX_CLIPMAP_DEPTH_SGIX 0x8177
#define GL_MAX_CLIPMAP_VIRTUAL_DEPTH_SGIX 0x8178
#define GL_NEAREST_CLIPMAP_NEAREST_SGIX 0x844D
#define GL_NEAREST_CLIPMAP_LINEAR_SGIX 0x844E
#define GL_LINEAR_CLIPMAP_NEAREST_SGIX 0x844F
#endif

#ifndef GL_SGIX_convolution_accuracy
#define GL_SGIX_convolution_accuracy
#define GL_CONVOLUTION_HINT_SGIX 0x8316
#endif

#ifndef GL_SGIX_depth_pass_instrument
#define GL_SGIX_depth_pass_instrument
#endif

#ifndef GL_SGIX_depth_texture
#define GL_SGIX_depth_texture
#define GL_DEPTH_COMPONENT16_SGIX 0x81A5
#define GL_DEPTH_COMPONENT24_SGIX 0x81A6
#define GL_DEPTH_COMPONENT32_SGIX 0x81A7
#endif

#ifndef GL_SGIX_flush_raster
#define GL_SGIX_flush_raster
#endif
extern void (APIENTRYP gglFlushRasterSGIX)();

#ifndef GL_SGIX_fog_offset
#define GL_SGIX_fog_offset
#define GL_FOG_OFFSET_SGIX 0x8198
#define GL_FOG_OFFSET_VALUE_SGIX 0x8199
#endif

#ifndef GL_SGIX_fragment_lighting
#define GL_SGIX_fragment_lighting
#define GL_FRAGMENT_LIGHTING_SGIX 0x8400
#define GL_FRAGMENT_COLOR_MATERIAL_SGIX 0x8401
#define GL_FRAGMENT_COLOR_MATERIAL_FACE_SGIX 0x8402
#define GL_FRAGMENT_COLOR_MATERIAL_PARAMETER_SGIX 0x8403
#define GL_MAX_FRAGMENT_LIGHTS_SGIX 0x8404
#define GL_MAX_ACTIVE_LIGHTS_SGIX 0x8405
#define GL_CURRENT_RASTER_NORMAL_SGIX 0x8406
#define GL_LIGHT_ENV_MODE_SGIX 0x8407
#define GL_FRAGMENT_LIGHT_MODEL_LOCAL_VIEWER_SGIX 0x8408
#define GL_FRAGMENT_LIGHT_MODEL_TWO_SIDE_SGIX 0x8409
#define GL_FRAGMENT_LIGHT_MODEL_AMBIENT_SGIX 0x840A
#define GL_FRAGMENT_LIGHT_MODEL_NORMAL_INTERPOLATION_SGIX 0x840B
#define GL_FRAGMENT_LIGHT0_SGIX 0x840C
#define GL_FRAGMENT_LIGHT1_SGIX 0x840D
#define GL_FRAGMENT_LIGHT2_SGIX 0x840E
#define GL_FRAGMENT_LIGHT3_SGIX 0x840F
#define GL_FRAGMENT_LIGHT4_SGIX 0x8410
#define GL_FRAGMENT_LIGHT5_SGIX 0x8411
#define GL_FRAGMENT_LIGHT6_SGIX 0x8412
#define GL_FRAGMENT_LIGHT7_SGIX 0x8413
#endif
extern void (APIENTRYP gglFragmentColorMaterialSGIX)(GLenum face, GLenum mode);
extern void (APIENTRYP gglFragmentLightfSGIX)(GLenum light, GLenum pname, GLfloat param);
extern void (APIENTRYP gglFragmentLightfvSGIX)(GLenum light, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglFragmentLightiSGIX)(GLenum light, GLenum pname, GLint param);
extern void (APIENTRYP gglFragmentLightivSGIX)(GLenum light, GLenum pname, const GLint *params);
extern void (APIENTRYP gglFragmentLightModelfSGIX)(GLenum pname, GLfloat param);
extern void (APIENTRYP gglFragmentLightModelfvSGIX)(GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglFragmentLightModeliSGIX)(GLenum pname, GLint param);
extern void (APIENTRYP gglFragmentLightModelivSGIX)(GLenum pname, const GLint *params);
extern void (APIENTRYP gglFragmentMaterialfSGIX)(GLenum face, GLenum pname, GLfloat param);
extern void (APIENTRYP gglFragmentMaterialfvSGIX)(GLenum face, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglFragmentMaterialiSGIX)(GLenum face, GLenum pname, GLint param);
extern void (APIENTRYP gglFragmentMaterialivSGIX)(GLenum face, GLenum pname, const GLint *params);
extern void (APIENTRYP gglGetFragmentLightfvSGIX)(GLenum light, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetFragmentLightivSGIX)(GLenum light, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetFragmentMaterialfvSGIX)(GLenum face, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetFragmentMaterialivSGIX)(GLenum face, GLenum pname, GLint *params);
extern void (APIENTRYP gglLightEnviSGIX)(GLenum pname, GLint param);

#ifndef GL_SGIX_framezoom
#define GL_SGIX_framezoom
#define GL_FRAMEZOOM_SGIX 0x818B
#define GL_FRAMEZOOM_FACTOR_SGIX 0x818C
#define GL_MAX_FRAMEZOOM_FACTOR_SGIX 0x818D
#endif
extern void (APIENTRYP gglFrameZoomSGIX)(GLint factor);

#ifndef GL_SGIX_igloo_interface
#define GL_SGIX_igloo_interface
#endif
extern void (APIENTRYP gglIglooInterfaceSGIX)(GLenum pname, const void *params);

#ifndef GL_SGIX_instruments
#define GL_SGIX_instruments
#define GL_INSTRUMENT_BUFFER_POINTER_SGIX 0x8180
#define GL_INSTRUMENT_MEASUREMENTS_SGIX 0x8181
#endif
extern GLint (APIENTRYP gglGetInstrumentsSGIX)();
extern void (APIENTRYP gglInstrumentsBufferSGIX)(GLsizei size, GLint *buffer);
extern GLint (APIENTRYP gglPollInstrumentsSGIX)(GLint *marker_p);
extern void (APIENTRYP gglReadInstrumentsSGIX)(GLint marker);
extern void (APIENTRYP gglStartInstrumentsSGIX)();
extern void (APIENTRYP gglStopInstrumentsSGIX)(GLint marker);

#ifndef GL_SGIX_interlace
#define GL_SGIX_interlace
#define GL_INTERLACE_SGIX 0x8094
#endif

#ifndef GL_SGIX_ir_instrument1
#define GL_SGIX_ir_instrument1
#define GL_IR_INSTRUMENT1_SGIX 0x817F
#endif

#ifndef GL_SGIX_list_priority
#define GL_SGIX_list_priority
#define GL_LIST_PRIORITY_SGIX 0x8182
#endif
extern void (APIENTRYP gglGetListParameterfvSGIX)(GLuint list, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetListParameterivSGIX)(GLuint list, GLenum pname, GLint *params);
extern void (APIENTRYP gglListParameterfSGIX)(GLuint list, GLenum pname, GLfloat param);
extern void (APIENTRYP gglListParameterfvSGIX)(GLuint list, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglListParameteriSGIX)(GLuint list, GLenum pname, GLint param);
extern void (APIENTRYP gglListParameterivSGIX)(GLuint list, GLenum pname, const GLint *params);

#ifndef GL_SGIX_pixel_texture
#define GL_SGIX_pixel_texture
#define GL_PIXEL_TEX_GEN_SGIX 0x8139
#define GL_PIXEL_TEX_GEN_MODE_SGIX 0x832B
#endif
extern void (APIENTRYP gglPixelTexGenSGIX)(GLenum mode);

#ifndef GL_SGIX_pixel_tiles
#define GL_SGIX_pixel_tiles
#define GL_PIXEL_TILE_BEST_ALIGNMENT_SGIX 0x813E
#define GL_PIXEL_TILE_CACHE_INCREMENT_SGIX 0x813F
#define GL_PIXEL_TILE_WIDTH_SGIX 0x8140
#define GL_PIXEL_TILE_HEIGHT_SGIX 0x8141
#define GL_PIXEL_TILE_GRID_WIDTH_SGIX 0x8142
#define GL_PIXEL_TILE_GRID_HEIGHT_SGIX 0x8143
#define GL_PIXEL_TILE_GRID_DEPTH_SGIX 0x8144
#define GL_PIXEL_TILE_CACHE_SIZE_SGIX 0x8145
#endif

#ifndef GL_SGIX_polynomial_ffd
#define GL_SGIX_polynomial_ffd
#define GL_TEXTURE_DEFORMATION_BIT_SGIX 0x00000001
#define GL_GEOMETRY_DEFORMATION_BIT_SGIX 0x00000002
#define GL_GEOMETRY_DEFORMATION_SGIX 0x8194
#define GL_TEXTURE_DEFORMATION_SGIX 0x8195
#define GL_DEFORMATIONS_MASK_SGIX 0x8196
#define GL_MAX_DEFORMATION_ORDER_SGIX 0x8197
#endif
extern void (APIENTRYP gglDeformationMap3dSGIX)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, const GLdouble *points);
extern void (APIENTRYP gglDeformationMap3fSGIX)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, const GLfloat *points);
extern void (APIENTRYP gglDeformSGIX)(GLbitfield mask);
extern void (APIENTRYP gglLoadIdentityDeformationMapSGIX)(GLbitfield mask);

#ifndef GL_SGIX_reference_plane
#define GL_SGIX_reference_plane
#define GL_REFERENCE_PLANE_SGIX 0x817D
#define GL_REFERENCE_PLANE_EQUATION_SGIX 0x817E
#endif
extern void (APIENTRYP gglReferencePlaneSGIX)(const GLdouble *equation);

#ifndef GL_SGIX_resample
#define GL_SGIX_resample
#define GL_PACK_RESAMPLE_SGIX 0x842E
#define GL_UNPACK_RESAMPLE_SGIX 0x842F
#define GL_RESAMPLE_REPLICATE_SGIX 0x8433
#define GL_RESAMPLE_ZERO_FILL_SGIX 0x8434
#define GL_RESAMPLE_DECIMATE_SGIX 0x8430
#endif

#ifndef GL_SGIX_scalebias_hint
#define GL_SGIX_scalebias_hint
#define GL_SCALEBIAS_HINT_SGIX 0x8322
#endif

#ifndef GL_SGIX_shadow
#define GL_SGIX_shadow
#define GL_TEXTURE_COMPARE_SGIX 0x819A
#define GL_TEXTURE_COMPARE_OPERATOR_SGIX 0x819B
#define GL_TEXTURE_LEQUAL_R_SGIX 0x819C
#define GL_TEXTURE_GEQUAL_R_SGIX 0x819D
#endif

#ifndef GL_SGIX_shadow_ambient
#define GL_SGIX_shadow_ambient
#define GL_SHADOW_AMBIENT_SGIX 0x80BF
#endif

#ifndef GL_SGIX_sprite
#define GL_SGIX_sprite
#define GL_SPRITE_SGIX 0x8148
#define GL_SPRITE_MODE_SGIX 0x8149
#define GL_SPRITE_AXIS_SGIX 0x814A
#define GL_SPRITE_TRANSLATION_SGIX 0x814B
#define GL_SPRITE_AXIAL_SGIX 0x814C
#define GL_SPRITE_OBJECT_ALIGNED_SGIX 0x814D
#define GL_SPRITE_EYE_ALIGNED_SGIX 0x814E
#endif
extern void (APIENTRYP gglSpriteParameterfSGIX)(GLenum pname, GLfloat param);
extern void (APIENTRYP gglSpriteParameterfvSGIX)(GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglSpriteParameteriSGIX)(GLenum pname, GLint param);
extern void (APIENTRYP gglSpriteParameterivSGIX)(GLenum pname, const GLint *params);

#ifndef GL_SGIX_subsample
#define GL_SGIX_subsample
#define GL_PACK_SUBSAMPLE_RATE_SGIX 0x85A0
#define GL_UNPACK_SUBSAMPLE_RATE_SGIX 0x85A1
#define GL_PIXEL_SUBSAMPLE_4444_SGIX 0x85A2
#define GL_PIXEL_SUBSAMPLE_2424_SGIX 0x85A3
#define GL_PIXEL_SUBSAMPLE_4242_SGIX 0x85A4
#endif

#ifndef GL_SGIX_tag_sample_buffer
#define GL_SGIX_tag_sample_buffer
#endif
extern void (APIENTRYP gglTagSampleBufferSGIX)();

#ifndef GL_SGIX_texture_add_env
#define GL_SGIX_texture_add_env
#define GL_TEXTURE_ENV_BIAS_SGIX 0x80BE
#endif

#ifndef GL_SGIX_texture_coordinate_clamp
#define GL_SGIX_texture_coordinate_clamp
#define GL_TEXTURE_MAX_CLAMP_S_SGIX 0x8369
#define GL_TEXTURE_MAX_CLAMP_T_SGIX 0x836A
#define GL_TEXTURE_MAX_CLAMP_R_SGIX 0x836B
#endif

#ifndef GL_SGIX_texture_lod_bias
#define GL_SGIX_texture_lod_bias
#define GL_TEXTURE_LOD_BIAS_S_SGIX 0x818E
#define GL_TEXTURE_LOD_BIAS_T_SGIX 0x818F
#define GL_TEXTURE_LOD_BIAS_R_SGIX 0x8190
#endif

#ifndef GL_SGIX_texture_multi_buffer
#define GL_SGIX_texture_multi_buffer
#define GL_TEXTURE_MULTI_BUFFER_HINT_SGIX 0x812E
#endif

#ifndef GL_SGIX_texture_scale_bias
#define GL_SGIX_texture_scale_bias
#define GL_POST_TEXTURE_FILTER_BIAS_SGIX 0x8179
#define GL_POST_TEXTURE_FILTER_SCALE_SGIX 0x817A
#define GL_POST_TEXTURE_FILTER_BIAS_RANGE_SGIX 0x817B
#define GL_POST_TEXTURE_FILTER_SCALE_RANGE_SGIX 0x817C
#endif

#ifndef GL_SGIX_vertex_preclip
#define GL_SGIX_vertex_preclip
#define GL_VERTEX_PRECLIP_SGIX 0x83EE
#define GL_VERTEX_PRECLIP_HINT_SGIX 0x83EF
#endif

#ifndef GL_SGIX_ycrcb
#define GL_SGIX_ycrcb
#define GL_YCRCB_422_SGIX 0x81BB
#define GL_YCRCB_444_SGIX 0x81BC
#endif

#ifndef GL_SGIX_ycrcb_subsample
#define GL_SGIX_ycrcb_subsample
#endif

#ifndef GL_SGIX_ycrcba
#define GL_SGIX_ycrcba
#define GL_YCRCB_SGIX 0x8318
#define GL_YCRCBA_SGIX 0x8319
#endif

#ifndef GL_SGI_color_matrix
#define GL_SGI_color_matrix
#define GL_COLOR_MATRIX_SGI 0x80B1
#define GL_COLOR_MATRIX_STACK_DEPTH_SGI 0x80B2
#define GL_MAX_COLOR_MATRIX_STACK_DEPTH_SGI 0x80B3
#define GL_POST_COLOR_MATRIX_RED_SCALE_SGI 0x80B4
#define GL_POST_COLOR_MATRIX_GREEN_SCALE_SGI 0x80B5
#define GL_POST_COLOR_MATRIX_BLUE_SCALE_SGI 0x80B6
#define GL_POST_COLOR_MATRIX_ALPHA_SCALE_SGI 0x80B7
#define GL_POST_COLOR_MATRIX_RED_BIAS_SGI 0x80B8
#define GL_POST_COLOR_MATRIX_GREEN_BIAS_SGI 0x80B9
#define GL_POST_COLOR_MATRIX_BLUE_BIAS_SGI 0x80BA
#define GL_POST_COLOR_MATRIX_ALPHA_BIAS_SGI 0x80BB
#endif

#ifndef GL_SGI_color_table
#define GL_SGI_color_table
#define GL_COLOR_TABLE_SGI 0x80D0
#define GL_POST_CONVOLUTION_COLOR_TABLE_SGI 0x80D1
#define GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI 0x80D2
#define GL_PROXY_COLOR_TABLE_SGI 0x80D3
#define GL_PROXY_POST_CONVOLUTION_COLOR_TABLE_SGI 0x80D4
#define GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE_SGI 0x80D5
#define GL_COLOR_TABLE_SCALE_SGI 0x80D6
#define GL_COLOR_TABLE_BIAS_SGI 0x80D7
#define GL_COLOR_TABLE_FORMAT_SGI 0x80D8
#define GL_COLOR_TABLE_WIDTH_SGI 0x80D9
#define GL_COLOR_TABLE_RED_SIZE_SGI 0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE_SGI 0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE_SGI 0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE_SGI 0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE_SGI 0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE_SGI 0x80DF
#endif
extern void (APIENTRYP gglColorTableSGI)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table);
extern void (APIENTRYP gglColorTableParameterfvSGI)(GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglColorTableParameterivSGI)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglCopyColorTableSGI)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
extern void (APIENTRYP gglGetColorTableSGI)(GLenum target, GLenum format, GLenum type, void *table);
extern void (APIENTRYP gglGetColorTableParameterfvSGI)(GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetColorTableParameterivSGI)(GLenum target, GLenum pname, GLint *params);

#ifndef GL_SGI_texture_color_table
#define GL_SGI_texture_color_table
#define GL_TEXTURE_COLOR_TABLE_SGI 0x80BC
#define GL_PROXY_TEXTURE_COLOR_TABLE_SGI 0x80BD
#endif

#ifndef GL_SUNX_constant_data
#define GL_SUNX_constant_data
#define GL_UNPACK_CONSTANT_DATA_SUNX 0x81D5
#define GL_TEXTURE_CONSTANT_DATA_SUNX 0x81D6
#endif
extern void (APIENTRYP gglFinishTextureSUNX)();

#ifndef GL_SUN_convolution_border_modes
#define GL_SUN_convolution_border_modes
#define GL_WRAP_BORDER_SUN 0x81D4
#endif

#ifndef GL_SUN_global_alpha
#define GL_SUN_global_alpha
#define GL_GLOBAL_ALPHA_SUN 0x81D9
#define GL_GLOBAL_ALPHA_FACTOR_SUN 0x81DA
#endif
extern void (APIENTRYP gglGlobalAlphaFactorbSUN)(GLbyte factor);
extern void (APIENTRYP gglGlobalAlphaFactorsSUN)(GLshort factor);
extern void (APIENTRYP gglGlobalAlphaFactoriSUN)(GLint factor);
extern void (APIENTRYP gglGlobalAlphaFactorfSUN)(GLfloat factor);
extern void (APIENTRYP gglGlobalAlphaFactordSUN)(GLdouble factor);
extern void (APIENTRYP gglGlobalAlphaFactorubSUN)(GLubyte factor);
extern void (APIENTRYP gglGlobalAlphaFactorusSUN)(GLushort factor);
extern void (APIENTRYP gglGlobalAlphaFactoruiSUN)(GLuint factor);

#ifndef GL_SUN_mesh_array
#define GL_SUN_mesh_array
#define GL_QUAD_MESH_SUN 0x8614
#define GL_TRIANGLE_MESH_SUN 0x8615
#endif
extern void (APIENTRYP gglDrawMeshArraysSUN)(GLenum mode, GLint first, GLsizei count, GLsizei width);

#ifndef GL_SUN_slice_accum
#define GL_SUN_slice_accum
#define GL_SLICE_ACCUM_SUN 0x85CC
#endif

#ifndef GL_SUN_triangle_list
#define GL_SUN_triangle_list
#define GL_RESTART_SUN 0x0001
#define GL_REPLACE_MIDDLE_SUN 0x0002
#define GL_REPLACE_OLDEST_SUN 0x0003
#define GL_TRIANGLE_LIST_SUN 0x81D7
#define GL_REPLACEMENT_CODE_SUN 0x81D8
#define GL_REPLACEMENT_CODE_ARRAY_SUN 0x85C0
#define GL_REPLACEMENT_CODE_ARRAY_TYPE_SUN 0x85C1
#define GL_REPLACEMENT_CODE_ARRAY_STRIDE_SUN 0x85C2
#define GL_REPLACEMENT_CODE_ARRAY_POINTER_SUN 0x85C3
#define GL_R1UI_V3F_SUN 0x85C4
#define GL_R1UI_C4UB_V3F_SUN 0x85C5
#define GL_R1UI_C3F_V3F_SUN 0x85C6
#define GL_R1UI_N3F_V3F_SUN 0x85C7
#define GL_R1UI_C4F_N3F_V3F_SUN 0x85C8
#define GL_R1UI_T2F_V3F_SUN 0x85C9
#define GL_R1UI_T2F_N3F_V3F_SUN 0x85CA
#define GL_R1UI_T2F_C4F_N3F_V3F_SUN 0x85CB
#endif
extern void (APIENTRYP gglReplacementCodeuiSUN)(GLuint code);
extern void (APIENTRYP gglReplacementCodeusSUN)(GLushort code);
extern void (APIENTRYP gglReplacementCodeubSUN)(GLubyte code);
extern void (APIENTRYP gglReplacementCodeuivSUN)(const GLuint *code);
extern void (APIENTRYP gglReplacementCodeusvSUN)(const GLushort *code);
extern void (APIENTRYP gglReplacementCodeubvSUN)(const GLubyte *code);
extern void (APIENTRYP gglReplacementCodePointerSUN)(GLenum type, GLsizei stride, const void **pointer);

#ifndef GL_SUN_vertex
#define GL_SUN_vertex
#endif
extern void (APIENTRYP gglColor4ubVertex2fSUN)(GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y);
extern void (APIENTRYP gglColor4ubVertex2fvSUN)(const GLubyte *c, const GLfloat *v);
extern void (APIENTRYP gglColor4ubVertex3fSUN)(GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglColor4ubVertex3fvSUN)(const GLubyte *c, const GLfloat *v);
extern void (APIENTRYP gglColor3fVertex3fSUN)(GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglColor3fVertex3fvSUN)(const GLfloat *c, const GLfloat *v);
extern void (APIENTRYP gglNormal3fVertex3fSUN)(GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglNormal3fVertex3fvSUN)(const GLfloat *n, const GLfloat *v);
extern void (APIENTRYP gglColor4fNormal3fVertex3fSUN)(GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglColor4fNormal3fVertex3fvSUN)(const GLfloat *c, const GLfloat *n, const GLfloat *v);
extern void (APIENTRYP gglTexCoord2fVertex3fSUN)(GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglTexCoord2fVertex3fvSUN)(const GLfloat *tc, const GLfloat *v);
extern void (APIENTRYP gglTexCoord4fVertex4fSUN)(GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglTexCoord4fVertex4fvSUN)(const GLfloat *tc, const GLfloat *v);
extern void (APIENTRYP gglTexCoord2fColor4ubVertex3fSUN)(GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglTexCoord2fColor4ubVertex3fvSUN)(const GLfloat *tc, const GLubyte *c, const GLfloat *v);
extern void (APIENTRYP gglTexCoord2fColor3fVertex3fSUN)(GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglTexCoord2fColor3fVertex3fvSUN)(const GLfloat *tc, const GLfloat *c, const GLfloat *v);
extern void (APIENTRYP gglTexCoord2fNormal3fVertex3fSUN)(GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglTexCoord2fNormal3fVertex3fvSUN)(const GLfloat *tc, const GLfloat *n, const GLfloat *v);
extern void (APIENTRYP gglTexCoord2fColor4fNormal3fVertex3fSUN)(GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglTexCoord2fColor4fNormal3fVertex3fvSUN)(const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
extern void (APIENTRYP gglTexCoord4fColor4fNormal3fVertex4fSUN)(GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglTexCoord4fColor4fNormal3fVertex4fvSUN)(const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
extern void (APIENTRYP gglReplacementCodeuiVertex3fSUN)(GLuint rc, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglReplacementCodeuiVertex3fvSUN)(const GLuint *rc, const GLfloat *v);
extern void (APIENTRYP gglReplacementCodeuiColor4ubVertex3fSUN)(GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglReplacementCodeuiColor4ubVertex3fvSUN)(const GLuint *rc, const GLubyte *c, const GLfloat *v);
extern void (APIENTRYP gglReplacementCodeuiColor3fVertex3fSUN)(GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglReplacementCodeuiColor3fVertex3fvSUN)(const GLuint *rc, const GLfloat *c, const GLfloat *v);
extern void (APIENTRYP gglReplacementCodeuiNormal3fVertex3fSUN)(GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglReplacementCodeuiNormal3fVertex3fvSUN)(const GLuint *rc, const GLfloat *n, const GLfloat *v);
extern void (APIENTRYP gglReplacementCodeuiColor4fNormal3fVertex3fSUN)(GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglReplacementCodeuiColor4fNormal3fVertex3fvSUN)(const GLuint *rc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
extern void (APIENTRYP gglReplacementCodeuiTexCoord2fVertex3fSUN)(GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglReplacementCodeuiTexCoord2fVertex3fvSUN)(const GLuint *rc, const GLfloat *tc, const GLfloat *v);
extern void (APIENTRYP gglReplacementCodeuiTexCoord2fNormal3fVertex3fSUN)(GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN)(const GLuint *rc, const GLfloat *tc, const GLfloat *n, const GLfloat *v);
extern void (APIENTRYP gglReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN)(GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN)(const GLuint *rc, const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);

#ifndef GL_WIN_phong_shading
#define GL_WIN_phong_shading
#define GL_PHONG_WIN 0x80EA
#define GL_PHONG_HINT_WIN 0x80EB
#endif

#ifndef GL_WIN_specular_fog
#define GL_WIN_specular_fog
#define GL_FOG_SPECULAR_TEXTURE_WIN 0x80EC
#endif

/* 552 extensions */
typedef struct {
	int _GL_3DFX_multisample : 1;
	int _GL_3DFX_tbuffer : 1;
	int _GL_3DFX_texture_compression_FXT1 : 1;
	int _GL_AMD_blend_minmax_factor : 1;
	int _GL_AMD_conservative_depth : 1;
	int _GL_AMD_debug_output : 1;
	int _GL_AMD_depth_clamp_separate : 1;
	int _GL_AMD_draw_buffers_blend : 1;
	int _GL_AMD_gcn_shader : 1;
	int _GL_AMD_gpu_shader_half_float : 1;
	int _GL_AMD_gpu_shader_int64 : 1;
	int _GL_AMD_interleaved_elements : 1;
	int _GL_AMD_multi_draw_indirect : 1;
	int _GL_AMD_name_gen_delete : 1;
	int _GL_AMD_occlusion_query_event : 1;
	int _GL_AMD_performance_monitor : 1;
	int _GL_AMD_pinned_memory : 1;
	int _GL_AMD_query_buffer_object : 1;
	int _GL_AMD_sample_positions : 1;
	int _GL_AMD_seamless_cubemap_per_texture : 1;
	int _GL_AMD_shader_atomic_counter_ops : 1;
	int _GL_AMD_shader_ballot : 1;
	int _GL_AMD_shader_stencil_export : 1;
	int _GL_AMD_shader_trinary_minmax : 1;
	int _GL_AMD_shader_explicit_vertex_parameter : 1;
	int _GL_AMD_sparse_texture : 1;
	int _GL_AMD_stencil_operation_extended : 1;
	int _GL_AMD_texture_texture4 : 1;
	int _GL_AMD_transform_feedback3_lines_triangles : 1;
	int _GL_AMD_transform_feedback4 : 1;
	int _GL_AMD_vertex_shader_layer : 1;
	int _GL_AMD_vertex_shader_tessellator : 1;
	int _GL_AMD_vertex_shader_viewport_index : 1;
	int _GL_APPLE_aux_depth_stencil : 1;
	int _GL_APPLE_client_storage : 1;
	int _GL_APPLE_element_array : 1;
	int _GL_APPLE_fence : 1;
	int _GL_APPLE_float_pixels : 1;
	int _GL_APPLE_flush_buffer_range : 1;
	int _GL_APPLE_object_purgeable : 1;
	int _GL_APPLE_rgb_422 : 1;
	int _GL_APPLE_row_bytes : 1;
	int _GL_APPLE_specular_vector : 1;
	int _GL_APPLE_texture_range : 1;
	int _GL_APPLE_transform_hint : 1;
	int _GL_APPLE_vertex_array_object : 1;
	int _GL_APPLE_vertex_array_range : 1;
	int _GL_APPLE_vertex_program_evaluators : 1;
	int _GL_APPLE_ycbcr_422 : 1;
	int _GL_ARB_ES2_compatibility : 1;
	int _GL_ARB_ES3_1_compatibility : 1;
	int _GL_ARB_ES3_2_compatibility : 1;
	int _GL_ARB_ES3_compatibility : 1;
	int _GL_ARB_arrays_of_arrays : 1;
	int _GL_ARB_base_instance : 1;
	int _GL_ARB_bindless_texture : 1;
	int _GL_ARB_blend_func_extended : 1;
	int _GL_ARB_buffer_storage : 1;
	int _GL_ARB_cl_event : 1;
	int _GL_ARB_clear_buffer_object : 1;
	int _GL_ARB_clear_texture : 1;
	int _GL_ARB_clip_control : 1;
	int _GL_ARB_color_buffer_float : 1;
	int _GL_ARB_compatibility : 1;
	int _GL_ARB_compressed_texture_pixel_storage : 1;
	int _GL_ARB_compute_shader : 1;
	int _GL_ARB_compute_variable_group_size : 1;
	int _GL_ARB_conditional_render_inverted : 1;
	int _GL_ARB_conservative_depth : 1;
	int _GL_ARB_copy_buffer : 1;
	int _GL_ARB_copy_image : 1;
	int _GL_ARB_cull_distance : 1;
	int _GL_ARB_debug_output : 1;
	int _GL_ARB_depth_buffer_float : 1;
	int _GL_ARB_depth_clamp : 1;
	int _GL_ARB_depth_texture : 1;
	int _GL_ARB_derivative_control : 1;
	int _GL_ARB_direct_state_access : 1;
	int _GL_ARB_draw_buffers : 1;
	int _GL_ARB_draw_buffers_blend : 1;
	int _GL_ARB_draw_elements_base_vertex : 1;
	int _GL_ARB_draw_indirect : 1;
	int _GL_ARB_draw_instanced : 1;
	int _GL_ARB_enhanced_layouts : 1;
	int _GL_ARB_explicit_attrib_location : 1;
	int _GL_ARB_explicit_uniform_location : 1;
	int _GL_ARB_fragment_coord_conventions : 1;
	int _GL_ARB_fragment_layer_viewport : 1;
	int _GL_ARB_fragment_program : 1;
	int _GL_ARB_fragment_program_shadow : 1;
	int _GL_ARB_fragment_shader : 1;
	int _GL_ARB_fragment_shader_interlock : 1;
	int _GL_ARB_framebuffer_no_attachments : 1;
	int _GL_ARB_framebuffer_object : 1;
	int _GL_ARB_framebuffer_sRGB : 1;
	int _GL_ARB_geometry_shader4 : 1;
	int _GL_ARB_get_program_binary : 1;
	int _GL_ARB_get_texture_sub_image : 1;
	int _GL_ARB_gpu_shader5 : 1;
	int _GL_ARB_gpu_shader_fp64 : 1;
	int _GL_ARB_gpu_shader_int64 : 1;
	int _GL_ARB_half_float_pixel : 1;
	int _GL_ARB_half_float_vertex : 1;
	int _GL_ARB_imaging : 1;
	int _GL_ARB_indirect_parameters : 1;
	int _GL_ARB_instanced_arrays : 1;
	int _GL_ARB_internalformat_query : 1;
	int _GL_ARB_internalformat_query2 : 1;
	int _GL_ARB_invalidate_subdata : 1;
	int _GL_ARB_map_buffer_alignment : 1;
	int _GL_ARB_map_buffer_range : 1;
	int _GL_ARB_matrix_palette : 1;
	int _GL_ARB_multi_bind : 1;
	int _GL_ARB_multi_draw_indirect : 1;
	int _GL_ARB_multisample : 1;
	int _GL_ARB_multitexture : 1;
	int _GL_ARB_occlusion_query : 1;
	int _GL_ARB_occlusion_query2 : 1;
	int _GL_ARB_parallel_shader_compile : 1;
	int _GL_ARB_pipeline_statistics_query : 1;
	int _GL_ARB_pixel_buffer_object : 1;
	int _GL_ARB_point_parameters : 1;
	int _GL_ARB_point_sprite : 1;
	int _GL_ARB_post_depth_coverage : 1;
	int _GL_ARB_program_interface_query : 1;
	int _GL_ARB_provoking_vertex : 1;
	int _GL_ARB_query_buffer_object : 1;
	int _GL_ARB_robust_buffer_access_behavior : 1;
	int _GL_ARB_robustness : 1;
	int _GL_ARB_robustness_isolation : 1;
	int _GL_ARB_sample_locations : 1;
	int _GL_ARB_sample_shading : 1;
	int _GL_ARB_sampler_objects : 1;
	int _GL_ARB_seamless_cube_map : 1;
	int _GL_ARB_seamless_cubemap_per_texture : 1;
	int _GL_ARB_separate_shader_objects : 1;
	int _GL_ARB_shader_atomic_counter_ops : 1;
	int _GL_ARB_shader_atomic_counters : 1;
	int _GL_ARB_shader_ballot : 1;
	int _GL_ARB_shader_bit_encoding : 1;
	int _GL_ARB_shader_clock : 1;
	int _GL_ARB_shader_draw_parameters : 1;
	int _GL_ARB_shader_group_vote : 1;
	int _GL_ARB_shader_image_load_store : 1;
	int _GL_ARB_shader_image_size : 1;
	int _GL_ARB_shader_objects : 1;
	int _GL_ARB_shader_precision : 1;
	int _GL_ARB_shader_stencil_export : 1;
	int _GL_ARB_shader_storage_buffer_object : 1;
	int _GL_ARB_shader_subroutine : 1;
	int _GL_ARB_shader_texture_image_samples : 1;
	int _GL_ARB_shader_texture_lod : 1;
	int _GL_ARB_shader_viewport_layer_array : 1;
	int _GL_ARB_shading_language_100 : 1;
	int _GL_ARB_shading_language_420pack : 1;
	int _GL_ARB_shading_language_include : 1;
	int _GL_ARB_shading_language_packing : 1;
	int _GL_ARB_shadow : 1;
	int _GL_ARB_shadow_ambient : 1;
	int _GL_ARB_sparse_buffer : 1;
	int _GL_ARB_sparse_texture : 1;
	int _GL_ARB_sparse_texture2 : 1;
	int _GL_ARB_sparse_texture_clamp : 1;
	int _GL_ARB_stencil_texturing : 1;
	int _GL_ARB_sync : 1;
	int _GL_ARB_tessellation_shader : 1;
	int _GL_ARB_texture_barrier : 1;
	int _GL_ARB_texture_border_clamp : 1;
	int _GL_ARB_texture_buffer_object : 1;
	int _GL_ARB_texture_buffer_object_rgb32 : 1;
	int _GL_ARB_texture_buffer_range : 1;
	int _GL_ARB_texture_compression : 1;
	int _GL_ARB_texture_compression_bptc : 1;
	int _GL_ARB_texture_compression_rgtc : 1;
	int _GL_ARB_texture_cube_map : 1;
	int _GL_ARB_texture_cube_map_array : 1;
	int _GL_ARB_texture_env_add : 1;
	int _GL_ARB_texture_env_combine : 1;
	int _GL_ARB_texture_env_crossbar : 1;
	int _GL_ARB_texture_env_dot3 : 1;
	int _GL_ARB_texture_filter_minmax : 1;
	int _GL_ARB_texture_float : 1;
	int _GL_ARB_texture_gather : 1;
	int _GL_ARB_texture_mirror_clamp_to_edge : 1;
	int _GL_ARB_texture_mirrored_repeat : 1;
	int _GL_ARB_texture_multisample : 1;
	int _GL_ARB_texture_non_power_of_two : 1;
	int _GL_ARB_texture_query_levels : 1;
	int _GL_ARB_texture_query_lod : 1;
	int _GL_ARB_texture_rectangle : 1;
	int _GL_ARB_texture_rg : 1;
	int _GL_ARB_texture_rgb10_a2ui : 1;
	int _GL_ARB_texture_stencil8 : 1;
	int _GL_ARB_texture_storage : 1;
	int _GL_ARB_texture_storage_multisample : 1;
	int _GL_ARB_texture_swizzle : 1;
	int _GL_ARB_texture_view : 1;
	int _GL_ARB_timer_query : 1;
	int _GL_ARB_transform_feedback2 : 1;
	int _GL_ARB_transform_feedback3 : 1;
	int _GL_ARB_transform_feedback_instanced : 1;
	int _GL_ARB_transform_feedback_overflow_query : 1;
	int _GL_ARB_transpose_matrix : 1;
	int _GL_ARB_uniform_buffer_object : 1;
	int _GL_ARB_vertex_array_bgra : 1;
	int _GL_ARB_vertex_array_object : 1;
	int _GL_ARB_vertex_attrib_64bit : 1;
	int _GL_ARB_vertex_attrib_binding : 1;
	int _GL_ARB_vertex_blend : 1;
	int _GL_ARB_vertex_buffer_object : 1;
	int _GL_ARB_vertex_program : 1;
	int _GL_ARB_vertex_shader : 1;
	int _GL_ARB_vertex_type_10f_11f_11f_rev : 1;
	int _GL_ARB_vertex_type_2_10_10_10_rev : 1;
	int _GL_ARB_viewport_array : 1;
	int _GL_ARB_window_pos : 1;
	int _GL_ATI_draw_buffers : 1;
	int _GL_ATI_element_array : 1;
	int _GL_ATI_envmap_bumpmap : 1;
	int _GL_ATI_fragment_shader : 1;
	int _GL_ATI_map_object_buffer : 1;
	int _GL_ATI_meminfo : 1;
	int _GL_ATI_pixel_format_float : 1;
	int _GL_ATI_pn_triangles : 1;
	int _GL_ATI_separate_stencil : 1;
	int _GL_ATI_text_fragment_shader : 1;
	int _GL_ATI_texture_env_combine3 : 1;
	int _GL_ATI_texture_float : 1;
	int _GL_ATI_texture_mirror_once : 1;
	int _GL_ATI_vertex_array_object : 1;
	int _GL_ATI_vertex_attrib_array_object : 1;
	int _GL_ATI_vertex_streams : 1;
	int _GL_EXT_422_pixels : 1;
	int _GL_EXT_abgr : 1;
	int _GL_EXT_bgra : 1;
	int _GL_EXT_bindable_uniform : 1;
	int _GL_EXT_blend_color : 1;
	int _GL_EXT_blend_equation_separate : 1;
	int _GL_EXT_blend_func_separate : 1;
	int _GL_EXT_blend_logic_op : 1;
	int _GL_EXT_blend_minmax : 1;
	int _GL_EXT_blend_subtract : 1;
	int _GL_EXT_clip_volume_hint : 1;
	int _GL_EXT_cmyka : 1;
	int _GL_EXT_color_subtable : 1;
	int _GL_EXT_compiled_vertex_array : 1;
	int _GL_EXT_convolution : 1;
	int _GL_EXT_coordinate_frame : 1;
	int _GL_EXT_copy_texture : 1;
	int _GL_EXT_cull_vertex : 1;
	int _GL_EXT_debug_label : 1;
	int _GL_EXT_debug_marker : 1;
	int _GL_EXT_depth_bounds_test : 1;
	int _GL_EXT_direct_state_access : 1;
	int _GL_EXT_draw_buffers2 : 1;
	int _GL_EXT_draw_instanced : 1;
	int _GL_EXT_draw_range_elements : 1;
	int _GL_EXT_fog_coord : 1;
	int _GL_EXT_framebuffer_blit : 1;
	int _GL_EXT_framebuffer_multisample : 1;
	int _GL_EXT_framebuffer_multisample_blit_scaled : 1;
	int _GL_EXT_framebuffer_object : 1;
	int _GL_EXT_framebuffer_sRGB : 1;
	int _GL_EXT_geometry_shader4 : 1;
	int _GL_EXT_gpu_program_parameters : 1;
	int _GL_EXT_gpu_shader4 : 1;
	int _GL_EXT_histogram : 1;
	int _GL_EXT_index_array_formats : 1;
	int _GL_EXT_index_func : 1;
	int _GL_EXT_index_material : 1;
	int _GL_EXT_index_texture : 1;
	int _GL_EXT_light_texture : 1;
	int _GL_EXT_misc_attribute : 1;
	int _GL_EXT_multi_draw_arrays : 1;
	int _GL_EXT_multisample : 1;
	int _GL_EXT_packed_depth_stencil : 1;
	int _GL_EXT_packed_float : 1;
	int _GL_EXT_packed_pixels : 1;
	int _GL_EXT_paletted_texture : 1;
	int _GL_EXT_pixel_buffer_object : 1;
	int _GL_EXT_pixel_transform : 1;
	int _GL_EXT_pixel_transform_color_table : 1;
	int _GL_EXT_point_parameters : 1;
	int _GL_EXT_polygon_offset : 1;
	int _GL_EXT_polygon_offset_clamp : 1;
	int _GL_EXT_post_depth_coverage : 1;
	int _GL_EXT_provoking_vertex : 1;
	int _GL_EXT_raster_multisample : 1;
	int _GL_EXT_rescale_normal : 1;
	int _GL_EXT_secondary_color : 1;
	int _GL_EXT_separate_shader_objects : 1;
	int _GL_EXT_separate_specular_color : 1;
	int _GL_EXT_shader_image_load_formatted : 1;
	int _GL_EXT_shader_image_load_store : 1;
	int _GL_EXT_shader_integer_mix : 1;
	int _GL_EXT_shadow_funcs : 1;
	int _GL_EXT_shared_texture_palette : 1;
	int _GL_EXT_sparse_texture2 : 1;
	int _GL_EXT_stencil_clear_tag : 1;
	int _GL_EXT_stencil_two_side : 1;
	int _GL_EXT_stencil_wrap : 1;
	int _GL_EXT_subtexture : 1;
	int _GL_EXT_texture : 1;
	int _GL_EXT_texture3D : 1;
	int _GL_EXT_texture_array : 1;
	int _GL_EXT_texture_buffer_object : 1;
	int _GL_EXT_texture_compression_latc : 1;
	int _GL_EXT_texture_compression_rgtc : 1;
	int _GL_EXT_texture_compression_s3tc : 1;
	int _GL_EXT_texture_cube_map : 1;
	int _GL_EXT_texture_env_add : 1;
	int _GL_EXT_texture_env_combine : 1;
	int _GL_EXT_texture_env_dot3 : 1;
	int _GL_EXT_texture_filter_anisotropic : 1;
	int _GL_EXT_texture_filter_minmax : 1;
	int _GL_EXT_texture_integer : 1;
	int _GL_EXT_texture_lod_bias : 1;
	int _GL_EXT_texture_mirror_clamp : 1;
	int _GL_EXT_texture_object : 1;
	int _GL_EXT_texture_perturb_normal : 1;
	int _GL_EXT_texture_sRGB : 1;
	int _GL_EXT_texture_sRGB_decode : 1;
	int _GL_EXT_texture_shared_exponent : 1;
	int _GL_EXT_texture_snorm : 1;
	int _GL_EXT_texture_swizzle : 1;
	int _GL_EXT_timer_query : 1;
	int _GL_EXT_transform_feedback : 1;
	int _GL_EXT_vertex_array : 1;
	int _GL_EXT_vertex_array_bgra : 1;
	int _GL_EXT_vertex_attrib_64bit : 1;
	int _GL_EXT_vertex_shader : 1;
	int _GL_EXT_vertex_weighting : 1;
	int _GL_EXT_window_rectangles : 1;
	int _GL_EXT_x11_sync_object : 1;
	int _GL_GREMEDY_frame_terminator : 1;
	int _GL_GREMEDY_string_marker : 1;
	int _GL_HP_convolution_border_modes : 1;
	int _GL_HP_image_transform : 1;
	int _GL_HP_occlusion_test : 1;
	int _GL_HP_texture_lighting : 1;
	int _GL_IBM_cull_vertex : 1;
	int _GL_IBM_multimode_draw_arrays : 1;
	int _GL_IBM_rasterpos_clip : 1;
	int _GL_IBM_static_data : 1;
	int _GL_IBM_texture_mirrored_repeat : 1;
	int _GL_IBM_vertex_array_lists : 1;
	int _GL_INGR_blend_func_separate : 1;
	int _GL_INGR_color_clamp : 1;
	int _GL_INGR_interlace_read : 1;
	int _GL_INTEL_conservative_rasterization : 1;
	int _GL_INTEL_fragment_shader_ordering : 1;
	int _GL_INTEL_framebuffer_CMAA : 1;
	int _GL_INTEL_map_texture : 1;
	int _GL_INTEL_parallel_arrays : 1;
	int _GL_INTEL_performance_query : 1;
	int _GL_KHR_blend_equation_advanced : 1;
	int _GL_KHR_blend_equation_advanced_coherent : 1;
	int _GL_KHR_context_flush_control : 1;
	int _GL_KHR_debug : 1;
	int _GL_KHR_no_error : 1;
	int _GL_KHR_robust_buffer_access_behavior : 1;
	int _GL_KHR_robustness : 1;
	int _GL_KHR_texture_compression_astc_hdr : 1;
	int _GL_KHR_texture_compression_astc_ldr : 1;
	int _GL_KHR_texture_compression_astc_sliced_3d : 1;
	int _GL_MESAX_texture_stack : 1;
	int _GL_MESA_pack_invert : 1;
	int _GL_MESA_resize_buffers : 1;
	int _GL_MESA_window_pos : 1;
	int _GL_MESA_ycbcr_texture : 1;
	int _GL_NVX_conditional_render : 1;
	int _GL_NVX_gpu_memory_info : 1;
	int _GL_NV_bindless_multi_draw_indirect : 1;
	int _GL_NV_bindless_multi_draw_indirect_count : 1;
	int _GL_NV_bindless_texture : 1;
	int _GL_NV_blend_equation_advanced : 1;
	int _GL_NV_blend_equation_advanced_coherent : 1;
	int _GL_NV_blend_square : 1;
	int _GL_NV_clip_space_w_scaling : 1;
	int _GL_NV_command_list : 1;
	int _GL_NV_compute_program5 : 1;
	int _GL_NV_conditional_render : 1;
	int _GL_NV_conservative_raster : 1;
	int _GL_NV_conservative_raster_dilate : 1;
	int _GL_NV_conservative_raster_pre_snap_triangles : 1;
	int _GL_NV_copy_depth_to_color : 1;
	int _GL_NV_copy_image : 1;
	int _GL_NV_deep_texture3D : 1;
	int _GL_NV_depth_buffer_float : 1;
	int _GL_NV_depth_clamp : 1;
	int _GL_NV_draw_texture : 1;
	int _GL_NV_evaluators : 1;
	int _GL_NV_explicit_multisample : 1;
	int _GL_NV_fence : 1;
	int _GL_NV_fill_rectangle : 1;
	int _GL_NV_float_buffer : 1;
	int _GL_NV_fog_distance : 1;
	int _GL_NV_fragment_coverage_to_color : 1;
	int _GL_NV_fragment_program : 1;
	int _GL_NV_fragment_program2 : 1;
	int _GL_NV_fragment_program4 : 1;
	int _GL_NV_fragment_program_option : 1;
	int _GL_NV_fragment_shader_interlock : 1;
	int _GL_NV_framebuffer_mixed_samples : 1;
	int _GL_NV_framebuffer_multisample_coverage : 1;
	int _GL_NV_geometry_program4 : 1;
	int _GL_NV_geometry_shader4 : 1;
	int _GL_NV_geometry_shader_passthrough : 1;
	int _GL_NV_gpu_program4 : 1;
	int _GL_NV_gpu_program5 : 1;
	int _GL_NV_gpu_program5_mem_extended : 1;
	int _GL_NV_gpu_shader5 : 1;
	int _GL_NV_half_float : 1;
	int _GL_NV_internalformat_sample_query : 1;
	int _GL_NV_light_max_exponent : 1;
	int _GL_NV_multisample_coverage : 1;
	int _GL_NV_multisample_filter_hint : 1;
	int _GL_NV_occlusion_query : 1;
	int _GL_NV_packed_depth_stencil : 1;
	int _GL_NV_parameter_buffer_object : 1;
	int _GL_NV_parameter_buffer_object2 : 1;
	int _GL_NV_path_rendering : 1;
	int _GL_NV_path_rendering_shared_edge : 1;
	int _GL_NV_pixel_data_range : 1;
	int _GL_NV_point_sprite : 1;
	int _GL_NV_present_video : 1;
	int _GL_NV_primitive_restart : 1;
	int _GL_NV_register_combiners : 1;
	int _GL_NV_register_combiners2 : 1;
	int _GL_NV_robustness_video_memory_purge : 1;
	int _GL_NV_sample_locations : 1;
	int _GL_NV_sample_mask_override_coverage : 1;
	int _GL_NV_shader_atomic_counters : 1;
	int _GL_NV_shader_atomic_float : 1;
	int _GL_NV_shader_atomic_float64 : 1;
	int _GL_NV_shader_atomic_fp16_vector : 1;
	int _GL_NV_shader_atomic_int64 : 1;
	int _GL_NV_shader_buffer_load : 1;
	int _GL_NV_shader_buffer_store : 1;
	int _GL_NV_shader_storage_buffer_object : 1;
	int _GL_NV_shader_thread_group : 1;
	int _GL_NV_shader_thread_shuffle : 1;
	int _GL_NV_stereo_view_rendering : 1;
	int _GL_NV_tessellation_program5 : 1;
	int _GL_NV_texgen_emboss : 1;
	int _GL_NV_texgen_reflection : 1;
	int _GL_NV_texture_barrier : 1;
	int _GL_NV_texture_compression_vtc : 1;
	int _GL_NV_texture_env_combine4 : 1;
	int _GL_NV_texture_expand_normal : 1;
	int _GL_NV_texture_multisample : 1;
	int _GL_NV_texture_rectangle : 1;
	int _GL_NV_texture_shader : 1;
	int _GL_NV_texture_shader2 : 1;
	int _GL_NV_texture_shader3 : 1;
	int _GL_NV_transform_feedback : 1;
	int _GL_NV_transform_feedback2 : 1;
	int _GL_NV_uniform_buffer_unified_memory : 1;
	int _GL_NV_vdpau_interop : 1;
	int _GL_NV_vertex_array_range : 1;
	int _GL_NV_vertex_array_range2 : 1;
	int _GL_NV_vertex_attrib_integer_64bit : 1;
	int _GL_NV_vertex_buffer_unified_memory : 1;
	int _GL_NV_vertex_program : 1;
	int _GL_NV_vertex_program1_1 : 1;
	int _GL_NV_vertex_program2 : 1;
	int _GL_NV_vertex_program2_option : 1;
	int _GL_NV_vertex_program3 : 1;
	int _GL_NV_vertex_program4 : 1;
	int _GL_NV_video_capture : 1;
	int _GL_NV_viewport_array2 : 1;
	int _GL_NV_viewport_swizzle : 1;
	int _GL_OES_byte_coordinates : 1;
	int _GL_OES_compressed_paletted_texture : 1;
	int _GL_OES_fixed_point : 1;
	int _GL_OES_query_matrix : 1;
	int _GL_OES_read_format : 1;
	int _GL_OES_single_precision : 1;
	int _GL_OML_interlace : 1;
	int _GL_OML_resample : 1;
	int _GL_OML_subsample : 1;
	int _GL_OVR_multiview : 1;
	int _GL_OVR_multiview2 : 1;
	int _GL_PGI_misc_hints : 1;
	int _GL_PGI_vertex_hints : 1;
	int _GL_REND_screen_coordinates : 1;
	int _GL_S3_s3tc : 1;
	int _GL_SGIS_detail_texture : 1;
	int _GL_SGIS_fog_function : 1;
	int _GL_SGIS_generate_mipmap : 1;
	int _GL_SGIS_multisample : 1;
	int _GL_SGIS_pixel_texture : 1;
	int _GL_SGIS_point_line_texgen : 1;
	int _GL_SGIS_point_parameters : 1;
	int _GL_SGIS_sharpen_texture : 1;
	int _GL_SGIS_texture4D : 1;
	int _GL_SGIS_texture_border_clamp : 1;
	int _GL_SGIS_texture_color_mask : 1;
	int _GL_SGIS_texture_edge_clamp : 1;
	int _GL_SGIS_texture_filter4 : 1;
	int _GL_SGIS_texture_lod : 1;
	int _GL_SGIS_texture_select : 1;
	int _GL_SGIX_async : 1;
	int _GL_SGIX_async_histogram : 1;
	int _GL_SGIX_async_pixel : 1;
	int _GL_SGIX_blend_alpha_minmax : 1;
	int _GL_SGIX_calligraphic_fragment : 1;
	int _GL_SGIX_clipmap : 1;
	int _GL_SGIX_convolution_accuracy : 1;
	int _GL_SGIX_depth_pass_instrument : 1;
	int _GL_SGIX_depth_texture : 1;
	int _GL_SGIX_flush_raster : 1;
	int _GL_SGIX_fog_offset : 1;
	int _GL_SGIX_fragment_lighting : 1;
	int _GL_SGIX_framezoom : 1;
	int _GL_SGIX_igloo_interface : 1;
	int _GL_SGIX_instruments : 1;
	int _GL_SGIX_interlace : 1;
	int _GL_SGIX_ir_instrument1 : 1;
	int _GL_SGIX_list_priority : 1;
	int _GL_SGIX_pixel_texture : 1;
	int _GL_SGIX_pixel_tiles : 1;
	int _GL_SGIX_polynomial_ffd : 1;
	int _GL_SGIX_reference_plane : 1;
	int _GL_SGIX_resample : 1;
	int _GL_SGIX_scalebias_hint : 1;
	int _GL_SGIX_shadow : 1;
	int _GL_SGIX_shadow_ambient : 1;
	int _GL_SGIX_sprite : 1;
	int _GL_SGIX_subsample : 1;
	int _GL_SGIX_tag_sample_buffer : 1;
	int _GL_SGIX_texture_add_env : 1;
	int _GL_SGIX_texture_coordinate_clamp : 1;
	int _GL_SGIX_texture_lod_bias : 1;
	int _GL_SGIX_texture_multi_buffer : 1;
	int _GL_SGIX_texture_scale_bias : 1;
	int _GL_SGIX_vertex_preclip : 1;
	int _GL_SGIX_ycrcb : 1;
	int _GL_SGIX_ycrcb_subsample : 1;
	int _GL_SGIX_ycrcba : 1;
	int _GL_SGI_color_matrix : 1;
	int _GL_SGI_color_table : 1;
	int _GL_SGI_texture_color_table : 1;
	int _GL_SUNX_constant_data : 1;
	int _GL_SUN_convolution_border_modes : 1;
	int _GL_SUN_global_alpha : 1;
	int _GL_SUN_mesh_array : 1;
	int _GL_SUN_slice_accum : 1;
	int _GL_SUN_triangle_list : 1;
	int _GL_SUN_vertex : 1;
	int _GL_WIN_phong_shading : 1;
	int _GL_WIN_specular_fog : 1;
} gglext_t;

extern gglext_t gglext;

extern void ggl_init(int enableDebug);
extern void ggl_rebind(int enableDebug);

#ifdef __cplusplus
}
#endif

#endif /* __GGLCORE32_H__ */
