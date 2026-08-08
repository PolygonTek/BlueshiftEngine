// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "Math/Math.h"

BE_NAMESPACE_BEGIN

class File;

/// Image representation
class Image {
public:
    /// Various image format type
    enum class Format : uint8_t {
        Unknown,
        R32_FLOAT,
        R32_SINT,
        R32_UINT,
        R32G32_FLOAT,
        R32G32_SINT,
        R32G32_UINT,
        R32G32B32_FLOAT,
        R32G32B32_SINT,
        R32G32B32_UINT,
        R32G32B32A32_FLOAT,
        R32G32B32A32_SINT,
        R32G32B32A32_UINT,
        R16_FLOAT,
        R16,
        R16_SNORM,
        R16_SINT,
        R16_UINT,
        R16G16_FLOAT,
        R16G16,
        R16G16_SNORM,
        R16G16_SINT,
        R16G16_UINT,
        R16G16B16_FLOAT,
        R16G16B16,
        R16G16B16_SNORM,
        R16G16B16_SINT,
        R16G16B16_UINT,
        R16G16B16A16_FLOAT,
        R16G16B16A16,
        R16G16B16A16_SNORM,
        R16G16B16A16_SINT,
        R16G16B16A16_UINT,
        R9G9B9E5_FLOAT,
        R11G11B10_FLOAT,
        R8,
        R8_SNORM,
        R8_SINT,
        R8_UINT,
        R8G8,
        R8G8_SNORM,
        R8G8_SINT,
        R8G8_UINT,
        R8G8B8,
        R8G8B8_SNORM,
        R8G8B8_SINT,
        R8G8B8_UINT,
        B8G8R8,
        R8G8B8A8,
        R8G8B8A8_SNORM,
        R8G8B8A8_SINT,
        R8G8B8A8_UINT,
        B8G8R8A8,
        A8B8G8R8,
        A8R8G8B8,
        R8G8B8X8,
        B8G8R8X8,

        R10G10B10A2,
        R10G10B10A2_UINT,
        R4G4B4X4,
        B4G4R4X4,
        R4G4B4A4,
        B4G4R4A4,
        A4B4G4R4,
        A4R4G4B4,
        R5G5B5X1,
        B5G5R5X1,
        R5G5B5A1,
        B5G5R5A1,
        A1B5G5R5,
        A1R5G5B5,
        R5G6B5,
        B5G6R5,

        // Depth/Stencil formats
        D16,                // depth (16-bit)
        D24X8,              // depth (24-bit)
        D24S8,              // depth (24-bit) + stencil (8-bit)
        D32_FLOAT,          // depth (32-bit)
        D32_FLOAT_S8X24,    // depth (32-bit) + stencil (8-bit)

        // Luminance/Alpha
        L32_FLOAT,
        A32_FLOAT,
        L32A32_FLOAT,
        L16_FLOAT,
        A16_FLOAT,
        L16A16_FLOAT,
        L8,
        A8,
        L8A8,

        // Compressed (BC) formats
        BC1,                // BC1(aka DXT1): RGB 5:6:5 color block, optional 1-bit alpha
        BC2,                // BC2(aka DXT3): RGB 5:6:5 color block, explicit 4-bit alpha
        BC3,                // BC3(aka DXT5): RGB 5:6:5 color block, interpolated 8-bit alpha
        BC3XGBR,            // BC3(aka DXT5) XGBR: Red-alpha swapped BC3 (Doom 3 normal-map encoding)
        BC4,                // BC4(aka DXN1 or ATI1N): One 8-bit channel
        BC5,                // BC5(aka DXN2 or ATI2N): Two 8-bit channels
        BC6H_UF16,          // BC6H_UF16: Three HDR color channels, unsigned half float, 16-byte 4x4 block
        BC6H_SF16,          // BC6H_SF16: Three HDR color channels, signed half float, 16-byte 4x4 block
        BC7,                // BC7: RGB/RGBA UNorm, variable precision, 16-byte 4x4 block

        // Compressed (PVRTC) formats
        PVRTC12,            // PVRTC1 RGB 2BPP
        PVRTC14,            // PVRTC1 RGB 4BPP
        PVRTC12A,           // PVRTC1 RGBA 2BPP
        PVRTC14A,           // PVRTC1 RGBA 4BPP
        PVRTC22A,           // PVRTC2 RGBA 2BPP
        PVRTC24A,           // PVRTC2 RGBA 4BPP

