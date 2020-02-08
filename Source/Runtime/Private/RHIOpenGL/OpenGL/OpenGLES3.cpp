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

bool OpenGLES3::supportsMapBufferOES = false;
bool OpenGLES3::supportsFrameBufferSRGB = false;
bool OpenGLES3::supportsTextureBuffer = false;
bool OpenGLES3::supportsTimestampQueries = false;

int OpenGLES3::shaderFloatPrecisionLow = 0;
int OpenGLES3::shaderFloatPrecisionMedium = 0;
int OpenGLES3::shaderFloatPrecisionHigh = 0;
int OpenGLES3::shaderIntPrecisionLow = 0;
int OpenGLES3::shaderIntPrecisionMedium = 0;
int OpenGLES3::shaderIntPrecisionHigh = 0;

void OpenGLES3::Init() {
    OpenGLBase::Init();

#ifdef GL_OES_mapbuffer
    supportsMapBufferOES = gglext._GL_OES_mapbuffer ? true : false;
#endif

#ifdef GL_EXT_sRGB_write_control
    supportsFrameBufferSRGB = gglext._GL_EXT_sRGB_write_control ? true : false;
#endif
    
#ifdef GL_EXT_texture_buffer
    supportsTextureBuffer = gglext._GL_EXT_texture_buffer ? true : false;
#endif

#ifdef GL_EXT_disjoint_timer_query
    supportsTimestampQueries = gglext._GL_EXT_disjoint_timer_query ? true : false;
#endif

    int range[2];
    gglGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_LOW_FLOAT, range, &shaderFloatPrecisionLow);
    gglGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT, range, &shaderFloatPrecisionMedium);
    gglGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &shaderFloatPrecisionHigh);
    gglGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_LOW_INT, range, &shaderIntPrecisionLow);
    gglGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_MEDIUM_INT, range, &shaderIntPrecisionMedium);
    gglGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_INT, range, &shaderIntPrecisionHigh);

    BE_LOG("Fragment shader lowp float precision: %d\n", shaderFloatPrecisionLow);
    BE_LOG("Fragment shader mediump float precision: %d\n", shaderFloatPrecisionMedium);
    BE_LOG("Fragment shader highp float precision: %d\n", shaderFloatPrecisionHigh);
    BE_LOG("Fragment shader lowp int precision: %d\n", shaderFloatPrecisionLow);
    BE_LOG("Fragment shader mediump int precision: %d\n", shaderFloatPrecisionMedium);
    BE_LOG("Fragment shader highp int precision: %d\n", shaderFloatPrecisionHigh);
}

void OpenGLES3::QueryTimestampCounter(GLuint queryId) {
#ifdef GL_EXT_disjoint_timer_query
    gglQueryCounterEXT(queryId, GL_TIMESTAMP_EXT);
#endif
}

void OpenGLES3::DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
#ifdef GL_EXT_draw_elements_base_vertex
    if (gglext._GL_EXT_draw_elements_base_vertex) {
        gglDrawElementsBaseVertexEXT(mode, count, type, indices, basevertex);
    }
#endif
}

void OpenGLES3::DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex) {
#ifdef GL_EXT_draw_elements_base_vertex
    if (gglext._GL_EXT_draw_elements_base_vertex) {
        gglDrawElementsInstancedBaseVertexEXT(mode, count, type, indices, instancecount, basevertex);
    }
#endif
}

void OpenGLES3::DrawElementsIndirect(GLenum mode, GLenum type, const void *indirect) {
#ifdef GL_EXT_multi_draw_indirect
    if (gglext._GL_EXT_multi_draw_indirect) {
        gglMultiDrawElementsIndirectEXT(mode, type, indirect, 1, 0);
    }
#endif
}

void OpenGLES3::MultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride) {
#ifdef GL_EXT_multi_draw_indirect
    if (gglext._GL_EXT_multi_draw_indirect) {
        gglMultiDrawElementsIndirectEXT(mode, type, indirect, drawcount, stride);
    }
#endif
}

