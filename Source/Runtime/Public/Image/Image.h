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

#include "Math/Math.h"

BE_NAMESPACE_BEGIN

/// Image representation
class Image {
public:
    /// Various image format type
    /// Channels (RGBALX) are described by little-endian format (opposed to D3D style)
    enum Format {
        UnknownFormat,
        // plain bytes format
        L_8,
        A_8,
        LA_8_8,
        LA_16_16,
        R_8,
        R_SNORM_8,
        RG_8_8,
        RG_SNORM_8_8,
        RGB_8_8_8,
        RGB_SNORM_8_8_8,
        BGR_8_8_8, 
        RGBX_8_8_8_8,
        BGRX_8_8_8_8,
        RGBA_8_8_8_8,
        RGBA_SNORM_8_8_8_8,
        BGRA_8_8_8_8, 
        ABGR_8_8_8_8, 
        ARGB_8_8_8_8,
        // packed format
        RGBX_4_4_4_4,
        BGRX_4_4_4_4,
        RGBA_4_4_4_4,
        BGRA_4_4_4_4,
        ABGR_4_4_4_4,
        ARGB_4_4_4_4,
        RGBX_5_5_5_1,
        BGRX_5_5_5_1,
        RGBA_5_5_5_1,
        BGRA_5_5_5_1,
        ABGR_1_5_5_5,
        ARGB_1_5_5_5,
        RGB_5_6_5,
        BGR_5_6_5,
        // float format
        L_16F,
        A_16F,
        LA_16F_16F,
        R_16F,
        RG_16F_16F,
        RGB_16F_16F_16F,
        RGBA_16F_16F_16F_16F,
        L_32F,
        A_32F,
        LA_32F_32F,
        R_32F,
        RG_32F_32F,
        RGB_32F_32F_32F,
        RGBA_32F_32F_32F_32F,
        RGB_11F_11F_10F,
        RGBE_9_9_9_5,
        // DXT (BTC)
        RGBA_DXT1,
        RGBA_DXT3,
        RGBA_DXT5,
        XGBR_DXT5,
        DXN1,
        DXN2,
        // PVRTC
        RGB_PVRTC_2BPPV1,
        RGB_PVRTC_4BPPV1,
        RGBA_PVRTC_2BPPV1,
        RGBA_PVRTC_4BPPV1,
        RGBA_PVRTC_2BPPV2,
        RGBA_PVRTC_4BPPV2,
        // ETC
        RGB_8_ETC1,
        RGB_8_ETC2,
        RGBA_8_8_ETC2,
        RGBA_8_1_ETC2,
        R_11_EAC,
        SignedR_11_EAC,
        RG_11_11_EAC,
        SignedRG_11_11_EAC,
        // ATC
        RGB_ATC,
        RGBA_EA_ATC, // Explicit alpha
        RGBA_IA_ATC, // Interpolated alpha
        // depth format
        Depth_16,
        Depth_24,
        Depth_32F,
        DepthStencil_24_8,
        DepthStencil_32F_8,
        NumImageFormats
    };

    enum FormatType {
        Packed          = BIT(0),
        Float           = BIT(1),
        Half            = Float | BIT(2),
        Depth           = BIT(3),
        Stencil         = BIT(4),
        DepthStencil    = Depth | Stencil,
        Compressed      = BIT(5)
    };

    /// Image flags
    enum Flag {
        ClampFlag       = BIT(0),
        CubeMapFlag     = BIT(1),
        NormalMapFlag   = BIT(5),
        LinearSpaceFlag = BIT(7)
    };

    /// Cube map face
    enum CubeMapFace {
        PositiveX,
        NegativeX,
        PositiveY,
        NegativeY,
        PositiveZ,
        NegativeZ
    };

    /// Sample wrap mode
    enum SampleWrapMode {
        ClampMode,
        RepeatMode
    };

    /// Image resample filter
    enum ResampleFilter {
        Nearest,
        Bilinear,
        Bicubic
    };