        // Compressed (ETC1/ETC2/EAC) formats
        ETC1,               // ETC1 RGB8
        ETC2,               // ETC2 RGB8
        ETC2A1,             // ETC2 RGB8A1
        ETC2A,              // ETC2 RGBA8
        EACR11,             // EAC R11
        EACRG11,            // EAC RG11
        EACR11_SNORM,       // EAC R11 SNROM
        EACRG11_SNORM,      // EAC RG11 SNORM

        // Compressed (ATC) formats
        ATC,                // ATC RGB 4BPP
        ATCE,               // ATCE RGBA 8BPP explicit alpha
        ATCI,               // ATCI RGBA 8BPP interpolated alpha
        Count,

        // Legacy aliases
        DXT1 = BC1,
        DXT3 = BC2,
        DXT5 = BC3,
        DXT5XGBR = BC3XGBR,
        DXN1 = BC4,
        DXN2 = BC5
    };

    /// Format type
    enum class FormatType : uint16_t {
        None                = 0,
        UNorm               = BIT(0),
        SNorm               = BIT(1),
        UInt                = BIT(2),
        SInt                = BIT(3),
        Float               = BIT(4),
        Depth               = BIT(5),
        Stencil             = BIT(6),
        DepthStencil        = Depth | Stencil,
        Packed              = BIT(7),
        Compressed          = BIT(8)
    };

    friend constexpr std::true_type EnableBitmaskEnum(FormatType) noexcept { return {}; }

    /// Enum for the different kinds of gamma spaces we expect to need to convert from/to.
    enum class GammaSpace : uint8_t {
        DontCare,
        Linear,
        Pow22,
        sRGB
    };

    /// BTEX payload image format.
    enum class PayloadFormat : uint8_t {
        Unknown,
        DDS,
        PVR,
        TGA,
        PNG
    };

    /// Image flags
    enum class Flag : uint8_t {
        None                = 0,
        CubeMap             = BIT(0),

        // Texture hints
        NormalMap           = BIT(1),
        NoMipmaps           = BIT(2),
        NoCompression       = BIT(3),
        NoStreaming         = BIT(4),
        TextureHintsMask    = NormalMap | NoMipmaps | NoCompression | NoStreaming
    };

    friend constexpr std::true_type EnableBitmaskEnum(Flag) noexcept { return {}; }

    /// Cube map face in engine axis order. The numeric order matches serialized DDS face slices.
    enum class CubeMapFace : uint8_t {
        PositiveX = 0,      ///< Face centered on the engine +X axis.
        NegativeX = 1,      ///< Face centered on the engine -X axis.
        PositiveY = 2,      ///< Face centered on the engine +Y axis.
        NegativeY = 3,      ///< Face centered on the engine -Y axis.
        PositiveZ = 4,      ///< Face centered on the engine +Z axis.
        NegativeZ = 5       ///< Face centered on the engine -Z axis.
    };

    static_assert(static_cast<uint8_t>(CubeMapFace::PositiveX) == 0 &&
        static_cast<uint8_t>(CubeMapFace::NegativeX) == 1 &&
        static_cast<uint8_t>(CubeMapFace::PositiveY) == 2 &&
        static_cast<uint8_t>(CubeMapFace::NegativeY) == 3 &&
        static_cast<uint8_t>(CubeMapFace::PositiveZ) == 4 &&
        static_cast<uint8_t>(CubeMapFace::NegativeZ) == 5, "Unexpected cube map face order");

    /// Sample wrap mode
    enum class SampleWrapMode : uint8_t {
        Clamp,
        Repeat
    };

    enum class SampleFilter : uint8_t {
        Nearest,
        Bilinear
    };

    /// Image resample filter
    enum class ResampleFilter : uint8_t {
        Nearest,
        Bilinear,
        Bicubic
    };

    /// Compression quality
    enum class CompressionQuality : uint8_t {
        Fast,
        Normal,
        HighQuality
    };

    /// Mipmap generation mode
    enum class MipmapGenerationMode : uint8_t {
        NoMipmaps,
        Mipmaps,
        MipmapsWithAlphaCoverage
    };

