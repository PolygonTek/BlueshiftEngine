/*********************************************************************************************
 *
 * ggles3.h
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

#ifndef __GGLES3_H__
#define __GGLES3_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#include "GLES3/gl3.h"
#elif defined(__APPLE__)
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#include "KHR/khrplatform.h"
#elif defined(ANDROID)
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES3/gl3platform.h>
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
typedef khronos_int8_t GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLclampx;
typedef khronos_uint8_t GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef khronos_float_t GLfloat;
typedef khronos_float_t GLclampf;
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
typedef khronos_int32_t GLfixed;
typedef khronos_intptr_t GLintptr;
typedef khronos_ssize_t GLsizeiptr;
typedef khronos_int64_t GLint64;
typedef khronos_uint64_t GLuint64;
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
typedef khronos_int64_t GLint64EXT;
typedef khronos_uint64_t GLuint64EXT;
typedef struct __GLsync *GLsync;
struct _cl_context;
struct _cl_event;
typedef void (APIENTRY *GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef void (APIENTRY *GLDEBUGPROCARB)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef void (APIENTRY *GLDEBUGPROCKHR)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
typedef void (APIENTRY *GLDEBUGPROCAMD)(GLuint id,GLenum category,GLenum severity,GLsizei length,const GLchar *message,void *userParam);
typedef unsigned short GLhalfNV;
typedef GLintptr GLvdpauSurfaceNV;

#ifndef GL_ES_VERSION_2_0
#define GL_ES_VERSION_2_0
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
#define GL_FUNC_ADD 0x8006
#define GL_BLEND_EQUATION 0x8009
#define GL_BLEND_EQUATION_RGB 0x8009
#define GL_BLEND_EQUATION_ALPHA 0x883D
#define GL_FUNC_SUBTRACT 0x800A
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#define GL_BLEND_DST_RGB 0x80C8
#define GL_BLEND_SRC_RGB 0x80C9
#define GL_BLEND_DST_ALPHA 0x80CA
#define GL_BLEND_SRC_ALPHA 0x80CB
#define GL_CONSTANT_COLOR 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR 0x8002
#define GL_CONSTANT_ALPHA 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x8004
#define GL_BLEND_COLOR 0x8005
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_ARRAY_BUFFER_BINDING 0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#define GL_STREAM_DRAW 0x88E0
#define GL_STATIC_DRAW 0x88E4
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_BUFFER_SIZE 0x8764
#define GL_BUFFER_USAGE 0x8765
#define GL_CURRENT_VERTEX_ATTRIB 0x8626
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_FRONT_AND_BACK 0x0408
#define GL_TEXTURE_2D 0x0DE1
#define GL_CULL_FACE 0x0B44
#define GL_BLEND 0x0BE2
#define GL_DITHER 0x0BD0
#define GL_STENCIL_TEST 0x0B90
#define GL_DEPTH_TEST 0x0B71
#define GL_SCISSOR_TEST 0x0C11
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_SAMPLE_ALPHA_TO_COVERAGE 0x809E
#define GL_SAMPLE_COVERAGE 0x80A0
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_OUT_OF_MEMORY 0x0505
#define GL_CW 0x0900
#define GL_CCW 0x0901
#define GL_LINE_WIDTH 0x0B21
#define GL_ALIASED_POINT_SIZE_RANGE 0x846D
#define GL_ALIASED_LINE_WIDTH_RANGE 0x846E
#define GL_CULL_FACE_MODE 0x0B45
#define GL_FRONT_FACE 0x0B46
#define GL_DEPTH_RANGE 0x0B70
#define GL_DEPTH_WRITEMASK 0x0B72
#define GL_DEPTH_CLEAR_VALUE 0x0B73
#define GL_DEPTH_FUNC 0x0B74
#define GL_STENCIL_CLEAR_VALUE 0x0B91
#define GL_STENCIL_FUNC 0x0B92
#define GL_STENCIL_FAIL 0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL 0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS 0x0B96
#define GL_STENCIL_REF 0x0B97
#define GL_STENCIL_VALUE_MASK 0x0B93
#define GL_STENCIL_WRITEMASK 0x0B98
#define GL_STENCIL_BACK_FUNC 0x8800
#define GL_STENCIL_BACK_FAIL 0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL 0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS 0x8803
#define GL_STENCIL_BACK_REF 0x8CA3
#define GL_STENCIL_BACK_VALUE_MASK 0x8CA4
#define GL_STENCIL_BACK_WRITEMASK 0x8CA5
#define GL_VIEWPORT 0x0BA2
#define GL_SCISSOR_BOX 0x0C10
#define GL_COLOR_CLEAR_VALUE 0x0C22
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_MAX_VIEWPORT_DIMS 0x0D3A
#define GL_SUBPIXEL_BITS 0x0D50
#define GL_RED_BITS 0x0D52
#define GL_GREEN_BITS 0x0D53
#define GL_BLUE_BITS 0x0D54
#define GL_ALPHA_BITS 0x0D55
#define GL_DEPTH_BITS 0x0D56
#define GL_STENCIL_BITS 0x0D57
#define GL_POLYGON_OFFSET_UNITS 0x2A00
#define GL_POLYGON_OFFSET_FACTOR 0x8038
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_SAMPLE_BUFFERS 0x80A8
#define GL_SAMPLES 0x80A9
#define GL_SAMPLE_COVERAGE_VALUE 0x80AA
#define GL_SAMPLE_COVERAGE_INVERT 0x80AB
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS 0x86A3
#define GL_DONT_CARE 0x1100
#define GL_FASTEST 0x1101
#define GL_NICEST 0x1102
#define GL_GENERATE_MIPMAP_HINT 0x8192
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_SHORT 0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_FIXED 0x140C
#define GL_DEPTH_COMPONENT 0x1902
#define GL_ALPHA 0x1906
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_LUMINANCE 0x1909
#define GL_LUMINANCE_ALPHA 0x190A
#define GL_UNSIGNED_SHORT_4_4_4_4 0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1 0x8034
#define GL_UNSIGNED_SHORT_5_6_5 0x8363
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_MAX_VERTEX_ATTRIBS 0x8869
#define GL_MAX_VERTEX_UNIFORM_VECTORS 0x8DFB
#define GL_MAX_VARYING_VECTORS 0x8DFC
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
#define GL_MAX_TEXTURE_IMAGE_UNITS 0x8872
#define GL_MAX_FRAGMENT_UNIFORM_VECTORS 0x8DFD
#define GL_SHADER_TYPE 0x8B4F
#define GL_DELETE_STATUS 0x8B80
#define GL_LINK_STATUS 0x8B82
#define GL_VALIDATE_STATUS 0x8B83
#define GL_ATTACHED_SHADERS 0x8B85
#define GL_ACTIVE_UNIFORMS 0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH 0x8B87
#define GL_ACTIVE_ATTRIBUTES 0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH 0x8B8A
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_NEVER 0x0200
#define GL_LESS 0x0201
#define GL_EQUAL 0x0202
#define GL_LEQUAL 0x0203
#define GL_GREATER 0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL 0x0206
#define GL_ALWAYS 0x0207
#define GL_KEEP 0x1E00
#define GL_REPLACE 0x1E01
#define GL_INCR 0x1E02
#define GL_DECR 0x1E03
#define GL_INVERT 0x150A
#define GL_INCR_WRAP 0x8507
#define GL_DECR_WRAP 0x8508
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
#define GL_TEXTURE 0x1702
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP 0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE 0x851C
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
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_MIRRORED_REPEAT 0x8370
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
#define GL_SAMPLER_2D 0x8B5E
#define GL_SAMPLER_CUBE 0x8B60
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE 0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE 0x8625
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED 0x886A
#define GL_VERTEX_ATTRIB_ARRAY_POINTER 0x8645
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING 0x889F
#define GL_IMPLEMENTATION_COLOR_READ_TYPE 0x8B9A
#define GL_IMPLEMENTATION_COLOR_READ_FORMAT 0x8B9B
#define GL_COMPILE_STATUS 0x8B81
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_SHADER_SOURCE_LENGTH 0x8B88
#define GL_SHADER_COMPILER 0x8DFA
#define GL_SHADER_BINARY_FORMATS 0x8DF8
#define GL_NUM_SHADER_BINARY_FORMATS 0x8DF9
#define GL_LOW_FLOAT 0x8DF0
#define GL_MEDIUM_FLOAT 0x8DF1
#define GL_HIGH_FLOAT 0x8DF2
#define GL_LOW_INT 0x8DF3
#define GL_MEDIUM_INT 0x8DF4
#define GL_HIGH_INT 0x8DF5
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_RGBA4 0x8056
#define GL_RGB5_A1 0x8057
#define GL_RGB565 0x8D62
#define GL_DEPTH_COMPONENT16 0x81A5
#define GL_STENCIL_INDEX8 0x8D48
#define GL_RENDERBUFFER_WIDTH 0x8D42
#define GL_RENDERBUFFER_HEIGHT 0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT 0x8D44
#define GL_RENDERBUFFER_RED_SIZE 0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE 0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE 0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE 0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE 0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE 0x8D55
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE 0x8CD3
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_STENCIL_ATTACHMENT 0x8D20
#define GL_NONE 0
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 0x8CD9
#define GL_FRAMEBUFFER_UNSUPPORTED 0x8CDD
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#define GL_RENDERBUFFER_BINDING 0x8CA7
#define GL_MAX_RENDERBUFFER_SIZE 0x84E8
#define GL_INVALID_FRAMEBUFFER_OPERATION 0x0506
#endif
extern void (APIENTRYP gglActiveTexture)(GLenum texture);
extern void (APIENTRYP gglAttachShader)(GLuint program, GLuint shader);
extern void (APIENTRYP gglBindAttribLocation)(GLuint program, GLuint index, const GLchar *name);
extern void (APIENTRYP gglBindBuffer)(GLenum target, GLuint buffer);
extern void (APIENTRYP gglBindFramebuffer)(GLenum target, GLuint framebuffer);
extern void (APIENTRYP gglBindRenderbuffer)(GLenum target, GLuint renderbuffer);
extern void (APIENTRYP gglBindTexture)(GLenum target, GLuint texture);
extern void (APIENTRYP gglBlendColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void (APIENTRYP gglBlendEquation)(GLenum mode);
extern void (APIENTRYP gglBlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
extern void (APIENTRYP gglBlendFunc)(GLenum sfactor, GLenum dfactor);
extern void (APIENTRYP gglBlendFuncSeparate)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
extern void (APIENTRYP gglBufferData)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
extern void (APIENTRYP gglBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
extern GLenum (APIENTRYP gglCheckFramebufferStatus)(GLenum target);
extern void (APIENTRYP gglClear)(GLbitfield mask);
extern void (APIENTRYP gglClearColor)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void (APIENTRYP gglClearDepthf)(GLfloat d);
extern void (APIENTRYP gglClearStencil)(GLint s);
extern void (APIENTRYP gglColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern void (APIENTRYP gglCompileShader)(GLuint shader);
extern void (APIENTRYP gglCompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern void (APIENTRYP gglCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern GLuint (APIENTRYP gglCreateProgram)();
extern GLuint (APIENTRYP gglCreateShader)(GLenum type);
extern void (APIENTRYP gglCullFace)(GLenum mode);
extern void (APIENTRYP gglDeleteBuffers)(GLsizei n, const GLuint *buffers);
extern void (APIENTRYP gglDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
extern void (APIENTRYP gglDeleteProgram)(GLuint program);
extern void (APIENTRYP gglDeleteRenderbuffers)(GLsizei n, const GLuint *renderbuffers);
extern void (APIENTRYP gglDeleteShader)(GLuint shader);
extern void (APIENTRYP gglDeleteTextures)(GLsizei n, const GLuint *textures);
extern void (APIENTRYP gglDepthFunc)(GLenum func);
extern void (APIENTRYP gglDepthMask)(GLboolean flag);
extern void (APIENTRYP gglDepthRangef)(GLfloat n, GLfloat f);
extern void (APIENTRYP gglDetachShader)(GLuint program, GLuint shader);
extern void (APIENTRYP gglDisable)(GLenum cap);
extern void (APIENTRYP gglDisableVertexAttribArray)(GLuint index);
extern void (APIENTRYP gglDrawArrays)(GLenum mode, GLint first, GLsizei count);
extern void (APIENTRYP gglDrawElements)(GLenum mode, GLsizei count, GLenum type, const void *indices);
extern void (APIENTRYP gglEnable)(GLenum cap);
extern void (APIENTRYP gglEnableVertexAttribArray)(GLuint index);
extern void (APIENTRYP gglFinish)();
extern void (APIENTRYP gglFlush)();
extern void (APIENTRYP gglFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
extern void (APIENTRYP gglFramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern void (APIENTRYP gglFrontFace)(GLenum mode);
extern void (APIENTRYP gglGenBuffers)(GLsizei n, GLuint *buffers);
extern void (APIENTRYP gglGenerateMipmap)(GLenum target);
extern void (APIENTRYP gglGenFramebuffers)(GLsizei n, GLuint *framebuffers);
extern void (APIENTRYP gglGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
extern void (APIENTRYP gglGenTextures)(GLsizei n, GLuint *textures);
extern void (APIENTRYP gglGetActiveAttrib)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
extern void (APIENTRYP gglGetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
extern void (APIENTRYP gglGetAttachedShaders)(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders);
extern GLint (APIENTRYP gglGetAttribLocation)(GLuint program, const GLchar *name);
extern void (APIENTRYP gglGetBooleanv)(GLenum pname, GLboolean *data);
extern void (APIENTRYP gglGetBufferParameteriv)(GLenum target, GLenum pname, GLint *params);
extern GLenum (APIENTRYP gglGetError)();
extern void (APIENTRYP gglGetFloatv)(GLenum pname, GLfloat *data);
extern void (APIENTRYP gglGetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetIntegerv)(GLenum pname, GLint *data);
extern void (APIENTRYP gglGetProgramiv)(GLuint program, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
extern void (APIENTRYP gglGetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
extern void (APIENTRYP gglGetShaderPrecisionFormat)(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision);
extern void (APIENTRYP gglGetShaderSource)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);
extern const GLubyte * (APIENTRYP gglGetString)(GLenum name);
extern void (APIENTRYP gglGetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetUniformfv)(GLuint program, GLint location, GLfloat *params);
extern void (APIENTRYP gglGetUniformiv)(GLuint program, GLint location, GLint *params);
extern GLint (APIENTRYP gglGetUniformLocation)(GLuint program, const GLchar *name);
extern void (APIENTRYP gglGetVertexAttribfv)(GLuint index, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglGetVertexAttribiv)(GLuint index, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetVertexAttribPointerv)(GLuint index, GLenum pname, void **pointer);
extern void (APIENTRYP gglHint)(GLenum target, GLenum mode);
extern GLboolean (APIENTRYP gglIsBuffer)(GLuint buffer);
extern GLboolean (APIENTRYP gglIsEnabled)(GLenum cap);
extern GLboolean (APIENTRYP gglIsFramebuffer)(GLuint framebuffer);
extern GLboolean (APIENTRYP gglIsProgram)(GLuint program);
extern GLboolean (APIENTRYP gglIsRenderbuffer)(GLuint renderbuffer);
extern GLboolean (APIENTRYP gglIsShader)(GLuint shader);
extern GLboolean (APIENTRYP gglIsTexture)(GLuint texture);
extern void (APIENTRYP gglLineWidth)(GLfloat width);
extern void (APIENTRYP gglLinkProgram)(GLuint program);
extern void (APIENTRYP gglPixelStorei)(GLenum pname, GLint param);
extern void (APIENTRYP gglPolygonOffset)(GLfloat factor, GLfloat units);
extern void (APIENTRYP gglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
extern void (APIENTRYP gglReleaseShaderCompiler)();
extern void (APIENTRYP gglRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglSampleCoverage)(GLfloat value, GLboolean invert);
extern void (APIENTRYP gglScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglShaderBinary)(GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length);
extern void (APIENTRYP gglShaderSource)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
extern void (APIENTRYP gglStencilFunc)(GLenum func, GLint ref, GLuint mask);
extern void (APIENTRYP gglStencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
extern void (APIENTRYP gglStencilMask)(GLuint mask);
extern void (APIENTRYP gglStencilMaskSeparate)(GLenum face, GLuint mask);
extern void (APIENTRYP gglStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
extern void (APIENTRYP gglStencilOpSeparate)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
extern void (APIENTRYP gglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTexParameterf)(GLenum target, GLenum pname, GLfloat param);
extern void (APIENTRYP gglTexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
extern void (APIENTRYP gglTexParameteri)(GLenum target, GLenum pname, GLint param);
extern void (APIENTRYP gglTexParameteriv)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglUniform1f)(GLint location, GLfloat v0);
extern void (APIENTRYP gglUniform1fv)(GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglUniform1i)(GLint location, GLint v0);
extern void (APIENTRYP gglUniform1iv)(GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglUniform2f)(GLint location, GLfloat v0, GLfloat v1);
extern void (APIENTRYP gglUniform2fv)(GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglUniform2i)(GLint location, GLint v0, GLint v1);
extern void (APIENTRYP gglUniform2iv)(GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern void (APIENTRYP gglUniform3fv)(GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
extern void (APIENTRYP gglUniform3iv)(GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void (APIENTRYP gglUniform4fv)(GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
extern void (APIENTRYP gglUniform4iv)(GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglUniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUseProgram)(GLuint program);
extern void (APIENTRYP gglValidateProgram)(GLuint program);
extern void (APIENTRYP gglVertexAttrib1f)(GLuint index, GLfloat x);
extern void (APIENTRYP gglVertexAttrib1fv)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib2f)(GLuint index, GLfloat x, GLfloat y);
extern void (APIENTRYP gglVertexAttrib2fv)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib3f)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
extern void (APIENTRYP gglVertexAttrib3fv)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttrib4f)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void (APIENTRYP gglVertexAttrib4fv)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
extern void (APIENTRYP gglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

#ifndef GL_ES_VERSION_3_0
#define GL_ES_VERSION_3_0
#define GL_READ_BUFFER 0x0C02
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#define GL_PACK_ROW_LENGTH 0x0D02
#define GL_PACK_SKIP_ROWS 0x0D03
#define GL_PACK_SKIP_PIXELS 0x0D04
#define GL_COLOR 0x1800
#define GL_DEPTH 0x1801
#define GL_STENCIL 0x1802
#define GL_RED 0x1903
#define GL_RGB8 0x8051
#define GL_RGBA8 0x8058
#define GL_RGB10_A2 0x8059
#define GL_TEXTURE_BINDING_3D 0x806A
#define GL_UNPACK_SKIP_IMAGES 0x806D
#define GL_UNPACK_IMAGE_HEIGHT 0x806E
#define GL_TEXTURE_3D 0x806F
#define GL_TEXTURE_WRAP_R 0x8072
#define GL_MAX_3D_TEXTURE_SIZE 0x8073
#define GL_UNSIGNED_INT_2_10_10_10_REV 0x8368
#define GL_MAX_ELEMENTS_VERTICES 0x80E8
#define GL_MAX_ELEMENTS_INDICES 0x80E9
#define GL_TEXTURE_MIN_LOD 0x813A
#define GL_TEXTURE_MAX_LOD 0x813B
#define GL_TEXTURE_BASE_LEVEL 0x813C
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_MIN 0x8007
#define GL_MAX 0x8008
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_MAX_TEXTURE_LOD_BIAS 0x84FD
#define GL_TEXTURE_COMPARE_MODE 0x884C
#define GL_TEXTURE_COMPARE_FUNC 0x884D
#define GL_CURRENT_QUERY 0x8865
#define GL_QUERY_RESULT 0x8866
#define GL_QUERY_RESULT_AVAILABLE 0x8867
#define GL_BUFFER_MAPPED 0x88BC
#define GL_BUFFER_MAP_POINTER 0x88BD
#define GL_STREAM_READ 0x88E1
#define GL_STREAM_COPY 0x88E2
#define GL_STATIC_READ 0x88E5
#define GL_STATIC_COPY 0x88E6
#define GL_DYNAMIC_READ 0x88E9
#define GL_DYNAMIC_COPY 0x88EA
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
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS 0x8B4A
#define GL_SAMPLER_3D 0x8B5F
#define GL_SAMPLER_2D_SHADOW 0x8B62
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT 0x8B8B
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
#define GL_SRGB8_ALPHA8 0x8C43
#define GL_COMPARE_REF_TO_TEXTURE 0x884E
#define GL_MAJOR_VERSION 0x821B
#define GL_MINOR_VERSION 0x821C
#define GL_NUM_EXTENSIONS 0x821D
#define GL_RGBA32F 0x8814
#define GL_RGB32F 0x8815
#define GL_RGBA16F 0x881A
#define GL_RGB16F 0x881B
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER 0x88FD
#define GL_MAX_ARRAY_TEXTURE_LAYERS 0x88FF
#define GL_MIN_PROGRAM_TEXEL_OFFSET 0x8904
#define GL_MAX_PROGRAM_TEXEL_OFFSET 0x8905
#define GL_MAX_VARYING_COMPONENTS 0x8B4B
#define GL_TEXTURE_2D_ARRAY 0x8C1A
#define GL_TEXTURE_BINDING_2D_ARRAY 0x8C1D
#define GL_R11F_G11F_B10F 0x8C3A
#define GL_UNSIGNED_INT_10F_11F_11F_REV 0x8C3B
#define GL_RGB9_E5 0x8C3D
#define GL_UNSIGNED_INT_5_9_9_9_REV 0x8C3E
#define GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH 0x8C76
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE 0x8C7F
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS 0x8C80
#define GL_TRANSFORM_FEEDBACK_VARYINGS 0x8C83
#define GL_TRANSFORM_FEEDBACK_BUFFER_START 0x8C84
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE 0x8C85
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
#define GL_RGB_INTEGER 0x8D98
#define GL_RGBA_INTEGER 0x8D99
#define GL_SAMPLER_2D_ARRAY 0x8DC1
#define GL_SAMPLER_2D_ARRAY_SHADOW 0x8DC4
#define GL_SAMPLER_CUBE_SHADOW 0x8DC5
#define GL_UNSIGNED_INT_VEC2 0x8DC6
#define GL_UNSIGNED_INT_VEC3 0x8DC7
#define GL_UNSIGNED_INT_VEC4 0x8DC8
#define GL_INT_SAMPLER_2D 0x8DCA
#define GL_INT_SAMPLER_3D 0x8DCB
#define GL_INT_SAMPLER_CUBE 0x8DCC
#define GL_INT_SAMPLER_2D_ARRAY 0x8DCF
#define GL_UNSIGNED_INT_SAMPLER_2D 0x8DD2
#define GL_UNSIGNED_INT_SAMPLER_3D 0x8DD3
#define GL_UNSIGNED_INT_SAMPLER_CUBE 0x8DD4
#define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY 0x8DD7
#define GL_BUFFER_ACCESS_FLAGS 0x911F
#define GL_BUFFER_MAP_LENGTH 0x9120
#define GL_BUFFER_MAP_OFFSET 0x9121
#define GL_DEPTH_COMPONENT32F 0x8CAC
#define GL_DEPTH32F_STENCIL8 0x8CAD
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD
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
#define GL_DEPTH_STENCIL 0x84F9
#define GL_UNSIGNED_INT_24_8 0x84FA
#define GL_DEPTH24_STENCIL8 0x88F0
#define GL_UNSIGNED_NORMALIZED 0x8C17
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#define GL_RENDERBUFFER_SAMPLES 0x8CAB
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER 0x8CD4
#define GL_MAX_COLOR_ATTACHMENTS 0x8CDF
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
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56
#define GL_MAX_SAMPLES 0x8D57
#define GL_HALF_FLOAT 0x140B
#define GL_MAP_READ_BIT 0x0001
#define GL_MAP_WRITE_BIT 0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT 0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT 0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT 0x0020
#define GL_RG 0x8227
#define GL_RG_INTEGER 0x8228
#define GL_R8 0x8229
#define GL_RG8 0x822B
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
#define GL_R8_SNORM 0x8F94
#define GL_RG8_SNORM 0x8F95
#define GL_RGB8_SNORM 0x8F96
#define GL_RGBA8_SNORM 0x8F97
#define GL_SIGNED_NORMALIZED 0x8F9C
#define GL_PRIMITIVE_RESTART_FIXED_INDEX 0x8D69
#define GL_COPY_READ_BUFFER 0x8F36
#define GL_COPY_WRITE_BUFFER 0x8F37
#define GL_COPY_READ_BUFFER_BINDING 0x8F36
#define GL_COPY_WRITE_BUFFER_BINDING 0x8F37
#define GL_UNIFORM_BUFFER 0x8A11
#define GL_UNIFORM_BUFFER_BINDING 0x8A28
#define GL_UNIFORM_BUFFER_START 0x8A29
#define GL_UNIFORM_BUFFER_SIZE 0x8A2A
#define GL_MAX_VERTEX_UNIFORM_BLOCKS 0x8A2B
#define GL_MAX_FRAGMENT_UNIFORM_BLOCKS 0x8A2D
#define GL_MAX_COMBINED_UNIFORM_BLOCKS 0x8A2E
#define GL_MAX_UNIFORM_BUFFER_BINDINGS 0x8A2F
#define GL_MAX_UNIFORM_BLOCK_SIZE 0x8A30
#define GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS 0x8A31
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
#define GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER 0x8A46
#define GL_INVALID_INDEX 0xFFFFFFFF
#define GL_MAX_VERTEX_OUTPUT_COMPONENTS 0x9122
#define GL_MAX_FRAGMENT_INPUT_COMPONENTS 0x9125
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
#define GL_SYNC_FLUSH_COMMANDS_BIT 0x00000001
#define GL_TIMEOUT_IGNORED 0xFFFFFFFFFFFFFFFF
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR 0x88FE
#define GL_ANY_SAMPLES_PASSED 0x8C2F
#define GL_ANY_SAMPLES_PASSED_CONSERVATIVE 0x8D6A
#define GL_SAMPLER_BINDING 0x8919
#define GL_RGB10_A2UI 0x906F
#define GL_TEXTURE_SWIZZLE_R 0x8E42
#define GL_TEXTURE_SWIZZLE_G 0x8E43
#define GL_TEXTURE_SWIZZLE_B 0x8E44
#define GL_TEXTURE_SWIZZLE_A 0x8E45
#define GL_GREEN 0x1904
#define GL_BLUE 0x1905
#define GL_INT_2_10_10_10_REV 0x8D9F
#define GL_TRANSFORM_FEEDBACK 0x8E22
#define GL_TRANSFORM_FEEDBACK_PAUSED 0x8E23
#define GL_TRANSFORM_FEEDBACK_ACTIVE 0x8E24
#define GL_TRANSFORM_FEEDBACK_BINDING 0x8E25
#define GL_PROGRAM_BINARY_RETRIEVABLE_HINT 0x8257
#define GL_PROGRAM_BINARY_LENGTH 0x8741
#define GL_NUM_PROGRAM_BINARY_FORMATS 0x87FE
#define GL_PROGRAM_BINARY_FORMATS 0x87FF
#define GL_COMPRESSED_R11_EAC 0x9270
#define GL_COMPRESSED_SIGNED_R11_EAC 0x9271
#define GL_COMPRESSED_RG11_EAC 0x9272
#define GL_COMPRESSED_SIGNED_RG11_EAC 0x9273
#define GL_COMPRESSED_RGB8_ETC2 0x9274
#define GL_COMPRESSED_SRGB8_ETC2 0x9275
#define GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9276
#define GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 0x9277
#define GL_COMPRESSED_RGBA8_ETC2_EAC 0x9278
#define GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC 0x9279
#define GL_TEXTURE_IMMUTABLE_FORMAT 0x912F
#define GL_MAX_ELEMENT_INDEX 0x8D6B
#define GL_NUM_SAMPLE_COUNTS 0x9380
#define GL_TEXTURE_IMMUTABLE_LEVELS 0x82DF
#endif
extern void (APIENTRYP gglReadBuffer)(GLenum src);
extern void (APIENTRYP gglDrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
extern void (APIENTRYP gglTexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglCopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglCompressedTexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglGenQueries)(GLsizei n, GLuint *ids);
extern void (APIENTRYP gglDeleteQueries)(GLsizei n, const GLuint *ids);
extern GLboolean (APIENTRYP gglIsQuery)(GLuint id);
extern void (APIENTRYP gglBeginQuery)(GLenum target, GLuint id);
extern void (APIENTRYP gglEndQuery)(GLenum target);
extern void (APIENTRYP gglGetQueryiv)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetQueryObjectuiv)(GLuint id, GLenum pname, GLuint *params);
extern GLboolean (APIENTRYP gglUnmapBuffer)(GLenum target);
extern void (APIENTRYP gglGetBufferPointerv)(GLenum target, GLenum pname, void **params);
extern void (APIENTRYP gglDrawBuffers)(GLsizei n, const GLenum *bufs);
extern void (APIENTRYP gglUniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglBlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
extern void (APIENTRYP gglRenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglFramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
extern void * (APIENTRYP gglMapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
extern void (APIENTRYP gglFlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length);
extern void (APIENTRYP gglBindVertexArray)(GLuint array);
extern void (APIENTRYP gglDeleteVertexArrays)(GLsizei n, const GLuint *arrays);
extern void (APIENTRYP gglGenVertexArrays)(GLsizei n, GLuint *arrays);
extern GLboolean (APIENTRYP gglIsVertexArray)(GLuint array);
extern void (APIENTRYP gglGetIntegeri_v)(GLenum target, GLuint index, GLint *data);
extern void (APIENTRYP gglBeginTransformFeedback)(GLenum primitiveMode);
extern void (APIENTRYP gglEndTransformFeedback)();
extern void (APIENTRYP gglBindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
extern void (APIENTRYP gglBindBufferBase)(GLenum target, GLuint index, GLuint buffer);
extern void (APIENTRYP gglTransformFeedbackVaryings)(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode);
extern void (APIENTRYP gglGetTransformFeedbackVarying)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
extern void (APIENTRYP gglVertexAttribIPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
extern void (APIENTRYP gglGetVertexAttribIiv)(GLuint index, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetVertexAttribIuiv)(GLuint index, GLenum pname, GLuint *params);
extern void (APIENTRYP gglVertexAttribI4i)(GLuint index, GLint x, GLint y, GLint z, GLint w);
extern void (APIENTRYP gglVertexAttribI4ui)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
extern void (APIENTRYP gglVertexAttribI4iv)(GLuint index, const GLint *v);
extern void (APIENTRYP gglVertexAttribI4uiv)(GLuint index, const GLuint *v);
extern void (APIENTRYP gglGetUniformuiv)(GLuint program, GLint location, GLuint *params);
extern GLint (APIENTRYP gglGetFragDataLocation)(GLuint program, const GLchar *name);
extern void (APIENTRYP gglUniform1ui)(GLint location, GLuint v0);
extern void (APIENTRYP gglUniform2ui)(GLint location, GLuint v0, GLuint v1);
extern void (APIENTRYP gglUniform3ui)(GLint location, GLuint v0, GLuint v1, GLuint v2);
extern void (APIENTRYP gglUniform4ui)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
extern void (APIENTRYP gglUniform1uiv)(GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglUniform2uiv)(GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglUniform3uiv)(GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglUniform4uiv)(GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglClearBufferiv)(GLenum buffer, GLint drawbuffer, const GLint *value);
extern void (APIENTRYP gglClearBufferuiv)(GLenum buffer, GLint drawbuffer, const GLuint *value);
extern void (APIENTRYP gglClearBufferfv)(GLenum buffer, GLint drawbuffer, const GLfloat *value);
extern void (APIENTRYP gglClearBufferfi)(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
extern const GLubyte * (APIENTRYP gglGetStringi)(GLenum name, GLuint index);
extern void (APIENTRYP gglCopyBufferSubData)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
extern void (APIENTRYP gglGetUniformIndices)(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices);
extern void (APIENTRYP gglGetActiveUniformsiv)(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
extern GLuint (APIENTRYP gglGetUniformBlockIndex)(GLuint program, const GLchar *uniformBlockName);
extern void (APIENTRYP gglGetActiveUniformBlockiv)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetActiveUniformBlockName)(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
extern void (APIENTRYP gglUniformBlockBinding)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
extern void (APIENTRYP gglDrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
extern void (APIENTRYP gglDrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
extern GLsync (APIENTRYP gglFenceSync)(GLenum condition, GLbitfield flags);
extern GLboolean (APIENTRYP gglIsSync)(GLsync sync);
extern void (APIENTRYP gglDeleteSync)(GLsync sync);
extern GLenum (APIENTRYP gglClientWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
extern void (APIENTRYP gglWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout);
extern void (APIENTRYP gglGetInteger64v)(GLenum pname, GLint64 *data);
extern void (APIENTRYP gglGetSynciv)(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
extern void (APIENTRYP gglGetInteger64i_v)(GLenum target, GLuint index, GLint64 *data);
extern void (APIENTRYP gglGetBufferParameteri64v)(GLenum target, GLenum pname, GLint64 *params);
extern void (APIENTRYP gglGenSamplers)(GLsizei count, GLuint *samplers);
extern void (APIENTRYP gglDeleteSamplers)(GLsizei count, const GLuint *samplers);
extern GLboolean (APIENTRYP gglIsSampler)(GLuint sampler);
extern void (APIENTRYP gglBindSampler)(GLuint unit, GLuint sampler);
extern void (APIENTRYP gglSamplerParameteri)(GLuint sampler, GLenum pname, GLint param);
extern void (APIENTRYP gglSamplerParameteriv)(GLuint sampler, GLenum pname, const GLint *param);
extern void (APIENTRYP gglSamplerParameterf)(GLuint sampler, GLenum pname, GLfloat param);
extern void (APIENTRYP gglSamplerParameterfv)(GLuint sampler, GLenum pname, const GLfloat *param);
extern void (APIENTRYP gglGetSamplerParameteriv)(GLuint sampler, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetSamplerParameterfv)(GLuint sampler, GLenum pname, GLfloat *params);
extern void (APIENTRYP gglVertexAttribDivisor)(GLuint index, GLuint divisor);
extern void (APIENTRYP gglBindTransformFeedback)(GLenum target, GLuint id);
extern void (APIENTRYP gglDeleteTransformFeedbacks)(GLsizei n, const GLuint *ids);
extern void (APIENTRYP gglGenTransformFeedbacks)(GLsizei n, GLuint *ids);
extern GLboolean (APIENTRYP gglIsTransformFeedback)(GLuint id);
extern void (APIENTRYP gglPauseTransformFeedback)();
extern void (APIENTRYP gglResumeTransformFeedback)();
extern void (APIENTRYP gglGetProgramBinary)(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
extern void (APIENTRYP gglProgramBinary)(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
extern void (APIENTRYP gglProgramParameteri)(GLuint program, GLenum pname, GLint value);
extern void (APIENTRYP gglInvalidateFramebuffer)(GLenum target, GLsizei numAttachments, const GLenum *attachments);
extern void (APIENTRYP gglInvalidateSubFramebuffer)(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglTexStorage2D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglTexStorage3D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
extern void (APIENTRYP gglGetInternalformativ)(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params);

#ifndef __APPLE__

#ifndef GL_AMD_compressed_3DC_texture
#define GL_AMD_compressed_3DC_texture
#define GL_3DC_X_AMD 0x87F9
#define GL_3DC_XY_AMD 0x87FA
#endif

#ifndef GL_AMD_compressed_ATC_texture
#define GL_AMD_compressed_ATC_texture
#define GL_ATC_RGB_AMD 0x8C92
#define GL_ATC_RGBA_EXPLICIT_ALPHA_AMD 0x8C93
#define GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD 0x87EE
#endif

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

#ifndef GL_AMD_program_binary_Z400
#define GL_AMD_program_binary_Z400
#define GL_Z400_BINARY_AMD 0x8740
#endif

#ifndef GL_ANDROID_extension_pack_es31a
#define GL_ANDROID_extension_pack_es31a
#endif

#ifndef GL_ANGLE_depth_texture
#define GL_ANGLE_depth_texture
/* reuse GL_DEPTH_COMPONENT */
#define GL_DEPTH_STENCIL_OES 0x84F9
/* reuse GL_UNSIGNED_SHORT */
/* reuse GL_UNSIGNED_INT */
#define GL_UNSIGNED_INT_24_8_OES 0x84FA
/* reuse GL_DEPTH_COMPONENT16 */
#define GL_DEPTH_COMPONENT32_OES 0x81A7
#define GL_DEPTH24_STENCIL8_OES 0x88F0
#endif

