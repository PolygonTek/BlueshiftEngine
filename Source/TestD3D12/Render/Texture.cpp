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

#include "Precompiled.h"
#include "Texture.h"
#include "Sampler.h"
#include "LoadingSystem.h"

Texture::InternalData &Texture::InternalData::operator=(InternalData &&rhs) noexcept {
    if (this != &rhs) {
        Purge();
        Swap(rhs);
    }
    return *this;
}

void Texture::InternalData::Purge() {
    if (rhiTexture) {
        RHI::renderer->DestroyTexture(rhiTexture);
        rhiTexture = nullptr;
    }

    rhiTextureType = RHI::TextureType::Unknown;
    format = BE1::Image::Format::Unknown;
    flags = Flag::None;
}

void Texture::InternalData::Swap(InternalData &rhs) {
    BE1::Swap(rhiTextureType, rhs.rhiTextureType);
    BE1::Swap(rhiTexture, rhs.rhiTexture);
    BE1::Swap(format, rhs.format);
    BE1::Swap(flags, rhs.flags);
}

bool Texture::IsReadyForRendering() const {
    return IsReady() && internalData.rhiTexture && internalData.rhiTexture->GetStatus() == RHI::GPUResourceStatus::Ready;
}

uint64_t Texture::MemRequired(bool includingMipmaps) const {
    assert(IsReady());

    uint32_t w = GetWidth();
    uint32_t h = GetHeight();
    uint32_t d = GetDepth();
    uint32_t numMipmaps = includingMipmaps ? BE1::Image::MaxMipLevels(w, h, d) : 1;
    uint32_t numSlices = GetArraySize();

    return BE1::Image::MemRequired(w, h, d, numMipmaps, numSlices, internalData.format);
}

uint64_t Texture::MemRequiredStreaming(int firstMip) const {
    assert(IsReady());

    if (streaming.originalMipLevels <= 0) {
        return 0;
    }

    firstMip = BE1::Max(0, BE1::Min(firstMip, streaming.originalMipLevels - 1));
    uint32_t w = BE1::Max(1, (int)(streaming.originalWidth >> firstMip));
    uint32_t h = BE1::Max(1, (int)(streaming.originalHeight >> firstMip));
    uint32_t d = BE1::Max(1, (int)(streaming.originalDepth >> firstMip));
    uint32_t numMipmaps = streaming.originalMipLevels - firstMip;
    uint32_t numSlices = GetArraySize();

    return BE1::Image::MemRequired(w, h, d, numMipmaps, numSlices, internalData.format);
}

void Texture::Purge() {
    if (streaming.streamingListIndex != TextureStreamingInvalidIndex) {
        textureManager.UnregisterStreamingTexture(this);
    } else if (streaming.pendingReplacementTexture) {
        RHI::renderer->DestroyTexture(streaming.pendingReplacementTexture);
        streaming.pendingReplacementTexture = nullptr;
    }

    internalData.Purge();
}

void Texture::Reset() {
    BumpLoadSerial(); // 진행 중인 로딩 completion 무효화
    state = State::Empty;

    Purge();
}

bool Texture::Create(RHI::TextureType textureType, SharedPtr<const BE1::Image> srcImage, Texture::Flag flags) {
    InternalData newInternalData;
    if (!Texture::Prepare(textureType, srcImage, flags, newInternalData)) {
        return false;
    }

    BumpLoadSerial(); // 진행 중인 async completion 무효화
    state = State::Empty;

    ApplyPrepared(newInternalData);

    state = State::Ready;
    return true;
}

void Texture::CreateDefaultTexture(int size, Texture::Flag flags) {
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Create2D(size, size, 1, BE1::Image::Format::R8G8B8A8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);
    byte *dstPtr = image->GetPixels();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (x == 0 || x == size - 1 || y == 0 || y == size - 1) {
                dstPtr[0] = (byte)(0);
                dstPtr[1] = (byte)(0);
                dstPtr[2] = (byte)(0);
                dstPtr[3] = (byte)(255);
            } else {
                dstPtr[0] = (byte)(255);
                dstPtr[1] = (byte)(0);
                dstPtr[2] = (byte)(255);
                dstPtr[3] = (byte)(255);
            }
            dstPtr += 4;
        }
    }

    flags |= Texture::Flag::HighQuality | Texture::Flag::NoCompression | Texture::Flag::UnorderedAccess | Texture::Flag::NoMipmaps | Texture::Flag::AllocateEmptyMipmaps;
    Create(RHI::TextureType::Texture2D, image, flags);

    // 컴퓨트 쉐이더를 통해 밉맵 생성
    // 컴퓨트 쉐이더로 밉맵을 생성하기 위해서는 대상 텍스쳐를 만들 때 하위 mip level 들이 존재해야 하고, UAV 도 만들어야 한다.
    // UAV 로 만들 sRGB 텍스쳐의 경우에는 텍스쳐 생성 함수에서 자동으로 Typeless 텍스쳐로 만들어 진다.
    // sRGB 텍스쳐를 생성할 때 Typeless 로 만들어야 SRV 는 sRGB 포맷으로, UAV 는 Linear 포맷으로 생성된다.
    GenerateMipmaps();
}

