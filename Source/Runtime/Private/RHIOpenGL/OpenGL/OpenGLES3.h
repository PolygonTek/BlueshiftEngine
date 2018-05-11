// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

/*
===============================================================================

    OpenGL ES 3.0

    OpenGL 4.3 provides full compatibility with OpenGL ES 3.0.

===============================================================================
*/

#include "OpenGL.h"

#ifndef GL_FILL
#define GL_FILL 0x1B02
#endif

#ifndef GL_POINT
#define GL_POINT 0x1B00
#endif

#ifndef GL_LINE
#define GL_LINE 0x1B01
#endif

#ifndef GL_LINE_SMOOTH
#define GL_LINE_SMOOTH 0x0B20
#endif

#ifndef GL_LINE_SMOOTH_HINT
#define GL_LINE_SMOOTH_HINT 0x0C52
#endif

#ifndef GL_POLYGON_SMOOTH
#define GL_POLYGON_SMOOTH 0x0B41
#endif

#ifndef GL_POLYGON_SMOOTH_HINT
#define GL_POLYGON_SMOOTH_HINT 0x0C53
#endif

#ifndef GL_POLYGON_OFFSET_LINE
#define GL_POLYGON_OFFSET_LINE 0x2A02
#endif

#ifndef GL_POLYGON_OFFSET_POINT
#define GL_POLYGON_OFFSET_POINT 0x2A01
#endif

#ifndef GL_DEPTH_CLAMP
#define GL_DEPTH_CLAMP 0x864F
#endif

#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif

#ifndef GL_TEXTURE_BORDER_COLOR
#define GL_TEXTURE_BORDER_COLOR 0x1004
#endif

#ifndef GL_TEXTURE_LOD_BIAS
#define GL_TEXTURE_LOD_BIAS 0x8501
#endif

#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE 0x84F5
#endif

#ifndef GL_TEXTURE_BUFFER
#define GL_TEXTURE_BUFFER 0x8C2A
#endif

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

#ifndef GL_WRITE_ONLY
#define GL_WRITE_ONLY GL_WRITE_ONLY_OES
#endif

#ifndef GL_TEXTURE_CUBE_MAP_ARRAY
#define GL_TEXTURE_CUBE_MAP_ARRAY 0x9009
#endif

#ifndef GL_MAX_TEXTURE_BUFFER_SIZE
#define GL_MAX_TEXTURE_BUFFER_SIZE 0x8C2B
#endif

#ifndef GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS 0x8C29
#endif

#ifndef GL_MAX_GEOMETRY_OUTPUT_VERTICES
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES 0x8DE0
#endif

BE_NAMESPACE_BEGIN

class OpenGLES3 : public OpenGLBase {
public:
    static const int        GLSL_VERSION = 300;
    static const char *     GLSL_VERSION_STRING;
    
    static void             Init();
    
    static bool             SupportsPolygonMode() { return false; }
    static bool             SupportsLineSmooth() { return false; }
    static bool             SupportsTextureBorderColor() { return false; }
    static bool             SupportsTextureLodBias() { return false; }
    static bool             SupportsPackedFloat() { return true; }
    static bool             SupportsDepthClamp() { return false; }
    static bool             SupportsDepthBufferFloat() { return true; }
    static bool             SupportsPixelBufferObject() { return true; }
    static bool             SupportsFrameBufferSRGB() { return supportsFrameBufferSRGB; }
    static bool             SupportsTextureRectangle() { return true; }
    static bool             SupportsTextureArray() { return true; }
    static bool             SupportsTextureBuffer() { return supportsTextureBuffer; }
    static bool             SupportsTextureCompressionS3TC() { return false; }
    static bool             SupportsTextureCompressionLATC() { return false; }
    static bool             SupportsTextureCompressionETC2() { return true; }
    static bool             SupportsCompressedGenMipmaps() { return false; }
    static bool             SupportsInstancedArrays() { return true; }
    static bool             SupportsDrawIndirect() { return false; }
    static bool             SupportsMultiDrawIndirect() { return false; }
    static bool             SupportsProgramBinary() { return gglProgramBinary != nullptr; }

    static void             PolygonMode(GLenum face, GLenum mode) { }
    static void             ClearDepth(GLdouble depth) { gglClearDepthf(depth); }
    static void             DepthRange(GLdouble znear, GLdouble zfar) { gglDepthRangef(znear, zfar); }
    static void             DrawBuffer(GLenum buffer) { gglDrawBuffers(1, &buffer); }
    static void             TexBuffer(GLenum internalFormat, GLuint buffer);

    static void             VertexAttribDivisor(int index, int divisor) { gglVertexAttribDivisor(index, divisor); }
    static void             DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
    static void             DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex);
    static void             DrawElementsIndirect(GLenum mode, GLenum type, const void *indirect);
    static void             MultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride);

    static void             SetTextureSwizzling(GLenum target, Image::Format format);
    static bool             ImageFormatToGLFormat(Image::Format imageFormat, bool isSRGB, GLenum *glFormat, GLenum *glType, GLenum *glInternal);
    static bool             SupportedImageFormat(Image::Format imageFormat) { return ImageFormatToGLFormat(imageFormat, false, nullptr, nullptr, nullptr); }
    static Image::Format    ToCompressedImageFormat(Image::Format inFormat, bool useNormalMap);
    static Image::Format    ToUncompressedImageFormat(Image::Format inFormat);

private:
    static bool             supportsFrameBufferSRGB;
    static bool             supportsTextureBuffer;

    static int              shaderFloatPrecisionLow;
    static int              shaderFloatPrecisionMedium;
    static int              shaderFloatPrecisionHigh;
    static int              shaderIntPrecisionLow;
    static int              shaderIntPrecisionMedium;
    static int              shaderIntPrecisionHigh;
};

BE_NAMESPACE_END