#ifndef GL_ANGLE_framebuffer_blit
#define GL_ANGLE_framebuffer_blit
#define GL_READ_FRAMEBUFFER_ANGLE 0x8CA8
#define GL_DRAW_FRAMEBUFFER_ANGLE 0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_ANGLE 0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_ANGLE 0x8CAA
#endif
extern void (APIENTRYP gglBlitFramebufferANGLE)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

#ifndef GL_ANGLE_framebuffer_multisample
#define GL_ANGLE_framebuffer_multisample
#define GL_RENDERBUFFER_SAMPLES_ANGLE 0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE 0x8D56
#define GL_MAX_SAMPLES_ANGLE 0x8D57
#endif
extern void (APIENTRYP gglRenderbufferStorageMultisampleANGLE)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

#ifndef GL_ANGLE_instanced_arrays
#define GL_ANGLE_instanced_arrays
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR_ANGLE 0x88FE
#endif
extern void (APIENTRYP gglDrawArraysInstancedANGLE)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
extern void (APIENTRYP gglDrawElementsInstancedANGLE)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
extern void (APIENTRYP gglVertexAttribDivisorANGLE)(GLuint index, GLuint divisor);

#ifndef GL_ANGLE_pack_reverse_row_order
#define GL_ANGLE_pack_reverse_row_order
#define GL_PACK_REVERSE_ROW_ORDER_ANGLE 0x93A4
#endif

#ifndef GL_ANGLE_program_binary
#define GL_ANGLE_program_binary
#define GL_PROGRAM_BINARY_ANGLE 0x93A6
#endif

#ifndef GL_ANGLE_texture_compression_dxt3
#define GL_ANGLE_texture_compression_dxt3
#define GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE 0x83F2
#endif

#ifndef GL_ANGLE_texture_compression_dxt5
#define GL_ANGLE_texture_compression_dxt5
#define GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE 0x83F3
#endif

#ifndef GL_ANGLE_texture_usage
#define GL_ANGLE_texture_usage
#define GL_TEXTURE_USAGE_ANGLE 0x93A2
#define GL_FRAMEBUFFER_ATTACHMENT_ANGLE 0x93A3
#endif

