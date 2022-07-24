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
#include "RHI/RHIOpenGL.h"
#include "RGLInternal.h"

BE_NAMESPACE_BEGIN

static const GLenum ToGLTextureTarget(RHI::TextureType::Enum type) {
    switch (type) {
    case RHI::TextureType::Texture2D:
        return GL_TEXTURE_2D;
    case RHI::TextureType::TextureRectangle:
        if (OpenGL::SupportsTextureRectangle()) {
            return GL_TEXTURE_RECTANGLE;
        }
        return GL_TEXTURE_2D;
    case RHI::TextureType::Texture3D:
        return GL_TEXTURE_3D;
    case RHI::TextureType::TextureCubeMap:
        return GL_TEXTURE_CUBE_MAP;
    case RHI::TextureType::Texture2DArray:
        return GL_TEXTURE_2D_ARRAY;
    case RHI::TextureType::TextureBuffer:
        return GL_TEXTURE_BUFFER;
    default:
        assert(0);
        return 0;
    }
}

static const GLenum ToGLTextureMinFilter(RHI::TextureFilter::Enum filter) {
    switch (filter) {
    case RHI::TextureFilter::Nearest:
        return GL_NEAREST;
    case RHI::TextureFilter::Linear:
        return GL_LINEAR;
    case RHI::TextureFilter::NearestMipmapNearest:
        return GL_NEAREST_MIPMAP_NEAREST;
    case RHI::TextureFilter::LinearMipmapNearest:
        return GL_LINEAR_MIPMAP_NEAREST;
    case RHI::TextureFilter::NearestMipmapLinear:
        return GL_NEAREST_MIPMAP_LINEAR;
    case RHI::TextureFilter::LinearMipmapLinear:
        return GL_LINEAR_MIPMAP_LINEAR;
    default:
        assert(0);
        return 0;
    }
}

static const GLenum ToGLTextureMagFilter(RHI::TextureFilter::Enum filter) {
    switch (filter) {
    case RHI::TextureFilter::Nearest:
        return GL_NEAREST;
    case RHI::TextureFilter::Linear:
        return GL_LINEAR;
    case RHI::TextureFilter::NearestMipmapNearest:
        return GL_NEAREST;
    case RHI::TextureFilter::LinearMipmapNearest:
        return GL_LINEAR;
    case RHI::TextureFilter::NearestMipmapLinear:
        return GL_NEAREST;
    case RHI::TextureFilter::LinearMipmapLinear:
        return GL_LINEAR;
    default:
        assert(0);
        return 0;
    }
}

static const GLint ToGLAddressMode(RHI::AddressMode::Enum mode) {
    switch (mode) {
    case RHI::AddressMode::Repeat:
        return GL_REPEAT;
    case RHI::AddressMode::MirroredRepeat:
        return GL_MIRRORED_REPEAT;
    case RHI::AddressMode::Clamp:
        return GL_CLAMP_TO_EDGE;
    case RHI::AddressMode::ClampToBorder:
        if (OpenGL::SupportsTextureBorderColor()) {
            return GL_CLAMP_TO_BORDER;
        }
        return GL_CLAMP_TO_EDGE;
    default:
        assert(0);
        return 0;
    }
}

RHI::Handle OpenGLRHI::CreateTexture(TextureType::Enum type) {
    GLuint object;
    gglGenTextures(1, &object);

    GLTexture *texture = new GLTexture;
    texture->type   = type;
    texture->target = ToGLTextureTarget(type);
    texture->object = object;

    int handle = textureList.FindNull();
    if (handle == -1) {
        handle = textureList.Append(texture);
    } else {
        textureList[handle] = texture;
    }

    return (Handle)handle;
}

void OpenGLRHI::DestroyTexture(Handle textureHandle) {
    GLTexture *texture = textureList[textureHandle];
    assert(texture);

    int currentTmu = currentContext->state->tmu;
    for (int i = 0; i < MaxTMU; i++) {
        if (currentContext->state->textureHandles[i] == textureHandle) {
            SelectTextureUnit(i);
            BindTexture(NullTexture);
        }
    }

    gglDeleteTextures(1, &texture->object);

    delete texture;
    textureList[textureHandle] = nullptr;

    SelectTextureUnit(currentTmu);
}