    /// Describes the original image dimensions and the loaded mip range.
    struct MipRange {
        int32_t         originalWidth = 0;
        int32_t         originalHeight = 0;
        int32_t         originalDepth = 0;
        int32_t         originalMipLevels = 0;
        int32_t         firstMip = 0;
        int32_t         mipCount = 0;
    };

    /// Default constructor.
    Image() = default;

    /// Constructs image with the given data.
    /// If data is not nullptr, the image data is initialized with given data.
    Image(int32_t width, int32_t height, int32_t depth, int32_t numSlices, int32_t numMipmaps, Format format, GammaSpace gammaSpace, byte *data, Flag flags);
    
    /// Copy constructor.
    Image(const Image &other);
    
    /// Assignment operator.
    Image &operator=(const Image &other);
    
    /// Move constructor.
    Image(Image &&other) noexcept;
    
    /// Move operator.
    Image &operator=(Image &&other) noexcept;
    
    /// Destructor.
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
                        /// Returns true if image format needs floating conversion in order to not lose precision.
    bool                NeedFloatConversion() const { return Image::NeedFloatConversion(format); }
                        /// Returns true if image is cube map.
    bool                IsCubeMap() const { return HasFlag(flags, Flag::CubeMap); }

                        /// Returns image width.
    int32_t             GetWidth() const { return width; }
                        /// Returns image width with the given mip level.
    int32_t             GetWidth(int32_t mipLevel) const;
                        /// Returns image height.
    int32_t             GetHeight() const { return height; }
                        /// Returns image height with the given mip level.
    int32_t             GetHeight(int32_t mipLevel) const;
                        /// Returns image depth.
    int32_t             GetDepth() const { return depth; }
                        /// Returns image depth with the given mip level.
    int32_t             GetDepth(int32_t mipLevel) const;
                        /// Returns array size (6 for cubic image)
    int32_t             GetArraySize() const { return arraySize; }
                        /// Returns number of mip levels.
    int32_t             NumMipmaps() const { return numMipLevels; }
                        /// Returns image flags.
    Flag                GetFlags() const { return flags; }
                        /// Returns texture hint flags.
    Flag                GetTextureHints() const;
                        /// Returns image format.
    Format              GetFormat() const { return format; }
                        /// Returns gamma space of this image.
    GammaSpace          GetGammaSpace() const { return gammaSpace; }
                        /// Sets gamma space.
    void                SetGammaSpace(GammaSpace gammaSpace) { this->gammaSpace = gammaSpace; }

                        /// Returns pixel data pointer.
    byte *              GetPixels() const { return pic; }
                        /// Returns pixel data pointer with the given mip level.
    byte *              GetPixels(int32_t mipLevel) const;
                        /// Returns pixel data pointer with the given mip level and slice index.
    byte *              GetPixels(int32_t mipLevel, int32_t sliceIndex) const;

                        /// Returns linearly interpolated Color4 sample with the given 2D coordinates.
    Color4              Sample2D(const Vec2 &uv, SampleWrapMode wrapModeU = SampleWrapMode::Clamp, SampleWrapMode wrapModeV = SampleWrapMode::Clamp, SampleFilter filter = SampleFilter::Bilinear, int32_t mipLevel = 0) const;
                        /// Returns linearly interpolated Color4 sample with the given cubemap coordinates.
    Color4              SampleCube(const Vec3 &direction, SampleFilter filter = SampleFilter::Bilinear, int32_t mipLevel = 0) const;

                        /// Returns number of pixels with the given mipmap levels.
    uint64_t            NumPixels(int32_t firstMipLevel = 0, int32_t numMipLevels = 1) const;

                        /// Returns RGB sum in linear space for the given mip level and slice.
    Color3              ComputeRGBSum(int32_t mipLevel = 0, int32_t sliceIndex = 0, bool ignoreNegativeValues = false) const;
                        /// Returns luminance sum in linear space for the given mip level and slice.
    float               ComputeLuminanceSum(int32_t mipLevel = 0, int32_t sliceIndex = 0, bool ignoreNegativeValues = false) const;
                        /// Returns luminance bounds in linear space for the given mip level and slice.
    bool                ComputeLuminanceBounds(int32_t mipLevel, int32_t sliceIndex, float luminanceThreshold, bool ignoreNegativeValues, Rect &outBounds) const;
                        /// Returns centered luminance bounds that retain at least the given fraction of total luminance in linear space.
    bool                ComputeLuminanceEnergyBounds(int32_t mipLevel, int32_t sliceIndex, float retainedEnergyFraction, bool ignoreNegativeValues, Rect &outBounds, float &outLuminanceSum) const;