#ifndef GL_ANGLE_translated_shader_source
#define GL_ANGLE_translated_shader_source
#define GL_TRANSLATED_SHADER_SOURCE_LENGTH_ANGLE 0x93A0
#endif
extern void (APIENTRYP gglGetTranslatedShaderSourceANGLE)(GLuint shader, GLsizei bufsize, GLsizei *length, GLchar *source);

#ifndef GL_APPLE_clip_distance
#define GL_APPLE_clip_distance
#define GL_MAX_CLIP_DISTANCES_APPLE 0x0D32
#define GL_CLIP_DISTANCE0_APPLE 0x3000
#define GL_CLIP_DISTANCE1_APPLE 0x3001
#define GL_CLIP_DISTANCE2_APPLE 0x3002
#define GL_CLIP_DISTANCE3_APPLE 0x3003
#define GL_CLIP_DISTANCE4_APPLE 0x3004
#define GL_CLIP_DISTANCE5_APPLE 0x3005
#define GL_CLIP_DISTANCE6_APPLE 0x3006
#define GL_CLIP_DISTANCE7_APPLE 0x3007
#endif

#ifndef GL_APPLE_color_buffer_packed_float
#define GL_APPLE_color_buffer_packed_float
#endif

#ifndef GL_APPLE_copy_texture_levels
#define GL_APPLE_copy_texture_levels
#endif
extern void (APIENTRYP gglCopyTextureLevelsAPPLE)(GLuint destinationTexture, GLuint sourceTexture, GLint sourceBaseLevel, GLsizei sourceLevelCount);

#ifndef GL_APPLE_framebuffer_multisample
#define GL_APPLE_framebuffer_multisample
#define GL_RENDERBUFFER_SAMPLES_APPLE 0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_APPLE 0x8D56
#define GL_MAX_SAMPLES_APPLE 0x8D57
#define GL_READ_FRAMEBUFFER_APPLE 0x8CA8
#define GL_DRAW_FRAMEBUFFER_APPLE 0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_APPLE 0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_APPLE 0x8CAA
#endif
extern void (APIENTRYP gglRenderbufferStorageMultisampleAPPLE)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglResolveMultisampleFramebufferAPPLE)();

#ifndef GL_APPLE_rgb_422
#define GL_APPLE_rgb_422
#define GL_RGB_422_APPLE 0x8A1F
#define GL_UNSIGNED_SHORT_8_8_APPLE 0x85BA
#define GL_UNSIGNED_SHORT_8_8_REV_APPLE 0x85BB
#define GL_RGB_RAW_422_APPLE 0x8A51
#endif

#ifndef GL_APPLE_sync
#define GL_APPLE_sync
#define GL_SYNC_OBJECT_APPLE 0x8A53
#define GL_MAX_SERVER_WAIT_TIMEOUT_APPLE 0x9111
#define GL_OBJECT_TYPE_APPLE 0x9112
#define GL_SYNC_CONDITION_APPLE 0x9113
#define GL_SYNC_STATUS_APPLE 0x9114
#define GL_SYNC_FLAGS_APPLE 0x9115
#define GL_SYNC_FENCE_APPLE 0x9116
#define GL_SYNC_GPU_COMMANDS_COMPLETE_APPLE 0x9117
#define GL_UNSIGNALED_APPLE 0x9118
#define GL_SIGNALED_APPLE 0x9119
#define GL_ALREADY_SIGNALED_APPLE 0x911A
#define GL_TIMEOUT_EXPIRED_APPLE 0x911B
#define GL_CONDITION_SATISFIED_APPLE 0x911C
#define GL_WAIT_FAILED_APPLE 0x911D
#define GL_SYNC_FLUSH_COMMANDS_BIT_APPLE 0x00000001
#define GL_TIMEOUT_IGNORED_APPLE 0xFFFFFFFFFFFFFFFF
#endif
extern GLsync (APIENTRYP gglFenceSyncAPPLE)(GLenum condition, GLbitfield flags);
extern GLboolean (APIENTRYP gglIsSyncAPPLE)(GLsync sync);
extern void (APIENTRYP gglDeleteSyncAPPLE)(GLsync sync);
extern GLenum (APIENTRYP gglClientWaitSyncAPPLE)(GLsync sync, GLbitfield flags, GLuint64 timeout);
extern void (APIENTRYP gglWaitSyncAPPLE)(GLsync sync, GLbitfield flags, GLuint64 timeout);
extern void (APIENTRYP gglGetInteger64vAPPLE)(GLenum pname, GLint64 *params);
extern void (APIENTRYP gglGetSyncivAPPLE)(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);

#ifndef GL_APPLE_texture_format_BGRA8888
#define GL_APPLE_texture_format_BGRA8888
#define GL_BGRA_EXT 0x80E1
#define GL_BGRA8_EXT 0x93A1
#endif

#ifndef GL_APPLE_texture_max_level
#define GL_APPLE_texture_max_level
#define GL_TEXTURE_MAX_LEVEL_APPLE 0x813D
#endif

#ifndef GL_APPLE_texture_packed_float
#define GL_APPLE_texture_packed_float
#define GL_UNSIGNED_INT_10F_11F_11F_REV_APPLE 0x8C3B
#define GL_UNSIGNED_INT_5_9_9_9_REV_APPLE 0x8C3E
#define GL_R11F_G11F_B10F_APPLE 0x8C3A
#define GL_RGB9_E5_APPLE 0x8C3D
#endif

#ifndef GL_ARB_sparse_texture2
#define GL_ARB_sparse_texture2
#endif

#ifndef GL_ARM_mali_program_binary
#define GL_ARM_mali_program_binary
#define GL_MALI_PROGRAM_BINARY_ARM 0x8F61
#endif

#ifndef GL_ARM_mali_shader_binary
#define GL_ARM_mali_shader_binary
#define GL_MALI_SHADER_BINARY_ARM 0x8F60
#endif

#ifndef GL_ARM_rgba8
#define GL_ARM_rgba8
#endif

#ifndef GL_ARM_shader_framebuffer_fetch
#define GL_ARM_shader_framebuffer_fetch
#define GL_FETCH_PER_SAMPLE_ARM 0x8F65
#define GL_FRAGMENT_SHADER_FRAMEBUFFER_FETCH_MRT_ARM 0x8F66
#endif

#ifndef GL_ARM_shader_framebuffer_fetch_depth_stencil
#define GL_ARM_shader_framebuffer_fetch_depth_stencil
#endif

#ifndef GL_DMP_program_binary
#define GL_DMP_program_binary
#define GL_SMAPHS30_PROGRAM_BINARY_DMP 0x9251
#define GL_SMAPHS_PROGRAM_BINARY_DMP 0x9252
#define GL_DMP_PROGRAM_BINARY_DMP 0x9253
#endif

#ifndef GL_DMP_shader_binary
#define GL_DMP_shader_binary
#define GL_SHADER_BINARY_DMP 0x9250
#endif

#ifndef GL_EXT_YUV_target
#define GL_EXT_YUV_target
#define GL_SAMPLER_EXTERNAL_2D_Y2Y_EXT 0x8BE7
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#define GL_TEXTURE_BINDING_EXTERNAL_OES 0x8D67
#define GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES 0x8D68
#endif

#ifndef GL_EXT_base_instance
#define GL_EXT_base_instance
#endif
extern void (APIENTRYP gglDrawArraysInstancedBaseInstanceEXT)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
extern void (APIENTRYP gglDrawElementsInstancedBaseInstanceEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance);
extern void (APIENTRYP gglDrawElementsInstancedBaseVertexBaseInstanceEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance);

#ifndef GL_EXT_blend_func_extended
#define GL_EXT_blend_func_extended
#define GL_SRC1_COLOR_EXT 0x88F9
#define GL_SRC1_ALPHA_EXT 0x8589
#define GL_ONE_MINUS_SRC1_COLOR_EXT 0x88FA
#define GL_ONE_MINUS_SRC1_ALPHA_EXT 0x88FB
#define GL_SRC_ALPHA_SATURATE_EXT 0x0308
#define GL_LOCATION_INDEX_EXT 0x930F
#define GL_MAX_DUAL_SOURCE_DRAW_BUFFERS_EXT 0x88FC
#endif
extern void (APIENTRYP gglBindFragDataLocationIndexedEXT)(GLuint program, GLuint colorNumber, GLuint index, const GLchar *name);
extern void (APIENTRYP gglBindFragDataLocationEXT)(GLuint program, GLuint color, const GLchar *name);
extern GLint (APIENTRYP gglGetProgramResourceLocationIndexEXT)(GLuint program, GLenum programInterface, const GLchar *name);
extern GLint (APIENTRYP gglGetFragDataIndexEXT)(GLuint program, const GLchar *name);

#ifndef GL_EXT_blend_minmax
#define GL_EXT_blend_minmax
#define GL_MIN_EXT 0x8007
#define GL_MAX_EXT 0x8008
#endif

#ifndef GL_EXT_buffer_storage
#define GL_EXT_buffer_storage
/* reuse GL_MAP_READ_BIT */
/* reuse GL_MAP_WRITE_BIT */
#define GL_MAP_PERSISTENT_BIT_EXT 0x0040
#define GL_MAP_COHERENT_BIT_EXT 0x0080
#define GL_DYNAMIC_STORAGE_BIT_EXT 0x0100
#define GL_CLIENT_STORAGE_BIT_EXT 0x0200
#define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT_EXT 0x00004000
#define GL_BUFFER_IMMUTABLE_STORAGE_EXT 0x821F
#define GL_BUFFER_STORAGE_FLAGS_EXT 0x8220
#endif
extern void (APIENTRYP gglBufferStorageEXT)(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);

#ifndef GL_EXT_clear_texture
#define GL_EXT_clear_texture
#endif
extern void (APIENTRYP gglClearTexImageEXT)(GLuint texture, GLint level, GLenum format, GLenum type, const void *data);
extern void (APIENTRYP gglClearTexSubImageEXT)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data);

#ifndef GL_EXT_clip_cull_distance
#define GL_EXT_clip_cull_distance
#define GL_MAX_CLIP_DISTANCES_EXT 0x0D32
#define GL_MAX_CULL_DISTANCES_EXT 0x82F9
#define GL_MAX_COMBINED_CLIP_AND_CULL_DISTANCES_EXT 0x82FA
#define GL_CLIP_DISTANCE0_EXT 0x3000
#define GL_CLIP_DISTANCE1_EXT 0x3001
#define GL_CLIP_DISTANCE2_EXT 0x3002
#define GL_CLIP_DISTANCE3_EXT 0x3003
#define GL_CLIP_DISTANCE4_EXT 0x3004
#define GL_CLIP_DISTANCE5_EXT 0x3005
#define GL_CLIP_DISTANCE6_EXT 0x3006
#define GL_CLIP_DISTANCE7_EXT 0x3007
#endif

#ifndef GL_EXT_color_buffer_float
#define GL_EXT_color_buffer_float
#endif

#ifndef GL_EXT_color_buffer_half_float
#define GL_EXT_color_buffer_half_float
#define GL_RGBA16F_EXT 0x881A
#define GL_RGB16F_EXT 0x881B
#define GL_RG16F_EXT 0x822F
#define GL_R16F_EXT 0x822D
#define GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE_EXT 0x8211
#define GL_UNSIGNED_NORMALIZED_EXT 0x8C17
#endif

#ifndef GL_EXT_conservative_depth
#define GL_EXT_conservative_depth
#endif

#ifndef GL_EXT_copy_image
#define GL_EXT_copy_image
#endif
extern void (APIENTRYP gglCopyImageSubDataEXT)(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);

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

#ifndef GL_EXT_discard_framebuffer
#define GL_EXT_discard_framebuffer
#define GL_COLOR_EXT 0x1800
#define GL_DEPTH_EXT 0x1801
#define GL_STENCIL_EXT 0x1802
#endif
extern void (APIENTRYP gglDiscardFramebufferEXT)(GLenum target, GLsizei numAttachments, const GLenum *attachments);

#ifndef GL_EXT_disjoint_timer_query
#define GL_EXT_disjoint_timer_query
#define GL_QUERY_COUNTER_BITS_EXT 0x8864
#define GL_CURRENT_QUERY_EXT 0x8865
#define GL_QUERY_RESULT_EXT 0x8866
#define GL_QUERY_RESULT_AVAILABLE_EXT 0x8867
#define GL_TIME_ELAPSED_EXT 0x88BF
#define GL_TIMESTAMP_EXT 0x8E28
#define GL_GPU_DISJOINT_EXT 0x8FBB
#endif
extern void (APIENTRYP gglGenQueriesEXT)(GLsizei n, GLuint *ids);
extern void (APIENTRYP gglDeleteQueriesEXT)(GLsizei n, const GLuint *ids);
extern GLboolean (APIENTRYP gglIsQueryEXT)(GLuint id);
extern void (APIENTRYP gglBeginQueryEXT)(GLenum target, GLuint id);
extern void (APIENTRYP gglEndQueryEXT)(GLenum target);
extern void (APIENTRYP gglQueryCounterEXT)(GLuint id, GLenum target);
extern void (APIENTRYP gglGetQueryivEXT)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetQueryObjectivEXT)(GLuint id, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetQueryObjectuivEXT)(GLuint id, GLenum pname, GLuint *params);
extern void (APIENTRYP gglGetQueryObjecti64vEXT)(GLuint id, GLenum pname, GLint64 *params);
extern void (APIENTRYP gglGetQueryObjectui64vEXT)(GLuint id, GLenum pname, GLuint64 *params);

#ifndef GL_EXT_draw_buffers
#define GL_EXT_draw_buffers
#define GL_MAX_COLOR_ATTACHMENTS_EXT 0x8CDF
#define GL_MAX_DRAW_BUFFERS_EXT 0x8824
#define GL_DRAW_BUFFER0_EXT 0x8825
#define GL_DRAW_BUFFER1_EXT 0x8826
#define GL_DRAW_BUFFER2_EXT 0x8827
#define GL_DRAW_BUFFER3_EXT 0x8828
#define GL_DRAW_BUFFER4_EXT 0x8829
#define GL_DRAW_BUFFER5_EXT 0x882A
#define GL_DRAW_BUFFER6_EXT 0x882B
#define GL_DRAW_BUFFER7_EXT 0x882C
#define GL_DRAW_BUFFER8_EXT 0x882D
#define GL_DRAW_BUFFER9_EXT 0x882E
#define GL_DRAW_BUFFER10_EXT 0x882F
#define GL_DRAW_BUFFER11_EXT 0x8830
#define GL_DRAW_BUFFER12_EXT 0x8831
#define GL_DRAW_BUFFER13_EXT 0x8832
#define GL_DRAW_BUFFER14_EXT 0x8833
#define GL_DRAW_BUFFER15_EXT 0x8834
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
#endif
extern void (APIENTRYP gglDrawBuffersEXT)(GLsizei n, const GLenum *bufs);

#ifndef GL_EXT_draw_buffers_indexed
#define GL_EXT_draw_buffers_indexed
/* reuse GL_BLEND_EQUATION_RGB */
/* reuse GL_BLEND_EQUATION_ALPHA */
/* reuse GL_BLEND_SRC_RGB */
/* reuse GL_BLEND_SRC_ALPHA */
/* reuse GL_BLEND_DST_RGB */
/* reuse GL_BLEND_DST_ALPHA */
/* reuse GL_COLOR_WRITEMASK */
/* reuse GL_BLEND */
/* reuse GL_FUNC_ADD */
/* reuse GL_FUNC_SUBTRACT */
/* reuse GL_FUNC_REVERSE_SUBTRACT */
/* reuse GL_MIN */
/* reuse GL_MAX */
/* reuse GL_ZERO */
/* reuse GL_ONE */
/* reuse GL_SRC_COLOR */
/* reuse GL_ONE_MINUS_SRC_COLOR */
/* reuse GL_DST_COLOR */
/* reuse GL_ONE_MINUS_DST_COLOR */
/* reuse GL_SRC_ALPHA */
/* reuse GL_ONE_MINUS_SRC_ALPHA */
/* reuse GL_DST_ALPHA */
/* reuse GL_ONE_MINUS_DST_ALPHA */
/* reuse GL_CONSTANT_COLOR */
/* reuse GL_ONE_MINUS_CONSTANT_COLOR */
/* reuse GL_CONSTANT_ALPHA */
/* reuse GL_ONE_MINUS_CONSTANT_ALPHA */
/* reuse GL_SRC_ALPHA_SATURATE */
#endif
extern void (APIENTRYP gglEnableiEXT)(GLenum target, GLuint index);
extern void (APIENTRYP gglDisableiEXT)(GLenum target, GLuint index);
extern void (APIENTRYP gglBlendEquationiEXT)(GLuint buf, GLenum mode);
extern void (APIENTRYP gglBlendEquationSeparateiEXT)(GLuint buf, GLenum modeRGB, GLenum modeAlpha);
extern void (APIENTRYP gglBlendFunciEXT)(GLuint buf, GLenum src, GLenum dst);
extern void (APIENTRYP gglBlendFuncSeparateiEXT)(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
extern void (APIENTRYP gglColorMaskiEXT)(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a);
extern GLboolean (APIENTRYP gglIsEnablediEXT)(GLenum target, GLuint index);

#ifndef GL_EXT_draw_elements_base_vertex
#define GL_EXT_draw_elements_base_vertex
#endif
extern void (APIENTRYP gglDrawElementsBaseVertexEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
extern void (APIENTRYP gglDrawRangeElementsBaseVertexEXT)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex);
extern void (APIENTRYP gglDrawElementsInstancedBaseVertexEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex);
extern void (APIENTRYP gglMultiDrawElementsBaseVertexEXT)(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount, const GLint *basevertex);

#ifndef GL_EXT_draw_instanced
#define GL_EXT_draw_instanced
#endif
extern void (APIENTRYP gglDrawArraysInstancedEXT)(GLenum mode, GLint start, GLsizei count, GLsizei primcount);
extern void (APIENTRYP gglDrawElementsInstancedEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);

#ifndef GL_EXT_draw_transform_feedback
#define GL_EXT_draw_transform_feedback
#endif
extern void (APIENTRYP gglDrawTransformFeedbackEXT)(GLenum mode, GLuint id);
extern void (APIENTRYP gglDrawTransformFeedbackInstancedEXT)(GLenum mode, GLuint id, GLsizei instancecount);

#ifndef GL_EXT_float_blend
#define GL_EXT_float_blend
#endif

#ifndef GL_EXT_geometry_point_size
#define GL_EXT_geometry_point_size
#endif

#ifndef GL_EXT_geometry_shader
#define GL_EXT_geometry_shader
#define GL_GEOMETRY_SHADER_EXT 0x8DD9
#define GL_GEOMETRY_SHADER_BIT_EXT 0x00000004
#define GL_GEOMETRY_LINKED_VERTICES_OUT_EXT 0x8916
#define GL_GEOMETRY_LINKED_INPUT_TYPE_EXT 0x8917
#define GL_GEOMETRY_LINKED_OUTPUT_TYPE_EXT 0x8918
#define GL_GEOMETRY_SHADER_INVOCATIONS_EXT 0x887F
#define GL_LAYER_PROVOKING_VERTEX_EXT 0x825E
#define GL_LINES_ADJACENCY_EXT 0x000A
#define GL_LINE_STRIP_ADJACENCY_EXT 0x000B
#define GL_TRIANGLES_ADJACENCY_EXT 0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY_EXT 0x000D
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT 0x8DDF
#define GL_MAX_GEOMETRY_UNIFORM_BLOCKS_EXT 0x8A2C
#define GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS_EXT 0x8A32
#define GL_MAX_GEOMETRY_INPUT_COMPONENTS_EXT 0x9123
#define GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_EXT 0x9124
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT 0x8DE0
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT 0x8DE1
#define GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT 0x8E5A
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT 0x8C29
#define GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT 0x92CF
#define GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT 0x92D5
#define GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT 0x90CD
#define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT 0x90D7
#define GL_FIRST_VERTEX_CONVENTION_EXT 0x8E4D
#define GL_LAST_VERTEX_CONVENTION_EXT 0x8E4E
#define GL_UNDEFINED_VERTEX_EXT 0x8260
#define GL_PRIMITIVES_GENERATED_EXT 0x8C87
#define GL_FRAMEBUFFER_DEFAULT_LAYERS_EXT 0x9312
#define GL_MAX_FRAMEBUFFER_LAYERS_EXT 0x9317
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT 0x8DA8
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED_EXT 0x8DA7
#define GL_REFERENCED_BY_GEOMETRY_SHADER_EXT 0x9309
#endif
extern void (APIENTRYP gglFramebufferTextureEXT)(GLenum target, GLenum attachment, GLuint texture, GLint level);

#ifndef GL_EXT_gpu_shader5
#define GL_EXT_gpu_shader5
#endif

#ifndef GL_EXT_instanced_arrays
#define GL_EXT_instanced_arrays
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR_EXT 0x88FE
#endif
/* reuse void (APIENTRYP gglDrawArraysInstancedEXT)(GLenum mode, GLint start, GLsizei count, GLsizei primcount) */
/* reuse void (APIENTRYP gglDrawElementsInstancedEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount) */
extern void (APIENTRYP gglVertexAttribDivisorEXT)(GLuint index, GLuint divisor);

#ifndef GL_EXT_map_buffer_range
#define GL_EXT_map_buffer_range
#define GL_MAP_READ_BIT_EXT 0x0001
#define GL_MAP_WRITE_BIT_EXT 0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT_EXT 0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT_EXT 0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT_EXT 0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT_EXT 0x0020
#endif
extern void * (APIENTRYP gglMapBufferRangeEXT)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
extern void (APIENTRYP gglFlushMappedBufferRangeEXT)(GLenum target, GLintptr offset, GLsizeiptr length);

#ifndef GL_EXT_multi_draw_arrays
#define GL_EXT_multi_draw_arrays
#endif
extern void (APIENTRYP gglMultiDrawArraysEXT)(GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount);
extern void (APIENTRYP gglMultiDrawElementsEXT)(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount);

#ifndef GL_EXT_multi_draw_indirect
#define GL_EXT_multi_draw_indirect
#endif
extern void (APIENTRYP gglMultiDrawArraysIndirectEXT)(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride);
extern void (APIENTRYP gglMultiDrawElementsIndirectEXT)(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride);

#ifndef GL_EXT_multisampled_compatibility
#define GL_EXT_multisampled_compatibility
#define GL_MULTISAMPLE_EXT 0x809D
#define GL_SAMPLE_ALPHA_TO_ONE_EXT 0x809F
#endif

#ifndef GL_EXT_multisampled_render_to_texture
#define GL_EXT_multisampled_render_to_texture
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_SAMPLES_EXT 0x8D6C
#define GL_RENDERBUFFER_SAMPLES_EXT 0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT 0x8D56
#define GL_MAX_SAMPLES_EXT 0x8D57
#endif
extern void (APIENTRYP gglRenderbufferStorageMultisampleEXT)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglFramebufferTexture2DMultisampleEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);