void OpenGLES3::TexBuffer(GLenum internalFormat, GLuint buffer) {
#ifdef GL_EXT_texture_buffer
    if (gglext._GL_EXT_texture_buffer) {
        gglTexBufferEXT(GL_TEXTURE_BUFFER, internalFormat, buffer);
    }
#endif
}

void *OpenGLES3::MapBuffer(GLenum target, GLenum access) { 
#ifdef GL_OES_mapbuffer
    return gglMapBufferOES(target, access);
#else
    return nullptr;
#endif
}

bool OpenGLES3::UnmapBuffer(GLenum target) {
#ifdef GL_OES_mapbuffer
    return gglUnmapBufferOES(target);
#else
    return false;
#endif
}

void OpenGLES3::SetTextureSwizzling(GLenum target, Image::Format::Enum format) {
    switch (format) {
    case Image::Format::L_8:
    case Image::Format::L_16F:
    case Image::Format::L_32F:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        break;
    case Image::Format::A_8:
    case Image::Format::A_16F:
    case Image::Format::A_32F:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_ONE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_ONE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_ONE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_RED);
        break;
    case Image::Format::LA_8_8:
    case Image::Format::LA_16F_16F:
    case Image::Format::LA_32F_32F:
    case Image::Format::DXN1:
    case Image::Format::DXN2:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_GREEN);
        break;
    case Image::Format::BGRX_4_4_4_4:
    case Image::Format::BGRX_5_5_5_1:
    case Image::Format::BGRA_4_4_4_4:
    case Image::Format::BGRA_5_5_5_1:
    case Image::Format::ARGB_4_4_4_4:
    case Image::Format::ARGB_1_5_5_5:
    case Image::Format::RGB_5_6_5:
    case Image::Format::BGR_8_8_8:
    case Image::Format::BGRX_8_8_8_8:
    case Image::Format::BGRA_8_8_8_8:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
        break;
    case Image::Format::ABGR_8_8_8_8:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_ALPHA);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_BLUE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_GREEN);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_RED);
        break;
    case Image::Format::ARGB_8_8_8_8:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_ALPHA);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_GREEN);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_BLUE);
    default:
        break;
    }
    
    switch (format) {
    case Image::Format::RGBX_4_4_4_4:
    case Image::Format::RGBX_5_5_5_1:
    case Image::Format::BGRX_4_4_4_4:
    case Image::Format::BGRX_5_5_5_1:
    case Image::Format::BGRX_8_8_8_8:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        break;
    default:
        break;
    }
}

bool OpenGLES3::ImageFormatToGLFormat(Image::Format::Enum imageFormat, bool isSRGB, GLenum *glFormat, GLenum *glType, GLenum *glInternal) {
    switch (imageFormat) {
    case Image::Format::L_8:
    case Image::Format::A_8:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = GL_R8;
        return true;
    case Image::Format::R_8:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
#ifdef GL_EXT_texture_sRGB_R8
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_texture_sRGB_R8) ? GL_SR8_EXT : GL_R8;
#else
        if (glInternal) *glInternal = GL_R8;
#endif
        return true;
    case Image::Format::R_8_SNORM:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_R8_SNORM;
        return true;
    case Image::Format::LA_8_8:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = GL_RG8;
        return true;
    case Image::Format::RG_8_8:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
#ifdef GL_EXT_texture_sRGB_RG8
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_texture_sRGB_RG8) ? GL_SRG8_EXT : GL_RG8;
#else
        if (glInternal) *glInternal = GL_RG8;
#endif
        return true;
    case Image::Format::RG_8_8_SNORM:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RG8_SNORM;
        return true;
    case Image::Format::RGB_8_8_8:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB8;
        return true;
    case Image::Format::RGB_8_8_8_SNORM:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RGB8_SNORM;
        return true;
    case Image::Format::RGBX_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB8;
        return true;
    case Image::Format::BGRX_8_8_8_8:
        if (!gglext._GL_EXT_texture_format_BGRA8888) return false;
        if (glFormat)   *glFormat = GL_BGRA_EXT;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB8;
        return true;
    case Image::Format::RGBA_8_8_8_8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        return true;
    case Image::Format::RGBA_8_8_8_8_SNORM:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RGBA8_SNORM;
        return true;
    case Image::Format::BGRA_8_8_8_8:
        if (!gglext._GL_EXT_texture_format_BGRA8888) return false;
        if (glFormat)   *glFormat = GL_BGRA_EXT;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        return true;