void Texture::CreateColorTexture(int size, const BE1::Color4 &color, Flag flags) {
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Create2D(size, size, 1, BE1::Image::Format::R8G8B8A8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);
    uint32_t *dstPtr = (uint32_t *)image->GetPixels();
    uint32_t c = color.ToUInt32();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            *(uint32_t *)dstPtr++ = c;
        }
    }

    Create(RHI::TextureType::Texture2D, image, Texture::Flag::HighQuality | flags);
}

void Texture::CreateFlatNormalTexture(int size, Texture::Flag flags) {
    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Create2D(size, size, 1, BE1::Image::Format::R8G8B8, BE1::Image::GammaSpace::Linear, nullptr, BE1::Image::Flag::None);
    byte *dstPtr = image->GetPixels();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            dstPtr[0] = 127;
            dstPtr[1] = 127;
            dstPtr[2] = 255;
            dstPtr += 3;
        }
    }

    Create(RHI::TextureType::Texture2D, image, Texture::Flag::NormalMap | Texture::Flag::NoScaleDown | flags);
}

RHI::Texture *Texture::CreateRHITexture(RHI::TextureType textureType, RHI::TextureFlag flags, SharedPtr<const BE1::Image> srcImage, BE1::Image::Format dstFormat, bool generateMipmaps, bool allocateEmptyMipmaps, RHI::GPUResourceState initialState) {
    BE1::Image::Format srcFormat = srcImage->GetFormat();
    bool srcCompressed = BE1::Image::IsCompressed(srcFormat);
    bool srcFormatSupported = RHI::renderer->IsSupportedImageFormat(srcFormat);

    if (!srcImage->IsEmpty()) {
        bool dstFormatSupported = RHI::renderer->IsSupportedImageFormat(dstFormat);

        if (!dstFormatSupported) {
            BE_WARNLOG("Texture::PrepareTextureImage: Unsupported internal image format %s\n", BE1::Image::FormatName(dstFormat));
            return nullptr;
        }

        // 필요하다면 밉맵을 생성한다.
        if (generateMipmaps) {
            int w = srcImage->GetWidth();
            int h = srcImage->GetHeight();
            int d = srcImage->GetDepth();
            int maxMipLevels = BE1::Image::MaxMipLevels(w, h, d);

            // srcImage 가 이미 밉맵을 포함하고 있으면 무시된다.
            if (srcImage->NumMipmaps() < maxMipLevels) {
                if (srcImage->IsPacked() || srcImage->IsCompressed()) {
                    // 밉맵을 생성해야 한다면, 지원되는 가장 비슷한 무압축 포맷으로 컨버팅한다.
                    BE1::Image::Format supportedUncompressedFormat = RHI::renderer->ToUncompressedImageFormat(srcFormat);

                    SharedPtr<BE1::Image> uncompressedImage = MakeShared<BE1::Image>();
                    srcImage->ConvertFormat(supportedUncompressedFormat, *uncompressedImage);
                    srcImage = uncompressedImage;

                    srcFormat = supportedUncompressedFormat;
                    srcFormatSupported = RHI::renderer->IsSupportedImageFormat(srcFormat);
                    srcCompressed = false;
                }

                // CPU 에서 밉맵을 직접 생성한다.
                SharedPtr<BE1::Image> mipmappedImage = MakeShared<BE1::Image>();
                mipmappedImage->Create(w, h, d, srcImage->GetArraySize(), maxMipLevels, srcImage->GetFormat(), srcImage->GetGammaSpace(), nullptr, srcImage->GetFlags());
                mipmappedImage->CopyFrom(*srcImage, 0, 1);
                mipmappedImage->GenerateMipmaps();
                srcImage = mipmappedImage;
            }
        }

        // dstFormat 으로 컨버팅
        if (srcFormat != dstFormat) {
            SharedPtr<BE1::Image> dstImage = MakeShared<BE1::Image>();
            srcImage->ConvertFormat(dstFormat, *dstImage);
            srcImage = dstImage;
        }
    }

    return RHI::renderer->CreateTexture(textureType, flags, srcImage, allocateEmptyMipmaps, {}, 1, initialState);
}