#ifndef GL_EXT_multiview_draw_buffers
#define GL_EXT_multiview_draw_buffers
#define GL_COLOR_ATTACHMENT_EXT 0x90F0
#define GL_MULTIVIEW_EXT 0x90F1
#define GL_DRAW_BUFFER_EXT 0x0C01
#define GL_READ_BUFFER_EXT 0x0C02
#define GL_MAX_MULTIVIEW_BUFFERS_EXT 0x90F2
#endif
extern void (APIENTRYP gglReadBufferIndexedEXT)(GLenum src, GLint index);
extern void (APIENTRYP gglDrawBuffersIndexedEXT)(GLint n, const GLenum *location, const GLint *indices);
extern void (APIENTRYP gglGetIntegeri_vEXT)(GLenum target, GLuint index, GLint *data);

#ifndef GL_EXT_occlusion_query_boolean
#define GL_EXT_occlusion_query_boolean
#define GL_ANY_SAMPLES_PASSED_EXT 0x8C2F
#define GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT 0x8D6A
/* reuse GL_CURRENT_QUERY_EXT */
/* reuse GL_QUERY_RESULT_EXT */
/* reuse GL_QUERY_RESULT_AVAILABLE_EXT */
#endif
/* reuse void (APIENTRYP gglGenQueriesEXT)(GLsizei n, GLuint *ids) */
/* reuse void (APIENTRYP gglDeleteQueriesEXT)(GLsizei n, const GLuint *ids) */
/* reuse GLboolean (APIENTRYP gglIsQueryEXT)(GLuint id) */
/* reuse void (APIENTRYP gglBeginQueryEXT)(GLenum target, GLuint id) */
/* reuse void (APIENTRYP gglEndQueryEXT)(GLenum target) */
/* reuse void (APIENTRYP gglGetQueryivEXT)(GLenum target, GLenum pname, GLint *params) */
/* reuse void (APIENTRYP gglGetQueryObjectuivEXT)(GLuint id, GLenum pname, GLuint *params) */

#ifndef GL_EXT_polygon_offset_clamp
#define GL_EXT_polygon_offset_clamp
#define GL_POLYGON_OFFSET_CLAMP_EXT 0x8E1B
#endif
extern void (APIENTRYP gglPolygonOffsetClampEXT)(GLfloat factor, GLfloat units, GLfloat clamp);

#ifndef GL_EXT_post_depth_coverage
#define GL_EXT_post_depth_coverage
#endif

#ifndef GL_EXT_primitive_bounding_box
#define GL_EXT_primitive_bounding_box
#define GL_PRIMITIVE_BOUNDING_BOX_EXT 0x92BE
#endif
extern void (APIENTRYP gglPrimitiveBoundingBoxEXT)(GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW);

#ifndef GL_EXT_protected_textures
#define GL_EXT_protected_textures
#define GL_CONTEXT_FLAG_PROTECTED_CONTENT_BIT_EXT 0x00000010
#define GL_TEXTURE_PROTECTED_EXT 0x8BFA
#endif

#ifndef GL_EXT_pvrtc_sRGB
#define GL_EXT_pvrtc_sRGB
#define GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT 0x8A54
#define GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT 0x8A55
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT 0x8A56
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT 0x8A57
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV2_IMG 0x93F0
#define GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV2_IMG 0x93F1
#endif

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

#ifndef GL_EXT_read_format_bgra
#define GL_EXT_read_format_bgra
/* reuse GL_BGRA_EXT */
#define GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT 0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT 0x8366
#endif

#ifndef GL_EXT_render_snorm
#define GL_EXT_render_snorm
/* reuse GL_BYTE */
/* reuse GL_SHORT */
/* reuse GL_R8_SNORM */
/* reuse GL_RG8_SNORM */
/* reuse GL_RGBA8_SNORM */
#define GL_R16_SNORM_EXT 0x8F98
#define GL_RG16_SNORM_EXT 0x8F99
#define GL_RGBA16_SNORM_EXT 0x8F9B
#endif

#ifndef GL_EXT_robustness
#define GL_EXT_robustness
/* reuse GL_NO_ERROR */
#define GL_GUILTY_CONTEXT_RESET_EXT 0x8253
#define GL_INNOCENT_CONTEXT_RESET_EXT 0x8254
#define GL_UNKNOWN_CONTEXT_RESET_EXT 0x8255
#define GL_CONTEXT_ROBUST_ACCESS_EXT 0x90F3
#define GL_RESET_NOTIFICATION_STRATEGY_EXT 0x8256
#define GL_LOSE_CONTEXT_ON_RESET_EXT 0x8252
#define GL_NO_RESET_NOTIFICATION_EXT 0x8261
#endif
extern GLenum (APIENTRYP gglGetGraphicsResetStatusEXT)();
extern void (APIENTRYP gglReadnPixelsEXT)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data);
extern void (APIENTRYP gglGetnUniformfvEXT)(GLuint program, GLint location, GLsizei bufSize, GLfloat *params);
extern void (APIENTRYP gglGetnUniformivEXT)(GLuint program, GLint location, GLsizei bufSize, GLint *params);

#ifndef GL_EXT_sRGB
#define GL_EXT_sRGB
#define GL_SRGB_EXT 0x8C40
#define GL_SRGB_ALPHA_EXT 0x8C42
#define GL_SRGB8_ALPHA8_EXT 0x8C43
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_EXT 0x8210
#endif

#ifndef GL_EXT_sRGB_write_control
#define GL_EXT_sRGB_write_control
#define GL_FRAMEBUFFER_SRGB_EXT 0x8DB9
#endif

#ifndef GL_EXT_separate_shader_objects
#define GL_EXT_separate_shader_objects
#define GL_VERTEX_SHADER_BIT_EXT 0x00000001
#define GL_FRAGMENT_SHADER_BIT_EXT 0x00000002
#define GL_ALL_SHADER_BITS_EXT 0xFFFFFFFF
#define GL_PROGRAM_SEPARABLE_EXT 0x8258
#define GL_ACTIVE_PROGRAM_EXT 0x8259
#define GL_PROGRAM_PIPELINE_BINDING_EXT 0x825A
#endif
extern void (APIENTRYP gglActiveShaderProgramEXT)(GLuint pipeline, GLuint program);
extern void (APIENTRYP gglBindProgramPipelineEXT)(GLuint pipeline);
extern GLuint (APIENTRYP gglCreateShaderProgramvEXT)(GLenum type, GLsizei count, const GLchar **strings);
extern void (APIENTRYP gglDeleteProgramPipelinesEXT)(GLsizei n, const GLuint *pipelines);
extern void (APIENTRYP gglGenProgramPipelinesEXT)(GLsizei n, GLuint *pipelines);
extern void (APIENTRYP gglGetProgramPipelineInfoLogEXT)(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
extern void (APIENTRYP gglGetProgramPipelineivEXT)(GLuint pipeline, GLenum pname, GLint *params);
extern GLboolean (APIENTRYP gglIsProgramPipelineEXT)(GLuint pipeline);
extern void (APIENTRYP gglProgramParameteriEXT)(GLuint program, GLenum pname, GLint value);
extern void (APIENTRYP gglProgramUniform1fEXT)(GLuint program, GLint location, GLfloat v0);
extern void (APIENTRYP gglProgramUniform1fvEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglProgramUniform1iEXT)(GLuint program, GLint location, GLint v0);
extern void (APIENTRYP gglProgramUniform1ivEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglProgramUniform2fEXT)(GLuint program, GLint location, GLfloat v0, GLfloat v1);
extern void (APIENTRYP gglProgramUniform2fvEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglProgramUniform2iEXT)(GLuint program, GLint location, GLint v0, GLint v1);
extern void (APIENTRYP gglProgramUniform2ivEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglProgramUniform3fEXT)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
extern void (APIENTRYP gglProgramUniform3fvEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglProgramUniform3iEXT)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
extern void (APIENTRYP gglProgramUniform3ivEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglProgramUniform4fEXT)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
extern void (APIENTRYP gglProgramUniform4fvEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
extern void (APIENTRYP gglProgramUniform4iEXT)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
extern void (APIENTRYP gglProgramUniform4ivEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
extern void (APIENTRYP gglProgramUniformMatrix2fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix3fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix4fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUseProgramStagesEXT)(GLuint pipeline, GLbitfield stages, GLuint program);
extern void (APIENTRYP gglValidateProgramPipelineEXT)(GLuint pipeline);
extern void (APIENTRYP gglProgramUniform1uiEXT)(GLuint program, GLint location, GLuint v0);
extern void (APIENTRYP gglProgramUniform2uiEXT)(GLuint program, GLint location, GLuint v0, GLuint v1);
extern void (APIENTRYP gglProgramUniform3uiEXT)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2);
extern void (APIENTRYP gglProgramUniform4uiEXT)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
extern void (APIENTRYP gglProgramUniform1uivEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglProgramUniform2uivEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglProgramUniform3uivEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
extern void (APIENTRYP gglProgramUniform4uivEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
/* reuse void (APIENTRYP gglProgramUniformMatrix4fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) */
extern void (APIENTRYP gglProgramUniformMatrix2x3fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix3x2fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix2x4fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix4x2fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix3x4fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglProgramUniformMatrix4x3fvEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

#ifndef GL_EXT_shader_framebuffer_fetch
#define GL_EXT_shader_framebuffer_fetch
#define GL_FRAGMENT_SHADER_DISCARDS_SAMPLES_EXT 0x8A52
#endif

#ifndef GL_EXT_shader_group_vote
#define GL_EXT_shader_group_vote
#endif

#ifndef GL_EXT_shader_implicit_conversions
#define GL_EXT_shader_implicit_conversions
#endif

#ifndef GL_EXT_shader_integer_mix
#define GL_EXT_shader_integer_mix
#endif

#ifndef GL_EXT_shader_io_blocks
#define GL_EXT_shader_io_blocks
#endif

#ifndef GL_EXT_shader_non_constant_global_initializers
#define GL_EXT_shader_non_constant_global_initializers
#endif

#ifndef GL_EXT_shader_pixel_local_storage
#define GL_EXT_shader_pixel_local_storage
#define GL_MAX_SHADER_PIXEL_LOCAL_STORAGE_FAST_SIZE_EXT 0x8F63
#define GL_MAX_SHADER_PIXEL_LOCAL_STORAGE_SIZE_EXT 0x8F67
#define GL_SHADER_PIXEL_LOCAL_STORAGE_EXT 0x8F64
#endif

#ifndef GL_EXT_shader_pixel_local_storage2
#define GL_EXT_shader_pixel_local_storage2
#define GL_MAX_SHADER_COMBINED_LOCAL_STORAGE_FAST_SIZE_EXT 0x9650
#define GL_MAX_SHADER_COMBINED_LOCAL_STORAGE_SIZE_EXT 0x9651
#define GL_FRAMEBUFFER_INCOMPLETE_INSUFFICIENT_SHADER_COMBINED_LOCAL_STORAGE_EXT 0x9652
#endif
extern void (APIENTRYP gglFramebufferPixelLocalStorageSizeEXT)(GLuint target, GLsizei size);
extern GLsizei (APIENTRYP gglGetFramebufferPixelLocalStorageSizeEXT)(GLuint target);
extern void (APIENTRYP gglClearPixelLocalStorageuiEXT)(GLsizei offset, GLsizei n, const GLuint *values);

#ifndef GL_EXT_shader_texture_lod
#define GL_EXT_shader_texture_lod
#endif

#ifndef GL_EXT_shadow_samplers
#define GL_EXT_shadow_samplers
#define GL_TEXTURE_COMPARE_MODE_EXT 0x884C
#define GL_TEXTURE_COMPARE_FUNC_EXT 0x884D
#define GL_COMPARE_REF_TO_TEXTURE_EXT 0x884E
#define GL_SAMPLER_2D_SHADOW_EXT 0x8B62
#endif

#ifndef GL_EXT_sparse_texture
#define GL_EXT_sparse_texture
#define GL_TEXTURE_SPARSE_EXT 0x91A6
#define GL_VIRTUAL_PAGE_SIZE_INDEX_EXT 0x91A7
#define GL_NUM_SPARSE_LEVELS_EXT 0x91AA
#define GL_NUM_VIRTUAL_PAGE_SIZES_EXT 0x91A8
#define GL_VIRTUAL_PAGE_SIZE_X_EXT 0x9195
#define GL_VIRTUAL_PAGE_SIZE_Y_EXT 0x9196
#define GL_VIRTUAL_PAGE_SIZE_Z_EXT 0x9197
/* reuse GL_TEXTURE_2D */
/* reuse GL_TEXTURE_2D_ARRAY */
/* reuse GL_TEXTURE_CUBE_MAP */
#define GL_TEXTURE_CUBE_MAP_ARRAY_OES 0x9009
/* reuse GL_TEXTURE_3D */
#define GL_MAX_SPARSE_TEXTURE_SIZE_EXT 0x9198
#define GL_MAX_SPARSE_3D_TEXTURE_SIZE_EXT 0x9199
#define GL_MAX_SPARSE_ARRAY_TEXTURE_LAYERS_EXT 0x919A
#define GL_SPARSE_TEXTURE_FULL_ARRAY_CUBE_MIPMAPS_EXT 0x91A9
#endif
extern void (APIENTRYP gglTexPageCommitmentEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit);

#ifndef GL_EXT_tessellation_point_size
#define GL_EXT_tessellation_point_size
#endif

#ifndef GL_EXT_tessellation_shader
#define GL_EXT_tessellation_shader
#define GL_PATCHES_EXT 0x000E
#define GL_PATCH_VERTICES_EXT 0x8E72
#define GL_TESS_CONTROL_OUTPUT_VERTICES_EXT 0x8E75
#define GL_TESS_GEN_MODE_EXT 0x8E76
#define GL_TESS_GEN_SPACING_EXT 0x8E77
#define GL_TESS_GEN_VERTEX_ORDER_EXT 0x8E78
#define GL_TESS_GEN_POINT_MODE_EXT 0x8E79
/* reuse GL_TRIANGLES */
#define GL_ISOLINES_EXT 0x8E7A
#define GL_QUADS_EXT 0x0007
/* reuse GL_EQUAL */
#define GL_FRACTIONAL_ODD_EXT 0x8E7B
#define GL_FRACTIONAL_EVEN_EXT 0x8E7C
/* reuse GL_CCW */
/* reuse GL_CW */
#define GL_MAX_PATCH_VERTICES_EXT 0x8E7D
#define GL_MAX_TESS_GEN_LEVEL_EXT 0x8E7E
#define GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS_EXT 0x8E7F
#define GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS_EXT 0x8E80
#define GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS_EXT 0x8E81
#define GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS_EXT 0x8E82
#define GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS_EXT 0x8E83
#define GL_MAX_TESS_PATCH_COMPONENTS_EXT 0x8E84
#define GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS_EXT 0x8E85
#define GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS_EXT 0x8E86
#define GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS_EXT 0x8E89
#define GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS_EXT 0x8E8A
#define GL_MAX_TESS_CONTROL_INPUT_COMPONENTS_EXT 0x886C
#define GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS_EXT 0x886D
#define GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS_EXT 0x8E1E
#define GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS_EXT 0x8E1F
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS_EXT 0x92CD
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS_EXT 0x92CE
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS_EXT 0x92D3
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS_EXT 0x92D4
#define GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS_EXT 0x90CB
#define GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS_EXT 0x90CC
#define GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS_EXT 0x90D8
#define GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS_EXT 0x90D9
#define GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED 0x8221
#define GL_IS_PER_PATCH_EXT 0x92E7
#define GL_REFERENCED_BY_TESS_CONTROL_SHADER_EXT 0x9307
#define GL_REFERENCED_BY_TESS_EVALUATION_SHADER_EXT 0x9308
#define GL_TESS_CONTROL_SHADER_EXT 0x8E88
#define GL_TESS_EVALUATION_SHADER_EXT 0x8E87
#define GL_TESS_CONTROL_SHADER_BIT_EXT 0x00000008
#define GL_TESS_EVALUATION_SHADER_BIT_EXT 0x00000010
#endif
extern void (APIENTRYP gglPatchParameteriEXT)(GLenum pname, GLint value);

#ifndef GL_EXT_texture_border_clamp
#define GL_EXT_texture_border_clamp
#define GL_TEXTURE_BORDER_COLOR_EXT 0x1004
#define GL_CLAMP_TO_BORDER_EXT 0x812D
#endif
extern void (APIENTRYP gglTexParameterIivEXT)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglTexParameterIuivEXT)(GLenum target, GLenum pname, const GLuint *params);
extern void (APIENTRYP gglGetTexParameterIivEXT)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetTexParameterIuivEXT)(GLenum target, GLenum pname, GLuint *params);
extern void (APIENTRYP gglSamplerParameterIivEXT)(GLuint sampler, GLenum pname, const GLint *param);
extern void (APIENTRYP gglSamplerParameterIuivEXT)(GLuint sampler, GLenum pname, const GLuint *param);
extern void (APIENTRYP gglGetSamplerParameterIivEXT)(GLuint sampler, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetSamplerParameterIuivEXT)(GLuint sampler, GLenum pname, GLuint *params);

#ifndef GL_EXT_texture_buffer
#define GL_EXT_texture_buffer
#define GL_TEXTURE_BUFFER_EXT 0x8C2A
#define GL_TEXTURE_BUFFER_BINDING_EXT 0x8C2A
#define GL_MAX_TEXTURE_BUFFER_SIZE_EXT 0x8C2B
#define GL_TEXTURE_BINDING_BUFFER_EXT 0x8C2C
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING_EXT 0x8C2D
#define GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT_EXT 0x919F
#define GL_SAMPLER_BUFFER_EXT 0x8DC2
#define GL_INT_SAMPLER_BUFFER_EXT 0x8DD0
#define GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT 0x8DD8
#define GL_IMAGE_BUFFER_EXT 0x9051
#define GL_INT_IMAGE_BUFFER_EXT 0x905C
#define GL_UNSIGNED_INT_IMAGE_BUFFER_EXT 0x9067
#define GL_TEXTURE_BUFFER_OFFSET_EXT 0x919D
#define GL_TEXTURE_BUFFER_SIZE_EXT 0x919E
#endif
extern void (APIENTRYP gglTexBufferEXT)(GLenum target, GLenum internalformat, GLuint buffer);
extern void (APIENTRYP gglTexBufferRangeEXT)(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);

#ifndef GL_EXT_texture_compression_dxt1
#define GL_EXT_texture_compression_dxt1
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif

#ifndef GL_EXT_texture_compression_s3tc
#define GL_EXT_texture_compression_s3tc
/* reuse GL_COMPRESSED_RGB_S3TC_DXT1_EXT */
/* reuse GL_COMPRESSED_RGBA_S3TC_DXT1_EXT */
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

#ifndef GL_EXT_texture_cube_map_array
#define GL_EXT_texture_cube_map_array
#define GL_TEXTURE_CUBE_MAP_ARRAY_EXT 0x9009
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_EXT 0x900A
#define GL_SAMPLER_CUBE_MAP_ARRAY_EXT 0x900C
#define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW_EXT 0x900D
#define GL_INT_SAMPLER_CUBE_MAP_ARRAY_EXT 0x900E
#define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY_EXT 0x900F
#define GL_IMAGE_CUBE_MAP_ARRAY_EXT 0x9054
#define GL_INT_IMAGE_CUBE_MAP_ARRAY_EXT 0x905F
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY_EXT 0x906A
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

#ifndef GL_EXT_texture_format_BGRA8888
#define GL_EXT_texture_format_BGRA8888
/* reuse GL_BGRA_EXT */
#endif

#ifndef GL_EXT_texture_norm16
#define GL_EXT_texture_norm16
#define GL_R16_EXT 0x822A
#define GL_RG16_EXT 0x822C
#define GL_RGBA16_EXT 0x805B
#define GL_RGB16_EXT 0x8054
/* reuse GL_R16_SNORM_EXT */
/* reuse GL_RG16_SNORM_EXT */
#define GL_RGB16_SNORM_EXT 0x8F9A
/* reuse GL_RGBA16_SNORM_EXT */
#endif

#ifndef GL_EXT_texture_rg
#define GL_EXT_texture_rg
#define GL_RED_EXT 0x1903
#define GL_RG_EXT 0x8227
#define GL_R8_EXT 0x8229
#define GL_RG8_EXT 0x822B
#endif

#ifndef GL_EXT_texture_sRGB_R8
#define GL_EXT_texture_sRGB_R8
#define GL_SR8_EXT 0x8FBD
#endif

#ifndef GL_EXT_texture_sRGB_RG8
#define GL_EXT_texture_sRGB_RG8
#define GL_SRG8_EXT 0x8FBE
#endif

#ifndef GL_EXT_texture_sRGB_decode
#define GL_EXT_texture_sRGB_decode
#define GL_TEXTURE_SRGB_DECODE_EXT 0x8A48
#define GL_DECODE_EXT 0x8A49
#define GL_SKIP_DECODE_EXT 0x8A4A
#endif

#ifndef GL_EXT_texture_storage
#define GL_EXT_texture_storage
#define GL_TEXTURE_IMMUTABLE_FORMAT_EXT 0x912F
#define GL_ALPHA8_EXT 0x803C
#define GL_LUMINANCE8_EXT 0x8040
#define GL_LUMINANCE8_ALPHA8_EXT 0x8045
#define GL_RGBA32F_EXT 0x8814
#define GL_RGB32F_EXT 0x8815
#define GL_ALPHA32F_EXT 0x8816
#define GL_LUMINANCE32F_EXT 0x8818
#define GL_LUMINANCE_ALPHA32F_EXT 0x8819
/* reuse GL_RGBA16F_EXT */
/* reuse GL_RGB16F_EXT */
#define GL_ALPHA16F_EXT 0x881C
#define GL_LUMINANCE16F_EXT 0x881E
#define GL_LUMINANCE_ALPHA16F_EXT 0x881F
#define GL_RGB10_A2_EXT 0x8059
#define GL_RGB10_EXT 0x8052
/* reuse GL_BGRA8_EXT */
/* reuse GL_R8_EXT */
/* reuse GL_RG8_EXT */
#define GL_R32F_EXT 0x822E
#define GL_RG32F_EXT 0x8230
/* reuse GL_R16F_EXT */
/* reuse GL_RG16F_EXT */
#endif
extern void (APIENTRYP gglTexStorage1DEXT)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width);
extern void (APIENTRYP gglTexStorage2DEXT)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglTexStorage3DEXT)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
extern void (APIENTRYP gglTextureStorage1DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width);
extern void (APIENTRYP gglTextureStorage2DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglTextureStorage3DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

#ifndef GL_EXT_texture_type_2_10_10_10_REV
#define GL_EXT_texture_type_2_10_10_10_REV
#define GL_UNSIGNED_INT_2_10_10_10_REV_EXT 0x8368
#endif

#ifndef GL_EXT_texture_view
#define GL_EXT_texture_view
#define GL_TEXTURE_VIEW_MIN_LEVEL_EXT 0x82DB
#define GL_TEXTURE_VIEW_NUM_LEVELS_EXT 0x82DC
#define GL_TEXTURE_VIEW_MIN_LAYER_EXT 0x82DD
#define GL_TEXTURE_VIEW_NUM_LAYERS_EXT 0x82DE
/* reuse GL_TEXTURE_IMMUTABLE_LEVELS */
#endif
extern void (APIENTRYP gglTextureViewEXT)(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers);

#ifndef GL_EXT_unpack_subimage
#define GL_EXT_unpack_subimage
#define GL_UNPACK_ROW_LENGTH_EXT 0x0CF2
#define GL_UNPACK_SKIP_ROWS_EXT 0x0CF3
#define GL_UNPACK_SKIP_PIXELS_EXT 0x0CF4
#endif

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

#ifndef GL_FJ_shader_binary_GCCSO
#define GL_FJ_shader_binary_GCCSO
#define GL_GCCSO_SHADER_BINARY_FJ 0x9260
#endif

#ifndef GL_IMG_bindless_texture
#define GL_IMG_bindless_texture
#endif
extern GLuint64 (APIENTRYP gglGetTextureHandleIMG)(GLuint texture);
extern GLuint64 (APIENTRYP gglGetTextureSamplerHandleIMG)(GLuint texture, GLuint sampler);
extern void (APIENTRYP gglUniformHandleui64IMG)(GLint location, GLuint64 value);
extern void (APIENTRYP gglUniformHandleui64vIMG)(GLint location, GLsizei count, const GLuint64 *value);
extern void (APIENTRYP gglProgramUniformHandleui64IMG)(GLuint program, GLint location, GLuint64 value);
extern void (APIENTRYP gglProgramUniformHandleui64vIMG)(GLuint program, GLint location, GLsizei count, const GLuint64 *values);

#ifndef GL_IMG_framebuffer_downsample
#define GL_IMG_framebuffer_downsample
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_AND_DOWNSAMPLE_IMG 0x913C
#define GL_NUM_DOWNSAMPLE_SCALES_IMG 0x913D
#define GL_DOWNSAMPLE_SCALES_IMG 0x913E
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_SCALE_IMG 0x913F
#endif
extern void (APIENTRYP gglFramebufferTexture2DDownsampleIMG)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint xscale, GLint yscale);
extern void (APIENTRYP gglFramebufferTextureLayerDownsampleIMG)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer, GLint xscale, GLint yscale);