#ifdef GL_EXT_read_format_bgra
    case Image::Format::RGBX_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
    case Image::Format::BGRX_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
    case Image::Format::RGBA_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
    case Image::Format::BGRA_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
#endif
    case Image::Format::ABGR_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
    case Image::Format::ARGB_4_4_4_4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
#ifdef GL_EXT_read_format_bgra
    case Image::Format::RGBX_5_5_5_1:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA;
        return true;
    case Image::Format::BGRX_5_5_5_1:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA;
        return true;
    case Image::Format::RGBA_5_5_5_1:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
        if (glInternal) *glInternal = GL_RGB5_A1;
        return true;
    case Image::Format::BGRA_5_5_5_1:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
        if (glInternal) *glInternal = GL_RGB5_A1;
        return true;
#endif
    case Image::Format::ABGR_1_5_5_5:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_5_5_1;
        if (glInternal) *glInternal = GL_RGB5_A1;
        return true;
    case Image::Format::ARGB_1_5_5_5:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_5_5_1;
        if (glInternal) *glInternal = GL_RGB5_A1;
        return true;
    case Image::Format::RGB_5_6_5:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_6_5;
        if (glInternal) *glInternal = GL_RGB8;
        return true;
    case Image::Format::BGR_5_6_5:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_6_5;
        if (glInternal) *glInternal = GL_RGB8;
        return true;
    case Image::Format::RGBE_9_9_9_5:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_INT_5_9_9_9_REV;
        if (glInternal) *glInternal = GL_RGB9_E5;
        return true;
    case Image::Format::L_16F:
    case Image::Format::A_16F:
    case Image::Format::R_16F:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_HALF_FLOAT;
        if (glInternal) *glInternal = GL_R16F;
        return true;
    case Image::Format::L_32F:
    case Image::Format::A_32F:
    case Image::Format::R_32F:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_R32F;
        return true;
    case Image::Format::LA_16F_16F:
    case Image::Format::RG_16F_16F:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_HALF_FLOAT;
        if (glInternal) *glInternal = GL_RG16F;
        return true;
    case Image::Format::LA_32F_32F:
    case Image::Format::RG_32F_32F:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_RG32F;
        return true;
    case Image::Format::RGB_11F_11F_10F:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_INT_10F_11F_11F_REV;
        if (glInternal) *glInternal = GL_R11F_G11F_B10F;
        return true;
    case Image::Format::RGB_16F_16F_16F:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_HALF_FLOAT;
        if (glInternal) *glInternal = GL_RGB16F;
        return true;
    case Image::Format::RGB_32F_32F_32F:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_RGB32F;
        return true;
    case Image::Format::RGBA_16F_16F_16F_16F:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_HALF_FLOAT;
        if (glInternal) *glInternal = GL_RGBA16F;
        return true;
    case Image::Format::RGBA_32F_32F_32F_32F:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_RGBA32F;
        return true;
    case Image::Format::RGBA_DXT1:
#ifdef GL_EXT_texture_compression_s3tc
        if (!gglext._GL_EXT_texture_compression_s3tc) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        return true;
#else
        return false;
#endif
    case Image::Format::RGBA_DXT3:
#ifdef GL_EXT_texture_compression_s3tc
        if (!gglext._GL_EXT_texture_compression_s3tc) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        return true;
#else
        return false;
#endif
    case Image::Format::RGBA_DXT5:
    case Image::Format::XGBR_DXT5:
#ifdef GL_EXT_texture_compression_s3tc
        if (!gglext._GL_EXT_texture_compression_s3tc) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        return true;
#else
        return false;
#endif
    case Image::Format::DXN1:
#ifdef GL_EXT_texture_compression_rgtc
        if (!gglext._GL_EXT_texture_compression_rgtc) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RED_RGTC1;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RED_RGTC1;
        return true;
#else
        return false;
#endif
    case Image::Format::DXN2:
#ifdef GL_EXT_texture_compression_rgtc
        if (!gglext._GL_EXT_texture_compression_rgtc) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RG_RGTC2;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RG_RGTC2;
        return true;
#else
        return false;
#endif
    case Image::Format::RGB_PVRTC_2BPPV1:
        if (!gglext._GL_IMG_texture_compression_pvrtc) return false;
        if (glFormat)   *glFormat = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
        return true;
    case Image::Format::RGB_PVRTC_4BPPV1:
        if (!gglext._GL_IMG_texture_compression_pvrtc) return false;
        if (glFormat)   *glFormat = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
        return true;
    case Image::Format::RGBA_PVRTC_2BPPV1:
        if (!gglext._GL_IMG_texture_compression_pvrtc) return false;
        if (glFormat)   *glFormat = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        return true;
    case Image::Format::RGBA_PVRTC_4BPPV1:
        if (!gglext._GL_IMG_texture_compression_pvrtc) return false;
        if (glFormat)   *glFormat = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        return true;
    case Image::Format::RGBA_PVRTC_2BPPV2:
#ifdef GL_IMG_texture_compression_pvrtc2
        if (!gglext._GL_IMG_texture_compression_pvrtc2) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
        return true;
#else
        return false;
#endif
    case Image::Format::RGBA_PVRTC_4BPPV2:
#ifdef GL_IMG_texture_compression_pvrtc2
        if (!gglext._GL_IMG_texture_compression_pvrtc2) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
        return true;
#else
        return false;
#endif
    case Image::Format::RGB_8_ETC1:
#ifdef GL_OES_compressed_ETC1_RGB8_texture
        if (!gglext._GL_OES_compressed_ETC1_RGB8_texture) return false;
        if (glFormat)   *glFormat = GL_ETC1_RGB8_OES;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_ETC1_RGB8_OES;
        return true;
#else
        return false;
#endif
    case Image::Format::RGB_8_ETC2:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB8_ETC2 : GL_COMPRESSED_RGB8_ETC2;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB8_ETC2 : GL_COMPRESSED_RGB8_ETC2;
        return true;
    case Image::Format::RGBA_8_1_ETC2:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 : GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 : GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        return true;
    case Image::Format::RGBA_8_8_ETC2:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC : GL_COMPRESSED_RGBA8_ETC2_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC : GL_COMPRESSED_RGBA8_ETC2_EAC;
        return true;
    case Image::Format::R_11_EAC:
        if (glFormat)   *glFormat = GL_COMPRESSED_R11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_R11_EAC;
        return true;
    case Image::Format::SignedR_11_EAC:
        if (glFormat)   *glFormat = GL_COMPRESSED_SIGNED_R11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_SIGNED_R11_EAC;
        return true;
    case Image::Format::RG_11_11_EAC:
        if (glFormat)   *glFormat = GL_COMPRESSED_RG11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RG11_EAC;
        return true;
    case Image::Format::SignedRG_11_11_EAC:
        if (glFormat)   *glFormat = GL_COMPRESSED_SIGNED_RG11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_SIGNED_RG11_EAC;
        return true;
    case Image::Format::RGB_ATC:
#ifdef GL_AMD_compressed_ATC_texture
        if (!gglext._GL_AMD_compressed_ATC_texture) return false;
        if (glFormat)   *glFormat = GL_ATC_RGB_AMD;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_ATC_RGB_AMD;
        return true;
#else
        return false;
#endif
    case Image::Format::RGBA_EA_ATC:
#ifdef GL_AMD_compressed_ATC_texture
        if (!gglext._GL_AMD_compressed_ATC_texture) return false;
        if (glFormat)   *glFormat = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
        return true;
#else
        return false;
#endif
    case Image::Format::RGBA_IA_ATC:
#ifdef GL_AMD_compressed_ATC_texture
        if (!gglext._GL_AMD_compressed_ATC_texture) return false;
        if (glFormat)   *glFormat = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
        return true;
#else
        return false;
