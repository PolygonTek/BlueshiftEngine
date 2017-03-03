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
#define GL_POINT 0x1B00
#define GL_LINE 0x1B01
#define GL_LINE_SMOOTH 0x0B20
#define GL_LINE_SMOOTH_HINT 0x0C52
#define GL_POLYGON_SMOOTH 0x0B41
#define GL_POLYGON_SMOOTH_HINT 0x0C53
#define GL_POLYGON_OFFSET_LINE 0x2A02
#define GL_POLYGON_OFFSET_POINT 0x2A01
#define GL_DEPTH_CLAMP 0x864F
#define GL_CLAMP_TO_BORDER 0x812D
#define GL_TEXTURE_BORDER_COLOR 0x1004
#define GL_TEXTURE_LOD_BIAS 0x8501
#define GL_TEXTURE_RECTANGLE 0x84F5
#define GL_FRAMEBUFFER_SRGB 0x8DB9

#define GL_WRITE_ONLY       GL_WRITE_ONLY_OES
#define GL_TEXTURE_CUBE_MAP_ARRAY GL_TEXTURE_CUBE_MAP_ARRAY_EXT

//#define gglMapBuffer        gglMapBufferOES
//#define gglUnmapBuffer      gglUnmapBufferOES

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
    static bool             SupportsFrameBufferSRGB() { return false; } // sRGB framebuffer is always enabled in ES3
    static bool             SupportsTextureRectangle() { return true; }
    static bool             SupportsTextureArray() { return true; }
    static bool             SupportsTextureCompressionS3TC() { return false; }
    static bool             SupportsTextureCompressionLATC() { return false; }
    static bool             SupportsTextureCompressionETC2() { return true; }
    static bool             SupportsCompressedGenMipmaps() { return false; }
    static bool             SupportsProgramBinary() { return gglProgramBinary != nullptr; }

    static void             PolygonMode(GLenum face, GLenum mode) { }
    static void             ClearDepth(GLdouble depth) { gglClearDepthf(depth); }
    static void             DepthRange(GLdouble znear, GLdouble zfar) { gglDepthRangef(znear, zfar); }
    static void             DrawBuffer(GLenum buffer) { gglDrawBuffers(1, &buffer); }
    static void             TexBuffer(GLenum internalFormat, GLuint buffer) { gglTexBufferEXT(GL_TEXTURE_BUFFER_EXT, internalFormat, buffer); }

    static void             SetTextureSwizzling(GLenum target, Image::Format format);
    static bool             ImageFormatToGLFormat(Image::Format imageFormat, bool isSRGB, GLenum *glFormat, GLenum *glType, GLenum *glInternal);
    static bool             SupportedImageFormat(Image::Format imageFormat) { return ImageFormatToGLFormat(imageFormat, false, nullptr, nullptr, nullptr); }
    static Image::Format    ToCompressedImageFormat(Image::Format inFormat, bool useNormalMap);
    static Image::Format    ToUncompressedImageFormat(Image::Format inFormat);

private:
    static int              shaderFloatPrecisionLow;
    static int              shaderFloatPrecisionMedium;
    static int              shaderFloatPrecisionHigh;
    static int              shaderIntPrecisionLow;
    static int              shaderIntPrecisionMedium;
    static int              shaderIntPrecisionHigh;
};

BE_NAMESPACE_END