    /// Compression quality
    enum CompressionQuality {
        Fast,
        Normal,
        HighQuality
    };

    /// Default constructor
    Image();

    /// Constructs image with the given data.
    /// If data is not nullptr, the image data is initialized with given data
    Image(int width, int height, int depth, int numSlices, int numMipmaps, Image::Format format, byte *data, int flags);
    
    /// Copy constructor
    Image(const Image &other);
    
    /// Assignment operator
    Image &operator=(const Image &other);
    
    /// Move constructor
    Image(Image &&other);
    
    /// Move operator
    Image &operator=(Image &&other);
    
    /// Destructor
    ~Image();

                        /// Returns true if image has no pixel data.
    bool                IsEmpty() const { return pic == nullptr; }

                        /// Returns image format name.
    const char *        FormatName() const { return Image::FormatName(format); }
                        /// Returns bytes per pixel.
    int                 BytesPerPixel() const { return Image::BytesPerPixel(format); }
                        /// Returns bytes per block for block compressed image.
    int                 BytesPerBlock() const { return Image::BytesPerBlock(format); }
                        /// Returns number of components.
    int                 NumComponents() const { return Image::NumComponents(format); }
                        /// Returns bits per pixel.
    void                GetBits(int *redBits, int *greenBits, int *blueBits, int *alphaBits) const { Image::GetBits(format, redBits, greenBits, blueBits, alphaBits); }
                        /// Returns true if image format has alpha channel.
    bool                HasAlpha() const { return Image::HasAlpha(format); }
                        /// Returns true if image format has 1 bit alpha channel.
    bool                HasOneBitAlpha() const { return Image::HasOneBitAlpha(format); }
                        /// Returns true if image is in the linear space.
    bool                IsLinearSpace() const { return (flags & LinearSpaceFlag) ? true : false; }
                        /// Returns true if image format is packed.
    bool                IsPacked() const { return Image::IsPacked(format); }
                        /// Returns true if image format is compressed.
    bool                IsCompressed() const { return Image::IsCompressed(format); }
                        /// Returns true if image format is float.
    bool                IsFloatFormat() const { return Image::IsFloatFormat(format); }
                        /// Returns true if image format is half float.
    bool                IsHalfFormat() const { return Image::IsHalfFormat(format); }
                        /// Returns true if image format is depth.
    bool                IsDepthFormat() const { return Image::IsDepthFormat(format); }
                        /// Returns true if image format is depth & stencil.
    bool                IsDepthStencilFormat() const { return Image::IsDepthStencilFormat(format); }
                        /// Returns true if image is cube map.
    bool                IsCubeMap() const { return !!(flags & CubeMapFlag) && numSlices == 6; }

                        /// Returns image width.
    int                 GetWidth() const { return width; }
                        /// Returns image width with the given mip level.
    int                 GetWidth(int mipMapLevel) const;
                        /// Returns image height.
    int                 GetHeight() const { return height; }
                        /// Returns image height with the given mip level.
    int                 GetHeight(int mipMapLevel) const;
                        /// Returns image depth.
    int                 GetDepth() const { return depth; }
                        /// Returns image depth with the given mip level.
    int                 GetDepth(int mipMapLevel) const;
                        /// Returns number of mip levels.
    int                 NumMipmaps() const { return numMipmaps; }
                        /// Returns number of slices.
    int                 NumSlices() const { return numSlices; }
                        /// Returns image flags.
    int                 GetFlags() const { return flags; }
                        /// Returns image format.
    Image::Format       GetFormat() const { return format; }

                        /// Returns pixel data pointer.
    byte *              GetPixels() const { return pic; }
                        /// Returns pixel data pointer with the given mip level.
    byte *              GetPixels(int level) const;
                        /// Returns pixel data pointer with the given mip level and slice index.
    byte *              GetPixels(int level, int sliceIndex) const;