void OpenGLRHI::SelectTextureUnit(unsigned int unit) {
    assert(unit >= 0 && unit < MaxTMU);

    if (currentContext->state->tmu != unit) {
        currentContext->state->tmu = unit;
        gglActiveTexture(GL_TEXTURE0 + unit);
    }
}

void OpenGLRHI::BindTexture(Handle textureHandle) {
    Handle oldTextureHandle = currentContext->state->textureHandles[currentContext->state->tmu];
    if (oldTextureHandle != textureHandle) {
        const GLTexture *texture = textureList[textureHandle];
        assert(texture);

        GLenum target = textureHandle == NullTexture ? textureList[oldTextureHandle]->target : texture->target;
        gglBindTexture(target, texture->object);

        currentContext->state->textureHandles[currentContext->state->tmu] = textureHandle;
    }
}

void OpenGLRHI::SetTexture(int textureUnit, Handle textureHandle) {
    SelectTextureUnit(textureUnit);
    BindTexture(textureHandle);
}

void OpenGLRHI::SetTextureAddressMode(AddressMode::Enum addressMode) {
    const GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
    assert(texture);

    GLint mode = ToGLAddressMode(addressMode);
    
    switch (texture->type) {
    case TextureType::Texture2D:
    case TextureType::TextureRectangle:
    case TextureType::TextureBuffer:
        gglTexParameterf(texture->target, GL_TEXTURE_WRAP_S, mode);
        gglTexParameterf(texture->target, GL_TEXTURE_WRAP_T, mode);
        break;
    case TextureType::Texture3D:
    case TextureType::Texture2DArray:
    case TextureType::TextureCubeMap:
        gglTexParameterf(texture->target, GL_TEXTURE_WRAP_S, mode);
        gglTexParameterf(texture->target, GL_TEXTURE_WRAP_T, mode);
        gglTexParameterf(texture->target, GL_TEXTURE_WRAP_R, mode);
        break;
    }
}

void OpenGLRHI::SetTextureFilter(TextureFilter::Enum filter) {
    const GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
    assert(texture);

    GLenum minFilter = ToGLTextureMinFilter(filter);
    GLenum magFilter = ToGLTextureMagFilter(filter);

    gglTexParameterf(texture->target, GL_TEXTURE_MIN_FILTER, minFilter);
    gglTexParameterf(texture->target, GL_TEXTURE_MAG_FILTER, magFilter);
}

void OpenGLRHI::SetTextureAnisotropy(int aniso) {
    if (OpenGL::SupportsTextureFilterAnisotropic()) {
        const GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
        assert(texture);
    
        Clamp(aniso, 1, hwLimit.maxTextureAnisotropy);
        gglTexParameterf(texture->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)aniso);
    }
}

void OpenGLRHI::SetTextureBorderColor(const Color4 &rgba) {
    if (OpenGL::SupportsTextureBorderColor()) {
        const GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
        assert(texture);

        gglTexParameterfv(texture->target, GL_TEXTURE_BORDER_COLOR, rgba);
    }
}