bool Texture::Prepare(RHI::TextureType textureType, SharedPtr<const BE1::Image> srcImage, Flag flags, InternalData &outInternalData) {
    outInternalData.Purge();

    if (!srcImage) {
        return false;
    }

    bool useUAV = BE1::HasFlag(flags, Texture::Flag::UnorderedAccess);
    bool useNormalMap = BE1::HasFlag(flags, Texture::Flag::NormalMap);
    bool useCompression = !BE1::HasFlag(flags, Texture::Flag::NoCompression);
    bool useNPOT = BE1::HasFlag(flags, Texture::Flag::NonPowerOfTwo);
    bool allocateEmptyMipmaps = BE1::HasFlag(flags, Texture::Flag::AllocateEmptyMipmaps);
    bool generateMipmaps = !allocateEmptyMipmaps && !BE1::HasFlag(flags, Texture::Flag::NoMipmaps);

    BE1::Image::Format forceFormat = BE1::Image::Format::Unknown;
    BE1::Image::Format dstFormat;
    if (forceFormat != BE1::Image::Format::Unknown) {
        dstFormat = forceFormat;
    } else {
        RHI::renderer->AdjustTextureFormat(useCompression, useNormalMap, useUAV, srcImage->GetFormat(), &dstFormat);
    }

    uint32_t dstWidth, dstHeight, dstDepth;
    RHI::renderer->AdjustTextureSize(textureType, useNPOT, srcImage->GetWidth(), srcImage->GetHeight(), srcImage->GetDepth(), &dstWidth, &dstHeight, &dstDepth);
    assert(srcImage->GetDepth() == dstDepth);

    if (!srcImage->IsEmpty()) {
        if (srcImage->GetWidth() != dstWidth || srcImage->GetHeight() != dstHeight) {
            SharedPtr<BE1::Image> scaledImage = MakeShared<BE1::Image>();
            srcImage->Resize(dstWidth, dstHeight, BE1::Image::ResampleFilter::Bicubic, *scaledImage);
            srcImage = scaledImage;
        }
    }

    RHI::TextureFlag resourceFlags = RHI::TextureFlag::ShaderResource;
    if (useUAV) {
        resourceFlags |= RHI::TextureFlag::UnorderedAccess;

        // UAV 가 필요하다면, sRGB 포맷은 UAV 로 만들 수 없기 때문에 Typeless 텍스쳐로 만든다.
        if (srcImage->GetGammaSpace() != BE1::Image::GammaSpace::Linear) {
            resourceFlags |= RHI::TextureFlag::Typeless;
        }
    }

    // Depth 포맷은 Typeless 텍스쳐로 생성한다.
    // SRV 에서 Depth 포맷을 사용할 수는 없고, 그와 호환되는 포맷을 사용해야 한다.
    if (srcImage->IsDepthFormat()) {
        resourceFlags |= RHI::TextureFlag::Typeless;
    }

    RHI::GPUResourceState initialState = BE1::HasFlag(flags, Texture::Flag::Compute) ? RHI::GPUResourceState::ShaderResourceCompute : RHI::GPUResourceState::ShaderResource;

    outInternalData.rhiTextureType = textureType;
    outInternalData.format = dstFormat;
    outInternalData.flags = flags;
    outInternalData.rhiTexture = Texture::CreateRHITexture(textureType, resourceFlags, srcImage, dstFormat, generateMipmaps, allocateEmptyMipmaps, initialState);

    return outInternalData.rhiTexture != nullptr;
}

