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
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Core/Cmds.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

static const struct {
    const char *name;
    RHI::TextureFilter::Enum filter;
} textureFilterNames[] = {
    { "Nearest", RHI::TextureFilter::Nearest },
    { "Linear", RHI::TextureFilter::Linear },
    { "NearestMipmapNearest", RHI::TextureFilter::NearestMipmapNearest },
    { "LinearMipmapNearest", RHI::TextureFilter::LinearMipmapNearest },
    { "NearestMipmapLinear", RHI::TextureFilter::NearestMipmapLinear },
    { "LinearMipmapLinear", RHI::TextureFilter::LinearMipmapLinear },
};

TextureManager textureManager;

CVar TextureManager::texture_filter("texture_filter", "LinearMipmapLinear", CVar::Flag::Archive, "changes texture filtering on mipmapped texture");
CVar TextureManager::texture_anisotropy("texture_anisotropy", "8", CVar::Flag::Archive | CVar::Flag::Integer, "set the maximum texture anisotropy if available");
CVar TextureManager::texture_lodBias("texture_lodBias", "0", CVar::Flag::Archive | CVar::Flag::Integer, "change lod bias on mipmapped images");
CVar TextureManager::texture_useCompression("texture_useCompression", "1", CVar::Flag::Archive | CVar::Flag::Bool, "");
CVar TextureManager::texture_useNormalCompression("texture_useNormalCompression", "1", CVar::Flag::Bool | CVar::Flag::Archive, "normal map compression");
CVar TextureManager::texture_mipLevel("texture_mipLevel", "0", CVar::Flag::Archive | CVar::Flag::Integer, "");

void TextureManager::Init() {
    cmdSystem.AddCommand("listTextures", Cmd_ListTextures);
    cmdSystem.AddCommand("reloadTexture", Cmd_ReloadTexture);
    cmdSystem.AddCommand("dumpTexture", Cmd_DumpTexture);
    cmdSystem.AddCommand("convertNormalAR2RGB", Cmd_ConvertNormalAR2RGB);

    textureHashMap.Init(1024, 1024, 1024);

    // Set texture filtering mode
    SetFilter(texture_filter.GetString());//"LinearMipmapNearest");

    // Set texture anisotropy mode
    SetAnisotropy(texture_anisotropy.GetFloat());

    // Create pre-defined textures
    CreateEngineTextures();
}

void TextureManager::Shutdown() {
    cmdSystem.RemoveCommand("listTextures");
    cmdSystem.RemoveCommand("reloadTexture");
    cmdSystem.RemoveCommand("dumpTexture");
    cmdSystem.RemoveCommand("convertNormalAR2RGB");

    textureHashMap.DeleteContents(true);
}