void OpenGLRHI::SetTextureShadowFunc(bool set) {
    const GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
    assert(texture);

    if (set) {
        // NOTE: GL_COMPARE_REF_DEPTH_TO_TEXTURE_EXT is simply an alias for the 
        // existing GL_COMPARE_R_TO_TEXTURE token in OpenGL 2.0; the alternate name  
        // reflects the fact that the R coordinate is not always used.)  
        gglTexParameteri(texture->target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        gglTexParameteri(texture->target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    } else {
        gglTexParameteri(texture->target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }	
}

void OpenGLRHI::SetTextureLODBias(float bias) {
    const GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
    assert(texture);

    if (OpenGL::SupportsTextureLodBias()) {
        gglTexParameterf(texture->target, GL_TEXTURE_LOD_BIAS, bias);
    }
}

void OpenGLRHI::SetTextureLevel(int baseLevel, int maxLevel) {
    const GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
    assert(texture);

    gglTexParameteri(texture->target, GL_TEXTURE_BASE_LEVEL, baseLevel);
    gglTexParameteri(texture->target, GL_TEXTURE_MAX_LEVEL, maxLevel);
}

void OpenGLRHI::GenerateMipmap() {
    const GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
    assert(texture);

    // NOTE: GL_INVALID_OPERATION occurs in compressed format in OpenGL ES.
    gglGenerateMipmap(texture->target);
}

void OpenGLRHI::AdjustTextureSize(TextureType::Enum type, bool useNPOT, int inWidth, int inHeight, int inDepth, int *outWidth, int *outHeight, int *outDepth) {
    int w, h, d;

    // NOTE: Without the GL_ARB_texture_non_power_of_two extension string,
    // NPOT textures may not hw-accelerate depending on the mipmap or wrapmode.
    if (useNPOT || type == TextureType::TextureRectangle) {
        w = inWidth;
        h = inHeight;
        d = inDepth;
    } else {
        // Fit multiplier size of 2.
        w = Math::RoundUpPowerOfTwo(inWidth);
        h = Math::RoundUpPowerOfTwo(inHeight);
        d = Math::RoundUpPowerOfTwo(inDepth);
    }

    switch (type) {
    case TextureType::Texture2D:
    case TextureType::Texture2DArray:
        if (w > hwLimit.maxTextureSize) w = hwLimit.maxTextureSize;
        if (h > hwLimit.maxTextureSize) h = hwLimit.maxTextureSize;
        break;
    case TextureType::Texture3D:
        if (w > hwLimit.max3dTextureSize) w = hwLimit.max3dTextureSize;
        if (h > hwLimit.max3dTextureSize) h = hwLimit.max3dTextureSize;
        if (d > hwLimit.max3dTextureSize) d = hwLimit.max3dTextureSize;
        break;
    case TextureType::TextureCubeMap:
        if (w > hwLimit.maxCubeMapTextureSize) w = hwLimit.maxCubeMapTextureSize;
        if (h > hwLimit.maxCubeMapTextureSize) h = hwLimit.maxCubeMapTextureSize;

        // For cubemaps, set the width and height equally.
        if (w < h) {
            h = w;
        } else {
            w = h;
        }
        break;
    case TextureType::TextureRectangle:
        if (w > hwLimit.maxRectangleTextureSize) w = hwLimit.maxRectangleTextureSize;
        if (h > hwLimit.maxRectangleTextureSize) h = hwLimit.maxRectangleTextureSize;
        break;
    default:
        assert(0);
        break;
    }

    if (outWidth) *outWidth = w;
    if (outHeight) *outHeight = h;
    if (outDepth) *outDepth = d;
}

void OpenGLRHI::AdjustTextureFormat(TextureType::Enum type, bool useCompression, bool useNormalMap, Image::Format::Enum inFormat, Image::Format::Enum *outFormat) {
    if (Image::IsDepthFormat(inFormat) || Image::IsDepthStencilFormat(inFormat)) {
        *outFormat = inFormat;
        return;
    }

    if (Image::IsCompressed(inFormat)) {
        if (OpenGL::SupportedImageFormat(inFormat)) {
            *outFormat = inFormat;
            return;
        }
        
        inFormat = OpenGL::ToUncompressedImageFormat(inFormat);
    }

    *outFormat = useCompression ? OpenGL::ToCompressedImageFormat(inFormat, useNormalMap) : inFormat;
}

void OpenGLRHI::BeginUnpackAlignment(int pitch) {
    int mask = pitch & 3;
    currentContext->state->newUnpackAlignment = mask & 1 ? 1 : (mask & 2 ? 2 : 4);

    gglGetIntegerv(GL_UNPACK_ALIGNMENT, &currentContext->state->oldUnpackAlignment);
    if (currentContext->state->oldUnpackAlignment != currentContext->state->newUnpackAlignment) {
        gglPixelStorei(GL_UNPACK_ALIGNMENT, currentContext->state->newUnpackAlignment);
    }
}

void OpenGLRHI::EndUnpackAlignment() {
    if (currentContext->state->oldUnpackAlignment != currentContext->state->newUnpackAlignment) {
        gglPixelStorei(GL_UNPACK_ALIGNMENT, currentContext->state->oldUnpackAlignment);
    }
}

void OpenGLRHI::SetTextureImage(TextureType::Enum textureType, const Image *srcImage, Image::Format::Enum dstFormat, bool useMipmaps, bool isSRGB) {
    GLenum format;
    GLenum type;
    GLenum internalFormat;
    Image uncompressedImage;
    Image tmpImage;

    Image::Format::Enum srcFormat = srcImage->GetFormat();
    
    bool srcCompressed = Image::IsCompressed(srcFormat);
    bool dstCompressed = Image::IsCompressed(dstFormat);
    
    bool srcFormatSupported = OpenGL::SupportedImageFormat(srcFormat);
    bool dstFormatSupported = OpenGL::SupportedImageFormat(dstFormat);
    
    if (srcCompressed && !srcFormatSupported) {
        Image::Format::Enum uncompressedFormat = OpenGL::ToUncompressedImageFormat(srcFormat);
        
        srcImage->ConvertFormat(uncompressedFormat, uncompressedImage);
        srcImage = &uncompressedImage;
        
        srcFormat = uncompressedFormat;
        srcCompressed = false;
    }
    
    if (dstCompressed) {
        if (!dstFormatSupported) {
            dstFormat = OpenGL::ToUncompressedImageFormat(dstFormat);
            dstCompressed = false;
        } else {
            if (dstFormat == Image::Format::DXN2 && srcFormat != Image::Format::DXN2) {
                srcImage->ConvertFormat(Image::Format::DXN2, tmpImage, Image::GammaSpace::Linear);
                srcImage = &tmpImage;

                srcFormat = Image::Format::DXN2;
                srcCompressed = true;
            } else if (dstFormat == Image::Format::XGBR_DXT5 && srcFormat != Image::Format::XGBR_DXT5) {
                srcImage->ConvertFormat(Image::Format::RGBA_8_8_8_8, tmpImage, Image::GammaSpace::Linear);
                tmpImage.SwapRedAlphaRGBA8888();
                srcImage = &tmpImage;

                srcFormat = Image::Format::RGBA_8_8_8_8;
                srcCompressed = false;
            }
        }
    }
    
    if (srcCompressed && srcFormat != dstFormat) {
        BE_WARNLOG("Wrong requested image format %s -> %s\n", Image::FormatName(srcFormat), Image::FormatName(dstFormat));
        return;
    }

    srcFormatSupported = OpenGL::ImageFormatToGLFormat(srcFormat, isSRGB, &format, &type, nullptr);
    if (!srcFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", Image::FormatName(srcFormat));
        return;
    }

    dstFormatSupported = OpenGL::ImageFormatToGLFormat(dstFormat, isSRGB, nullptr, nullptr, &internalFormat);
    if (!dstFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", Image::FormatName(dstFormat));
        return;
    }

    Image mipmapedImage;
    bool generateMipmaps = false;
    int maxLevel = srcImage->NumMipmaps();
    int maxLevelAlloc = maxLevel;

    if (useMipmaps) {
        if (srcImage->NumMipmaps() == 1) {
            int w = srcImage->GetWidth();
            int h = srcImage->GetHeight();
            int d = srcImage->GetDepth();
            int maxGenLevels = Image::MaxMipMapLevels(w, h, d);

            if (srcImage->IsPacked() || (srcImage->IsCompressed() && OpenGL::SupportsCompressedGenMipmaps())) {
                generateMipmaps = true;
            } else {
                mipmapedImage.Create(w, h, d, srcImage->NumSlices(), maxGenLevels, srcImage->GetFormat(), srcImage->GetGammaSpace(), nullptr, srcImage->GetFlags());
                mipmapedImage.CopyFrom(*srcImage, 0, 1);
                mipmapedImage.GenerateMipmaps();
                srcImage = &mipmapedImage;
            }

            maxLevel = srcImage->NumMipmaps();
            maxLevelAlloc = maxGenLevels;
        }
    }

    const byte *pic = srcImage->GetPixels();
    
    BeginUnpackAlignment(Image::BytesPerPixel(srcFormat) * srcImage->GetWidth());

    switch (textureType) {
    case TextureType::Texture2D:
        gglTexStorage2D(GL_TEXTURE_2D, maxLevelAlloc, internalFormat, srcImage->GetWidth(), srcImage->GetHeight());
        if (pic) {
            for (int level = 0; level < maxLevel; level++) {
                int w = srcImage->GetWidth(level);
                int h = srcImage->GetHeight(level);
                int size = srcImage->GetSize(level);
                if (srcCompressed) {
                    gglCompressedTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, w, h, format, size, pic);
                } else {
                    gglTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, w, h, format, type, pic);
                }
                pic += size;
            }
        }
        if (generateMipmaps) {
            gglGenerateMipmap(GL_TEXTURE_2D);
        }
        OpenGL::SetTextureSwizzling(GL_TEXTURE_2D, srcFormat);
        break;
    case TextureType::Texture3D:
        gglTexStorage3D(GL_TEXTURE_3D, maxLevelAlloc, internalFormat, srcImage->GetWidth(), srcImage->GetHeight(), srcImage->GetDepth());
        if (pic) {
            for (int level = 0; level < maxLevel; level++) {
                int w = srcImage->GetWidth(level);
                int h = srcImage->GetHeight(level);
                int d = srcImage->GetDepth(level);
                int size = srcImage->GetSize(level);
                if (srcCompressed) {
                    gglCompressedTexSubImage3D(GL_TEXTURE_3D, level, 0, 0, 0, w, h, d, format, size, pic);
                } else {
                    gglTexSubImage3D(GL_TEXTURE_3D, level, 0, 0, 0, w, h, d, format, type, pic);
                }
                pic += size;
            }
        }
        if (generateMipmaps) {
            gglGenerateMipmap(GL_TEXTURE_3D);
        }
        OpenGL::SetTextureSwizzling(GL_TEXTURE_3D, srcFormat);
        break;
    case TextureType::Texture2DArray:
        gglTexStorage3D(GL_TEXTURE_2D_ARRAY, maxLevelAlloc, internalFormat, srcImage->GetWidth(), srcImage->GetHeight(), srcImage->NumSlices());
        if (pic) {
            for (int level = 0; level < maxLevel; level++) {
                int w = srcImage->GetWidth(level);
                int h = srcImage->GetHeight(level);
                int size = srcImage->GetSize(level);
                if (srcCompressed) {
                    gglCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, 0, 0, 0, w, h, srcImage->NumSlices(), format, size, pic);
                } else {
                    gglTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, 0, 0, 0, w, h, srcImage->NumSlices(), format, type, pic);
                }
                pic += size;
            }
        }
        if (generateMipmaps) {
            gglGenerateMipmap(GL_TEXTURE_2D_ARRAY);
        }
        OpenGL::SetTextureSwizzling(GL_TEXTURE_2D_ARRAY, srcFormat);
        break;
    case TextureType::TextureCubeMap:
        gglTexStorage2D(GL_TEXTURE_CUBE_MAP, maxLevelAlloc, internalFormat, srcImage->GetWidth(), srcImage->GetWidth());
        if (pic) {
            for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                for (int level = 0; level < maxLevel; level++) {
                    int w = srcImage->GetWidth(level);
                    int size = srcImage->GetSliceSize(level);
                    if (srcCompressed) {
                        gglCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, level, 0, 0, w, w, format, size, pic);
                    } else {
                        gglTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, level, 0, 0, w, w, format, type, pic);
                    }
                    pic += size;
                }
            }
        }
        if (generateMipmaps) {
            gglGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        }
        OpenGL::SetTextureSwizzling(GL_TEXTURE_CUBE_MAP, srcFormat);
        break;
    case TextureType::TextureRectangle:
        gglTexStorage2D(GL_TEXTURE_RECTANGLE, maxLevelAlloc, internalFormat, srcImage->GetWidth(), srcImage->GetWidth());
        if (pic) {
            int w = srcImage->GetWidth(0);
            int h = srcImage->GetHeight(0);
            int size = srcImage->GetSize(0);
            if (srcCompressed) {
                gglCompressedTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, w, h, format, size, pic);
            } else {
                gglTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, 0, 0, w, h, format, type, pic);
            }
        }
        OpenGL::SetTextureSwizzling(GL_TEXTURE_RECTANGLE, srcFormat);
        break;
    default:
        assert(0);
        break;
    }

    EndUnpackAlignment();
}