bool Texture::PrepareDirect(RHI::TextureType textureType, SharedPtr<const BE1::Image> srcImage, Flag flags, InternalData &outInternalData) {
    outInternalData.Purge();

    if (!srcImage) {
        return false;
    }

    RHI::TextureFlag resourceFlags = RHI::TextureFlag::ShaderResource;
    if (BE1::HasFlag(flags, Texture::Flag::UnorderedAccess)) {
        resourceFlags |= RHI::TextureFlag::UnorderedAccess;

        // UAV 가 필요하다면, sRGB 포맷은 UAV 로 만들 수 없기 때문에 Typeless 텍스쳐로 만든다.
        if (srcImage->GetGammaSpace() != BE1::Image::GammaSpace::Linear) {
            resourceFlags |= RHI::TextureFlag::Typeless;
        }
    }

    // Depth 포맷은 Typeless 텍스쳐로 생성한다.
    // SRV 에서 Depth 포맷을 사용할 수는 없고, 그와 호환되는 포맷을 사용해야 한다.
    if (srcImage->IsDepthFormat()) {
        resourceFlags |= RHI::TextureFlag::Typeless;
    }

    RHI::GPUResourceState initialState = BE1::HasFlag(flags, Texture::Flag::Compute) ? RHI::GPUResourceState::ShaderResourceCompute : RHI::GPUResourceState::ShaderResource;

    RHI::Texture *texture = RHI::renderer->CreateTexture(textureType, resourceFlags, srcImage, false, {}, 1, initialState);
    if (!texture) {
        return false;
    }

    outInternalData.rhiTextureType = textureType;
    outInternalData.format = srcImage->GetFormat();
    outInternalData.flags = flags;
    outInternalData.rhiTexture = texture;

    return outInternalData.rhiTexture != nullptr;
}

void Texture::ApplyPrepared(InternalData &newInternalData) {
    assert(state == State::Empty || state == State::Loading || state == State::Ready || state == State::Reloading);
    assert(internalData.rhiTexture != newInternalData.rhiTexture);

    Purge();

    internalData.Swap(newInternalData);
}

void Texture::GenerateMipmaps() {
    assert(state == State::Ready);

    RHI::renderer->GenerateMipmaps(internalData.rhiTexture);
}

bool Texture::Load(const char *filename, Texture::Flag flags) {
    BE_LOG("Loading texture '%s'...\n", filename);

    SharedPtr<BE1::Image> image = MakeShared<BE1::Image>();
    image->Load(filename);

    if (image->IsEmpty()) {
        BE_WARNLOG("Couldn't load texture \"%s\"\n", filename);
        return false;
    }

    RHI::TextureType textureType;

    if (image->GetDepth() > 1) {
        textureType = RHI::TextureType::Texture3D;
    } else if (image->IsCubeMap()) {
        textureType = RHI::TextureType::TextureCube;
    } else {
        textureType = RHI::TextureType::Texture2D;
    }

    return Create(textureType, image, flags | Texture::Flag::LoadedFromFile);
}

bool Texture::LoadAsync(const char *filename, Flag flags) {
    state = State::Loading;
    uint32_t newSerial = BumpLoadSerial();

    if (!loadingSystem.EnqueueTextureLoad(shared_from_this(), filename, flags, false, newSerial)) {
        state = State::Failed;
        return false;
    }
    return true;
}

bool Texture::Reload() {
    if (!BE1::HasFlag(internalData.flags, Flag::LoadedFromFile)) {
        BE_WARNLOG("Texture::Reload: cannot reload texture '%s' because it was not loaded from a file\n", GetHashName());
        return false;
    }

    if (state != State::Ready) {
        BE_WARNLOG("Texture::Reload: texture '%s' is not ready\n", GetHashName());
        return false;
    }

    // 스트리밍 텍스쳐인 경우, 동기 로딩으로 리로드하는 것은 지원하지 않는다.
    if (textureManager.IsRegisteredStreamingTexture(this)) {
        BE_WARNLOG("Texture::Reload: cannot reload streaming texture '%s'\n", GetHashName());
        return false;
    }

    return Load(hashName, internalData.flags);
}

bool Texture::ReloadAsync() {
    if (!BE1::HasFlag(internalData.flags, Flag::LoadedFromFile)) {
        BE_WARNLOG("Texture::Reload: cannot reload texture '%s' because it was not loaded from a file\n", GetHashName());
        return false;
    }

    if (!IsReady()) {
        BE_WARNLOG("Texture::ReloadAsync: texture '%s' is not ready\n", GetHashName());
        return false;
    }

    uint32_t newSerial = BumpLoadSerial();
    state = State::Reloading;

    if (!loadingSystem.EnqueueTextureLoad(shared_from_this(), hashName, internalData.flags, true, newSerial)) {
        state = State::Ready;
        return false;
    }

    // 스트리밍 텍스쳐인 경우, 스트리밍 시스템에서 제거한다.
    textureManager.UnregisterStreamingTexture(this);

    return true;
}

bool Texture::WaitReady() {
    assert(BE1::Engine::IsInMainThread());

    if (GetState() == State::Loading) {
        loadingSystem.BoostTextureLoad(this, GetLoadSerial(), LoadingSystem::LoadPriority::Critical);
    }

    while (GetState() == State::Loading) {
        loadingSystem.ProcessLoadCompletions();

        BE1::PlatformThread::Yield();
    }

    return IsReady();
}

