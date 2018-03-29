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

const GLenum ToGLTextureTarget(RHI::TextureType type) {
    switch (type) {
    case RHI::Texture2D:
        return GL_TEXTURE_2D;
    case RHI::TextureRectangle:
        if (OpenGL::SupportsTextureRectangle()) {
            return GL_TEXTURE_RECTANGLE;
        }
        return GL_TEXTURE_2D;
    case RHI::Texture3D:
        return GL_TEXTURE_3D;
    case RHI::TextureCubeMap:
        return GL_TEXTURE_CUBE_MAP;
    case RHI::Texture2DArray:
        return GL_TEXTURE_2D_ARRAY;
    case RHI::TextureBuffer:
        return GL_TEXTURE_BUFFER;
    default:
        assert(0);
        return 0;
    }
}

const GLenum ToGLTextureMinFilter(RHI::TextureFilter filter) {
    switch (filter) {
    case RHI::Nearest:
        return GL_NEAREST;
    case RHI::Linear:
        return GL_LINEAR;
    case RHI::NearestMipmapNearest:
        return GL_NEAREST_MIPMAP_NEAREST;
    case RHI::LinearMipmapNearest:
        return GL_LINEAR_MIPMAP_NEAREST;
    case RHI::NearestMipmapLinear:
        return GL_NEAREST_MIPMAP_LINEAR;
    case RHI::LinearMipmapLinear:
        return GL_LINEAR_MIPMAP_LINEAR;
    default:
        assert(0);
        return 0;
    }
}

const GLenum ToGLTextureMagFilter(RHI::TextureFilter filter) {
    switch (filter) {
    case RHI::Nearest:
        return GL_NEAREST;
    case RHI::Linear:
        return GL_LINEAR;
    case RHI::NearestMipmapNearest:
        return GL_NEAREST;
    case RHI::LinearMipmapNearest:
        return GL_LINEAR;
    case RHI::NearestMipmapLinear:
        return GL_NEAREST;
    case RHI::LinearMipmapLinear:
        return GL_LINEAR;
    default:
        assert(0);
        return 0;
    }
}

const GLint ToGLAddressMode(RHI::AddressMode mode) {
    switch (mode) {
    case RHI::Repeat:
        return GL_REPEAT;
    case RHI::MirroredRepeat:
        return GL_MIRRORED_REPEAT;
    case RHI::Clamp:
        return GL_CLAMP_TO_EDGE;
    case RHI::ClampToBorder:
        if (OpenGL::SupportsTextureBorderColor()) {
            return GL_CLAMP_TO_BORDER;
        }
        return GL_CLAMP_TO_EDGE;
    default:
        assert(0);
        return 0;
    }
}