void TextureManager::CreateEngineTextures() {
    byte *  data;
    Image   image;

    // Create default texture
    defaultTexture = AllocTexture("_defaultTexture");
    defaultTexture->CreateDefaultTexture(16, Texture::Flag::Permanence);

    // Create zeroClamp texture
    zeroClampTexture = AllocTexture("_zeroClampTexture");
    zeroClampTexture->CreateZeroClampTexture(16, Texture::Flag::Permanence);

    // Create defaultCube texture
    defaultCubeMapTexture = AllocTexture("_defaultCubeTexture");
    defaultCubeMapTexture->CreateDefaultCubeMapTexture(16, Texture::Flag::Permanence);

    // Create blackCube texture
    blackCubeMapTexture = AllocTexture("_blackCubeTexture");
    blackCubeMapTexture->CreateBlackCubeMapTexture(8, Texture::Flag::Permanence);

    // Create white texture
    image.Create2D(8, 8, 1, Image::Format::L_8, nullptr, 0);
    data = image.GetPixels();
    memset(data, 0xFF, 8 * 8);
    whiteTexture = AllocTexture("_whiteTexture");
    whiteTexture->Create(RHI::TextureType::Texture2D, image, Texture::Flag::Permanence | Texture::Flag::NoScaleDown);

    // Create black texture
    image.Create2D(8, 8, 1, Image::Format::L_8, nullptr, 0);
    data = image.GetPixels();
    memset(data, 0, 8 * 8);
    blackTexture = AllocTexture("_blackTexture");
    blackTexture->Create(RHI::TextureType::Texture2D, image, Texture::Flag::Permanence | Texture::Flag::NoScaleDown);

    // Create grey texture
    image.Create2D(8, 8, 1, Image::Format::L_8, nullptr, 0);
    data = image.GetPixels();
    memset(data, 0x80, 8 * 8);
    greyTexture = AllocTexture("_greyTexture");
    greyTexture->Create(RHI::TextureType::Texture2D, image, Texture::Flag::Permanence | Texture::Flag::NoScaleDown);

    // Create flatNormal texture
    flatNormalTexture = AllocTexture("_flatNormalTexture");
    flatNormalTexture->CreateFlatNormalTexture(16, Texture::Flag::Permanence);

    // Create normalCube texture
    /*normalCubeMapTexture = AllocTexture("_normalCubeTexture");
    normalCubeMapTexture->CreateNormalizationCubeMapTexture(32, Texture::Flag::Permanence);*/

    // Create _cubicNormalCube texture
    cubicNormalCubeMapTexture = AllocTexture("_cubicNormalCubeTexture");
    cubicNormalCubeMapTexture->CreateCubicNormalCubeMapTexture(1, Texture::Flag::Permanence);

    // Create fog texture
    fogTexture = AllocTexture("_fogTexture");
    fogTexture->CreateFogTexture(Texture::Flag::Permanence);

    // Create fogEnter texture
    fogEnterTexture = AllocTexture("_fogEnterTexture");
    fogEnterTexture->CreateFogEnterTexture(Texture::Flag::Permanence);

    // Create randomRotMat texture
    randomRotMatTexture = AllocTexture("_randomRotMatTexture");
    randomRotMatTexture->CreateRandomRotMatTexture(64, Texture::Flag::Permanence);

    // Create randomRot4x4 texture
    randomDir4x4Texture = AllocTexture("_randomDir4x4Texture");
    randomDir4x4Texture->CreateRandomDir4x4Texture(Texture::Flag::Permanence);
}

void TextureManager::DestroyUnusedTextures() {
    Array<Texture *> removeArray;

    for (int i = 0; i < textureHashMap.Count(); i++) {
        const auto *entry = textureHashMap.GetByIndex(i);
        Texture *texture = entry->second;

        if (texture && !texture->permanence && texture->refCount == 0) {
            removeArray.Append(texture);
        }
    }

    for (int i = 0; i < removeArray.Count(); i++) {
        DestroyTexture(removeArray[i]);
    }
}

void TextureManager::PrecacheTexture(const char *filename) {
    Texture *texture = GetTexture(filename);
    ReleaseTexture(texture);
}

void TextureManager::SetFilter(const char *filterName) {
    int mode;
    for (mode = 0; mode < 6; mode++) {
        if (!Str::Cmp(textureFilterNames[mode].name, filterName))
            break;
    }

    if (mode == 6) {
        BE_LOG("bad filter name\n");
        return;
    }

    textureFilter = textureFilterNames[mode].filter;
    
    for (int i = 0; i < textureHashMap.Count(); i++) {
        const auto *entry = textureHashMap.GetByIndex(i);
        Texture *texture = entry->second;

        if (texture->hasMipmaps && !(texture->flags & Texture::Flag::Nearest)) {
            rhi.BindTexture(texture->textureHandle);
            rhi.SetTextureFilter(textureFilter);
        }
    }
}

void TextureManager::SetAnisotropy(float degree) {
    textureAnisotropy = degree;

    for (int i = 0; i < textureHashMap.Count(); i++) {
        const auto *entry = textureHashMap.GetByIndex(i);
        Texture *texture = entry->second;
        
        if (texture->hasMipmaps && !(texture->flags & Texture::Flag::Nearest)) {
            rhi.BindTexture(texture->textureHandle);
            rhi.SetTextureAnisotropy(degree);
        }
    }
}

void TextureManager::SetLodBias(float lodBias) const {
    /*for (int i = 0; i < rhi.hwLimits.maxTextureImageUnits; i++) {
        rhi.SelectTextureUnit(i);
        rhi.SetTextureLODBias(lodBias);
    }

    SelectTextureUnit(0);*/
}

Texture *TextureManager::AllocTexture(const char *hashName) {
    if (textureHashMap.Get(hashName)) {
        BE_FATALERROR("%s texture already allocated", hashName);
    }
    
    Texture *texture = new Texture;
    texture->hashName = hashName;
    texture->name = hashName;
    texture->name.StripPath();
    texture->name.StripFileExtension();
    texture->refCount = 1;

    textureHashMap.Set(texture->hashName, texture);

    return texture;
}

