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
bool OpenGLES3::supportsTimestampQueries = false;

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

uint32_t OpenGLES3::QueryResult32(GLuint queryId) {
    GLuint result;
    gglGetQueryObjectuiv(queryId, GL_QUERY_RESULT, &result);
    return result;
}

uint64_t OpenGLES3::QueryResult64(GLuint queryId) {
#ifdef GL_EXT_disjoint_timer_query
    GLuint64 result;
    gglGetQueryObjectui64vEXT(queryId, GL_QUERY_RESULT, &result);
    return result;
#else
    return 0;
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

void OpenGLES3::SetTextureSwizzling(GLenum target, Image::Format format) {
    switch (format) {
    case Image::Format::L8:
    case Image::Format::L16_FLOAT:
    case Image::Format::L32_FLOAT:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        break;
    case Image::Format::A8:
    case Image::Format::A16_FLOAT:
    case Image::Format::A32_FLOAT:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_ONE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_ONE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_ONE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_RED);
        break;
    case Image::Format::L8A8:
    case Image::Format::L16A16_FLOAT:
    case Image::Format::L32A32_FLOAT:
    case Image::Format::DXN1:
    case Image::Format::DXN2:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_GREEN);
        break;
    case Image::Format::B4G4R4X4:
    case Image::Format::B5G5R5X1:
    case Image::Format::B4G4R4A4:
    case Image::Format::B5G5R5A1:
    case Image::Format::A4R4G4B4:
    case Image::Format::A1R5G5B5:
    case Image::Format::R5G6B5:
    case Image::Format::B8G8R8:
    case Image::Format::B8G8R8X8:
    case Image::Format::B8G8R8A8:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_RED);
        break;
    case Image::Format::A8B8G8R8:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_ALPHA);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_BLUE);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_GREEN);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_RED);
        break;
    case Image::Format::A8R8G8B8:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_R, GL_ALPHA);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_G, GL_RED);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_B, GL_GREEN);
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_BLUE);
    default:
        break;
    }
    
    switch (format) {
    case Image::Format::R4G4B4X4:
    case Image::Format::R5G5B5X1:
    case Image::Format::B4G4R4X4:
    case Image::Format::B5G5R5X1:
    case Image::Format::B8G8R8X8:
        gglTexParameteri(target, GL_TEXTURE_SWIZZLE_A, GL_ONE);
        break;
    default:
        break;
    }
}

bool OpenGLES3::ImageFormatToGLFormat(Image::Format imageFormat, bool isSRGB, GLenum *glFormat, GLenum *glType, GLenum *glInternal) {
    switch (imageFormat) {
    case Image::Format::L8:
    case Image::Format::A8:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = GL_R8;
        return true;
    case Image::Format::R8:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
#ifdef GL_EXT_texture_sRGB_R8
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_texture_sRGB_R8) ? GL_SR8_EXT : GL_R8;
#else
        if (glInternal) *glInternal = GL_R8;
#endif
        return true;
    case Image::Format::R8_SNORM:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_R8_SNORM;
        return true;
    case Image::Format::R8_SINT:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_R8I;
        return true;
    case Image::Format::R8_UINT:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_R8UI;
        return true;
    case Image::Format::L8A8:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = GL_RG8;
        return true;
    case Image::Format::R8G8:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
#ifdef GL_EXT_texture_sRGB_RG8
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_texture_sRGB_RG8) ? GL_SRG8_EXT : GL_RG8;
#else
        if (glInternal) *glInternal = GL_RG8;
#endif
        return true;
    case Image::Format::R8G8_SNORM:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RG8_SNORM;
        return true;
    case Image::Format::R8G8_SINT:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RG8I;
        return true;
    case Image::Format::R8G8_UINT:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RG8UI;
        return true;
    case Image::Format::R8G8B8:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB8;
        return true;
    case Image::Format::R8G8B8_SNORM:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RGB8_SNORM;
        return true;
    case Image::Format::R8G8B8_SINT:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RGB8I;
        return true;
    case Image::Format::R8G8B8_UINT:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RGB8UI;
        return true;
    case Image::Format::R8G8B8X8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB8;
        return true;
    case Image::Format::B8G8R8X8:
        if (!gglext._GL_EXT_texture_format_BGRA8888) return false;
        if (glFormat)   *glFormat = GL_BGRA_EXT;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8 : GL_RGB8;
        return true;
    case Image::Format::R8G8B8A8:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        return true;
    case Image::Format::R8G8B8A8_SNORM:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RGBA8_SNORM;
        return true;
    case Image::Format::R8G8B8A8_SINT:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RGBA8I;
        return true;
    case Image::Format::R8G8B8A8_UINT:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_BYTE;
        if (glInternal) *glInternal = GL_RGBA8UI;
        return true;
    case Image::Format::B8G8R8A8:
        if (!gglext._GL_EXT_texture_format_BGRA8888) return false;
        if (glFormat)   *glFormat = GL_BGRA_EXT;
        if (glType)     *glType = GL_UNSIGNED_BYTE;
        if (glInternal) *glInternal = isSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        return true;
