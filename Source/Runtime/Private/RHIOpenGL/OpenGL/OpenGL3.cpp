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

#include "Precompiled.h"
#include "OpenGL3.h"
#include "Core/CVars.h"

BE_NAMESPACE_BEGIN

const char *OpenGL3::GLSL_VERSION_STRING = "330";

 void OpenGL3::DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
    int severityLevel;
    switch (severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            severityLevel = 4;
            break;
        case GL_DEBUG_SEVERITY_HIGH_ARB:
            // Any GL error; dangerous undefined behavior; any GLSL or ARB shader compiler and linker errors;
            severityLevel = 3;
            break;
        case GL_DEBUG_SEVERITY_MEDIUM_ARB:
            // Severe performance warnings; GLSL or other shader compiler and linker warnings; use of currently deprecated behavior
            severityLevel = 2;
            break;
        case GL_DEBUG_SEVERITY_LOW_ARB:
            // Performance warnings from redundant state changes; trivial undefined behavior
            severityLevel = 1;
            break;
        default:
            severityLevel = 0;
            break;
    }
    
    if (severityLevel > cvarSystem.GetCVarInteger(L"gl_debugLevel")) {
        return;
    }
    
    const char *sourceStr = "";
    switch (source) {
        case GL_DEBUG_SOURCE_API_ARB:
            // The GL
            sourceStr = "OpenGL API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
            // The window system, such as WGL or GLX
            sourceStr = "Window System";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
            // The GLSL shader compiler or compilers for other extension - provided languages
            sourceStr = "GLSL Shader Compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
            // External debuggers or third - party middleware libraries
            sourceStr = "Third Party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION_ARB:
            // The application
            sourceStr = "Application";
            break;
        case GL_DEBUG_SOURCE_OTHER_ARB:
            // Sources that do not fit to any of the ones listed above
            sourceStr = "Other";
            break;
    }
    
    const char *typeStr = "";
    switch (type) {
        case GL_DEBUG_TYPE_ERROR_ARB:
            // Events that generated an error
            typeStr = "Error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
            // Behavior that has been marked for deprecation
            typeStr = "Deprecated behavior";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
            // Behavior that is undefined according to the specification
            typeStr = "Undefined behavior";
            break;
        case GL_DEBUG_TYPE_PORTABILITY_ARB:
            // Use of extensions or shaders in a way that is highly vendor - specific
            typeStr = "Portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE_ARB:
            // Implementation - dependent performance warnings
            typeStr = "Performance";
            break;
        case GL_DEBUG_TYPE_OTHER_ARB:
            // Types of events that do not fit any of the ones listed above
            typeStr = "Others";
            break;
    }
    
    BE_WARNLOG(L"GL Debug: %hs %hs - %hs\n", sourceStr, typeStr, message);
}

void OpenGL3::SetTextureSwizzling(GLenum target, Image::Format format) {
    static const GLint swiz_l[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
    static const GLint swiz_a[4] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
    static const GLint swiz_la[4] = { GL_RED, GL_RED, GL_RED, GL_GREEN };

    switch (format) {
    case Image::L_8:
    case Image::L_16F:
    case Image::L_32F:
        gglTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swiz_l);
        break;
    case Image::A_8:
    case Image::A_16F:
    case Image::A_32F:
        gglTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swiz_a);
        break;
    case Image::LA_8_8:
    case Image::LA_16_16:
    case Image::LA_16F_16F:
    case Image::LA_32F_32F:
    case Image::DXN1:
    case Image::DXN2:
        gglTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swiz_la);
        break;
    default:
        break;
    }
}

