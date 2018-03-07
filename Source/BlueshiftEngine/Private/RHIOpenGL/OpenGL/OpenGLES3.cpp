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
#include "OpenGLES3.h"

BE_NAMESPACE_BEGIN

const char *OpenGLES3::GLSL_VERSION_STRING = "300 es";

bool OpenGLES3::supportsFrameBufferSRGB = false;
bool OpenGLES3::supportsTextureBuffer = false;

int OpenGLES3::shaderFloatPrecisionLow = 0;
int OpenGLES3::shaderFloatPrecisionMedium = 0;
int OpenGLES3::shaderFloatPrecisionHigh = 0;
int OpenGLES3::shaderIntPrecisionLow = 0;
int OpenGLES3::shaderIntPrecisionMedium = 0;
int OpenGLES3::shaderIntPrecisionHigh = 0;

void OpenGLES3::Init() {
    OpenGLBase::Init();

#ifdef GL_EXT_sRGB_write_control
    supportsFrameBufferSRGB = gglext._GL_EXT_sRGB_write_control ? true : false;
#endif
    
#ifdef GL_EXT_texture_buffer
    supportsTextureBuffer = gglext._GL_EXT_texture_buffer ? true : false;
#endif

    int range[2];
    gglGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_LOW_FLOAT, range, &shaderFloatPrecisionLow);
    gglGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT, range, &shaderFloatPrecisionMedium);
    gglGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &shaderFloatPrecisionHigh);
    gglGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_LOW_INT, range, &shaderIntPrecisionLow);
    gglGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_MEDIUM_INT, range, &shaderIntPrecisionMedium);
    gglGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_INT, range, &shaderIntPrecisionHigh);

    BE_LOG(L"Fragment shader lowp float precision: %d\n", shaderFloatPrecisionLow);
    BE_LOG(L"Fragment shader mediump float precision: %d\n", shaderFloatPrecisionMedium);
    BE_LOG(L"Fragment shader highp float precision: %d\n", shaderFloatPrecisionHigh);
    BE_LOG(L"Fragment shader lowp int precision: %d\n", shaderFloatPrecisionLow);
    BE_LOG(L"Fragment shader mediump int precision: %d\n", shaderFloatPrecisionMedium);
    BE_LOG(L"Fragment shader highp int precision: %d\n", shaderFloatPrecisionHigh);
}

void OpenGLES3::TexBuffer(GLenum internalFormat, GLuint buffer) {
#ifdef GL_EXT_texture_buffer
    gglTexBufferEXT(GL_TEXTURE_BUFFER, internalFormat, buffer);
#endif
}

void OpenGLES3::SetTextureSwizzling(GLenum target, Image::Format format) {
    switch (format) {
    case Image::L_8:
    case Image::L_16F:
    case Image::L_32F:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        break;
    case Image::A_8:
    case Image::A_16F:
    case Image::A_32F:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_ONE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_ONE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_ONE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_RED);
        break;
    case Image::LA_8_8:
    case Image::LA_16_16:
    case Image::LA_16F_16F:
    case Image::LA_32F_32F:
    case Image::DXN1:
    case Image::DXN2:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_GREEN);
        break;
    case Image::BGRX_4_4_4_4:
    case Image::BGRX_5_5_5_1:
    case Image::BGRA_4_4_4_4:
    case Image::BGRA_5_5_5_1:
    case Image::ARGB_4_4_4_4:
    case Image::ARGB_1_5_5_5:
    case Image::RGB_5_6_5:
    case Image::BGR_8_8_8:
    case Image::BGRX_8_8_8_8:
    case Image::BGRA_8_8_8_8:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
        break;
    case Image::ABGR_8_8_8_8:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_ALPHA);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_BLUE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_GREEN);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_RED);
        break;
    case Image::ARGB_8_8_8_8:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_ALPHA);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_GREEN);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_BLUE);
    default:
        break;
    }
    
    switch (format) {
    case Image::RGBX_4_4_4_4:
    case Image::RGBX_5_5_5_1:
    case Image::BGRX_4_4_4_4:
    case Image::BGRX_5_5_5_1:
    case Image::BGRX_8_8_8_8:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        break;
    default:
        break;
    }
}