#ifndef GL_IMG_multisampled_render_to_texture
#define GL_IMG_multisampled_render_to_texture
#define GL_RENDERBUFFER_SAMPLES_IMG 0x9133
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_IMG 0x9134
#define GL_MAX_SAMPLES_IMG 0x9135
#define GL_TEXTURE_SAMPLES_IMG 0x9136
#endif
extern void (APIENTRYP gglRenderbufferStorageMultisampleIMG)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
extern void (APIENTRYP gglFramebufferTexture2DMultisampleIMG)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);

#ifndef GL_IMG_program_binary
#define GL_IMG_program_binary
#define GL_SGX_PROGRAM_BINARY_IMG 0x9130
#endif

#ifndef GL_IMG_read_format
#define GL_IMG_read_format
#define GL_BGRA_IMG 0x80E1
#define GL_UNSIGNED_SHORT_4_4_4_4_REV_IMG 0x8365
#endif

#ifndef GL_IMG_shader_binary
#define GL_IMG_shader_binary
#define GL_SGX_BINARY_IMG 0x8C0A
#endif

#ifndef GL_IMG_texture_compression_pvrtc
#define GL_IMG_texture_compression_pvrtc
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#endif

#ifndef GL_IMG_texture_compression_pvrtc2
#define GL_IMG_texture_compression_pvrtc2
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG 0x9137
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG 0x9138
#endif

#ifndef GL_IMG_texture_filter_cubic
#define GL_IMG_texture_filter_cubic
#define GL_CUBIC_IMG 0x9139
#define GL_CUBIC_MIPMAP_NEAREST_IMG 0x913A
#define GL_CUBIC_MIPMAP_LINEAR_IMG 0x913B
#endif

#ifndef GL_INTEL_conservative_rasterization
#define GL_INTEL_conservative_rasterization
#define GL_CONSERVATIVE_RASTERIZATION_INTEL 0x83FE
#endif

#ifndef GL_INTEL_framebuffer_CMAA
#define GL_INTEL_framebuffer_CMAA
#endif
extern void (APIENTRYP gglApplyFramebufferAttachmentCMAAINTEL)();

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
#define GL_CONTEXT_RELEASE_BEHAVIOR_KHR 0x82FB
#define GL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_KHR 0x82FC
/* reuse GL_NONE */
#endif

#ifndef GL_KHR_debug
#define GL_KHR_debug
#define GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR 0x8242
#define GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_KHR 0x8243
#define GL_DEBUG_CALLBACK_FUNCTION_KHR 0x8244
#define GL_DEBUG_CALLBACK_USER_PARAM_KHR 0x8245
#define GL_DEBUG_SOURCE_API_KHR 0x8246
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM_KHR 0x8247
#define GL_DEBUG_SOURCE_SHADER_COMPILER_KHR 0x8248
#define GL_DEBUG_SOURCE_THIRD_PARTY_KHR 0x8249
#define GL_DEBUG_SOURCE_APPLICATION_KHR 0x824A
#define GL_DEBUG_SOURCE_OTHER_KHR 0x824B
#define GL_DEBUG_TYPE_ERROR_KHR 0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_KHR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_KHR 0x824E
#define GL_DEBUG_TYPE_PORTABILITY_KHR 0x824F
#define GL_DEBUG_TYPE_PERFORMANCE_KHR 0x8250
#define GL_DEBUG_TYPE_OTHER_KHR 0x8251
#define GL_DEBUG_TYPE_MARKER_KHR 0x8268
#define GL_DEBUG_TYPE_PUSH_GROUP_KHR 0x8269
#define GL_DEBUG_TYPE_POP_GROUP_KHR 0x826A
#define GL_DEBUG_SEVERITY_NOTIFICATION_KHR 0x826B
#define GL_MAX_DEBUG_GROUP_STACK_DEPTH_KHR 0x826C
#define GL_DEBUG_GROUP_STACK_DEPTH_KHR 0x826D
#define GL_BUFFER_KHR 0x82E0
#define GL_SHADER_KHR 0x82E1
#define GL_PROGRAM_KHR 0x82E2
#define GL_VERTEX_ARRAY_KHR 0x8074
#define GL_QUERY_KHR 0x82E3
#define GL_PROGRAM_PIPELINE_KHR 0x82E4
#define GL_SAMPLER_KHR 0x82E6
#define GL_MAX_LABEL_LENGTH_KHR 0x82E8
#define GL_MAX_DEBUG_MESSAGE_LENGTH_KHR 0x9143
#define GL_MAX_DEBUG_LOGGED_MESSAGES_KHR 0x9144
#define GL_DEBUG_LOGGED_MESSAGES_KHR 0x9145
#define GL_DEBUG_SEVERITY_HIGH_KHR 0x9146
#define GL_DEBUG_SEVERITY_MEDIUM_KHR 0x9147
#define GL_DEBUG_SEVERITY_LOW_KHR 0x9148
#define GL_DEBUG_OUTPUT_KHR 0x92E0
#define GL_CONTEXT_FLAG_DEBUG_BIT_KHR 0x00000002
#define GL_STACK_OVERFLOW_KHR 0x0503
#define GL_STACK_UNDERFLOW_KHR 0x0504
#endif
extern void (APIENTRYP gglDebugMessageControlKHR)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
extern void (APIENTRYP gglDebugMessageInsertKHR)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf);
extern void (APIENTRYP gglDebugMessageCallbackKHR)(GLDEBUGPROCKHR callback, const void *userParam);
extern GLuint (APIENTRYP gglGetDebugMessageLogKHR)(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog);
extern void (APIENTRYP gglPushDebugGroupKHR)(GLenum source, GLuint id, GLsizei length, const GLchar *message);
extern void (APIENTRYP gglPopDebugGroupKHR)();
extern void (APIENTRYP gglObjectLabelKHR)(GLenum identifier, GLuint name, GLsizei length, const GLchar *label);
extern void (APIENTRYP gglGetObjectLabelKHR)(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label);
extern void (APIENTRYP gglObjectPtrLabelKHR)(const void *ptr, GLsizei length, const GLchar *label);
extern void (APIENTRYP gglGetObjectPtrLabelKHR)(const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label);
extern void (APIENTRYP gglGetPointervKHR)(GLenum pname, void **params);

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
#define GL_CONTEXT_ROBUST_ACCESS_KHR 0x90F3
#define GL_LOSE_CONTEXT_ON_RESET_KHR 0x8252
#define GL_GUILTY_CONTEXT_RESET_KHR 0x8253
#define GL_INNOCENT_CONTEXT_RESET_KHR 0x8254
#define GL_UNKNOWN_CONTEXT_RESET_KHR 0x8255
#define GL_RESET_NOTIFICATION_STRATEGY_KHR 0x8256
#define GL_NO_RESET_NOTIFICATION_KHR 0x8261
#define GL_CONTEXT_LOST_KHR 0x0507
#endif
extern GLenum (APIENTRYP gglGetGraphicsResetStatusKHR)();
extern void (APIENTRYP gglReadnPixelsKHR)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data);
extern void (APIENTRYP gglGetnUniformfvKHR)(GLuint program, GLint location, GLsizei bufSize, GLfloat *params);
extern void (APIENTRYP gglGetnUniformivKHR)(GLuint program, GLint location, GLsizei bufSize, GLint *params);
extern void (APIENTRYP gglGetnUniformuivKHR)(GLuint program, GLint location, GLsizei bufSize, GLuint *params);

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

#ifndef GL_NV_conservative_raster_pre_snap_triangles
#define GL_NV_conservative_raster_pre_snap_triangles
#define GL_CONSERVATIVE_RASTER_MODE_NV 0x954D
#define GL_CONSERVATIVE_RASTER_MODE_POST_SNAP_NV 0x954E
#define GL_CONSERVATIVE_RASTER_MODE_PRE_SNAP_TRIANGLES_NV 0x954F
/* reuse GL_CONSERVATIVE_RASTER_MODE_NV */
#endif
extern void (APIENTRYP gglConservativeRasterParameteriNV)(GLenum pname, GLint param);

#ifndef GL_NV_copy_buffer
#define GL_NV_copy_buffer
#define GL_COPY_READ_BUFFER_NV 0x8F36
#define GL_COPY_WRITE_BUFFER_NV 0x8F37
#endif
extern void (APIENTRYP gglCopyBufferSubDataNV)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);

#ifndef GL_NV_coverage_sample
#define GL_NV_coverage_sample
#define GL_COVERAGE_COMPONENT_NV 0x8ED0
#define GL_COVERAGE_COMPONENT4_NV 0x8ED1
#define GL_COVERAGE_ATTACHMENT_NV 0x8ED2
#define GL_COVERAGE_BUFFERS_NV 0x8ED3
#define GL_COVERAGE_SAMPLES_NV 0x8ED4
#define GL_COVERAGE_ALL_FRAGMENTS_NV 0x8ED5
#define GL_COVERAGE_EDGE_FRAGMENTS_NV 0x8ED6
#define GL_COVERAGE_AUTOMATIC_NV 0x8ED7
#define GL_COVERAGE_BUFFER_BIT_NV 0x00008000
#endif
extern void (APIENTRYP gglCoverageMaskNV)(GLboolean mask);
extern void (APIENTRYP gglCoverageOperationNV)(GLenum operation);

#ifndef GL_NV_depth_nonlinear
#define GL_NV_depth_nonlinear
#define GL_DEPTH_COMPONENT16_NONLINEAR_NV 0x8E2C
#endif