                        /// Returns number of bytes with the given mipmap levels.
    size_t              SizeInBytes(int32_t firstMipLevel = 0, int32_t numMipLevels = 1) const;
                        /// Returns number of bytes of single cubemap face with the given mipmap levels.
    size_t              SizeInBytesForSlice(int32_t firstMipLevel = 0, int32_t numMipLevels = 1) const;
                        
                        /// Clears allocated pixel data.
    void                Clear();

                        /// Creates an image with the given memory.
    Image &             InitFromMemory(int32_t width, int32_t height, int32_t depth, int32_t numSlices, int32_t numMipLevels, Format format, GammaSpace gammaSpace, byte *data, Flag flags);

                        /// Creates an image.
                        /// If data is nullptr, just allocate the memory.
    Image &             Create(int32_t width, int32_t height, int32_t depth, int32_t numSlices, int32_t numMipLevels, Format format, GammaSpace gammaSpace, const byte *data, Flag flags);

    Image &             Create2D(int32_t width, int32_t height, int32_t numMipLevels, Format format, GammaSpace gammaSpace, const byte *data, Flag flags);
    Image &             Create3D(int32_t width, int32_t height, int32_t depth, int32_t numMipLevels, Format format, GammaSpace gammaSpace, const byte *data, Flag flags);
    Image &             CreateCube(int32_t size, int32_t numMipLevels, Format format, GammaSpace gammaSpace, const byte *data, Flag flags);
    Image &             Create2DArray(int32_t width, int32_t height, int32_t numSlices, int32_t numMipLevels, Format format, GammaSpace gammaSpace, const byte *data, Flag flags);

                        /// Creates a cubic image from six square images.
    Image &             CreateCubeFrom6Faces(const Image *faceImages);
                        /// Creates a cubic image from single equirectangular spherical image.
    Image &             CreateCubeFromEquirectangular(const Image &equirectangularImage, int32_t faceSize);
                        /// Creates an equirectangular spherical image from cubic image.
    Image &             CreateEquirectangularFromCube(const Image &cubeImage);

                        /// Copies image data from another.
                        /// Nothing happen if source image dimensions are not match with this image.
    Image &             CopyFrom(const Image &srcImage, int32_t firstMipLevel = 0, int32_t numMipLevels = 1);

                        /// Updates sub region.
    void                Update2D(int32_t mipLevel, int32_t x, int32_t y, int32_t width, int32_t height, const byte *data);
    
                        /// Generates all mipmaps this image has.
    Image &             GenerateMipmaps(bool preserveAlphaCoverage = false);

                        /// Converts this image to the given target image.
    bool                ConvertFormat(Format dstFormat, Image &dstImage, 
                            GammaSpace dstGammaSpace = GammaSpace::DontCare, MipmapGenerationMode regenerateMipmaps = MipmapGenerationMode::NoMipmaps, CompressionQuality compressionQuality = CompressionQuality::Normal) const;
                        /// Converts this image in-place.
    bool                ConvertFormatSelf(Format dstFormat, 
                            GammaSpace dstGammaSpace = GammaSpace::DontCare, MipmapGenerationMode regenerateMipmaps = MipmapGenerationMode::NoMipmaps, CompressionQuality compressionQuality = CompressionQuality::Normal);

                        /// Resizes this image to the given target image.
    bool                Resize(int32_t width, int32_t height, ResampleFilter resampleFilter, Image &dstImage) const;
                        /// Resizes this image in-places.
    bool                ResizeSelf(int32_t width, int32_t height, ResampleFilter resampleFilter);

                        /// Flips vertically.
    Image &             FlipX();
                        /// Flips horizontally.
    Image &             FlipY();

                        /// Adjusts brightness of this image, in-place.
    Image &             AdjustBrightness(float factor);

    Image &             ApplyGammaRampTableRGB888(const uint16_t table[768]);

                        /// Returns dilated image.
    Image               MakeDilation() const;