bool OpenGL3::ImageFormatToGLFormat(Image::Format imageFormat, bool isSRGB, GLenum *glFormat, GLenum *glType, GLenum *glInternal) {
    switch (imageFormat) {
    case Image::L_8:
    case Image::A_8:
    case Image::R_8:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = GL_R8;
        return true;
    case Image::R_SNORM_8:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_R8_SNORM;
        return true;
    case Image::LA_8_8:
    case Image::RG_8_8:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = GL_RG8;
        return true;
    case Image::RG_SNORM_8_8:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RG8_SNORM;
        return true;
    case Image::LA_16_16:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_UNSIGNED_SHORT;
        if (glInternal) *glInternal = GL_RG16;
        return true;
    case Image::RGB_8_8_8:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB8;
        return true;
    case Image::RGB_SNORM_8_8_8:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RGB8_SNORM;
        return true;
    case Image::BGR_8_8_8:
        if (glFormat)   *glFormat = GL_BGR;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB8;
        return true;
    case Image::RGBX_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_INT_8_8_8_8_REV;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB8;
        return true;
    case Image::BGRX_8_8_8_8:
        if (glFormat)   *glFormat = GL_BGRA;
        if (glType)     *glType = GL_UNSIGNED_INT_8_8_8_8_REV;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB8;
        return true;
    case Image::RGBA_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_INT_8_8_8_8_REV;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        return true;
    case Image::RGBA_SNORM_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RGBA8_SNORM;
        return true;
    case Image::BGRA_8_8_8_8:
        if (glFormat)   *glFormat = GL_BGRA;
        if (glType)     *glType = GL_UNSIGNED_INT_8_8_8_8_REV;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        return true;
    case Image::ABGR_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_INT_8_8_8_8;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        return true;
    case Image::ARGB_8_8_8_8:
        if (glFormat)   *glFormat = GL_BGRA;
        if (glType)     *glType = GL_UNSIGNED_INT_8_8_8_8;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        return true;
    case Image::RGBX_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB4;
        return true;
    case Image::BGRX_4_4_4_4:
        if (glFormat)   *glFormat = GL_BGRA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB4;
        return true;
    case Image::RGBA_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA4;
        return true;
    case Image::BGRA_4_4_4_4:
        if (glFormat)   *glFormat = GL_BGRA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA4;
        return true;
    case Image::ABGR_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA4;
        return true;
    case Image::ARGB_4_4_4_4:
        if (glFormat)   *glFormat = GL_BGRA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA4;
        return true;
    case Image::RGBX_5_5_5_1:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB5;
        return true;
    case Image::BGRX_5_5_5_1:
        if (glFormat)   *glFormat = GL_BGRA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB5;
        return true;
    case Image::RGBA_5_5_5_1:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGB5_A1;
        return true;
    case Image::BGRA_5_5_5_1:
        if (glFormat)   *glFormat = GL_BGRA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGB5_A1;
        return true;
    case Image::ABGR_1_5_5_5:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_5_5_1;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGB5_A1;
        return true;
    case Image::ARGB_1_5_5_5:
        if (glFormat)   *glFormat = GL_BGRA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_5_5_1;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGB5_A1;
        return true;
    case Image::RGB_5_6_5:
        if (glFormat)   *glFormat = GL_BGR;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_6_5;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB5;
        return true;
    case Image::BGR_5_6_5:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_6_5;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB5;
        return true;
    case Image::RGBE_9_9_9_5:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_INT_5_9_9_9_REV;
        if (glInternal) *glInternal = GL_RGB9_E5;
        return true;
    case Image::L_16F:
    case Image::A_16F:
    case Image::R_16F:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_HALF_FLOAT;
        if (glInternal) *glInternal = GL_R16F;
        return true;
    case Image::L_32F:
    case Image::A_32F:
    case Image::R_32F:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_R32F;
        return true;
    case Image::LA_16F_16F:
    case Image::RG_16F_16F:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_HALF_FLOAT;
        if (glInternal) *glInternal = GL_RG16F;
        return true;
    case Image::LA_32F_32F:
    case Image::RG_32F_32F:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_RG32F;
        return true;
    case Image::RGB_11F_11F_10F:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_INT_10F_11F_11F_REV;
        if (glInternal) *glInternal = GL_R11F_G11F_B10F;
        return true;
    case Image::RGB_16F_16F_16F:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_HALF_FLOAT;
        if (glInternal) *glInternal = GL_RGB16F;
        return true;
    case Image::RGB_32F_32F_32F:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_RGB32F;
        return true;
    case Image::RGBA_16F_16F_16F_16F:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_HALF_FLOAT;
        if (glInternal) *glInternal = GL_RGBA16F;
        return true;
    case Image::RGBA_32F_32F_32F_32F:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_RGBA32F;
        return true;
    case Image::RGBA_DXT1:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        return true;
    case Image::RGBA_DXT3:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        return true;
    case Image::RGBA_DXT5:
    case Image::XGBR_DXT5:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        return true;
    case Image::DXN1:
        if (glFormat)   *glFormat = GL_COMPRESSED_RED_RGTC1;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RED_RGTC1;//GL_COMPRESSED_SIGNED_RED_RGTC1 GL_COMPRESSED_LUMINANCE_LATC1_EXT;
        return true;
    case Image::DXN2:
        if (glFormat)   *glFormat = GL_COMPRESSED_RG_RGTC2;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RG_RGTC2;//GL_COMPRESSED_SIGNED_RG_RGTC2 GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT;
        return true;
    case Image::Depth_16:
        if (glFormat)   *glFormat = GL_DEPTH_COMPONENT;
        if (glType)     *glType = GL_UNSIGNED_SHORT;
        if (glInternal) *glInternal = GL_DEPTH_COMPONENT16;
        return true;
    case Image::Depth_24:
        if (glFormat)   *glFormat = GL_DEPTH_COMPONENT;
        if (glType)     *glType = GL_UNSIGNED_INT;
        if (glInternal) *glInternal = GL_DEPTH_COMPONENT24;
        return true;
    case Image::Depth_32F:
        if (glFormat)   *glFormat = GL_DEPTH_COMPONENT;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_DEPTH_COMPONENT32F;
        return true;
    case Image::DepthStencil_24_8:
        if (glFormat)   *glFormat = GL_DEPTH_STENCIL;
        if (glType)     *glType = GL_UNSIGNED_INT_24_8;
        if (glInternal) *glInternal = GL_DEPTH24_STENCIL8;
        return true;
    case Image::DepthStencil_32F_8:
        if (glFormat)   *glFormat = GL_DEPTH_STENCIL;
        if (glType)     *glType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
        if (glInternal) *glInternal = GL_DEPTH32F_STENCIL8;
        return true;
    default:
        return OpenGLBase::ImageFormatToGLFormat(imageFormat, isSRGB, glFormat, glType, glInternal);
    }

    return false;
}