                        /// Returns Color4 sample with the given 2D coordinates.
    Color4              Sample2D(const Vec2 &st, SampleWrapMode wrapModeS = ClampMode, SampleWrapMode wrapModeT = ClampMode, int level = 0) const;
                        /// Returns Color4 sample with the given cubemap coordinates.
    Color4              SampleCube(const Vec3 &str, int level = 0) const;

                        /// Returns number of pixels with the given mipmap levels.
    int                 NumPixels(int firstLevel = 0, int numLevels = 1) const;

                        /// Returns number of bytes with the given mipmap levels.
    int                 GetSize(int firstLevel = 0, int numLevels = 1) const;
                        /// Returns number of bytes of a slice with the given mipmap levels.
                        /// A slice means single cubemap face or single texture of an array texture.
    int                 GetSliceSize(int firstLevel = 0, int numLevels = 1) const;
                        
                        /// Clears allocated pixel data.
    void                Clear();

                        /// Creates an image with the given memory.
    Image &             InitFromMemory(int width, int height, int depth, int numSlices, int numMipmaps, Image::Format format, byte *data, int flags);

                        /// Creates an image.
                        /// If data is nullptr, just allocate the memory.
    Image &             Create(int width, int height, int depth, int numSlices, int numMipmaps, Image::Format format, const byte *data, int flags);

    Image &             Create2D(int width, int height, int numMipmaps, Image::Format format, const byte *data, int flags);
    Image &             Create3D(int width, int height, int depth, int numMipmaps, Image::Format format, const byte *data, int flags);
    Image &             CreateCube(int size, int numMipmaps, Image::Format format, const byte *data, int flags);
    Image &             Create2DArray(int width, int height, int numSlices, int numMipmaps, Image::Format format, const byte *data, int flags);

                        /// Creates an cubic image from six square images.
    Image &             CreateCubeFrom6Faces(const Image *faceImages);
                        /// Creates an cubic image from single equirectangular spherical image.
    Image &             CreateCubeFromEquirectangular(const Image &equirectangularImage, int faceSize);
                        /// Creates an equirectangular spherical image from cubic image.
    Image &             CreateEquirectangularFromCube(const Image &cubeImage);

                        /// Copy image data from another.
                        /// Nothing happen if source image dimensions are not match with this image.
    Image &             CopyFrom(const Image &srcImage, int firstLevel = 0, int numLevels = 1);
    
                        /// Generates full mipmaps if this image has
    Image &             GenerateMipmaps();

                        /// Converts this image to the given targetimage.
    bool                ConvertFormat(Image::Format dstFormat, Image &dstImage, bool regenerateMipmaps = false, CompressionQuality compressionQuality = Normal) const;
                        /// Converts this image in-place.
    bool                ConvertFormatSelf(Image::Format dstFormat, bool regenerateMipmaps = false, CompressionQuality compressionQuality = Normal);

                        /// Resizes this image to the given target image.
    bool                Resize(int width, int height, Image::ResampleFilter resampleFilter, Image &dstImage) const;
                        /// Resizes this image in-places.
    bool                ResizeSelf(int width, int height, Image::ResampleFilter resampleFilter);

                        /// Flips vertically.
    Image &             FlipX();
                        /// Flips horizontally.
    Image &             FlipY();

    Image &             AdjustBrightness(float factor);
    Image &             GammaCorrectRGB888(uint16_t ramp[768]);

                        /// Swaps the component red with alpha.
    Image &             SwapRedAlphaRGBA8888();

                        /// Makes height map to normal map.
    Image               MakeNormalMapRGBA8888(float bumpiness) const;

                        /// Adds normal map to another normal map.
    Image &             AddNormalMapRGBA8888(const Image &normalMap);

                        /// Loads image from the file.
    bool                Load(const char *filename);

                        /// Writes image to the file.
    bool                Write(const char *filename) const;