void OpenGLRHI::SetTextureImageBuffer(Image::Format::Enum dstFormat, bool isSRGB, int bufferHandle) {
    GLenum internalFormat;

    bool dstFormatSupported = OpenGL::ImageFormatToGLFormat(dstFormat, isSRGB, nullptr, nullptr, &internalFormat);
    if (!dstFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", Image::FormatName(dstFormat));
        return;
    }

    if (OpenGL::SupportsTextureBuffer()) {
        OpenGL::TexBuffer(internalFormat, bufferList[bufferHandle]->object);
    }
    
    OpenGL::SetTextureSwizzling(GL_TEXTURE_BUFFER, dstFormat);
}

void OpenGLRHI::SetTextureSubImage2D(int level, int xoffset, int yoffset, int width, int height, Image::Format::Enum srcFormat, const void *pixels) {
    GLenum format;
    GLenum type;

    bool srcFormatSupported = OpenGL::ImageFormatToGLFormat(srcFormat, false, &format, &type, nullptr);
    if (!srcFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", Image::FormatName(srcFormat));
        return;
    }

    bool srcCompressed = Image::IsCompressed(srcFormat);

    BeginUnpackAlignment(Image::BytesPerPixel(srcFormat) * width);

    if (srcCompressed) {
        int size = Image::MemRequired(width, height, 1, 1, srcFormat);
        gglCompressedTexSubImage2D(GL_TEXTURE_2D, level, xoffset, yoffset, width, height, format, size, pixels);
    } else {
        gglTexSubImage2D(GL_TEXTURE_2D, level, xoffset, yoffset, width, height, format, type, pixels);
    }

    EndUnpackAlignment();
}