Image::Format OpenGL3::ToCompressedImageFormat(Image::Format inFormat, bool useNormalMap) {
    if (Image::IsCompressed(inFormat)) {
        assert(0);
        return inFormat;
    }

    int redBits, greenBits, blueBits, alphaBits;
    Image::GetBits(inFormat, &redBits, &greenBits, &blueBits, &alphaBits);

    Image::Format outFormat = inFormat;

    if (redBits > 0 && greenBits > 0 && blueBits > 0) {
        if (Image::IsFloatFormat(inFormat)) {
            if (alphaBits == 0) {
                outFormat = Image::RGBE_9_9_9_5;
            }
        } else if (useNormalMap) {
            outFormat = Image::DXN2;
        } else {
            if (alphaBits <= 1) {
                outFormat = Image::RGBA_DXT1;
            } else if (alphaBits <= 4) {
                outFormat = Image::RGBA_DXT3;
            } else {
                outFormat = Image::RGBA_DXT5;
            }
        }
    }

    return outFormat;
}

Image::Format OpenGL3::ToUncompressedImageFormat(Image::Format inFormat) {
    if (!Image::IsCompressed(inFormat)) {
        assert(0);
        return inFormat;
    }

    Image::Format outFormat;

    switch (inFormat) {
    case Image::RGBA_DXT1:
        outFormat = Image::RGBA_5_5_5_1;
        break;
    case Image::RGBA_DXT3:
        outFormat = Image::RGBA_4_4_4_4;
        break;
    case Image::RGBA_DXT5:
        outFormat = Image::RGBA_8_8_8_8;
        break;
    case Image::RGB_PVRTC_2BPPV1:
    case Image::RGB_PVRTC_4BPPV1:
        outFormat = Image::RGB_8_8_8;
        break;
    case Image::RGBA_PVRTC_2BPPV1:
    case Image::RGBA_PVRTC_2BPPV2:
    case Image::RGBA_PVRTC_4BPPV1:
    case Image::RGBA_PVRTC_4BPPV2:
        outFormat = Image::RGBA_8_8_8_8;
        break;
    case Image::RGB_8_ETC1:
    case Image::RGB_8_ETC2:
        outFormat = Image::RGB_8_8_8;
        break;
    case Image::RGBA_8_8_ETC2:
    case Image::RGBA_8_1_ETC2:
        outFormat = Image::RGBA_8_8_8_8;
        break;
    default:
        assert(0);
        outFormat = inFormat;
        break;
    }

    return outFormat;
}

BE_NAMESPACE_END