    bool                WriteDDS(const char *filename) const;
    bool                WritePVR(const char *filename) const;
    bool                WriteBMP(const char *filename) const;
    bool                WritePCX(const char *filename) const;
    bool                WriteTGA(const char *filename) const;
                        /// @param quality  value has the range [0, 100]
    bool                WriteJPG(const char *filename, int quality = 100) const; 
    bool                WritePNG(const char *filename) const;
    bool                WriteHDR(const char *filename) const;
    
                        // static helper functions to get image information
    static const char * FormatName(Image::Format imageFormat);
    static int          BytesPerPixel(Image::Format imageFormat);
    static int          BytesPerBlock(Image::Format imageFormat);
    static int          NumComponents(Image::Format imageFormat);
    static void         GetBits(Image::Format imageFormat, int *redBits, int *greenBits, int *blueBits, int *alphaBits);
    static bool         HasAlpha(Image::Format imageFormat);
    static bool         HasOneBitAlpha(Image::Format imageFormat);
    static bool         IsPacked(Image::Format imageFormat);
    static bool         IsCompressed(Image::Format imageFormat);
    static bool         IsFloatFormat(Image::Format imageFormat);
    static bool         IsHalfFormat(Image::Format imageFormat);
    static bool         IsDepthFormat(Image::Format imageFormat);
    static bool         IsDepthStencilFormat(Image::Format imageFormat);
    static int          MemRequired(int width, int height, int depth, int numMipmaps, Image::Format imageFormat);
    static int          MaxMipMapLevels(int width, int height, int depth);

                        /// Converts an sRGB value in the range [0, 1] to a linear value in the range [0, 1].
    static float        GammaToLinear(float value);
                        /// Converts a linear value in the range [0, 1] to an sRGB value in the range [0, 1].
    static float        LinearToGamma(float value);

                        /// Converts 2D face coordinates to cubemap coordinates.
    static Vec3         FaceToCubeMapCoords(CubeMapFace cubeMapFace, float s, float t);
                        /// Converts cubemap coordinates to 2D face coordinates.
    static CubeMapFace  CubeMapToFaceCoords(const Vec3 &cubeMapCoords, float &s, float &t);

    static float        CubeMapTexelSolidAngle(float x, float y, int size);

    static Image *      NewImageFromFile(const char *filename);

private:
    template <typename T>
    T                   WrapCoord(T coord, T maxCoord, SampleWrapMode wrapMode) const;

    bool                LoadDDSFromMemory(const char *name, const byte *data, size_t size);
    bool                LoadPVRFromMemory(const char *name, const byte *data, size_t size);
    bool                LoadPVR2FromMemory(const char *name, const byte *data, size_t size);
    bool                LoadPVR3FromMemory(const char *name, const byte *data, size_t size);
    bool                LoadBMPFromMemory(const char *name, const byte *data, size_t size);
    bool                LoadPCXFromMemory(const char *name, const byte *data, size_t size);
    bool                LoadTGAFromMemory(const char *name, const byte *data, size_t size);
    bool                LoadJPGFromMemory(const char *name, const byte *data, size_t size);
    bool                LoadPNGFromMemory(const char *name, const byte *data, size_t size);
    bool                LoadHDRFromMemory(const char *name, const byte *data, size_t size);
    
    int                 width;          ///< Width
    int                 height;         ///< Height
    int                 depth;          ///< Depth
    int                 numSlices;      ///< Number of array images or 6 for cubic image
    int                 numMipmaps;     ///< Number of mipmaps
    Image::Format       format;         ///< Image format
    int                 flags;          ///< Image flags
    bool                alloced;        ///< Is memory allocated ?
    byte *              pic;            ///< Actual pixel data
};

BE_INLINE Image::Image() {
    width = 0;
    height = 0;
    depth = 0;
    numSlices = 0;
    numMipmaps = 0;
    format = Image::UnknownFormat;
    flags = 0;
    alloced = false;
    pic = nullptr;
}