RHI::Handle OpenGLRHI::CreateTexture(TextureType type) {
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

void OpenGLRHI::DeleteTexture(Handle textureHandle) {
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

void OpenGLRHI::SetTextureAddressMode(AddressMode addressMode) {
    const GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
    assert(texture);

    GLint mode = ToGLAddressMode(addressMode);
    
    switch (texture->type) {
    case Texture2D:
    case TextureRectangle:
    case TextureBuffer:
        gglTexParameterf(texture->target, GL_TEXTURE_WRAP_S, mode);
        gglTexParameterf(texture->target, GL_TEXTURE_WRAP_T, mode);
        break;
    case Texture3D:
    case Texture2DArray:
    case TextureCubeMap:
        gglTexParameterf(texture->target, GL_TEXTURE_WRAP_S, mode);
        gglTexParameterf(texture->target, GL_TEXTURE_WRAP_T, mode);
        gglTexParameterf(texture->target, GL_TEXTURE_WRAP_R, mode);
        break;
    }
}

void OpenGLRHI::SetTextureFilter(TextureFilter filter) {
    const GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
    assert(texture);

    GLenum minFilter = ToGLTextureMinFilter(filter);
    GLenum magFilter = ToGLTextureMagFilter(filter);

    gglTexParameterf(texture->target, GL_TEXTURE_MIN_FILTER, minFilter);
    gglTexParameterf(texture->target, GL_TEXTURE_MAG_FILTER, magFilter);	 
}

void OpenGLRHI::SetTextureAnisotropy(int aniso) {
    const GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
    assert(texture);

    if (OpenGL::SupportsTextureFilterAnisotropic()) {
        BE1::Clamp(aniso, 1, hwLimit.maxTextureAnisotropy);
        gglTexParameterf(texture->target, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)aniso);
    }
}

void OpenGLRHI::SetTextureBorderColor(const Color4 &rgba) {
    const GLTexture *texture = textureList[currentContext->state->textureHandles[currentContext->state->tmu]];
    assert(texture);

    if (OpenGL::SupportsTextureBorderColor()) {
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

    // NOTE: OpenGL ES 에서는 compressed format 일 경우 GL_INVALID_OPERATION
    gglGenerateMipmap(texture->target);
}

void OpenGLRHI::AdjustTextureSize(TextureType type, bool useNPOT, int inWidth, int inHeight, int inDepth, int *outWidth, int *outHeight, int *outDepth) {
    int w, h, d;

    // NOTE: GL_ARB_texture_non_power_of_two 익스텐션 스트링이 없다면,
    // NPOT 텍스쳐는 밉맵이나 wrapmode 에 따라서 hw-accelerate 되지 않을수도 있다.
    if (useNPOT || type == TextureRectangle) {
        w = inWidth;
        h = inHeight;
        d = inDepth;
    } else {
        // 2의 승수 사이즈 맞추기
        w = Math::CeilPowerOfTwo(inWidth);
        h = Math::CeilPowerOfTwo(inHeight);
        d = Math::CeilPowerOfTwo(inDepth);
    }	

    switch (type) {
    case Texture2D:
    case Texture2DArray:
        if (w > hwLimit.maxTextureSize) w = hwLimit.maxTextureSize;
        if (h > hwLimit.maxTextureSize) h = hwLimit.maxTextureSize;
        break;
    case Texture3D:
        if (w > hwLimit.max3dTextureSize) w = hwLimit.max3dTextureSize;
        if (h > hwLimit.max3dTextureSize) h = hwLimit.max3dTextureSize;
        if (d > hwLimit.max3dTextureSize) d = hwLimit.max3dTextureSize;
        break;
    case TextureCubeMap:
        if (w > hwLimit.maxCubeMapTextureSize) w = hwLimit.maxCubeMapTextureSize;
        if (h > hwLimit.maxCubeMapTextureSize) h = hwLimit.maxCubeMapTextureSize;

        // 큐브맵일 경우에는 가로 세로 길이를 동일하게 맞춘다
        if (w < h) {
            h = w;
        } else {
            w = h;
        }
        break;
    case TextureRectangle:
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

void OpenGLRHI::AdjustTextureFormat(TextureType type, bool useCompression, bool useNormalMap, Image::Format inFormat, Image::Format *outFormat) {
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

void OpenGLRHI::SetTextureImage(TextureType textureType, const Image *srcImage, Image::Format dstFormat, bool useMipmaps, bool useSRGB) {
    GLenum format;
    GLenum type;
    GLenum internalFormat;
    Image uncompressedImage;
    Image tmpImage;

    Image::Format srcFormat = srcImage->GetFormat();
    
    bool srcCompressed = Image::IsCompressed(srcFormat);
    bool dstCompressed = Image::IsCompressed(dstFormat);
    
    bool srcFormatSupported = OpenGL::SupportedImageFormat(srcFormat);
    bool dstFormatSupported = OpenGL::SupportedImageFormat(dstFormat);
    
    if (srcCompressed && !srcFormatSupported) {
        Image::Format uncompressedFormat = OpenGL::ToUncompressedImageFormat(srcFormat);
        
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
            if (dstFormat == Image::DXN2 && srcFormat != Image::DXN2) {
                srcImage->ConvertFormat(Image::DXN2, tmpImage);
                srcImage = &tmpImage;

                srcFormat = Image::DXN2;
                srcCompressed = true;
            } else if (dstFormat == Image::XGBR_DXT5 && srcFormat != Image::XGBR_DXT5) {
                srcImage->ConvertFormat(Image::RGBA_8_8_8_8, tmpImage);
                tmpImage.SwapRedAlphaRGBA8888();
                srcImage = &tmpImage;

                srcFormat = Image::RGBA_8_8_8_8;
                srcCompressed = false;
            }
        }
    }
    
    if (srcCompressed && srcFormat != dstFormat) {
        BE_WARNLOG(L"Wrong requested image format %hs -> %hs\n", Image::FormatName(srcFormat), Image::FormatName(dstFormat));
        return;
    }

    srcFormatSupported = OpenGL::ImageFormatToGLFormat(srcFormat, useSRGB, &format, &type, nullptr);
    if (!srcFormatSupported) {
        BE_WARNLOG(L"Unsupported image format %hs\n", Image::FormatName(srcFormat));
        return;
    }

    dstFormatSupported = OpenGL::ImageFormatToGLFormat(dstFormat, useSRGB, nullptr, nullptr, &internalFormat);
    if (!dstFormatSupported) {
        BE_WARNLOG(L"Unsupported image format %hs\n", Image::FormatName(dstFormat));
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

            if (srcImage->IsCompressed() && OpenGL::SupportsCompressedGenMipmaps()) {
                generateMipmaps = true;
            } else {
                mipmapedImage.Create(w, h, d, srcImage->NumSlices(), maxGenLevels, srcImage->GetFormat(), nullptr, srcImage->GetFlags());
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
    case Texture2D:
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
    case Texture3D:
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
    case Texture2DArray:
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
    case TextureCubeMap:
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
    case TextureRectangle:
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

void OpenGLRHI::SetTextureImageBuffer(Image::Format dstFormat, bool useSRGB, int bufferHandle) {
    GLenum internalFormat;

    bool dstFormatSupported = OpenGL::ImageFormatToGLFormat(dstFormat, useSRGB, nullptr, nullptr, &internalFormat);
    if (!dstFormatSupported) {
        BE_WARNLOG(L"Unsupported image format %hs\n", Image::FormatName(dstFormat));
        return;
    }

    if (OpenGL::SupportsTextureBufferObject()) {
        OpenGL::TexBuffer(internalFormat, bufferList[bufferHandle]->object);
    }
    
    OpenGL::SetTextureSwizzling(GL_TEXTURE_BUFFER, dstFormat);
}

void OpenGLRHI::SetTextureSubImage2D(int level, int xoffset, int yoffset, int width, int height, Image::Format srcFormat, const void *pixels) {
    GLenum format;
    GLenum type;

    bool srcFormatSupported = OpenGL::ImageFormatToGLFormat(srcFormat, false, &format, &type, nullptr);
    if (!srcFormatSupported) {
        BE_WARNLOG(L"Unsupported image format %hs\n", Image::FormatName(srcFormat));
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

void OpenGLRHI::SetTextureSubImage3D(int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, Image::Format srcFormat, const void *pixels) {
    GLenum format;
    GLenum type;

    bool srcFormatSupported = OpenGL::ImageFormatToGLFormat(srcFormat, false, &format, &type, nullptr);
    if (!srcFormatSupported) {
        BE_WARNLOG(L"Unsupported image format %hs\n", Image::FormatName(srcFormat));
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

void OpenGLRHI::SetTextureSubImage2DArray(int level, int xoffset, int yoffset, int zoffset, int width, int height, int arrays, Image::Format srcFormat, const void *pixels) {
    GLenum format;
    GLenum type;

    bool srcFormatSupported = OpenGL::ImageFormatToGLFormat(srcFormat, false, &format, &type, nullptr);
    if (!srcFormatSupported) {
        BE_WARNLOG(L"Unsupported image format %hs\n", Image::FormatName(srcFormat));
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

void OpenGLRHI::SetTextureSubImageCube(CubeMapFace face, int level, int xoffset, int yoffset, int width, int height, Image::Format srcFormat, const void *pixels) {
    GLenum format;
    GLenum type;

    bool srcFormatSupported = OpenGL::ImageFormatToGLFormat(srcFormat, false, &format, &type, nullptr);
    if (!srcFormatSupported) {
        BE_WARNLOG(L"Unsupported image format %hs\n", Image::FormatName(srcFormat));
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

void OpenGLRHI::SetTextureSubImageRect(int xoffset, int yoffset, int width, int height, Image::Format srcFormat, const void *pixels) {
    GLenum format;
    GLenum type;

    bool srcFormatSupported = OpenGL::ImageFormatToGLFormat(srcFormat, false, &format, &type, nullptr);
    if (!srcFormatSupported) {
        BE_WARNLOG(L"Unsupported image format %hs\n", Image::FormatName(srcFormat));
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

    // GL_READ_BUFFER 에서 읽어서 텍스쳐로 저장
    gglCopyTexSubImage2D(texture->target, 0, xoffset, yoffset, x, y, width, height);
}

void OpenGLRHI::GetTextureImage2D(int level, Image::Format dstFormat, void *pixels) {
#ifdef GL_VERSION_1_0
    GLenum format;
    GLenum type;
    
    bool dstFormatSupported = OpenGL::ImageFormatToGLFormat(dstFormat, false, &format, &type, nullptr);
    if (!dstFormatSupported) {
        BE_WARNLOG(L"Unsupported image format %hs\n", Image::FormatName(dstFormat));
        return;
    }
    
    gglFlush();
    gglGetTexImage(GL_TEXTURE_2D, level, format, type, pixels);
#endif
}

void OpenGLRHI::GetTextureImage3D(int level, Image::Format dstFormat, void *pixels) {
#ifdef GL_VERSION_1_0
    GLenum format;
    GLenum type;

    bool dstFormatSupported = OpenGL::ImageFormatToGLFormat(dstFormat, false, &format, &type, nullptr);
    if (!dstFormatSupported) {
        BE_WARNLOG(L"Unsupported image format %hs\n", Image::FormatName(dstFormat));
        return;
    }
    
    gglFlush();
    gglGetTexImage(GL_TEXTURE_3D, level, format, type, pixels);
#endif
}

void OpenGLRHI::GetTextureImageCube(CubeMapFace face, int level, Image::Format dstFormat, void *pixels) {
#ifdef GL_VERSION_1_0
    GLenum format;
    GLenum type;

    bool dstFormatSupported = OpenGL::ImageFormatToGLFormat(dstFormat, false, &format, &type, nullptr);
    if (!dstFormatSupported) {
        BE_WARNLOG(L"Unsupported image format %hs\n", Image::FormatName(dstFormat));
        return;
    }
    
    gglFlush();
    gglGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, format, type, pixels);
#endif
}

void OpenGLRHI::GetTextureImageRect(Image::Format dstFormat, void *pixels) {
#ifdef GL_VERSION_1_0
    GLenum format;
    GLenum type;

    bool dstFormatSupported = OpenGL::ImageFormatToGLFormat(dstFormat, false, &format, &type, nullptr);
    if (!dstFormatSupported) {
        BE_WARNLOG(L"Unsupported image format %hs\n", Image::FormatName(dstFormat));
        return;
    }
    
    gglFlush();
    gglGetTexImage(GL_TEXTURE_RECTANGLE, 0, format, type, pixels);
#endif
}

BE_NAMESPACE_END
