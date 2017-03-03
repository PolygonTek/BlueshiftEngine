#pragma once

/*
===============================================================================

    OpenGL 3.2

===============================================================================
*/

#include "OpenGL.h"

BE_NAMESPACE_BEGIN

class OpenGL3 : public OpenGLBase {
public:
    static const int        GLSL_VERSION = 330;
    static const char *     GLSL_VERSION_STRING;

    static bool             SupportsPolygonMode() { return true; }
    static bool             SupportsPackedFloat() { return true; }
    static bool             SupportsDepthClamp() { return true; }
    static bool             SupportsDepthBufferFloat() { return true; }
    static bool             SupportsPixelBufferObject() { return true; }
    static bool             SupportsFrameBufferSRGB() { return true; }
    static bool             SupportsTextureRectangle() { return true; }
    static bool             SupportsTextureArray() { return true; }
    static bool             SupportsTextureBufferObject() { return true; }
    static bool             SupportsTextureCompressionS3TC() { return true; }
    static bool             SupportsTextureCompressionLATC() { return true; }
    static bool             SupportsCompressedGenMipmaps() { return true; }
    static bool             SupportsProgramBinary() { return gglProgramBinary != nullptr; }
    
    static void             APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);

    static void             PolygonMode(GLenum face, GLenum mode) { gglPolygonMode(face, mode); }
    static void             ClearDepth(GLdouble depth) { gglClearDepth(depth); }
    static void             DepthRange(GLdouble znear, GLdouble zfar) { gglDepthRange(znear, zfar); }
    static void             DrawBuffer(GLenum buffer) { gglDrawBuffer(buffer); }
    static void             TexBuffer(GLenum internalFormat, GLuint buffer) { gglTexBuffer(GL_TEXTURE_BUFFER, internalFormat, buffer); }

    static void             SetTextureSwizzling(GLenum target, Image::Format format);
    static bool             ImageFormatToGLFormat(Image::Format imageFormat, bool isSRGB, GLenum *glFormat, GLenum *glType, GLenum *glInternal);
    static bool             SupportedImageFormat(Image::Format imageFormat) { return ImageFormatToGLFormat(imageFormat, false, nullptr, nullptr, nullptr); }
    static Image::Format    ToCompressedImageFormat(Image::Format inFormat, bool useNormalMap);
    static Image::Format    ToUncompressedImageFormat(Image::Format inFormat);
};

BE_NAMESPACE_END