void TextureManager::DestroyTexture(Texture *texture) {
    if (texture->refCount > 1) {
        BE_LOG("TextureManager::DestroyTexture: texture '%s' has %i reference count\n", texture->hashName.c_str(), texture->refCount);
    }

    textureHashMap.Remove(texture->hashName);

    delete texture;
}

Texture *TextureManager::FindTexture(const char *hashName) const {
    const auto *entry = textureHashMap.Get(Str(hashName));
    if (entry) {
        return entry->second;
    }
    
    return nullptr;
}

Texture *TextureManager::GetTextureWithoutTextureInfo(const char *hashName, int creationFlags) {
    if (!hashName || !hashName[0]) {
        return defaultTexture;
    }

    Texture *texture = FindTexture(hashName);
    if (texture) {
        texture->refCount++;
        return texture;
    }

    texture = AllocTexture(hashName);
    if (!texture->Load(hashName, creationFlags)) {
        DestroyTexture(texture);
        return defaultTexture;
    }

    return texture;
}

Texture *TextureManager::GetTexture(const char *hashName) {
    if (!hashName || !hashName[0]) {
        return defaultTexture;
    }

    Texture *texture = FindTexture(hashName);
    if (texture) {
        texture->refCount++;
        return texture;
    }

    const Str textureInfoPath = Str(hashName) + ".texture";
    int flags = LoadTextureInfo(textureInfoPath);

    texture = AllocTexture(hashName);
    if (!texture->Load(hashName, flags)) {
        DestroyTexture(texture);
        return defaultTexture;
    }

    return texture;
}

int TextureManager::LoadTextureInfo(const char *filename) const {
    int flags;

    flags = Texture::Flag::Clamp | Texture::Flag::SRGBColorSpace;

    if (!filename || !filename[0]) {
        return flags;
    }

    char *text = nullptr;
    size_t size = fileSystem.LoadFile(filename, true, (void **)&text);
    if (!text) {
        return flags;
    }

    Json::Value node;
    Json::Reader jsonReader;
    if (!jsonReader.parse(text, node)) {
        BE_WARNLOG("Failed to parse JSON text\n");
        return flags;
    }

    flags = 0;

    int version = node["version"].asInt();
    if (version >= 4) {
        const Json::Value textureTypeValue = node.get("textureType", "2D");
        const char *textureTypeString = textureTypeValue.asCString();
        if (!Str::Icmp(textureTypeString, "Sprite")) {
            flags |= Texture::Flag::HighQuality | Texture::Flag::NonPowerOfTwo;
        }

        const Json::Value normalMapValue = node.get("normalMap", "false");
        const char *normalMapString = normalMapValue.asCString();
        if (!Str::Icmp(normalMapString, "true")) {
            flags |= Texture::Flag::NormalMap;
        }

        const Json::Value wrapModeValue = node.get("wrapMode", "Clamp");
        const char *wrapModeString = wrapModeValue.asCString();
        if (!Str::Icmp(wrapModeString, "Clamp")) {
            flags |= Texture::Flag::Clamp;
        } else if (!Str::Icmp(wrapModeString, "Repeat")) {
            flags |= Texture::Flag::Repeat;
        }

        const Json::Value filterModeValue = node.get("filterMode", "Bilinear");
        const char *filterModeString = filterModeValue.asCString();
        if (!Str::Icmp(filterModeString, "Point")) {
            flags |= Texture::Flag::Nearest | Texture::Flag::NoMipmaps;
        } else if (!Str::Icmp(filterModeString, "Bilinear")) {
        } else if (!Str::Icmp(filterModeString, "Trilinear")) {
        }

        const Json::Value sRGBValue = node.get("colorSpace", "sRGB");
        const char *sRGBString = sRGBValue.asCString();
        if (!Str::Icmp(sRGBString, "sRGB")) {
            flags |= Texture::Flag::SRGBColorSpace;
        }

        const Json::Value useMipmapsValue = node.get("useMipmaps", "true");
        const char *useMipmapsString = useMipmapsValue.asCString();
        if (!Str::Icmp(useMipmapsString, "false")) {
            flags |= Texture::Flag::NoMipmaps;
        }

        const Json::Value compressionLevelValue = node.get("compressionLevel", "Normal");
        const char *compressionLevelString = compressionLevelValue.asCString();
        if (!Str::Icmp(compressionLevelString, "None")) {
            flags |= Texture::Flag::NoCompression;
        }
    }

    if (flags & Texture::Flag::NormalMap) {
        flags &= ~Texture::Flag::SRGBColorSpace;
    }

    return flags;
}