                        /// Returns eroded image.
    Image               MakeErosion() const;

                        /// Returns SDF image.
    Image               MakeSDF(int32_t spread) const;

                        /// Swaps the component red with alpha.
    Image &             SwapRedAlphaRGBA8888();

                        /// Makes height map to normal map.
    Image               MakeNormalMapRGBA8888(float bumpiness) const;

                        /// Adds normal map to another normal map.
    Image &             AddNormalMapRGBA8888(const Image &normalMap);

                        /// Loads image from the file.
    bool                Load(const char *filename);

                        /// Loads the specified streamable image mip range from a file.
    bool                LoadMipRange(const char *filename, int32_t firstMip, int32_t mipCount, MipRange *outInfo);

                        /// Loads the streamable image mip range whose largest dimension is at most maxDimension.
    bool                LoadMipRangeForMaxDimension(const char *filename, int32_t maxDimension, MipRange *outInfo);

                        /// Writes image to the file.
    bool                Write(const char *filename) const;

    bool                WriteDDS(const char *filename) const;
    bool                WritePVR(const char *filename) const;
    bool                WriteBMP(const char *filename) const;
    bool                WritePCX(const char *filename) const;
    bool                WriteTGA(const char *filename) const;
    bool                WriteBTEX(const char *filename, PayloadFormat payloadFormat = PayloadFormat::DDS) const;
                        /// @param quality  value has the range [0, 100]
    bool                WriteJPG(const char *filename, int quality = 100) const; 
    bool                WritePNG(const char *filename) const;
    bool                WriteHDR(const char *filename) const;
    
                        // static helper functions to get image information
    static const char * FormatName(Format imageFormat);
    static int          BytesPerPixel(Format imageFormat);
    static int          BytesPerBlock(Format imageFormat);
    static int          NumComponents(Format imageFormat);
    static void         GetBits(Format imageFormat, int *redBits, int *greenBits, int *blueBits, int *alphaBits);
    static bool         HasAlpha(Format imageFormat);
    static bool         HasOneBitAlpha(Format imageFormat);
    static bool         IsPacked(Format imageFormat);
    static bool         IsCompressed(Format imageFormat);
    static bool         IsFloatFormat(Format imageFormat);
    static bool         IsHalfFormat(Format imageFormat);
    static bool         IsDepthFormat(Format imageFormat);
    static bool         IsDepthStencilFormat(Format imageFormat);
    static bool         NeedFloatConversion(Format imageFormat);
    static uint64_t     MemRequired(int32_t width, int32_t height, int32_t depth, int32_t numMipmaps, int32_t numSlices, Format imageFormat);
    static int32_t      MaxMipLevels(int32_t width, int32_t height, int32_t depth);
    static int32_t      ComputeFirstMipForMaxDimension(int32_t width, int32_t height, int32_t depth, int32_t numMipLevels, int32_t maxDimension);

                        /// Converts an sRGB value in the range [0, 1] to a linear value in the range [0, 1].
    static float        GammaToLinear(float value);
                        /// Converts a linear value in the range [0, 1] to an sRGB value in the range [0, 1].
    static float        LinearToGamma(float value);

    static float        GammaToLinearApprox(float value);
    static float        LinearToGammaApprox(float value);

    static float        GammaToLinearFast(float value);
    static float        LinearToGammaFast(float value);

                        /// Convert RGB to luminance with RGB in linear space with sRGB primaries and D65 white point.
    static float        GetLuminance(const Color3 &linearColor);

                        /// Converts 2D face coordinates to a direction in engine axes.
    static Vec3         FaceToCubeMapCoords(CubeMapFace cubeMapFace, float u, float v);
                        /// Converts a direction in engine axes to 2D face coordinates.
    static CubeMapFace  CubeMapToFaceCoords(const Vec3 &direction, float &u, float &v);

                        /// Converts equirectangular UV coordinates to a direction in engine axes.
    static Vec3         EquirectangularUVToDirection(float u, float v);
                        /// Converts a direction in engine axes to equirectangular UV coordinates.
    static Vec2         DirectionToEquirectangularUV(const Vec3 &direction);

                        /// Returns the solid angle in steradians of the given cubemap texel.
    static float        CubeMapTexelSolidAngle(float x, float y, int32_t size);