void OpenGLRHI::SetTextureSubImage3D(int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, Image::Format::Enum srcFormat, const void *pixels) {
    GLenum format;
    GLenum type;

    bool srcFormatSupported = OpenGL::ImageFormatToGLFormat(srcFormat, false, &format, &type, nullptr);
    if (!srcFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", Image::FormatName(srcFormat));
        return;
    }
    
    bool srcCompressed = Image::IsCompressed(srcFormat);

    BeginUnpackAlignment(Image::BytesPerPixel(srcFormat) * width);

    if (srcCompressed) {
        int size = Image::MemRequired(width, height, depth, 1, srcFormat);
        gglCompressedTexSubImage3D(GL_TEXTURE_3D, level, xoffset, yoffset, zoffset, width, height, depth, format, size, pixels);
    } else {
        gglTexSubImage3D(GL_TEXTURE_3D, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    }

    EndUnpackAlignment();
}

void OpenGLRHI::SetTextureSubImage2DArray(int level, int xoffset, int yoffset, int zoffset, int width, int height, int arrays, Image::Format::Enum srcFormat, const void *pixels) {
    GLenum format;
    GLenum type;

    bool srcFormatSupported = OpenGL::ImageFormatToGLFormat(srcFormat, false, &format, &type, nullptr);
    if (!srcFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", Image::FormatName(srcFormat));
        return;
    }
    
    bool srcCompressed = Image::IsCompressed(srcFormat);

    BeginUnpackAlignment(Image::BytesPerPixel(srcFormat) * width);

    if (srcCompressed) {
        int size = Image::MemRequired(width, height, arrays, 1, srcFormat);
        gglCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, zoffset, width, height, arrays, format, size, pixels);
    } else {
        gglTexSubImage3D(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, zoffset, width, height, arrays, format, type, pixels);
    }

    EndUnpackAlignment();
}