Texture *TextureManager::TextureFromGenerator(const char *hashName, const TextureGeneratorBase &generator) {
    Texture *texture;

    texture = FindTexture(hashName);
    if (texture) {
        BE_LOG("TextureManager::TextureFromGenerator: same name already exist\n");
        texture->refCount++;
        return texture;
    }

    texture = AllocTexture(hashName);
    generator.Generate(texture);

    return texture;
}

void TextureManager::RenameTexture(Texture *texture, const Str &newName) {
    const auto *entry = textureHashMap.Get(texture->hashName);
    if (entry) {
        textureHashMap.Remove(texture->hashName);

        texture->hashName = newName;
        texture->name = newName;
        texture->name.StripPath();
        texture->name.StripFileExtension();

        textureHashMap.Set(newName, texture);
    }
}

void TextureManager::ReleaseTexture(Texture *texture, bool immediateDestroy) {
    if (texture->permanence) {
        return;
    }

    if (texture->refCount > 0) {
        texture->refCount--;
    }

    if (immediateDestroy && texture->refCount == 0) {
        DestroyTexture(texture);
    }
}

//--------------------------------------------------------------------------------------------------

void TextureManager::Cmd_ListTextures(const CmdArgs &args) {
    const char *type;
    const char *addr;

    int count = 0;
    int totalBytes = 0;
    
    BE_LOG("NUM. REF. TEX. .W.. .H.. .D.. BYTES..... FORMAT.............. MM ADDR NAME\n");

    for (int i = 0; i < textureManager.textureHashMap.Count(); i++) {
        const auto *entry = textureManager.textureHashMap.GetByIndex(i);
        Texture *texture = entry->second;

        switch (texture->type) {
        case RHI::TextureType::Texture2D:           type = "2D  "; break;
        case RHI::TextureType::Texture3D:           type = "3D  "; break;
        case RHI::TextureType::TextureCubeMap:      type = "Cube"; break;
        case RHI::TextureType::TextureRectangle:    type = "Rect"; break;
        case RHI::TextureType::Texture2DArray:      type = "2DAr"; break;
        case RHI::TextureType::TextureBuffer:       type = "Buff"; break;
        }

        const char *internalFormatName = Image::FormatName(texture->format);
        
        switch (texture->addressMode) {
        case RHI::AddressMode::Repeat:              addr = "R   "; break;
        case RHI::AddressMode::Clamp:               addr = "C   "; break;
        case RHI::AddressMode::ClampToBorder:       addr = "CB  "; break;
        case RHI::AddressMode::MirroredRepeat:      addr = "MR  "; break;
        }

        int numMipmaps = texture->hasMipmaps ? Image::MaxMipMapLevels(texture->width, texture->height, texture->depth) : 1;
        int bytes = Image::MemRequired(texture->width, texture->height, texture->depth, numMipmaps, texture->format) * texture->numSlices;
        
        BE_LOG("%4d %4d %s %4d %4d %4d %10s %-20s %s %s %s\n",
            i,
            texture->refCount,
            type,
            texture->width,
            texture->height,
            texture->depth,
            Str::FormatBytes(bytes).c_str(),
            internalFormatName,
            texture->hasMipmaps ? "mm" : "--",
            addr, 
            texture->hashName.c_str());

        totalBytes += bytes;
        count++;
    }

    BE_LOG("total %s (including mipmaps)\n", Str::FormatBytes(totalBytes).c_str());
    BE_LOG("total %i textures\n", count);
}

void TextureManager::Cmd_ReloadTexture(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG("reloadTexture <filename>\n");
        return;
    }

    if (!Str::Icmp(args.Argv(1), "all")) {
        int count = textureManager.textureHashMap.Count();

        for (int i = 0; i < count; i++) {
            const auto entry = textureManager.textureHashMap.GetByIndex(i);
            Texture *texture = entry->second;
            texture->Reload();
        }
    } else {
        Texture *texture = textureManager.FindTexture(args.Argv(1));
        if (!texture) {
            BE_WARNLOG("Couldn't find texture to reload \"%s\"\n", args.Argv(1));
            return;
        }

        texture->Reload();
    }
}