    static Image        ImageFromFile(const char *filename);
    static std::shared_ptr<Image> SharedImageFromFile(const char *filename);

    static float        pow22ToLinearTable[256];
    static float        sRGBToLinearTable[256];

private:
    bool                LoadBTEXDDSMipRange(const char *filename, int32_t firstMip, int32_t mipCount, MipRange *outInfo);
    bool                LoadBTEXDDSMipRangeForMaxDimension(const char *filename, int32_t maxDimension, MipRange *outInfo);
    bool                LoadDDSMipRange(const char *filename, int32_t firstMip, int32_t mipCount, MipRange *outInfo);
    bool                LoadDDSMipRange(File *fp, int32_t firstMip, int32_t mipCount, MipRange *outInfo);
    bool                LoadDDSMipRangeForMaxDimension(const char *filename, int32_t maxDimension, MipRange *outInfo);
    bool                LoadDDSMipRangeForMaxDimension(File *fp, int32_t maxDimension, MipRange *outInfo);
    bool                LoadDDSMipRangeData(File *fp, int32_t originalWidth, int32_t originalHeight, int32_t originalDepth, int32_t ddsArraySize, int32_t originalMipLevels, bool isCubemap, int32_t firstMip, int32_t mipCount, MipRange *outInfo);

    template <typename T>
    T                   WrapCoord(T coord, T maxCoord, SampleWrapMode wrapMode) const;

    Color4              Sample2DNearest(const byte *src, const Vec2 &uv, SampleWrapMode wrapModeU, SampleWrapMode wrapModeV) const;
    Color4              Sample2DBilinear(const byte *src, const Vec2 &uv, SampleWrapMode wrapModeU, SampleWrapMode wrapModeV) const;

    bool                WriteDDS(File *fp) const;
    bool                WritePVR(File *fp) const;
    bool                WriteBMP(File *fp) const;
    bool                WritePCX(File *fp) const;
    bool                WriteTGA(File *fp) const;
    bool                WritePNG(File *fp) const;
    bool                WriteHDR(File *fp) const;

    bool                LoadBTEXFromMemory(const char *name, const byte *data, size_t size);
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
    bool                LoadEXRFromMemory(const char *name, const byte *data, size_t size);

    byte *              pic = nullptr;                  ///< Pixel data
    int32_t             width = 0;                      ///< Width
    int32_t             height = 0;                     ///< Height
    int32_t             depth = 0;                      ///< Depth
    int32_t             arraySize = 0;                  ///< Number of array images (6 for cubic image)
    int32_t             numMipLevels = 0;               ///< Number of mip levels
    Format              format = Format::Unknown;       ///< Image format
    GammaSpace          gammaSpace = GammaSpace::sRGB;  ///< Gamma space enum
    Flag                flags = Flag::None;             ///< Image flags
    bool                alloced = false;                ///< Is memory allocated ?
};

BE_INLINE Image::Flag Image::GetTextureHints() const {
    return flags & Flag::TextureHintsMask;
}

BE_INLINE Image::Image(int32_t width, int32_t height, int32_t depth, int32_t numSlices, int32_t numMipmaps, Format format, GammaSpace gammaSpace, byte *data, Flag flags) {
    alloced = false;
    InitFromMemory(width, height, depth, numSlices, numMipmaps, format, gammaSpace, data, flags);
    //Create(width, height, depth, numSlices, numMipmaps, format, gammaSpace, data, flags);
}

BE_INLINE Image::Image(const Image &rhs) {
    alloced = false;
    Create(rhs.width, rhs.height, rhs.depth, rhs.arraySize, rhs.numMipLevels, rhs.format, rhs.gammaSpace, rhs.pic, rhs.flags);
}

BE_INLINE Image::Image(Image &&rhs) noexcept : Image() {
    BE1::Swap(width, rhs.width);
    BE1::Swap(height, rhs.height);
    BE1::Swap(depth, rhs.depth);
    BE1::Swap(arraySize, rhs.arraySize);
    BE1::Swap(numMipLevels, rhs.numMipLevels);
    BE1::Swap(format, rhs.format);
    BE1::Swap(gammaSpace, rhs.gammaSpace);
    BE1::Swap(flags, rhs.flags);
    BE1::Swap(alloced, rhs.alloced);
    BE1::Swap(pic, rhs.pic);
}