bool Texture::WaitLoadComplete() {
    assert(BE1::Engine::IsInMainThread());

    if (IsLoading()) {
        loadingSystem.BoostTextureLoad(this, GetLoadSerial(), LoadingSystem::LoadPriority::Critical);
    }

    while (IsLoading()) {
        loadingSystem.ProcessLoadCompletions();

        BE1::PlatformThread::Yield();
    }

    return GetState() == State::Ready;
}

void Texture::RequestStreamingMaxDimension(uint32_t requestedMaxDimension) {
    streaming.requestedMaxDimension = BE1::Max(streaming.requestedMaxDimension, requestedMaxDimension);
}

bool Texture::Update2D(int mipLevel, int xoffset, int yoffset, int width, int height, BE1::Image::Format format, const byte *data) {
    if (state != State::Ready) {
        BE_WARNLOG("Texture::Update2D: texture '%s' is not ready\n", GetHashName());
        return false;
    }
    return RHI::renderer->SetTextureSubImage2D(internalData.rhiTexture, mipLevel, xoffset, yoffset, width, height, format, data);
}

bool Texture::Update3D(int mipLevel, int xoffset, int yoffset, int zoffset, int width, int height, int depth, BE1::Image::Format format, const byte *data) {
    if (state != State::Ready) {
        BE_WARNLOG("Texture::Update3D: texture '%s' is not ready\n", GetHashName());
        return false;
    }
    return RHI::renderer->SetTextureSubImage3D(internalData.rhiTexture, mipLevel, xoffset, yoffset, zoffset, width, height, depth, format, data);
}

bool Texture::UpdateCubeFace(RHI::CubemapFace face, int mipLevel, int xoffset, int yoffset, int width, int height, BE1::Image::Format format, const byte *data) {
    if (state != State::Ready) {
        BE_WARNLOG("Texture::UpdateCubeFace: texture '%s' is not ready\n", GetHashName());
        return false;
    }
    return RHI::renderer->SetTextureSubImageCubeFace(internalData.rhiTexture, face, mipLevel, xoffset, yoffset, width, height, format, data);
}

bool Texture::GetTexels2D(int mipLevel, BE1::Image::Format format, void *pixels) const {
    if (!IsReady()) {
        BE_WARNLOG("Texture::GetTexels2D: texture '%s' is not ready\n", GetHashName());
        return false;
    }
    return RHI::renderer->GetTextureImage2D(internalData.rhiTexture, mipLevel, format, pixels);
}

bool Texture::GetTexels3D(int mipLevel, BE1::Image::Format format, void *pixels) const {
    if (!IsReady()) {
        BE_WARNLOG("Texture::GetTexels3D: texture '%s' is not ready\n", GetHashName());
        return false;
    }
    return RHI::renderer->GetTextureImage3D(internalData.rhiTexture, mipLevel, format, pixels);
}

bool Texture::GetTexelsCubeFace(RHI::CubemapFace face, int mipLevel, BE1::Image::Format format, void *pixels) const {
    if (!IsReady()) {
        BE_WARNLOG("Texture::GetTexelsCubeFace: texture '%s' is not ready\n", GetHashName());
        return false;
    }
    return RHI::renderer->GetTextureImageCubeFace(internalData.rhiTexture, face, mipLevel, format, pixels);
}

bool Texture::GetCubeImageFromCubeTexture(const Texture *cubeTexture, int numMipLevels, BE1::Image &outCubeImage) {
    assert(cubeTexture && cubeTexture->IsReady());

    BE1::Image::GammaSpace gammaSpace = BE1::Image::NeedFloatConversion(cubeTexture->internalData.format) ? BE1::Image::GammaSpace::Linear : BE1::Image::GammaSpace::sRGB;
    BE1::Image faceImages[6];

    for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
        faceImages[faceIndex].Create2D(cubeTexture->GetWidth(), cubeTexture->GetWidth(), numMipLevels, cubeTexture->GetFormat(), gammaSpace, nullptr, BE1::Image::Flag::None);

        for (int mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
            if (!cubeTexture->GetTexelsCubeFace((RHI::CubemapFace)faceIndex, mipLevel, cubeTexture->GetFormat(), faceImages[faceIndex].GetPixels(mipLevel))) {
                return false;
            }
        }
    }

    outCubeImage.CreateCubeFrom6Faces(faceImages);
    return true;
}