void TextureManager::Cmd_DumpTexture(const CmdArgs &args) {
    if (args.Argc() < 2) {
        BE_LOG("dumpTexture <index>\n");
        return;
    }

    int index = atoi(args.Argv(1));

    if (index < 0 || index >= textureManager.textureHashMap.Count()) {
        BE_WARNLOG("Invalid index\n");
        return;
    }

    const auto *entry = textureManager.textureHashMap.GetByIndex(index);
    Texture *texture = entry->second;

    if (texture->type != RHI::TextureType::Texture2D &&
        texture->type != RHI::TextureType::Texture3D &&
        texture->type != RHI::TextureType::TextureCubeMap &&
        texture->type != RHI::TextureType::TextureRectangle) {
        BE_WARNLOG("Not supported type\n");
        return;
    }

    Image bitmapImage;
    bitmapImage.Create(texture->GetWidth(), texture->GetHeight(), texture->GetDepth(), texture->NumSlices(), 1, texture->GetFormat(), nullptr, 0);

    texture->Bind();

    switch (texture->type) {
    case RHI::TextureType::Texture2D:
        texture->GetTexels2D(0, texture->GetFormat(), bitmapImage.GetPixels(0));
        break;
    case RHI::TextureType::Texture3D:
        texture->GetTexels3D(0, texture->GetFormat(), bitmapImage.GetPixels(0));
        break;
    case RHI::TextureType::TextureCubeMap:
        for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
            texture->GetTexelsCubemap(faceIndex, 0, texture->GetFormat(), bitmapImage.GetPixels(0, faceIndex));
        }
        break;
    case RHI::TextureType::TextureRectangle:
        texture->GetTexelsRect(texture->GetFormat(), bitmapImage.GetPixels(0));
        break;
    }

    Str filename = "DumpTextures";
    filename.AppendPath(texture->GetName());

    if (texture->type == RHI::TextureType::Texture2D || texture->type == RHI::TextureType::TextureRectangle) {
        filename.Append(".png");
        bitmapImage.WritePNG(filename);
    } else {
        filename.Append(".dds");
        bitmapImage.WriteDDS(filename);
    }

    BE_LOG("Dumped texture to %s\n", filename.c_str());
}

void TextureManager::Cmd_ConvertNormalAR2RGB(const CmdArgs &args) {
    char path[MaxAbsolutePath];
    
    if (args.Argc() != 3) {
        BE_LOG("convertNormalAR2RGB <rootdir> <filter>\n");
        return;
    }
    
    FileArray fileArray;
    int numFiles = fileSystem.ListFiles(args.Argv(1), args.Argv(2), fileArray);
    if (!numFiles) {
        BE_WARNLOG("Files not founded\n");
        return;
    }

    Image image1;
    Image image2;

    for (int i = 0; i < numFiles; i++) {
        Str::snPrintf(path, sizeof(path), "%s/%s", args.Argv(1), fileArray.GetFileName(i));
        image1.Load(path);
        if (image1.IsEmpty())
            continue;

        BE_LOG("Converting '%s'\n", path);

        if (image1.GetFormat() != Image::Format::RGBA_8_8_8_8) {
            image1.ConvertFormatSelf(Image::Format::RGBA_8_8_8_8);
        }

        image2.Create2D(image1.GetWidth(), image1.GetHeight(), 1, Image::Format::RGB_8_8_8, nullptr, Image::Flag::LinearSpace);
        byte *data2Ptr = image2.GetPixels();
        byte *data1Ptr = image1.GetPixels();
        byte *endData1 = data1Ptr + image1.GetWidth() * image1.GetHeight() * 4;
        for (; data1Ptr < endData1; data1Ptr += 4, data2Ptr += 3) {
            data2Ptr[0] = data1Ptr[3];
            data2Ptr[1] = data1Ptr[0];
            float x = ((float)data1Ptr[3] / 255.0f) * 2.0f - 1.0f;
            float y = ((float)data1Ptr[0] / 255.0f) * 2.0f - 1.0f;
            data2Ptr[2] = (byte)(255 * (Math::Sqrt(Math::Fabs(1.0 - x * x - y * y)) * 0.5f + 0.5f));
        }

        image2.FlipY();
        image2.Write(path);
    }

    BE_LOG("all done\n");
}

BE_NAMESPACE_END