bool OpenGLES3::ImageFormatToGLFormat(Image::Format imageFormat, bool isSRGB, GLenum *glFormat, GLenum *glType, GLenum *glInternal) {
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
        if (glFormat)   *glFormat = GL_RG_INTEGER;
        if (glType)     *glType = GL_UNSIGNED_SHORT;
        if (glInternal) *glInternal = GL_RG16UI;
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
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = GL_RGB8;
        return true;
    case Image::RGBX_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = GL_RGB8;
        return true;
    case Image::BGRX_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = GL_RGB8;
        return true;
    case Image::RGBA_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        return true;
    case Image::RGBA_SNORM_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RGBA8_SNORM;
        return true;
    case Image::BGRA_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = GL_RGBA8;
        return true;
    case Image::ABGR_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = GL_RGBA8;
        return true;
    case Image::ARGB_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = GL_RGBA8;
        return true;
#ifdef GL_EXT_read_format_bgra
    case Image::RGBX_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
    case Image::BGRX_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
    case Image::RGBA_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
    case Image::BGRA_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
#endif
    case Image::ABGR_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
    case Image::ARGB_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
#ifdef GL_EXT_read_format_bgra
    case Image::RGBX_5_5_5_1:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA;
        return true;
    case Image::BGRX_5_5_5_1:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA;
        return true;
    case Image::RGBA_5_5_5_1:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
        if (glInternal) *glInternal = GL_RGB5_A1;
        return true;
    case Image::BGRA_5_5_5_1:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
        if (glInternal) *glInternal = GL_RGB5_A1;
        return true;
#endif
    case Image::ABGR_1_5_5_5:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_5_5_1;
        if (glInternal) *glInternal = GL_RGB5_A1;
        return true;
    case Image::ARGB_1_5_5_5:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_5_5_1;
        if (glInternal) *glInternal = GL_RGB5_A1;
        return true;
    case Image::RGB_5_6_5:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_6_5;
        if (glInternal) *glInternal = GL_RGB8;
        return true;
    case Image::BGR_5_6_5:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_6_5;
        if (glInternal) *glInternal = GL_RGB8;
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
#ifdef GL_EXT_texture_compression_s3tc
        if (!gglext._GL_EXT_texture_compression_s3tc) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        return true;
#else
        return false;
#endif
    case Image::RGBA_DXT3:
#ifdef GL_EXT_texture_compression_s3tc
        if (!gglext._GL_EXT_texture_compression_s3tc) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        return true;
#else
        return false;
#endif
    case Image::RGBA_DXT5:
    case Image::XGBR_DXT5:
#ifdef GL_EXT_texture_compression_s3tc
        if (!gglext._GL_EXT_texture_compression_s3tc) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        return true;
#else
        return false;
#endif
    case Image::RGB_PVRTC_2BPPV1:
        if (!gglext._GL_IMG_texture_compression_pvrtc) return false;
        if (glFormat)   *glFormat = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
        return true;
    case Image::RGB_PVRTC_4BPPV1:
        if (!gglext._GL_IMG_texture_compression_pvrtc) return false;
        if (glFormat)   *glFormat = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
        return true;
    case Image::RGBA_PVRTC_2BPPV1:
        if (!gglext._GL_IMG_texture_compression_pvrtc) return false;
        if (glFormat)   *glFormat = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        return true;
    case Image::RGBA_PVRTC_4BPPV1:
        if (!gglext._GL_IMG_texture_compression_pvrtc) return false;
        if (glFormat)   *glFormat = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        return true;
    case Image::RGBA_PVRTC_2BPPV2:
#ifdef GL_IMG_texture_compression_pvrtc2
        if (!gglext._GL_IMG_texture_compression_pvrtc2) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
        return true;
#else
        return false;
#endif
    case Image::RGBA_PVRTC_4BPPV2:
#ifdef GL_IMG_texture_compression_pvrtc2
        if (!gglext._GL_IMG_texture_compression_pvrtc2) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
        return true;
#else
        return false;
#endif
    case Image::RGB_8_ETC1:
#ifdef GL_OES_compressed_ETC1_RGB8_texture
        if (!gglext._GL_OES_compressed_ETC1_RGB8_texture) return false;
        if (glFormat)   *glFormat = GL_ETC1_RGB8_OES;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_ETC1_RGB8_OES;
        return true;
#else
        return false;
#endif
    case Image::RGB_8_ETC2:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB8_ETC2 : GL_COMPRESSED_RGB8_ETC2;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB8_ETC2 : GL_COMPRESSED_RGB8_ETC2;
        return true;
    case Image::RGBA_8_1_ETC2:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 : GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 : GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        return true;
    case Image::RGBA_8_8_ETC2:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC : GL_COMPRESSED_RGBA8_ETC2_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC : GL_COMPRESSED_RGBA8_ETC2_EAC;
        return true;
    case Image::R_11_EAC:
        if (glFormat)   *glFormat = GL_COMPRESSED_R11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_R11_EAC;
        return true;
    case Image::SignedR_11_EAC:
        if (glFormat)   *glFormat = GL_COMPRESSED_SIGNED_R11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_SIGNED_R11_EAC;
        return true;
    case Image::RG_11_11_EAC:
        if (glFormat)   *glFormat = GL_COMPRESSED_RG11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RG11_EAC;
        return true;
    case Image::SignedRG_11_11_EAC:
        if (glFormat)   *glFormat = GL_COMPRESSED_SIGNED_RG11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_SIGNED_RG11_EAC;
        return true;
    case Image::RGB_ATC:
#ifdef GL_AMD_compressed_ATC_texture
        if (!gglext._GL_AMD_compressed_ATC_texture) return false;
        if (glFormat)   *glFormat = GL_ATC_RGB_AMD;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_ATC_RGB_AMD;
        return true;
#else
        return false;
#endif
    case Image::RGBA_EA_ATC:
#ifdef GL_AMD_compressed_ATC_texture
        if (!gglext._GL_AMD_compressed_ATC_texture) return false;
        if (glFormat)   *glFormat = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
        return true;
#else
        return false;
#endif
    case Image::RGBA_IA_ATC:
#ifdef GL_AMD_compressed_ATC_texture
        if (!gglext._GL_AMD_compressed_ATC_texture) return false;
        if (glFormat)   *glFormat = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
        return true;
#else
        return false;
#endif
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

Image::Format OpenGLES3::ToCompressedImageFormat(Image::Format inFormat, bool useNormalMap) {
    if (Image::IsCompressed(inFormat)) {
        assert(0);
        return inFormat;
    }

    return inFormat; //

    int redBits, greenBits, blueBits, alphaBits;
    Image::GetBits(inFormat, &redBits, &greenBits, &blueBits, &alphaBits);

    Image::Format outFormat = inFormat;

    if (redBits > 0 && greenBits > 0 && blueBits > 0) {
        if (Image::IsFloatFormat(inFormat)) {
            if (alphaBits == 0) {
                outFormat = Image::RGBE_9_9_9_5;
            }
        } else {
            if (alphaBits == 0) {
                outFormat = Image::RGB_8_ETC1;
            } else if (alphaBits == 1) {
                outFormat = Image::RGBA_8_1_ETC2;
            } else {
                outFormat = Image::RGBA_8_8_ETC2;
            }
        }
    }

    return outFormat;
}

Image::Format OpenGLES3::ToUncompressedImageFormat(Image::Format inFormat) {
    if (!Image::IsCompressed(inFormat)) {
        assert(0);
        return inFormat;
    }

    Image::Format outFormat;

    switch (inFormat) {
    case Image::DXN1:
    case Image::DXN2:
    case Image::RGB_PVRTC_2BPPV1:
    case Image::RGB_PVRTC_4BPPV1:
    case Image::RGB_8_ETC1:
    case Image::RGB_8_ETC2:
    case Image::RGB_ATC:
        outFormat = Image::RGB_8_8_8;
        break;
    case Image::RGBA_DXT1:
    case Image::RGBA_DXT3:
    case Image::RGBA_DXT5:
    case Image::RGBA_PVRTC_2BPPV1:
    case Image::RGBA_PVRTC_4BPPV1:
    case Image::RGBA_PVRTC_2BPPV2:
    case Image::RGBA_PVRTC_4BPPV2:
    case Image::RGBA_8_1_ETC2:
    case Image::RGBA_8_8_ETC2:
    case Image::RGBA_EA_ATC:
    case Image::RGBA_IA_ATC:
        outFormat = Image::RGBA_8_8_8_8;
        break;
    case Image::R_11_EAC:
    case Image::SignedR_11_EAC:
        outFormat = Image::R_8;
        break;
    case Image::RG_11_11_EAC:
    case Image::SignedRG_11_11_EAC:
        outFormat = Image::RG_8_8;
        break;
    default:
        assert(0);
        outFormat = inFormat;
        break;
    }

    return outFormat;
}

BE_NAMESPACE_END