#endif
    case Image::Format::Depth_16:
        if (glFormat)   *glFormat = GL_DEPTH_COMPONENT;
        if (glType)     *glType = GL_UNSIGNED_SHORT;
        if (glInternal) *glInternal = GL_DEPTH_COMPONENT16;
        return true;
    case Image::Format::Depth_24:
        if (glFormat)   *glFormat = GL_DEPTH_COMPONENT;
        if (glType)     *glType = GL_UNSIGNED_INT;
        if (glInternal) *glInternal = GL_DEPTH_COMPONENT24;
        return true;
    case Image::Format::Depth_32F:
        if (glFormat)   *glFormat = GL_DEPTH_COMPONENT;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_DEPTH_COMPONENT32F;
        return true;
    case Image::Format::DepthStencil_24_8:
        if (glFormat)   *glFormat = GL_DEPTH_STENCIL;
        if (glType)     *glType = GL_UNSIGNED_INT_24_8;
        if (glInternal) *glInternal = GL_DEPTH24_STENCIL8;
        return true;
    case Image::Format::DepthStencil_32F_8:
        if (glFormat)   *glFormat = GL_DEPTH_STENCIL;
        if (glType)     *glType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
        if (glInternal) *glInternal = GL_DEPTH32F_STENCIL8;
        return true;
    default:
        return OpenGLBase::ImageFormatToGLFormat(imageFormat, isSRGB, glFormat, glType, glInternal);
    }

    return false;
}

Image::Format::Enum OpenGLES3::ToCompressedImageFormat(Image::Format::Enum inFormat, bool useNormalMap) {
    if (Image::IsCompressed(inFormat)) {
        assert(0);
        return inFormat;
    }

    return inFormat; //

    int redBits, greenBits, blueBits, alphaBits;
    Image::GetBits(inFormat, &redBits, &greenBits, &blueBits, &alphaBits);

    Image::Format::Enum outFormat = inFormat;

    if (redBits > 0 && greenBits > 0 && blueBits > 0) {
        if (Image::IsFloatFormat(inFormat)) {
            if (alphaBits == 0) {
                outFormat = Image::Format::RGBE_9_9_9_5;
            }
        } else {
            if (alphaBits == 0) {
                outFormat = Image::Format::RGB_8_ETC1;
            } else if (alphaBits == 1) {
                outFormat = Image::Format::RGBA_8_1_ETC2;
            } else {
                outFormat = Image::Format::RGBA_8_8_ETC2;
            }
        }
    }

    return outFormat;
}

Image::Format::Enum OpenGLES3::ToUncompressedImageFormat(Image::Format::Enum inFormat) {
    if (!Image::IsCompressed(inFormat)) {
        assert(0);
        return inFormat;
    }

    Image::Format::Enum outFormat;

    switch (inFormat) {
    case Image::Format::DXN1:
    case Image::Format::DXN2:
    case Image::Format::RGB_PVRTC_2BPPV1:
    case Image::Format::RGB_PVRTC_4BPPV1:
    case Image::Format::RGB_8_ETC1:
    case Image::Format::RGB_8_ETC2:
    case Image::Format::RGB_ATC:
        outFormat = Image::Format::RGB_8_8_8;
        break;
    case Image::Format::RGBA_DXT1:
    case Image::Format::RGBA_DXT3:
    case Image::Format::RGBA_DXT5:
    case Image::Format::RGBA_PVRTC_2BPPV1:
    case Image::Format::RGBA_PVRTC_4BPPV1:
    case Image::Format::RGBA_PVRTC_2BPPV2:
    case Image::Format::RGBA_PVRTC_4BPPV2:
    case Image::Format::RGBA_8_1_ETC2:
    case Image::Format::RGBA_8_8_ETC2:
    case Image::Format::RGBA_EA_ATC:
    case Image::Format::RGBA_IA_ATC:
        outFormat = Image::Format::RGBA_8_8_8_8;
        break;
    case Image::Format::R_11_EAC:
    case Image::Format::SignedR_11_EAC:
        outFormat = Image::Format::R_8;
        break;
    case Image::Format::RG_11_11_EAC:
    case Image::Format::SignedRG_11_11_EAC:
        outFormat = Image::Format::RG_8_8;
        break;
    default:
        assert(0);
        outFormat = inFormat;
        break;
    }

    return outFormat;
}

BE_NAMESPACE_END