#ifndef GL_NV_draw_buffers
#define GL_NV_draw_buffers
#define GL_MAX_DRAW_BUFFERS_NV 0x8824
#define GL_DRAW_BUFFER0_NV 0x8825
#define GL_DRAW_BUFFER1_NV 0x8826
#define GL_DRAW_BUFFER2_NV 0x8827
#define GL_DRAW_BUFFER3_NV 0x8828
#define GL_DRAW_BUFFER4_NV 0x8829
#define GL_DRAW_BUFFER5_NV 0x882A
#define GL_DRAW_BUFFER6_NV 0x882B
#define GL_DRAW_BUFFER7_NV 0x882C
#define GL_DRAW_BUFFER8_NV 0x882D
#define GL_DRAW_BUFFER9_NV 0x882E
#define GL_DRAW_BUFFER10_NV 0x882F
#define GL_DRAW_BUFFER11_NV 0x8830
#define GL_DRAW_BUFFER12_NV 0x8831
#define GL_DRAW_BUFFER13_NV 0x8832
#define GL_DRAW_BUFFER14_NV 0x8833
#define GL_DRAW_BUFFER15_NV 0x8834
#define GL_COLOR_ATTACHMENT0_NV 0x8CE0
#define GL_COLOR_ATTACHMENT1_NV 0x8CE1
#define GL_COLOR_ATTACHMENT2_NV 0x8CE2
#define GL_COLOR_ATTACHMENT3_NV 0x8CE3
#define GL_COLOR_ATTACHMENT4_NV 0x8CE4
#define GL_COLOR_ATTACHMENT5_NV 0x8CE5
#define GL_COLOR_ATTACHMENT6_NV 0x8CE6
#define GL_COLOR_ATTACHMENT7_NV 0x8CE7
#define GL_COLOR_ATTACHMENT8_NV 0x8CE8
#define GL_COLOR_ATTACHMENT9_NV 0x8CE9
#define GL_COLOR_ATTACHMENT10_NV 0x8CEA
#define GL_COLOR_ATTACHMENT11_NV 0x8CEB
#define GL_COLOR_ATTACHMENT12_NV 0x8CEC
#define GL_COLOR_ATTACHMENT13_NV 0x8CED
#define GL_COLOR_ATTACHMENT14_NV 0x8CEE
#define GL_COLOR_ATTACHMENT15_NV 0x8CEF
#endif
extern void (APIENTRYP gglDrawBuffersNV)(GLsizei n, const GLenum *bufs);

#ifndef GL_NV_draw_instanced
#define GL_NV_draw_instanced
#endif
extern void (APIENTRYP gglDrawArraysInstancedNV)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
extern void (APIENTRYP gglDrawElementsInstancedNV)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);

#ifndef GL_NV_explicit_attrib_location
#define GL_NV_explicit_attrib_location
#endif

#ifndef GL_NV_fbo_color_attachments
#define GL_NV_fbo_color_attachments
#define GL_MAX_COLOR_ATTACHMENTS_NV 0x8CDF
/* reuse GL_COLOR_ATTACHMENT0_NV */
/* reuse GL_COLOR_ATTACHMENT1_NV */
/* reuse GL_COLOR_ATTACHMENT2_NV */
/* reuse GL_COLOR_ATTACHMENT3_NV */
/* reuse GL_COLOR_ATTACHMENT4_NV */
/* reuse GL_COLOR_ATTACHMENT5_NV */
/* reuse GL_COLOR_ATTACHMENT6_NV */
/* reuse GL_COLOR_ATTACHMENT7_NV */
/* reuse GL_COLOR_ATTACHMENT8_NV */
/* reuse GL_COLOR_ATTACHMENT9_NV */
/* reuse GL_COLOR_ATTACHMENT10_NV */
/* reuse GL_COLOR_ATTACHMENT11_NV */
/* reuse GL_COLOR_ATTACHMENT12_NV */
/* reuse GL_COLOR_ATTACHMENT13_NV */
/* reuse GL_COLOR_ATTACHMENT14_NV */
/* reuse GL_COLOR_ATTACHMENT15_NV */
#endif

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

#ifndef GL_NV_fragment_coverage_to_color
#define GL_NV_fragment_coverage_to_color
#define GL_FRAGMENT_COVERAGE_TO_COLOR_NV 0x92DD
#define GL_FRAGMENT_COVERAGE_COLOR_NV 0x92DE
#endif
extern void (APIENTRYP gglFragmentCoverageColorNV)(GLuint color);

#ifndef GL_NV_fragment_shader_interlock
#define GL_NV_fragment_shader_interlock
#endif

#ifndef GL_NV_framebuffer_blit
#define GL_NV_framebuffer_blit
#define GL_READ_FRAMEBUFFER_NV 0x8CA8
#define GL_DRAW_FRAMEBUFFER_NV 0x8CA9
#define GL_DRAW_FRAMEBUFFER_BINDING_NV 0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING_NV 0x8CAA
#endif
extern void (APIENTRYP gglBlitFramebufferNV)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

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

#ifndef GL_NV_framebuffer_multisample
#define GL_NV_framebuffer_multisample
#define GL_RENDERBUFFER_SAMPLES_NV 0x8CAB
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_NV 0x8D56
#define GL_MAX_SAMPLES_NV 0x8D57
#endif
extern void (APIENTRYP gglRenderbufferStorageMultisampleNV)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

#ifndef GL_NV_generate_mipmap_sRGB
#define GL_NV_generate_mipmap_sRGB
#endif

#ifndef GL_NV_geometry_shader_passthrough
#define GL_NV_geometry_shader_passthrough
#endif

#ifndef GL_NV_gpu_shader5
#define GL_NV_gpu_shader5
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
#define GL_FLOAT16_NV 0x8FF8
#define GL_FLOAT16_VEC2_NV 0x8FF9
#define GL_FLOAT16_VEC3_NV 0x8FFA
#define GL_FLOAT16_VEC4_NV 0x8FFB
#define GL_PATCHES 0x000E
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

#ifndef GL_NV_image_formats
#define GL_NV_image_formats
#endif

#ifndef GL_NV_instanced_arrays
#define GL_NV_instanced_arrays
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR_NV 0x88FE
#endif
extern void (APIENTRYP gglVertexAttribDivisorNV)(GLuint index, GLuint divisor);

#ifndef GL_NV_internalformat_sample_query
#define GL_NV_internalformat_sample_query
/* reuse GL_RENDERBUFFER */
#define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9102
#define GL_MULTISAMPLES_NV 0x9371
#define GL_SUPERSAMPLE_SCALE_X_NV 0x9372
#define GL_SUPERSAMPLE_SCALE_Y_NV 0x9373
#define GL_CONFORMANT_NV 0x9374
#endif
extern void (APIENTRYP gglGetInternalformatSampleivNV)(GLenum target, GLenum internalformat, GLsizei samples, GLenum pname, GLsizei bufSize, GLint *params);

#ifndef GL_NV_non_square_matrices
#define GL_NV_non_square_matrices
#define GL_FLOAT_MAT2x3_NV 0x8B65
#define GL_FLOAT_MAT2x4_NV 0x8B66
#define GL_FLOAT_MAT3x2_NV 0x8B67
#define GL_FLOAT_MAT3x4_NV 0x8B68
#define GL_FLOAT_MAT4x2_NV 0x8B69
#define GL_FLOAT_MAT4x3_NV 0x8B6A
#endif
extern void (APIENTRYP gglUniformMatrix2x3fvNV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix3x2fvNV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix2x4fvNV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix4x2fvNV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix3x4fvNV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
extern void (APIENTRYP gglUniformMatrix4x3fvNV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

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

#ifndef GL_NV_path_rendering_shared_edge
#define GL_NV_path_rendering_shared_edge
#define GL_SHARED_EDGE_NV 0xC0
#endif

#ifndef GL_NV_polygon_mode
#define GL_NV_polygon_mode
#define GL_POLYGON_MODE_NV 0x0B40
#define GL_POLYGON_OFFSET_POINT_NV 0x2A01
#define GL_POLYGON_OFFSET_LINE_NV 0x2A02
#define GL_POINT_NV 0x1B00
#define GL_LINE_NV 0x1B01
#define GL_FILL_NV 0x1B02
#endif
extern void (APIENTRYP gglPolygonModeNV)(GLenum face, GLenum mode);

#ifndef GL_NV_read_buffer
#define GL_NV_read_buffer
#define GL_READ_BUFFER_NV 0x0C02
#endif
extern void (APIENTRYP gglReadBufferNV)(GLenum mode);

#ifndef GL_NV_read_buffer_front
#define GL_NV_read_buffer_front
#endif

#ifndef GL_NV_read_depth
#define GL_NV_read_depth
#endif

#ifndef GL_NV_read_depth_stencil
#define GL_NV_read_depth_stencil
#endif

#ifndef GL_NV_read_stencil
#define GL_NV_read_stencil
#endif

#ifndef GL_NV_sRGB_formats
#define GL_NV_sRGB_formats
#define GL_SLUMINANCE_NV 0x8C46
#define GL_SLUMINANCE_ALPHA_NV 0x8C44
#define GL_SRGB8_NV 0x8C41
#define GL_SLUMINANCE8_NV 0x8C47
#define GL_SLUMINANCE8_ALPHA8_NV 0x8C45
#define GL_COMPRESSED_SRGB_S3TC_DXT1_NV 0x8C4C
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_NV 0x8C4D
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_NV 0x8C4E
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_NV 0x8C4F
#define GL_ETC1_SRGB8_NV 0x88EE
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

#ifndef GL_NV_shader_atomic_fp16_vector
#define GL_NV_shader_atomic_fp16_vector
#endif

#ifndef GL_NV_shader_noperspective_interpolation
#define GL_NV_shader_noperspective_interpolation
#endif

#ifndef GL_NV_shadow_samplers_array
#define GL_NV_shadow_samplers_array
#define GL_SAMPLER_2D_ARRAY_SHADOW_NV 0x8DC4
#endif

#ifndef GL_NV_shadow_samplers_cube
#define GL_NV_shadow_samplers_cube
#define GL_SAMPLER_CUBE_SHADOW_NV 0x8DC5
#endif

#ifndef GL_NV_texture_border_clamp
#define GL_NV_texture_border_clamp
#define GL_TEXTURE_BORDER_COLOR_NV 0x1004
#define GL_CLAMP_TO_BORDER_NV 0x812D
#endif

#ifndef GL_NV_texture_compression_s3tc_update
#define GL_NV_texture_compression_s3tc_update
#endif

#ifndef GL_NV_texture_npot_2D_mipmap
#define GL_NV_texture_npot_2D_mipmap
#endif

#ifndef GL_NV_viewport_array
#define GL_NV_viewport_array
#define GL_MAX_VIEWPORTS_NV 0x825B
#define GL_VIEWPORT_SUBPIXEL_BITS_NV 0x825C
#define GL_VIEWPORT_BOUNDS_RANGE_NV 0x825D
#define GL_VIEWPORT_INDEX_PROVOKING_VERTEX_NV 0x825F
/* reuse GL_SCISSOR_BOX */
/* reuse GL_VIEWPORT */
/* reuse GL_DEPTH_RANGE */
/* reuse GL_SCISSOR_TEST */
#endif
extern void (APIENTRYP gglViewportArrayvNV)(GLuint first, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglViewportIndexedfNV)(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h);
extern void (APIENTRYP gglViewportIndexedfvNV)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglScissorArrayvNV)(GLuint first, GLsizei count, const GLint *v);
extern void (APIENTRYP gglScissorIndexedNV)(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height);
extern void (APIENTRYP gglScissorIndexedvNV)(GLuint index, const GLint *v);
extern void (APIENTRYP gglDepthRangeArrayfvNV)(GLuint first, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglDepthRangeIndexedfNV)(GLuint index, GLfloat n, GLfloat f);
extern void (APIENTRYP gglGetFloati_vNV)(GLenum target, GLuint index, GLfloat *data);
extern void (APIENTRYP gglEnableiNV)(GLenum target, GLuint index);
extern void (APIENTRYP gglDisableiNV)(GLenum target, GLuint index);
extern GLboolean (APIENTRYP gglIsEnablediNV)(GLenum target, GLuint index);

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

#ifndef GL_OES_EGL_image
#define GL_OES_EGL_image
#endif
extern void (APIENTRYP gglEGLImageTargetTexture2DOES)(GLenum target, GLeglImageOES image);
extern void (APIENTRYP gglEGLImageTargetRenderbufferStorageOES)(GLenum target, GLeglImageOES image);

#ifndef GL_OES_EGL_image_external
#define GL_OES_EGL_image_external
/* reuse GL_TEXTURE_EXTERNAL_OES */
/* reuse GL_TEXTURE_BINDING_EXTERNAL_OES */
/* reuse GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES */
#define GL_SAMPLER_EXTERNAL_OES 0x8D66
#endif

#ifndef GL_OES_EGL_image_external_essl3
#define GL_OES_EGL_image_external_essl3
#endif

#ifndef GL_OES_compressed_ETC1_RGB8_sub_texture
#define GL_OES_compressed_ETC1_RGB8_sub_texture
#endif

#ifndef GL_OES_compressed_ETC1_RGB8_texture
#define GL_OES_compressed_ETC1_RGB8_texture
#define GL_ETC1_RGB8_OES 0x8D64
#endif

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

#ifndef GL_OES_copy_image
#define GL_OES_copy_image
#endif
extern void (APIENTRYP gglCopyImageSubDataOES)(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);

#ifndef GL_OES_depth24
#define GL_OES_depth24
#define GL_DEPTH_COMPONENT24_OES 0x81A6
#endif

#ifndef GL_OES_depth32
#define GL_OES_depth32
/* reuse GL_DEPTH_COMPONENT32_OES */
#endif

#ifndef GL_OES_depth_texture
#define GL_OES_depth_texture
/* reuse GL_DEPTH_COMPONENT */
/* reuse GL_UNSIGNED_SHORT */
/* reuse GL_UNSIGNED_INT */
#endif

#ifndef GL_OES_draw_buffers_indexed
#define GL_OES_draw_buffers_indexed
/* reuse GL_BLEND_EQUATION_RGB */
/* reuse GL_BLEND_EQUATION_ALPHA */
/* reuse GL_BLEND_SRC_RGB */
/* reuse GL_BLEND_SRC_ALPHA */
/* reuse GL_BLEND_DST_RGB */
/* reuse GL_BLEND_DST_ALPHA */
/* reuse GL_COLOR_WRITEMASK */
/* reuse GL_BLEND */
/* reuse GL_FUNC_ADD */
/* reuse GL_FUNC_SUBTRACT */
/* reuse GL_FUNC_REVERSE_SUBTRACT */
/* reuse GL_MIN */
/* reuse GL_MAX */
/* reuse GL_ZERO */
/* reuse GL_ONE */
/* reuse GL_SRC_COLOR */
/* reuse GL_ONE_MINUS_SRC_COLOR */
/* reuse GL_DST_COLOR */
/* reuse GL_ONE_MINUS_DST_COLOR */
/* reuse GL_SRC_ALPHA */
/* reuse GL_ONE_MINUS_SRC_ALPHA */
/* reuse GL_DST_ALPHA */
/* reuse GL_ONE_MINUS_DST_ALPHA */
/* reuse GL_CONSTANT_COLOR */
/* reuse GL_ONE_MINUS_CONSTANT_COLOR */
/* reuse GL_CONSTANT_ALPHA */
/* reuse GL_ONE_MINUS_CONSTANT_ALPHA */
/* reuse GL_SRC_ALPHA_SATURATE */
#endif
extern void (APIENTRYP gglEnableiOES)(GLenum target, GLuint index);
extern void (APIENTRYP gglDisableiOES)(GLenum target, GLuint index);
extern void (APIENTRYP gglBlendEquationiOES)(GLuint buf, GLenum mode);
extern void (APIENTRYP gglBlendEquationSeparateiOES)(GLuint buf, GLenum modeRGB, GLenum modeAlpha);
extern void (APIENTRYP gglBlendFunciOES)(GLuint buf, GLenum src, GLenum dst);
extern void (APIENTRYP gglBlendFuncSeparateiOES)(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
extern void (APIENTRYP gglColorMaskiOES)(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a);
extern GLboolean (APIENTRYP gglIsEnablediOES)(GLenum target, GLuint index);

#ifndef GL_OES_draw_elements_base_vertex
#define GL_OES_draw_elements_base_vertex
#endif
extern void (APIENTRYP gglDrawElementsBaseVertexOES)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
extern void (APIENTRYP gglDrawRangeElementsBaseVertexOES)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex);
extern void (APIENTRYP gglDrawElementsInstancedBaseVertexOES)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex);
extern void (APIENTRYP gglMultiDrawElementsBaseVertexOES)(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount, const GLint *basevertex);

#ifndef GL_OES_element_index_uint
#define GL_OES_element_index_uint
/* reuse GL_UNSIGNED_INT */
#endif

#ifndef GL_OES_fbo_render_mipmap
#define GL_OES_fbo_render_mipmap
#endif

#ifndef GL_OES_fragment_precision_high
#define GL_OES_fragment_precision_high
#endif

#ifndef GL_OES_geometry_point_size
#define GL_OES_geometry_point_size
#endif

#ifndef GL_OES_geometry_shader
#define GL_OES_geometry_shader
#define GL_GEOMETRY_SHADER_OES 0x8DD9
#define GL_GEOMETRY_SHADER_BIT_OES 0x00000004
#define GL_GEOMETRY_LINKED_VERTICES_OUT_OES 0x8916
#define GL_GEOMETRY_LINKED_INPUT_TYPE_OES 0x8917
#define GL_GEOMETRY_LINKED_OUTPUT_TYPE_OES 0x8918
#define GL_GEOMETRY_SHADER_INVOCATIONS_OES 0x887F
#define GL_LAYER_PROVOKING_VERTEX_OES 0x825E
#define GL_LINES_ADJACENCY_OES 0x000A
#define GL_LINE_STRIP_ADJACENCY_OES 0x000B
#define GL_TRIANGLES_ADJACENCY_OES 0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY_OES 0x000D
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_OES 0x8DDF
#define GL_MAX_GEOMETRY_UNIFORM_BLOCKS_OES 0x8A2C
#define GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS_OES 0x8A32
#define GL_MAX_GEOMETRY_INPUT_COMPONENTS_OES 0x9123
#define GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_OES 0x9124
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES_OES 0x8DE0
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_OES 0x8DE1
#define GL_MAX_GEOMETRY_SHADER_INVOCATIONS_OES 0x8E5A
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_OES 0x8C29
#define GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_OES 0x92CF
#define GL_MAX_GEOMETRY_ATOMIC_COUNTERS_OES 0x92D5
#define GL_MAX_GEOMETRY_IMAGE_UNIFORMS_OES 0x90CD
#define GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_OES 0x90D7
#define GL_FIRST_VERTEX_CONVENTION_OES 0x8E4D
#define GL_LAST_VERTEX_CONVENTION_OES 0x8E4E
#define GL_UNDEFINED_VERTEX_OES 0x8260
#define GL_PRIMITIVES_GENERATED_OES 0x8C87
#define GL_FRAMEBUFFER_DEFAULT_LAYERS_OES 0x9312
#define GL_MAX_FRAMEBUFFER_LAYERS_OES 0x9317
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_OES 0x8DA8
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED_OES 0x8DA7
#define GL_REFERENCED_BY_GEOMETRY_SHADER_OES 0x9309
#endif
extern void (APIENTRYP gglFramebufferTextureOES)(GLenum target, GLenum attachment, GLuint texture, GLint level);