BE_INLINE Image::~Image() {
    Clear();
}

BE_INLINE int32_t Image::GetWidth(int32_t mipLevel) const {
    int32_t a = width >> mipLevel;
    return (a == 0) ? 1 : a;
}

BE_INLINE int32_t Image::GetHeight(int32_t mipLevel) const {
    int32_t a = height >> mipLevel;
    return (a == 0) ? 1 : a;
}

BE_INLINE int32_t Image::GetDepth(int32_t mipLevel) const {
    int32_t a = depth >> mipLevel;
    return (a == 0) ? 1 : a;
}

BE_INLINE byte *Image::GetPixels(int32_t mipLevel) const {
    return (mipLevel < numMipLevels) ? pic + SizeInBytes(0, mipLevel) : nullptr;
}

BE_INLINE byte *Image::GetPixels(int32_t mipLevel, int32_t sliceIndex) const {
    if (mipLevel >= numMipLevels || sliceIndex >= arraySize) {
        return nullptr;
    }
    // Image 의 데이터는 Slice 우선순으로 저장되어 있다.
    // Slice0: Mip0, Mip1, Mip2, ...
    // Slice1: Mip0, Mip1, Mip2, ...
    // ...
    return pic + SizeInBytesForSlice(0, numMipLevels) * sliceIndex + SizeInBytesForSlice(0, mipLevel);
}

BE_INLINE Image &Image::Create2D(int32_t width, int32_t height, int32_t numMipLevels, Format format, GammaSpace gammaSpace, const byte *data, Flag flags) {
    return Create(width, height, 1, 1, numMipLevels, format, gammaSpace, data, flags);
}

BE_INLINE Image &Image::Create3D(int32_t width, int32_t height, int32_t depth, int32_t numMipLevels, Format format, GammaSpace gammaSpace, const byte *data, Flag flags) {
    return Create(width, height, depth, 1, numMipLevels, format, gammaSpace, data, flags);
}

BE_INLINE Image &Image::CreateCube(int32_t size, int32_t numMipLevels, Format format, GammaSpace gammaSpace, const byte *data, Flag flags) {
    return Create(size, size, 1, 6, numMipLevels, format, gammaSpace, data, (flags | Flag::CubeMap));
}

BE_INLINE Image &Image::Create2DArray(int32_t width, int32_t height, int32_t numSlices, int32_t numMipLevels, Format format, GammaSpace gammaSpace, const byte *data, Flag flags) {
    return Create(width, height, 1, numSlices, numMipLevels, format, gammaSpace, data, flags);
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

// Fast sRGB to linear approximation.
// Reference: http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
BE_INLINE float Image::GammaToLinearApprox(float f) {
    return f * (f * (f * 0.305306011f + 0.682171111f) + 0.012522878f);
}

// An almost-perfect approximation.
// Reference: http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html?m=1
BE_INLINE float Image::LinearToGammaApprox(float f) {
    f = Max(f, 0.0f);
    return Max(1.055f * Math::Pow(f, 0.416666667f) - 0.055f, 0.0f);
}

BE_INLINE float Image::GammaToLinearFast(float f) {
    return Math::Pow(f, 2.2f);
}

BE_INLINE float Image::LinearToGammaFast(float f) {
    return Math::Pow(f, 1.0f / 2.2f);
}

BE_INLINE float Image::GetLuminance(const Color3 &linearRgb) {
    return linearRgb[0] * 0.2126729f + linearRgb[1] * 0.7151522f + linearRgb[2] * 0.0721750f;
}

template <typename T>
BE_INLINE T Image::WrapCoord(T coord, T maxCoord, SampleWrapMode wrapMode) const {
    if (wrapMode == SampleWrapMode::Clamp) {
        Clamp<T>(coord, 0, maxCoord);
    } else if (wrapMode == SampleWrapMode::Repeat) {
        Wrap<T>(coord, 0, maxCoord);
    }
    return coord;
}

BE_INLINE Image Image::ImageFromFile(const char *filename) {
    Image image;
    image.Load(filename);
    return image;
}

BE_INLINE std::shared_ptr<Image> Image::SharedImageFromFile(const char *filename) {
    std::shared_ptr<Image> image = std::make_shared<Image>();
    image->Load(filename);
    return image;
}

BE_NAMESPACE_END