BE_INLINE Image::Image(int width, int height, int depth, int numSlices, int numMipmaps, Image::Format format, byte *data, int flags) {
    alloced = false;
    InitFromMemory(width, height, depth, numSlices, numMipmaps, format, data, flags);
    //Create(width, height, depth, numSlices, numMipmaps, format, data, flags);
}

BE_INLINE Image::Image(const Image &rhs) {
    alloced = false;
    Create(rhs.width, rhs.height, rhs.depth, rhs.numSlices, rhs.numMipmaps, rhs.format, rhs.pic, rhs.flags);
}

BE_INLINE Image::Image(Image &&rhs) : Image() {
    BE1::Swap(width, rhs.width);
    BE1::Swap(height, rhs.height);
    BE1::Swap(depth, rhs.depth);
    BE1::Swap(numSlices, rhs.numSlices);
    BE1::Swap(numMipmaps, rhs.numMipmaps);
    BE1::Swap(format, rhs.format);
    BE1::Swap(flags, rhs.flags);
    BE1::Swap(alloced, rhs.alloced);
    BE1::Swap(pic, rhs.pic);
}

BE_INLINE Image::~Image() {
    Clear();
}
    
BE_INLINE int Image::GetWidth(int mipMapLevel) const {
    int a = width >> mipMapLevel;
    return (a == 0) ? 1 : a;
}

BE_INLINE int Image::GetHeight(int mipMapLevel) const {
    int a = height >> mipMapLevel;
    return (a == 0) ? 1 : a;
}

BE_INLINE int Image::GetDepth(int mipMapLevel) const {
    int a = depth >> mipMapLevel;
    return (a == 0) ? 1 : a;
}

BE_INLINE byte *Image::GetPixels(int level) const {
    return (level < numMipmaps) ? pic + GetSize(0, level) : nullptr;
}

BE_INLINE byte *Image::GetPixels(int level, int sliceIndex) const {
    if (level >= numMipmaps || sliceIndex >= numSlices) return nullptr;
    return pic + GetSliceSize(0, numMipmaps) * sliceIndex + GetSliceSize(0, level);
}

BE_INLINE Image &Image::Create2D(int width, int height, int numMipmaps, Image::Format format, const byte *data, int flags) {
    return Create(width, height, 1, 1, numMipmaps, format, data, flags);
}

BE_INLINE Image &Image::Create3D(int width, int height, int depth, int numMipmaps, Image::Format format, const byte *data, int flags) {
    return Create(width, height, depth, 1, numMipmaps, format, data, flags);
}

BE_INLINE Image &Image::CreateCube(int size, int numMipmaps, Image::Format format, const byte *data, int flags) {
    return Create(size, size, 1, 6, numMipmaps, format, data, flags | CubeMapFlag);
}

BE_INLINE Image &Image::Create2DArray(int width, int height, int numSlices, int numMipmaps, Image::Format format, const byte *data, int flags) {
    return Create(width, height, 1, numSlices, numMipmaps, format, data, flags);
}

BE_INLINE float Image::GammaToLinear(float f) {
    if (f <= 0.4045f) {
        return f / 12.92f;
    } else {
        return Math::Pow((f + 0.055f) / 1.055f, 2.4f);
    }
}

BE_INLINE float Image::LinearToGamma(float f) {
    if (f <= 0.0031308f) {
        return f * 12.92f;
    } else {
        return 1.055f * Math::Pow(f, 1.0f / 2.4f) - 0.055f;
    }
}

template <typename T>
BE_INLINE T Image::WrapCoord(T coord, T maxCoord, SampleWrapMode wrapMode) const {
    if (wrapMode == SampleWrapMode::ClampMode) {
        Clamp<T>(coord, 0, maxCoord);
    } else if (wrapMode == SampleWrapMode::RepeatMode) {
        Wrap<T>(coord, 0, maxCoord);
    }
    return coord;
}

BE_NAMESPACE_END