#ifndef GL_OES_get_program_binary
#define GL_OES_get_program_binary
#define GL_PROGRAM_BINARY_LENGTH_OES 0x8741
#define GL_NUM_PROGRAM_BINARY_FORMATS_OES 0x87FE
#define GL_PROGRAM_BINARY_FORMATS_OES 0x87FF
#endif
extern void (APIENTRYP gglGetProgramBinaryOES)(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
extern void (APIENTRYP gglProgramBinaryOES)(GLuint program, GLenum binaryFormat, const void *binary, GLint length);

#ifndef GL_OES_gpu_shader5
#define GL_OES_gpu_shader5
#endif

#ifndef GL_OES_mapbuffer
#define GL_OES_mapbuffer
#define GL_WRITE_ONLY_OES 0x88B9
#define GL_BUFFER_ACCESS_OES 0x88BB
#define GL_BUFFER_MAPPED_OES 0x88BC
#define GL_BUFFER_MAP_POINTER_OES 0x88BD
#endif
extern void * (APIENTRYP gglMapBufferOES)(GLenum target, GLenum access);
extern GLboolean (APIENTRYP gglUnmapBufferOES)(GLenum target);
extern void (APIENTRYP gglGetBufferPointervOES)(GLenum target, GLenum pname, void **params);

#ifndef GL_OES_packed_depth_stencil
#define GL_OES_packed_depth_stencil
/* reuse GL_DEPTH_STENCIL_OES */
/* reuse GL_UNSIGNED_INT_24_8_OES */
/* reuse GL_DEPTH24_STENCIL8_OES */
#endif

#ifndef GL_OES_primitive_bounding_box
#define GL_OES_primitive_bounding_box
#define GL_PRIMITIVE_BOUNDING_BOX_OES 0x92BE
#endif
extern void (APIENTRYP gglPrimitiveBoundingBoxOES)(GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW);

#ifndef GL_OES_required_internalformat
#define GL_OES_required_internalformat
#define GL_ALPHA8_OES 0x803C
#define GL_DEPTH_COMPONENT16_OES 0x81A5
/* reuse GL_DEPTH_COMPONENT24_OES */
/* reuse GL_DEPTH24_STENCIL8_OES */
/* reuse GL_DEPTH_COMPONENT32_OES */
#define GL_LUMINANCE4_ALPHA4_OES 0x8043
#define GL_LUMINANCE8_ALPHA8_OES 0x8045
#define GL_LUMINANCE8_OES 0x8040
#define GL_RGBA4_OES 0x8056
#define GL_RGB5_A1_OES 0x8057
#define GL_RGB565_OES 0x8D62
#define GL_RGB8_OES 0x8051
#define GL_RGBA8_OES 0x8058
/* reuse GL_RGB10_EXT */
/* reuse GL_RGB10_A2_EXT */
#endif

#ifndef GL_OES_rgb8_rgba8
#define GL_OES_rgb8_rgba8
/* reuse GL_RGB8_OES */
/* reuse GL_RGBA8_OES */
#endif

#ifndef GL_OES_sample_shading
#define GL_OES_sample_shading
#define GL_SAMPLE_SHADING_OES 0x8C36
#define GL_MIN_SAMPLE_SHADING_VALUE_OES 0x8C37
#endif
extern void (APIENTRYP gglMinSampleShadingOES)(GLfloat value);

#ifndef GL_OES_sample_variables
#define GL_OES_sample_variables
#endif

#ifndef GL_OES_shader_image_atomic
#define GL_OES_shader_image_atomic
#endif

#ifndef GL_OES_shader_io_blocks
#define GL_OES_shader_io_blocks
#endif

#ifndef GL_OES_shader_multisample_interpolation
#define GL_OES_shader_multisample_interpolation
#define GL_MIN_FRAGMENT_INTERPOLATION_OFFSET_OES 0x8E5B
#define GL_MAX_FRAGMENT_INTERPOLATION_OFFSET_OES 0x8E5C
#define GL_FRAGMENT_INTERPOLATION_OFFSET_BITS_OES 0x8E5D
#endif

#ifndef GL_OES_standard_derivatives
#define GL_OES_standard_derivatives
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES 0x8B8B
#endif

#ifndef GL_OES_stencil1
#define GL_OES_stencil1
#define GL_STENCIL_INDEX1_OES 0x8D46
#endif

#ifndef GL_OES_stencil4
#define GL_OES_stencil4
#define GL_STENCIL_INDEX4_OES 0x8D47
#endif

#ifndef GL_OES_surfaceless_context
#define GL_OES_surfaceless_context
#define GL_FRAMEBUFFER_UNDEFINED_OES 0x8219
#endif

#ifndef GL_OES_tessellation_point_size
#define GL_OES_tessellation_point_size
#endif

#ifndef GL_OES_tessellation_shader
#define GL_OES_tessellation_shader
#define GL_PATCHES_OES 0x000E
#define GL_PATCH_VERTICES_OES 0x8E72
#define GL_TESS_CONTROL_OUTPUT_VERTICES_OES 0x8E75
#define GL_TESS_GEN_MODE_OES 0x8E76
#define GL_TESS_GEN_SPACING_OES 0x8E77
#define GL_TESS_GEN_VERTEX_ORDER_OES 0x8E78
#define GL_TESS_GEN_POINT_MODE_OES 0x8E79
/* reuse GL_TRIANGLES */
#define GL_ISOLINES_OES 0x8E7A
#define GL_QUADS_OES 0x0007
/* reuse GL_EQUAL */
#define GL_FRACTIONAL_ODD_OES 0x8E7B
#define GL_FRACTIONAL_EVEN_OES 0x8E7C
/* reuse GL_CCW */
/* reuse GL_CW */
#define GL_MAX_PATCH_VERTICES_OES 0x8E7D
#define GL_MAX_TESS_GEN_LEVEL_OES 0x8E7E
#define GL_MAX_TESS_CONTROL_UNIFORM_COMPONENTS_OES 0x8E7F
#define GL_MAX_TESS_EVALUATION_UNIFORM_COMPONENTS_OES 0x8E80
#define GL_MAX_TESS_CONTROL_TEXTURE_IMAGE_UNITS_OES 0x8E81
#define GL_MAX_TESS_EVALUATION_TEXTURE_IMAGE_UNITS_OES 0x8E82
#define GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS_OES 0x8E83
#define GL_MAX_TESS_PATCH_COMPONENTS_OES 0x8E84
#define GL_MAX_TESS_CONTROL_TOTAL_OUTPUT_COMPONENTS_OES 0x8E85
#define GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS_OES 0x8E86
#define GL_MAX_TESS_CONTROL_UNIFORM_BLOCKS_OES 0x8E89
#define GL_MAX_TESS_EVALUATION_UNIFORM_BLOCKS_OES 0x8E8A
#define GL_MAX_TESS_CONTROL_INPUT_COMPONENTS_OES 0x886C
#define GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS_OES 0x886D
#define GL_MAX_COMBINED_TESS_CONTROL_UNIFORM_COMPONENTS_OES 0x8E1E
#define GL_MAX_COMBINED_TESS_EVALUATION_UNIFORM_COMPONENTS_OES 0x8E1F
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTER_BUFFERS_OES 0x92CD
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTER_BUFFERS_OES 0x92CE
#define GL_MAX_TESS_CONTROL_ATOMIC_COUNTERS_OES 0x92D3
#define GL_MAX_TESS_EVALUATION_ATOMIC_COUNTERS_OES 0x92D4
#define GL_MAX_TESS_CONTROL_IMAGE_UNIFORMS_OES 0x90CB
#define GL_MAX_TESS_EVALUATION_IMAGE_UNIFORMS_OES 0x90CC
#define GL_MAX_TESS_CONTROL_SHADER_STORAGE_BLOCKS_OES 0x90D8
#define GL_MAX_TESS_EVALUATION_SHADER_STORAGE_BLOCKS_OES 0x90D9
#define GL_PRIMITIVE_RESTART_FOR_PATCHES_SUPPORTED_OES 0x8221
#define GL_IS_PER_PATCH_OES 0x92E7
#define GL_REFERENCED_BY_TESS_CONTROL_SHADER_OES 0x9307
#define GL_REFERENCED_BY_TESS_EVALUATION_SHADER_OES 0x9308
#define GL_TESS_CONTROL_SHADER_OES 0x8E88
#define GL_TESS_EVALUATION_SHADER_OES 0x8E87
#define GL_TESS_CONTROL_SHADER_BIT_OES 0x00000008
#define GL_TESS_EVALUATION_SHADER_BIT_OES 0x00000010
#endif
extern void (APIENTRYP gglPatchParameteriOES)(GLenum pname, GLint value);

#ifndef GL_OES_texture_3D
#define GL_OES_texture_3D
#define GL_TEXTURE_WRAP_R_OES 0x8072
#define GL_TEXTURE_3D_OES 0x806F
#define GL_TEXTURE_BINDING_3D_OES 0x806A
#define GL_MAX_3D_TEXTURE_SIZE_OES 0x8073
#define GL_SAMPLER_3D_OES 0x8B5F
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_OES 0x8CD4
#endif
extern void (APIENTRYP gglTexImage3DOES)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglTexSubImage3DOES)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
extern void (APIENTRYP gglCopyTexSubImage3DOES)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void (APIENTRYP gglCompressedTexImage3DOES)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglCompressedTexSubImage3DOES)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
extern void (APIENTRYP gglFramebufferTexture3DOES)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);

#ifndef GL_OES_texture_border_clamp
#define GL_OES_texture_border_clamp
#define GL_TEXTURE_BORDER_COLOR_OES 0x1004
#define GL_CLAMP_TO_BORDER_OES 0x812D
#endif
extern void (APIENTRYP gglTexParameterIivOES)(GLenum target, GLenum pname, const GLint *params);
extern void (APIENTRYP gglTexParameterIuivOES)(GLenum target, GLenum pname, const GLuint *params);
extern void (APIENTRYP gglGetTexParameterIivOES)(GLenum target, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetTexParameterIuivOES)(GLenum target, GLenum pname, GLuint *params);
extern void (APIENTRYP gglSamplerParameterIivOES)(GLuint sampler, GLenum pname, const GLint *param);
extern void (APIENTRYP gglSamplerParameterIuivOES)(GLuint sampler, GLenum pname, const GLuint *param);
extern void (APIENTRYP gglGetSamplerParameterIivOES)(GLuint sampler, GLenum pname, GLint *params);
extern void (APIENTRYP gglGetSamplerParameterIuivOES)(GLuint sampler, GLenum pname, GLuint *params);

#ifndef GL_OES_texture_buffer
#define GL_OES_texture_buffer
#define GL_TEXTURE_BUFFER_OES 0x8C2A
#define GL_TEXTURE_BUFFER_BINDING_OES 0x8C2A
#define GL_MAX_TEXTURE_BUFFER_SIZE_OES 0x8C2B
#define GL_TEXTURE_BINDING_BUFFER_OES 0x8C2C
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING_OES 0x8C2D
#define GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT_OES 0x919F
#define GL_SAMPLER_BUFFER_OES 0x8DC2
#define GL_INT_SAMPLER_BUFFER_OES 0x8DD0
#define GL_UNSIGNED_INT_SAMPLER_BUFFER_OES 0x8DD8
#define GL_IMAGE_BUFFER_OES 0x9051
#define GL_INT_IMAGE_BUFFER_OES 0x905C
#define GL_UNSIGNED_INT_IMAGE_BUFFER_OES 0x9067
#define GL_TEXTURE_BUFFER_OFFSET_OES 0x919D
#define GL_TEXTURE_BUFFER_SIZE_OES 0x919E
#endif
extern void (APIENTRYP gglTexBufferOES)(GLenum target, GLenum internalformat, GLuint buffer);
extern void (APIENTRYP gglTexBufferRangeOES)(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);

#ifndef GL_OES_texture_compression_astc
#define GL_OES_texture_compression_astc
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
#define GL_COMPRESSED_RGBA_ASTC_3x3x3_OES 0x93C0
#define GL_COMPRESSED_RGBA_ASTC_4x3x3_OES 0x93C1
#define GL_COMPRESSED_RGBA_ASTC_4x4x3_OES 0x93C2
#define GL_COMPRESSED_RGBA_ASTC_4x4x4_OES 0x93C3
#define GL_COMPRESSED_RGBA_ASTC_5x4x4_OES 0x93C4
#define GL_COMPRESSED_RGBA_ASTC_5x5x4_OES 0x93C5
#define GL_COMPRESSED_RGBA_ASTC_5x5x5_OES 0x93C6
#define GL_COMPRESSED_RGBA_ASTC_6x5x5_OES 0x93C7
#define GL_COMPRESSED_RGBA_ASTC_6x6x5_OES 0x93C8
#define GL_COMPRESSED_RGBA_ASTC_6x6x6_OES 0x93C9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES 0x93E0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES 0x93E1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES 0x93E2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES 0x93E3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES 0x93E4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES 0x93E5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES 0x93E6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES 0x93E7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES 0x93E8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES 0x93E9
#endif

#ifndef GL_OES_texture_cube_map_array
#define GL_OES_texture_cube_map_array
/* reuse GL_TEXTURE_CUBE_MAP_ARRAY_OES */
#define GL_TEXTURE_BINDING_CUBE_MAP_ARRAY_OES 0x900A
#define GL_SAMPLER_CUBE_MAP_ARRAY_OES 0x900C
#define GL_SAMPLER_CUBE_MAP_ARRAY_SHADOW_OES 0x900D
#define GL_INT_SAMPLER_CUBE_MAP_ARRAY_OES 0x900E
#define GL_UNSIGNED_INT_SAMPLER_CUBE_MAP_ARRAY_OES 0x900F
#define GL_IMAGE_CUBE_MAP_ARRAY_OES 0x9054
#define GL_INT_IMAGE_CUBE_MAP_ARRAY_OES 0x905F
#define GL_UNSIGNED_INT_IMAGE_CUBE_MAP_ARRAY_OES 0x906A
#endif

#ifndef GL_OES_texture_float
#define GL_OES_texture_float
/* reuse GL_FLOAT */
#endif

#ifndef GL_OES_texture_float_linear
#define GL_OES_texture_float_linear
#endif

#ifndef GL_OES_texture_half_float
#define GL_OES_texture_half_float
#define GL_HALF_FLOAT_OES 0x8D61
#endif

#ifndef GL_OES_texture_half_float_linear
#define GL_OES_texture_half_float_linear
#endif

#ifndef GL_OES_texture_npot
#define GL_OES_texture_npot
#endif

#ifndef GL_OES_texture_stencil8
#define GL_OES_texture_stencil8
#define GL_STENCIL_INDEX_OES 0x1901
#define GL_STENCIL_INDEX8_OES 0x8D48
#endif

#ifndef GL_OES_texture_storage_multisample_2d_array
#define GL_OES_texture_storage_multisample_2d_array
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY_OES 0x9102
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY_OES 0x9105
#define GL_SAMPLER_2D_MULTISAMPLE_ARRAY_OES 0x910B
#define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY_OES 0x910C
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY_OES 0x910D
#endif
extern void (APIENTRYP gglTexStorage3DMultisampleOES)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);

#ifndef GL_OES_texture_view
#define GL_OES_texture_view
#define GL_TEXTURE_VIEW_MIN_LEVEL_OES 0x82DB
#define GL_TEXTURE_VIEW_NUM_LEVELS_OES 0x82DC
#define GL_TEXTURE_VIEW_MIN_LAYER_OES 0x82DD
#define GL_TEXTURE_VIEW_NUM_LAYERS_OES 0x82DE
/* reuse GL_TEXTURE_IMMUTABLE_LEVELS */
#endif
extern void (APIENTRYP gglTextureViewOES)(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers);

#ifndef GL_OES_vertex_array_object
#define GL_OES_vertex_array_object
#define GL_VERTEX_ARRAY_BINDING_OES 0x85B5
#endif
extern void (APIENTRYP gglBindVertexArrayOES)(GLuint array);
extern void (APIENTRYP gglDeleteVertexArraysOES)(GLsizei n, const GLuint *arrays);
extern void (APIENTRYP gglGenVertexArraysOES)(GLsizei n, GLuint *arrays);
extern GLboolean (APIENTRYP gglIsVertexArrayOES)(GLuint array);

#ifndef GL_OES_vertex_half_float
#define GL_OES_vertex_half_float
/* reuse GL_HALF_FLOAT_OES */
#endif

#ifndef GL_OES_vertex_type_10_10_10_2
#define GL_OES_vertex_type_10_10_10_2
#define GL_UNSIGNED_INT_10_10_10_2_OES 0x8DF6
#define GL_INT_10_10_10_2_OES 0x8DF7
#endif

#ifndef GL_OES_viewport_array
#define GL_OES_viewport_array
/* reuse GL_SCISSOR_BOX */
/* reuse GL_VIEWPORT */
/* reuse GL_DEPTH_RANGE */
/* reuse GL_SCISSOR_TEST */
#define GL_MAX_VIEWPORTS_OES 0x825B
#define GL_VIEWPORT_SUBPIXEL_BITS_OES 0x825C
#define GL_VIEWPORT_BOUNDS_RANGE_OES 0x825D
#define GL_VIEWPORT_INDEX_PROVOKING_VERTEX_OES 0x825F
#endif
extern void (APIENTRYP gglViewportArrayvOES)(GLuint first, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglViewportIndexedfOES)(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h);
extern void (APIENTRYP gglViewportIndexedfvOES)(GLuint index, const GLfloat *v);
extern void (APIENTRYP gglScissorArrayvOES)(GLuint first, GLsizei count, const GLint *v);
extern void (APIENTRYP gglScissorIndexedOES)(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height);
extern void (APIENTRYP gglScissorIndexedvOES)(GLuint index, const GLint *v);
extern void (APIENTRYP gglDepthRangeArrayfvOES)(GLuint first, GLsizei count, const GLfloat *v);
extern void (APIENTRYP gglDepthRangeIndexedfOES)(GLuint index, GLfloat n, GLfloat f);
extern void (APIENTRYP gglGetFloati_vOES)(GLenum target, GLuint index, GLfloat *data);
/* reuse void (APIENTRYP gglEnableiOES)(GLenum target, GLuint index) */
/* reuse void (APIENTRYP gglDisableiOES)(GLenum target, GLuint index) */
/* reuse GLboolean (APIENTRYP gglIsEnablediOES)(GLenum target, GLuint index) */

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

#ifndef GL_OVR_multiview_multisampled_render_to_texture
#define GL_OVR_multiview_multisampled_render_to_texture
#endif
extern void (APIENTRYP gglFramebufferTextureMultisampleMultiviewOVR)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLsizei samples, GLint baseViewIndex, GLsizei numViews);

#ifndef GL_QCOM_alpha_test
#define GL_QCOM_alpha_test
#define GL_ALPHA_TEST_QCOM 0x0BC0
#define GL_ALPHA_TEST_FUNC_QCOM 0x0BC1
#define GL_ALPHA_TEST_REF_QCOM 0x0BC2
#endif
extern void (APIENTRYP gglAlphaFuncQCOM)(GLenum func, GLclampf ref);

#ifndef GL_QCOM_binning_control
#define GL_QCOM_binning_control
#define GL_BINNING_CONTROL_HINT_QCOM 0x8FB0
#define GL_CPU_OPTIMIZED_QCOM 0x8FB1
#define GL_GPU_OPTIMIZED_QCOM 0x8FB2
#define GL_RENDER_DIRECT_TO_FRAMEBUFFER_QCOM 0x8FB3
#endif

#ifndef GL_QCOM_driver_control
#define GL_QCOM_driver_control
#endif
extern void (APIENTRYP gglGetDriverControlsQCOM)(GLint *num, GLsizei size, GLuint *driverControls);
extern void (APIENTRYP gglGetDriverControlStringQCOM)(GLuint driverControl, GLsizei bufSize, GLsizei *length, GLchar *driverControlString);
extern void (APIENTRYP gglEnableDriverControlQCOM)(GLuint driverControl);
extern void (APIENTRYP gglDisableDriverControlQCOM)(GLuint driverControl);