void OpenGLRHI::SetTextureSubImageCube(CubeMapFace::Enum face, int level, int xoffset, int yoffset, int width, int height, Image::Format::Enum srcFormat, const void *pixels) {
    GLenum format;
    GLenum type;

    bool srcFormatSupported = OpenGL::ImageFormatToGLFormat(srcFormat, false, &format, &type, nullptr);
    if (!srcFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", Image::FormatName(srcFormat));
        return;
    }
    
    bool srcCompressed = Image::IsCompressed(srcFormat);

    BeginUnpackAlignment(Image::BytesPerPixel(srcFormat) * width);

    if (srcCompressed) {
        int size = Image::MemRequired(width, height, 1, 1, srcFormat);
        gglCompressedTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, xoffset, yoffset, width, height, format, size, pixels);
    } else {
        gglTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, xoffset, yoffset, width, height, format, type, pixels);
    }

    EndUnpackAlignment();
}

void OpenGLRHI::SetTextureSubImageRect(int xoffset, int yoffset, int width, int height, Image::Format::Enum srcFormat, const void *pixels) {
    GLenum format;
    GLenum type;

    bool srcFormatSupported = OpenGL::ImageFormatToGLFormat(srcFormat, false, &format, &type, nullptr);
    if (!srcFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", Image::FormatName(srcFormat));
        return;
    }
    
    bool srcCompressed = Image::IsCompressed(srcFormat);	

    BeginUnpackAlignment(Image::BytesPerPixel(srcFormat) * width);

    if (srcCompressed) {
        int size = Image::MemRequired(width, height, 1, 1, srcFormat);
        gglCompressedTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, xoffset, yoffset, width, height, format, size, pixels);
    } else {
        gglTexSubImage2D(GL_TEXTURE_RECTANGLE, 0, xoffset, yoffset, width, height, format, type, pixels);
    }

    EndUnpackAlignment();
}