#ifdef GL_EXT_read_format_bgra
    case Image::Format::R4G4B4X4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
    case Image::Format::B4G4R4X4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
    case Image::Format::R4G4B4A4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
    case Image::Format::B4G4R4A4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
#endif
    case Image::Format::A4B4G4R4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
    case Image::Format::A4R4G4B4:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_4_4_4_4;
        if (glInternal) *glInternal = GL_RGBA4;
        return true;
#ifdef GL_EXT_read_format_bgra
    case Image::Format::R5G5B5X1:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA;
        return true;
    case Image::Format::B5G5R5X1:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
        if (glInternal) *glInternal = GL_RGBA;
        return true;
    case Image::Format::R5G5B5A1:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
        if (glInternal) *glInternal = GL_RGB5_A1;
        return true;
    case Image::Format::B5G5R5A1:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT;
        if (glInternal) *glInternal = GL_RGB5_A1;
        return true;
#endif
    case Image::Format::A1B5G5R5:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_5_5_1;
        if (glInternal) *glInternal = GL_RGB5_A1;
        return true;
    case Image::Format::A1R5G5B5:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_5_5_1;
        if (glInternal) *glInternal = GL_RGB5_A1;
        return true;
    case Image::Format::R5G6B5:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_6_5;
        if (glInternal) *glInternal = GL_RGB8;
        return true;
    case Image::Format::B5G6R5:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_SHORT_5_6_5;
        if (glInternal) *glInternal = GL_RGB8;
        return true;
    case Image::Format::R9G9B9E5_FLOAT:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_INT_5_9_9_9_REV;
        if (glInternal) *glInternal = GL_RGB9_E5;
        return true;
    case Image::Format::L16_FLOAT:
    case Image::Format::A16_FLOAT:
    case Image::Format::R16_FLOAT:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_HALF_FLOAT;
        if (glInternal) *glInternal = GL_R16F;
        return true;
    case Image::Format::L32_FLOAT:
    case Image::Format::A32_FLOAT:
    case Image::Format::R32_FLOAT:
        if (glFormat)   *glFormat = GL_RED;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_R32F;
        return true;
    case Image::Format::L16A16_FLOAT:
    case Image::Format::R16G16_FLOAT:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_HALF_FLOAT;
        if (glInternal) *glInternal = GL_RG16F;
        return true;
    case Image::Format::L32A32_FLOAT:
    case Image::Format::R32G32_FLOAT:
        if (glFormat)   *glFormat = GL_RG;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_RG32F;
        return true;
    case Image::Format::R11G11B10_FLOAT:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_UNSIGNED_INT_10F_11F_11F_REV;
        if (glInternal) *glInternal = GL_R11F_G11F_B10F;
        return true;
    case Image::Format::R16G16B16_FLOAT:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_HALF_FLOAT;
        if (glInternal) *glInternal = GL_RGB16F;
        return true;
    case Image::Format::R32G32B32_FLOAT:
        if (glFormat)   *glFormat = GL_RGB;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_RGB32F;
        return true;
    case Image::Format::R16G16B16A16_FLOAT:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_HALF_FLOAT;
        if (glInternal) *glInternal = GL_RGBA16F;
        return true;
    case Image::Format::R32G32B32A32_FLOAT:
        if (glFormat)   *glFormat = GL_RGBA;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_RGBA32F;
        return true;
    case Image::Format::DXT1:
#ifdef GL_EXT_texture_compression_s3tc
        if (!gglext._GL_EXT_texture_compression_s3tc) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        return true;
#else
        return false;
#endif
    case Image::Format::DXT3:
#ifdef GL_EXT_texture_compression_s3tc
        if (!gglext._GL_EXT_texture_compression_s3tc) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        return true;
#else
        return false;
#endif
    case Image::Format::DXT5:
    case Image::Format::DXT5XGBR:
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
    case Image::Format::PVRTC12:
        if (!gglext._GL_IMG_texture_compression_pvrtc) return false;
        if (glFormat)   *glFormat = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
        return true;
    case Image::Format::PVRTC14:
        if (!gglext._GL_IMG_texture_compression_pvrtc) return false;
        if (glFormat)   *glFormat = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
        return true;
    case Image::Format::PVRTC12A:
        if (!gglext._GL_IMG_texture_compression_pvrtc) return false;
        if (glFormat)   *glFormat = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_2BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
        return true;
    case Image::Format::PVRTC14A:
        if (!gglext._GL_IMG_texture_compression_pvrtc) return false;
        if (glFormat)   *glFormat = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = (isSRGB && gglext._GL_EXT_pvrtc_sRGB) ? GL_COMPRESSED_SRGB_ALPHA_PVRTC_4BPPV1_EXT : GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        return true;
    case Image::Format::PVRTC22A:
#ifdef GL_IMG_texture_compression_pvrtc2
        if (!gglext._GL_IMG_texture_compression_pvrtc2) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
        return true;
#else
        return false;
#endif
    case Image::Format::PVRTC24A:
#ifdef GL_IMG_texture_compression_pvrtc2
        if (!gglext._GL_IMG_texture_compression_pvrtc2) return false;
        if (glFormat)   *glFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
        return true;
#else
        return false;
#endif
    case Image::Format::ETC1:
#ifdef GL_OES_compressed_ETC1_RGB8_texture
        if (!gglext._GL_OES_compressed_ETC1_RGB8_texture) return false;
        if (glFormat)   *glFormat = GL_ETC1_RGB8_OES;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_ETC1_RGB8_OES;
        return true;
#else
        return false;
#endif
    case Image::Format::ETC2:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB8_ETC2 : GL_COMPRESSED_RGB8_ETC2;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB8_ETC2 : GL_COMPRESSED_RGB8_ETC2;
        return true;
    case Image::Format::ETC2A1:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 : GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 : GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        return true;
    case Image::Format::ETC2A:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC : GL_COMPRESSED_RGBA8_ETC2_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC : GL_COMPRESSED_RGBA8_ETC2_EAC;
        return true;
    case Image::Format::EACR11:
        if (glFormat)   *glFormat = GL_COMPRESSED_R11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_R11_EAC;
        return true;
    case Image::Format::EACR11_SNORM:
        if (glFormat)   *glFormat = GL_COMPRESSED_SIGNED_R11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_SIGNED_R11_EAC;
        return true;
    case Image::Format::EACRG11:
        if (glFormat)   *glFormat = GL_COMPRESSED_RG11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RG11_EAC;
        return true;
    case Image::Format::EACRG11_SNORM:
        if (glFormat)   *glFormat = GL_COMPRESSED_SIGNED_RG11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_SIGNED_RG11_EAC;
        return true;
    case Image::Format::ATC:
#ifdef GL_AMD_compressed_ATC_texture
        if (!gglext._GL_AMD_compressed_ATC_texture) return false;
        if (glFormat)   *glFormat = GL_ATC_RGB_AMD;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_ATC_RGB_AMD;
        return true;
#else
        return false;
#endif
    case Image::Format::ATCE:
#ifdef GL_AMD_compressed_ATC_texture
        if (!gglext._GL_AMD_compressed_ATC_texture) return false;
        if (glFormat)   *glFormat = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
        return true;
#else
        return false;
#endif
    case Image::Format::ATCI:
#ifdef GL_AMD_compressed_ATC_texture
        if (!gglext._GL_AMD_compressed_ATC_texture) return false;
        if (glFormat)   *glFormat = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
        return true;
#else
        return false;
#endif
    case Image::Format::D16:
        if (glFormat)   *glFormat = GL_DEPTH_COMPONENT;
        if (glType)     *glType = GL_UNSIGNED_SHORT;
        if (glInternal) *glInternal = GL_DEPTH_COMPONENT16;
        return true;
    case Image::Format::D24X8:
        if (glFormat)   *glFormat = GL_DEPTH_COMPONENT;
        if (glType)     *glType = GL_UNSIGNED_INT;
        if (glInternal) *glInternal = GL_DEPTH_COMPONENT24;
        return true;
    case Image::Format::D32_FLOAT:
        if (glFormat)   *glFormat = GL_DEPTH_COMPONENT;
        if (glType)     *glType = GL_FLOAT;
        if (glInternal) *glInternal = GL_DEPTH_COMPONENT32F;
        return true;
    case Image::Format::D24S8:
        if (glFormat)   *glFormat = GL_DEPTH_STENCIL;
        if (glType)     *glType = GL_UNSIGNED_INT_24_8;
        if (glInternal) *glInternal = GL_DEPTH24_STENCIL8;
        return true;
    case Image::Format::D32_FLOAT_S8X24:
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
        if (Image::IsFloatFormat(inFormat) || Image::IsHalfFormat(inFormat)) {
            if (alphaBits == 0) {
                outFormat = Image::Format::R9G9B9E5_FLOAT;
            }
        } else {
            if (alphaBits == 0) {
                outFormat = Image::Format::ETC1;
            } else if (alphaBits == 1) {
                outFormat = Image::Format::ETC2A1;
            } else {
                outFormat = Image::Format::ETC2A;
            }
        }
    }

    return outFormat;
}

BE_NAMESPACE_END