#ifndef GL_QCOM_extended_get
#define GL_QCOM_extended_get
#define GL_TEXTURE_WIDTH_QCOM 0x8BD2
#define GL_TEXTURE_HEIGHT_QCOM 0x8BD3
#define GL_TEXTURE_DEPTH_QCOM 0x8BD4
#define GL_TEXTURE_INTERNAL_FORMAT_QCOM 0x8BD5
#define GL_TEXTURE_FORMAT_QCOM 0x8BD6
#define GL_TEXTURE_TYPE_QCOM 0x8BD7
#define GL_TEXTURE_IMAGE_VALID_QCOM 0x8BD8
#define GL_TEXTURE_NUM_LEVELS_QCOM 0x8BD9
#define GL_TEXTURE_TARGET_QCOM 0x8BDA
#define GL_TEXTURE_OBJECT_VALID_QCOM 0x8BDB
#define GL_STATE_RESTORE 0x8BDC
#endif
extern void (APIENTRYP gglExtGetTexturesQCOM)(GLuint *textures, GLint maxTextures, GLint *numTextures);
extern void (APIENTRYP gglExtGetBuffersQCOM)(GLuint *buffers, GLint maxBuffers, GLint *numBuffers);
extern void (APIENTRYP gglExtGetRenderbuffersQCOM)(GLuint *renderbuffers, GLint maxRenderbuffers, GLint *numRenderbuffers);
extern void (APIENTRYP gglExtGetFramebuffersQCOM)(GLuint *framebuffers, GLint maxFramebuffers, GLint *numFramebuffers);
extern void (APIENTRYP gglExtGetTexLevelParameterivQCOM)(GLuint texture, GLenum face, GLint level, GLenum pname, GLint *params);
extern void (APIENTRYP gglExtTexObjectStateOverrideiQCOM)(GLenum target, GLenum pname, GLint param);
extern void (APIENTRYP gglExtGetTexSubImageQCOM)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, void *texels);
extern void (APIENTRYP gglExtGetBufferPointervQCOM)(GLenum target, void **params);

#ifndef GL_QCOM_extended_get2
#define GL_QCOM_extended_get2
#endif
extern void (APIENTRYP gglExtGetShadersQCOM)(GLuint *shaders, GLint maxShaders, GLint *numShaders);
extern void (APIENTRYP gglExtGetProgramsQCOM)(GLuint *programs, GLint maxPrograms, GLint *numPrograms);
extern GLboolean (APIENTRYP gglExtIsProgramBinaryQCOM)(GLuint program);
extern void (APIENTRYP gglExtGetProgramBinarySourceQCOM)(GLuint program, GLenum shadertype, GLchar *source, GLint *length);

#ifndef GL_QCOM_perfmon_global_mode
#define GL_QCOM_perfmon_global_mode
#define GL_PERFMON_GLOBAL_MODE_QCOM 0x8FA0
#endif

#ifndef GL_QCOM_tiled_rendering
#define GL_QCOM_tiled_rendering
#define GL_COLOR_BUFFER_BIT0_QCOM 0x00000001
#define GL_COLOR_BUFFER_BIT1_QCOM 0x00000002
#define GL_COLOR_BUFFER_BIT2_QCOM 0x00000004
#define GL_COLOR_BUFFER_BIT3_QCOM 0x00000008
#define GL_COLOR_BUFFER_BIT4_QCOM 0x00000010
#define GL_COLOR_BUFFER_BIT5_QCOM 0x00000020
#define GL_COLOR_BUFFER_BIT6_QCOM 0x00000040
#define GL_COLOR_BUFFER_BIT7_QCOM 0x00000080
#define GL_DEPTH_BUFFER_BIT0_QCOM 0x00000100
#define GL_DEPTH_BUFFER_BIT1_QCOM 0x00000200
#define GL_DEPTH_BUFFER_BIT2_QCOM 0x00000400
#define GL_DEPTH_BUFFER_BIT3_QCOM 0x00000800
#define GL_DEPTH_BUFFER_BIT4_QCOM 0x00001000
#define GL_DEPTH_BUFFER_BIT5_QCOM 0x00002000
#define GL_DEPTH_BUFFER_BIT6_QCOM 0x00004000
#define GL_DEPTH_BUFFER_BIT7_QCOM 0x00008000
#define GL_STENCIL_BUFFER_BIT0_QCOM 0x00010000
#define GL_STENCIL_BUFFER_BIT1_QCOM 0x00020000
#define GL_STENCIL_BUFFER_BIT2_QCOM 0x00040000
#define GL_STENCIL_BUFFER_BIT3_QCOM 0x00080000
#define GL_STENCIL_BUFFER_BIT4_QCOM 0x00100000
#define GL_STENCIL_BUFFER_BIT5_QCOM 0x00200000
#define GL_STENCIL_BUFFER_BIT6_QCOM 0x00400000
#define GL_STENCIL_BUFFER_BIT7_QCOM 0x00800000
#define GL_MULTISAMPLE_BUFFER_BIT0_QCOM 0x01000000
#define GL_MULTISAMPLE_BUFFER_BIT1_QCOM 0x02000000
#define GL_MULTISAMPLE_BUFFER_BIT2_QCOM 0x04000000
#define GL_MULTISAMPLE_BUFFER_BIT3_QCOM 0x08000000
#define GL_MULTISAMPLE_BUFFER_BIT4_QCOM 0x10000000
#define GL_MULTISAMPLE_BUFFER_BIT5_QCOM 0x20000000
#define GL_MULTISAMPLE_BUFFER_BIT6_QCOM 0x40000000
#define GL_MULTISAMPLE_BUFFER_BIT7_QCOM 0x80000000
#endif
extern void (APIENTRYP gglStartTilingQCOM)(GLuint x, GLuint y, GLuint width, GLuint height, GLbitfield preserveMask);
extern void (APIENTRYP gglEndTilingQCOM)(GLbitfield preserveMask);

#ifndef GL_QCOM_writeonly_rendering
#define GL_QCOM_writeonly_rendering
#define GL_WRITEONLY_RENDERING_QCOM 0x8823
#endif

#ifndef GL_VIV_shader_binary
#define GL_VIV_shader_binary
#define GL_SHADER_BINARY_VIV 0x8FC4
#endif

#endif // __APPLE__

/* 242 extensions */
typedef struct {
	int _GL_AMD_compressed_3DC_texture : 1;
	int _GL_AMD_compressed_ATC_texture : 1;
	int _GL_AMD_performance_monitor : 1;
	int _GL_AMD_program_binary_Z400 : 1;
	int _GL_ANDROID_extension_pack_es31a : 1;
	int _GL_ANGLE_depth_texture : 1;
	int _GL_ANGLE_framebuffer_blit : 1;
	int _GL_ANGLE_framebuffer_multisample : 1;
	int _GL_ANGLE_instanced_arrays : 1;
	int _GL_ANGLE_pack_reverse_row_order : 1;
	int _GL_ANGLE_program_binary : 1;
	int _GL_ANGLE_texture_compression_dxt3 : 1;
	int _GL_ANGLE_texture_compression_dxt5 : 1;
	int _GL_ANGLE_texture_usage : 1;
	int _GL_ANGLE_translated_shader_source : 1;
	int _GL_APPLE_clip_distance : 1;
	int _GL_APPLE_color_buffer_packed_float : 1;
	int _GL_APPLE_copy_texture_levels : 1;
	int _GL_APPLE_framebuffer_multisample : 1;
	int _GL_APPLE_rgb_422 : 1;
	int _GL_APPLE_sync : 1;
	int _GL_APPLE_texture_format_BGRA8888 : 1;
	int _GL_APPLE_texture_max_level : 1;
	int _GL_APPLE_texture_packed_float : 1;
	int _GL_ARB_sparse_texture2 : 1;
	int _GL_ARM_mali_program_binary : 1;
	int _GL_ARM_mali_shader_binary : 1;
	int _GL_ARM_rgba8 : 1;
	int _GL_ARM_shader_framebuffer_fetch : 1;
	int _GL_ARM_shader_framebuffer_fetch_depth_stencil : 1;
	int _GL_DMP_program_binary : 1;
	int _GL_DMP_shader_binary : 1;
	int _GL_EXT_YUV_target : 1;
	int _GL_EXT_base_instance : 1;
	int _GL_EXT_blend_func_extended : 1;
	int _GL_EXT_blend_minmax : 1;
	int _GL_EXT_buffer_storage : 1;
	int _GL_EXT_clear_texture : 1;
	int _GL_EXT_clip_cull_distance : 1;
	int _GL_EXT_color_buffer_float : 1;
	int _GL_EXT_color_buffer_half_float : 1;
	int _GL_EXT_conservative_depth : 1;
	int _GL_EXT_copy_image : 1;
	int _GL_EXT_debug_label : 1;
	int _GL_EXT_debug_marker : 1;
	int _GL_EXT_discard_framebuffer : 1;
	int _GL_EXT_disjoint_timer_query : 1;
	int _GL_EXT_draw_buffers : 1;
	int _GL_EXT_draw_buffers_indexed : 1;
	int _GL_EXT_draw_elements_base_vertex : 1;
	int _GL_EXT_draw_instanced : 1;
	int _GL_EXT_draw_transform_feedback : 1;
	int _GL_EXT_float_blend : 1;
	int _GL_EXT_geometry_point_size : 1;
	int _GL_EXT_geometry_shader : 1;
	int _GL_EXT_gpu_shader5 : 1;
	int _GL_EXT_instanced_arrays : 1;
	int _GL_EXT_map_buffer_range : 1;
	int _GL_EXT_multi_draw_arrays : 1;
	int _GL_EXT_multi_draw_indirect : 1;
	int _GL_EXT_multisampled_compatibility : 1;
	int _GL_EXT_multisampled_render_to_texture : 1;
	int _GL_EXT_multiview_draw_buffers : 1;
	int _GL_EXT_occlusion_query_boolean : 1;
	int _GL_EXT_polygon_offset_clamp : 1;
	int _GL_EXT_post_depth_coverage : 1;
	int _GL_EXT_primitive_bounding_box : 1;
	int _GL_EXT_protected_textures : 1;
	int _GL_EXT_pvrtc_sRGB : 1;
	int _GL_EXT_raster_multisample : 1;
	int _GL_EXT_read_format_bgra : 1;
	int _GL_EXT_render_snorm : 1;
	int _GL_EXT_robustness : 1;
	int _GL_EXT_sRGB : 1;
	int _GL_EXT_sRGB_write_control : 1;
	int _GL_EXT_separate_shader_objects : 1;
	int _GL_EXT_shader_framebuffer_fetch : 1;
	int _GL_EXT_shader_group_vote : 1;
	int _GL_EXT_shader_implicit_conversions : 1;
	int _GL_EXT_shader_integer_mix : 1;
	int _GL_EXT_shader_io_blocks : 1;
	int _GL_EXT_shader_non_constant_global_initializers : 1;
	int _GL_EXT_shader_pixel_local_storage : 1;
	int _GL_EXT_shader_pixel_local_storage2 : 1;
	int _GL_EXT_shader_texture_lod : 1;
	int _GL_EXT_shadow_samplers : 1;
	int _GL_EXT_sparse_texture : 1;
	int _GL_EXT_tessellation_point_size : 1;
	int _GL_EXT_tessellation_shader : 1;
	int _GL_EXT_texture_border_clamp : 1;
	int _GL_EXT_texture_buffer : 1;
	int _GL_EXT_texture_compression_dxt1 : 1;
	int _GL_EXT_texture_compression_s3tc : 1;
	int _GL_EXT_texture_cube_map_array : 1;
	int _GL_EXT_texture_filter_anisotropic : 1;
	int _GL_EXT_texture_filter_minmax : 1;
	int _GL_EXT_texture_format_BGRA8888 : 1;
	int _GL_EXT_texture_norm16 : 1;
	int _GL_EXT_texture_rg : 1;
	int _GL_EXT_texture_sRGB_R8 : 1;
	int _GL_EXT_texture_sRGB_RG8 : 1;
	int _GL_EXT_texture_sRGB_decode : 1;
	int _GL_EXT_texture_storage : 1;
	int _GL_EXT_texture_type_2_10_10_10_REV : 1;
	int _GL_EXT_texture_view : 1;
	int _GL_EXT_unpack_subimage : 1;
	int _GL_EXT_window_rectangles : 1;
	int _GL_FJ_shader_binary_GCCSO : 1;
	int _GL_IMG_bindless_texture : 1;
	int _GL_IMG_framebuffer_downsample : 1;
	int _GL_IMG_multisampled_render_to_texture : 1;
	int _GL_IMG_program_binary : 1;
	int _GL_IMG_read_format : 1;
	int _GL_IMG_shader_binary : 1;
	int _GL_IMG_texture_compression_pvrtc : 1;
	int _GL_IMG_texture_compression_pvrtc2 : 1;
	int _GL_IMG_texture_filter_cubic : 1;
	int _GL_INTEL_conservative_rasterization : 1;
	int _GL_INTEL_framebuffer_CMAA : 1;
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
	int _GL_NV_bindless_texture : 1;
	int _GL_NV_blend_equation_advanced : 1;
	int _GL_NV_blend_equation_advanced_coherent : 1;
	int _GL_NV_conditional_render : 1;
	int _GL_NV_conservative_raster : 1;
	int _GL_NV_conservative_raster_pre_snap_triangles : 1;
	int _GL_NV_copy_buffer : 1;
	int _GL_NV_coverage_sample : 1;
	int _GL_NV_depth_nonlinear : 1;
	int _GL_NV_draw_buffers : 1;
	int _GL_NV_draw_instanced : 1;
	int _GL_NV_explicit_attrib_location : 1;
	int _GL_NV_fbo_color_attachments : 1;
	int _GL_NV_fence : 1;
	int _GL_NV_fill_rectangle : 1;
	int _GL_NV_fragment_coverage_to_color : 1;
	int _GL_NV_fragment_shader_interlock : 1;
	int _GL_NV_framebuffer_blit : 1;
	int _GL_NV_framebuffer_mixed_samples : 1;
	int _GL_NV_framebuffer_multisample : 1;
	int _GL_NV_generate_mipmap_sRGB : 1;
	int _GL_NV_geometry_shader_passthrough : 1;
	int _GL_NV_gpu_shader5 : 1;
	int _GL_NV_image_formats : 1;
	int _GL_NV_instanced_arrays : 1;
	int _GL_NV_internalformat_sample_query : 1;
	int _GL_NV_non_square_matrices : 1;
	int _GL_NV_path_rendering : 1;
	int _GL_NV_path_rendering_shared_edge : 1;
	int _GL_NV_polygon_mode : 1;
	int _GL_NV_read_buffer : 1;
	int _GL_NV_read_buffer_front : 1;
	int _GL_NV_read_depth : 1;
	int _GL_NV_read_depth_stencil : 1;
	int _GL_NV_read_stencil : 1;
	int _GL_NV_sRGB_formats : 1;
	int _GL_NV_sample_locations : 1;
	int _GL_NV_sample_mask_override_coverage : 1;
	int _GL_NV_shader_atomic_fp16_vector : 1;
	int _GL_NV_shader_noperspective_interpolation : 1;
	int _GL_NV_shadow_samplers_array : 1;
	int _GL_NV_shadow_samplers_cube : 1;
	int _GL_NV_texture_border_clamp : 1;
	int _GL_NV_texture_compression_s3tc_update : 1;
	int _GL_NV_texture_npot_2D_mipmap : 1;
	int _GL_NV_viewport_array : 1;
	int _GL_NV_viewport_array2 : 1;
	int _GL_NV_viewport_swizzle : 1;
	int _GL_OES_EGL_image : 1;
	int _GL_OES_EGL_image_external : 1;
	int _GL_OES_EGL_image_external_essl3 : 1;
	int _GL_OES_compressed_ETC1_RGB8_sub_texture : 1;
	int _GL_OES_compressed_ETC1_RGB8_texture : 1;
	int _GL_OES_compressed_paletted_texture : 1;
	int _GL_OES_copy_image : 1;
	int _GL_OES_depth24 : 1;
	int _GL_OES_depth32 : 1;
	int _GL_OES_depth_texture : 1;
	int _GL_OES_draw_buffers_indexed : 1;
	int _GL_OES_draw_elements_base_vertex : 1;
	int _GL_OES_element_index_uint : 1;
	int _GL_OES_fbo_render_mipmap : 1;
	int _GL_OES_fragment_precision_high : 1;
	int _GL_OES_geometry_point_size : 1;
	int _GL_OES_geometry_shader : 1;
	int _GL_OES_get_program_binary : 1;
	int _GL_OES_gpu_shader5 : 1;
	int _GL_OES_mapbuffer : 1;
	int _GL_OES_packed_depth_stencil : 1;
	int _GL_OES_primitive_bounding_box : 1;
	int _GL_OES_required_internalformat : 1;
	int _GL_OES_rgb8_rgba8 : 1;
	int _GL_OES_sample_shading : 1;
	int _GL_OES_sample_variables : 1;
	int _GL_OES_shader_image_atomic : 1;
	int _GL_OES_shader_io_blocks : 1;
	int _GL_OES_shader_multisample_interpolation : 1;
	int _GL_OES_standard_derivatives : 1;
	int _GL_OES_stencil1 : 1;
	int _GL_OES_stencil4 : 1;
	int _GL_OES_surfaceless_context : 1;
	int _GL_OES_tessellation_point_size : 1;
	int _GL_OES_tessellation_shader : 1;
	int _GL_OES_texture_3D : 1;
	int _GL_OES_texture_border_clamp : 1;
	int _GL_OES_texture_buffer : 1;
	int _GL_OES_texture_compression_astc : 1;
	int _GL_OES_texture_cube_map_array : 1;
	int _GL_OES_texture_float : 1;
	int _GL_OES_texture_float_linear : 1;
	int _GL_OES_texture_half_float : 1;
	int _GL_OES_texture_half_float_linear : 1;
	int _GL_OES_texture_npot : 1;
	int _GL_OES_texture_stencil8 : 1;
	int _GL_OES_texture_storage_multisample_2d_array : 1;
	int _GL_OES_texture_view : 1;
	int _GL_OES_vertex_array_object : 1;
	int _GL_OES_vertex_half_float : 1;
	int _GL_OES_vertex_type_10_10_10_2 : 1;
	int _GL_OES_viewport_array : 1;
	int _GL_OVR_multiview : 1;
	int _GL_OVR_multiview2 : 1;
	int _GL_OVR_multiview_multisampled_render_to_texture : 1;
	int _GL_QCOM_alpha_test : 1;
	int _GL_QCOM_binning_control : 1;
	int _GL_QCOM_driver_control : 1;
	int _GL_QCOM_extended_get : 1;
	int _GL_QCOM_extended_get2 : 1;
	int _GL_QCOM_perfmon_global_mode : 1;
	int _GL_QCOM_tiled_rendering : 1;
	int _GL_QCOM_writeonly_rendering : 1;
	int _GL_VIV_shader_binary : 1;
} gglext_t;

extern gglext_t gglext;

extern void ggl_init(int enableDebug);
extern void ggl_rebind(int enableDebug);

#ifdef __cplusplus
}
#endif

#endif /* __GGLES3_H__ */