void OpenGLRHI::CopyTextureSubImage2D(int xoffset, int yoffset, int x, int y, int width, int height) {
    GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
    assert(texture);

    // Read from GL_READ_BUFFER and save as texture.
    gglCopyTexSubImage2D(texture->target, 0, xoffset, yoffset, x, y, width, height);
}

void OpenGLRHI::CopyImageSubData(Handle srcTextureHandle, int srcLevel, int srcX, int srcY, int srcZ, Handle dstTextureHandle, int dstLevel, int dstX, int dstY, int dstZ, int width, int height, int depth) {
    GLTexture *srcTexture = textureList[srcTextureHandle];
    GLTexture *dstTexture = textureList[dstTextureHandle];
    assert(srcTexture);
    assert(dstTexture);

    if (OpenGL::SupportsCopyImage()) {
        OpenGL::CopyImageSubData(srcTexture->object, srcTexture->target, srcLevel, srcX, srcY, srcZ, dstTexture->object, dstTexture->target, dstLevel, dstX, dstY, dstZ, width, height, depth);
    }
}

void OpenGLRHI::GetTextureImage2D(int level, Image::Format::Enum dstFormat, void *pixels) {
#ifdef GL_VERSION_1_0
    GLenum format;
    GLenum type;
    
    bool dstFormatSupported = OpenGL::ImageFormatToGLFormat(dstFormat, false, &format, &type, nullptr);
    if (!dstFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", Image::FormatName(dstFormat));
        return;
    }
    
    gglFlush();
    gglGetTexImage(GL_TEXTURE_2D, level, format, type, pixels);
#endif
}

void OpenGLRHI::GetTextureImage3D(int level, Image::Format::Enum dstFormat, void *pixels) {
#ifdef GL_VERSION_1_0
    GLenum format;
    GLenum type;

    bool dstFormatSupported = OpenGL::ImageFormatToGLFormat(dstFormat, false, &format, &type, nullptr);
    if (!dstFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", Image::FormatName(dstFormat));
        return;
    }
    
    gglFlush();
    gglGetTexImage(GL_TEXTURE_3D, level, format, type, pixels);
#endif
}

void OpenGLRHI::GetTextureImageCube(CubeMapFace::Enum face, int level, Image::Format::Enum dstFormat, void *pixels) {
#ifdef GL_VERSION_1_0
    GLenum format;
    GLenum type;

    bool dstFormatSupported = OpenGL::ImageFormatToGLFormat(dstFormat, false, &format, &type, nullptr);
    if (!dstFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", Image::FormatName(dstFormat));
        return;
    }
    
    gglFlush();
    gglGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, format, type, pixels);
#endif
}

void OpenGLRHI::GetTextureImageRect(Image::Format::Enum dstFormat, void *pixels) {
#ifdef GL_VERSION_1_0
    GLenum format;
    GLenum type;

    bool dstFormatSupported = OpenGL::ImageFormatToGLFormat(dstFormat, false, &format, &type, nullptr);
    if (!dstFormatSupported) {
        BE_WARNLOG("Unsupported image format %s\n", Image::FormatName(dstFormat));
        return;
    }
    
    gglFlush();
    gglGetTexImage(GL_TEXTURE_RECTANGLE, 0, format, type, pixels);
#endif
}

BE_NAMESPACE_END
