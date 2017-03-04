#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Core/Cmds.h"
#include "File/FileSystem.h"

BE_NAMESPACE_BEGIN

static const struct {
    const char *                name;
    Renderer::TextureFilter     filter;
} textureFilterNames[] = {
    { "Nearest",                Renderer::Nearest }, 
    { "Linear",                 Renderer::Linear }, 
    { "NearestMipmapNearest",   Renderer::NearestMipmapNearest }, 
    { "LinearMipmapNearest",    Renderer::LinearMipmapNearest }, 
    { "NearestMipmapLinear",    Renderer::NearestMipmapLinear }, 
    { "LinearMipmapLinear",     Renderer::LinearMipmapLinear }, 
};

TextureManager textureManager;

CVar TextureManager::texture_filter(L"texture_filter", L"LinearMipmapLinear", CVar::Archive, L"changes texture filtering on mipmapped texture");
CVar TextureManager::texture_anisotropy(L"texture_anisotropy", L"8", CVar::Archive | CVar::Integer, L"set the maximum texture anisotropy if available");
CVar TextureManager::texture_lodBias(L"texture_lodBias", L"0", CVar::Archive | CVar::Integer, L"change lod bias on mipmapped images");
CVar TextureManager::texture_sRGB(L"texture_sRGB", L"1", CVar::Archive | CVar::Bool, L"");
CVar TextureManager::texture_useCompression(L"texture_useCompression", L"1", CVar::Archive | CVar::Bool, L"");
CVar TextureManager::texture_useNormalCompression(L"texture_useNormalCompression", L"1", CVar::Bool | CVar::Archive, L"normal map compression");
CVar TextureManager::texture_mipLevel(L"texture_mipLevel", L"0", CVar::Archive | CVar::Integer, L"");

void TextureManager::Init() {
    byte *  data;
    Image   image;

    cmdSystem.AddCommand(L"listTextures", Cmd_ListTextures);
    cmdSystem.AddCommand(L"reloadTexture", Cmd_ReloadTexture);
    cmdSystem.AddCommand(L"convertNormalAR2RGB", Cmd_ConvertNormalAR2RGB);

    textureHashMap.Init(1024, 1024, 1024);

    // bilinear filtering
    SetFilter(WStr::ToStr(texture_filter.GetString()));//"LinearMipmapNearest");

    // texture anisotropy
    SetAnisotropy(texture_anisotropy.GetFloat());

    // _default
    defaultTexture = AllocTexture("_defaultTexture");
    defaultTexture->CreateDefaultTexture(16, Texture::Permanence);

    // _zeroClamp
    zeroClampTexture = AllocTexture("_zeroClampTexture");
    zeroClampTexture->CreateZeroClampTexture(16, Texture::Permanence);

    // _defaultCube
    defaultCubeMapTexture = AllocTexture("_defaultCubeTexture");
    defaultCubeMapTexture->CreateDefaultCubeMapTexture(16, Texture::Permanence);

    // _blackCube
    blackCubeMapTexture = AllocTexture("_blackCubeTexture");
    blackCubeMapTexture->CreateBlackCubeMapTexture(8, Texture::Permanence);

    // _white
    image.Create2D(8, 8, 1, Image::L_8, nullptr, Image::SRGBFlag);
    data = image.GetPixels();
    memset(data, 0xFF, 8 * 8);
    whiteTexture = AllocTexture("_whiteTexture");
    whiteTexture->Create(Renderer::Texture2D, image, Texture::Permanence | Texture::NoScaleDown);

    // _black
    image.Create2D(8, 8, 1, Image::L_8, nullptr, Image::SRGBFlag);
    data = image.GetPixels();
    memset(data, 0, 8 * 8);
    blackTexture = AllocTexture("_blackTexture");
    blackTexture->Create(Renderer::Texture2D, image, Texture::Permanence | Texture::NoScaleDown);

    // _grey
    image.Create2D(8, 8, 1, Image::L_8, nullptr, Image::SRGBFlag);
    data = image.GetPixels();
    memset(data, 0x80, 8 * 8);
    greyTexture = AllocTexture("_greyTexture");
    greyTexture->Create(Renderer::Texture2D, image, Texture::Permanence | Texture::NoScaleDown);

    // _linear
    image.Create2D(256, 1, 1, Image::L_8, nullptr, Image::LinearFlag);
    data = image.GetPixels();
    for (int i = 0; i < 256; i++) {
        data[i] = i;
    }
    linearTexture = AllocTexture("_linearTexture");
    linearTexture->Create(Renderer::Texture2D, image, Texture::Permanence | Texture::NoMipmaps | Texture::Clamp | Texture::HighQuality);

    // _exponent
    exponentTexture = AllocTexture("_exponentTexture");
    exponentTexture->CreateExponentTexture(Texture::Permanence);

    // _flatNormal
    flatNormalTexture = AllocTexture("_flatNormalTexture");
    flatNormalTexture->CreateFlatNormalTexture(16, Texture::Permanence);

    // _normalCube
    normalCubeMapTexture = AllocTexture("_normalCubeTexture");
    normalCubeMapTexture->CreateNormalizationCubeMapTexture(32, Texture::Permanence);

    // _cubicNormalCube
    cubicNormalCubeMapTexture = AllocTexture("_cubicNormalCubeTexture");
    cubicNormalCubeMapTexture->CreateCubicNormalCubeMapTexture(1, Texture::Permanence);

    // _fog
    fogTexture = AllocTexture("_fogTexture");
    fogTexture->CreateFogTexture(Texture::Permanence);

    // _fogEnter
    fogEnterTexture = AllocTexture("_fogEnterTexture");
    fogEnterTexture->CreateFogEnterTexture(Texture::Permanence);

    // _randomRotMat
    randomRotMatTexture = AllocTexture("_randomRotMatTexture");
    randomRotMatTexture->CreateRandomRotMatTexture(64, Texture::Permanence);

    // _randomRot4x4
    randomDir4x4Texture = AllocTexture("_randomDir4x4Texture");
    randomDir4x4Texture->CreateRandomDir4x4Texture(Texture::Permanence);
}

void TextureManager::Shutdown() {
    cmdSystem.RemoveCommand(L"listTextures");
    cmdSystem.RemoveCommand(L"reloadTexture");
    cmdSystem.RemoveCommand(L"convertNormalAR2RGB");

    textureHashMap.DeleteContents(true);
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

void TextureManager::SetFilter(const char *filterName) {
    int mode;
    for (mode = 0; mode < 6; mode++) {
        if (!Str::Cmp(textureFilterNames[mode].name, filterName))
            break;
    }

    if (mode == 6) {
        BE_LOG(L"bad filter name\n");
        return;
    }

    textureFilter = textureFilterNames[mode].filter;
    
    for (int i = 0; i < textureHashMap.Count(); i++) {
        const auto *entry = textureHashMap.GetByIndex(i);
        Texture *texture = entry->second;

        if (texture->hasMipmaps && !(texture->flags & Texture::Nearest)) {
            glr.BindTexture(texture->textureHandle);
            glr.SetTextureFilter(textureFilter);
        }
    }
}

void TextureManager::SetAnisotropy(float degree) {
    textureAnisotropy = degree;

    for (int i = 0; i < textureHashMap.Count(); i++) {
        const auto *entry = textureHashMap.GetByIndex(i);
        Texture *texture = entry->second;
        
        if (texture->hasMipmaps && !(texture->flags & Texture::Nearest)) {
            glr.BindTexture(texture->textureHandle);
            glr.SetTextureAnisotropy(degree);
        }
    }
}

void TextureManager::SetLodBias(float lodBias) const {
    /*for (int i = 0; i < glr.hwLimits.maxTextureImageUnits; i++) {
        glr.SelectTextureUnit(i);
        glr.SetTextureLODBias(lodBias);
    }

    SelectTextureUnit(0);*/
}

Texture *TextureManager::AllocTexture(const char *hashName) {
    if (textureHashMap.Get(hashName)) {
        BE_FATALERROR(L"%hs texture already allocated", hashName);
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
        BE_LOG(L"TextureManager::DestroyTexture: texture '%hs' has %i reference count\n", texture->hashName.c_str(), texture->refCount);
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

Texture *TextureManager::GetTexture(const char *hashName, int creationFlags) {
    if (!hashName || !hashName[0]) {
        return defaultTexture;
    }

    Texture *texture = FindTexture(hashName);
    if (texture) {
        texture->refCount++;
        return texture;
    }

    if (creationFlags == 0) {
        const Str textureInfoPath = Str(hashName) + ".texinfo";
        creationFlags = LoadTextureInfo(textureInfoPath);
    }

    texture = AllocTexture(hashName);
    if (!texture->Load(hashName, creationFlags)) {
        DestroyTexture(texture);
        return defaultTexture;
    }

    return texture;
}

int TextureManager::LoadTextureInfo(const char *filename) const {
    int flags = 0;

    if (!filename || !filename[0]) {
        return flags;
    }

    int32_t *data;
    size_t size = fileSystem.LoadFile(filename, true, (void **)&data);
    if (!data) {
        return 0;
    }

    int32_t *dataPtr = data;
    int version = *dataPtr++;
    if (version == 1) {
        int textureType = *dataPtr++;
        switch (textureType) {
            case 1:
            flags |= Texture::HighQuality;
            break;
        }

        int wrapMode = *dataPtr++;
        switch (wrapMode) {
        case 0:
            flags |= Texture::Repeat;
            break;
        case 1:
            flags |= Texture::Clamp;
            break;
        }

        int filterMode = *dataPtr++;
        switch (filterMode) {
        case 0:
            flags |= Texture::Nearest | Texture::NoMipmaps;
            break;
        case 1:
            break;        
        case 2:
            break;
        }

        int normalMap = *dataPtr++;
        if (normalMap) {
            flags |= Texture::NormalMap;
        }

        int sRGB = *dataPtr++;
        if (sRGB) {
            flags |= Texture::SRGB;
        }

        if (flags & Texture::NormalMap) {
            flags &= ~Texture::SRGB;
        }
    }

    fileSystem.FreeFile(data);

    return flags;
}

Texture *TextureManager::TextureFromGenerator(const char *hashName, const TextureGeneratorBase &generator) {
    Texture *texture;

    texture = FindTexture(hashName);
    if (texture) {
        BE_LOG(L"TextureManager::TextureFromGenerator: same name already exist\n");
        texture->refCount++;
        return texture;
    }

    texture = AllocTexture(hashName);
    generator.Generate(texture);

    return texture;
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
    
    BE_LOG(L"NUM. REF. TEX. .W.. .H.. .D.. BYTES..... FORMAT.............. MM ADDR NAME\n");

    for (int i = 0; i < textureManager.textureHashMap.Count(); i++) {
        const auto *entry = textureManager.textureHashMap.GetByIndex(i);
        Texture *texture = entry->second;

        switch (texture->type) {
        case Renderer::Texture2D:           type = "2D  "; break;
        case Renderer::Texture3D:           type = "3D  "; break;
        case Renderer::TextureCubeMap:      type = "Cube"; break;
        case Renderer::TextureRectangle:    type = "Rect"; break;
        case Renderer::Texture2DArray:      type = "2DAr"; break;
        case Renderer::TextureBuffer:       type = "Buff"; break;
        }

        const char *internalFormatName = Image::FormatName(texture->format);
        
        switch (texture->addressMode) {
        case Renderer::Repeat:              addr = "R   "; break;
        case Renderer::Clamp:               addr = "C   "; break;
        case Renderer::ClampToBorder:       addr = "CB  "; break;
        case Renderer::MirroredRepeat:      addr = "MR  "; break;
        }

        int numMipmaps = texture->hasMipmaps ? Image::MaxMipMapLevels(texture->width, texture->height, texture->depth) : 1;
        int bytes = Image::MemRequired(texture->width, texture->height, texture->depth, numMipmaps, texture->format) * texture->numSlices;
        
        BE_LOG(L"%4d %4d %hs %4d %4d %4d %10hs %-20hs %hs %hs %hs\n",
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

    BE_LOG(L"total %hs (including mipmaps)\n", Str::FormatBytes(totalBytes).c_str());
    BE_LOG(L"total %i textures\n", count);
}

void TextureManager::Cmd_ReloadTexture(const CmdArgs &args) {
    if (args.Argc() != 2) {
        BE_LOG(L"reloadTexture <filename>\n");
        return;
    }

    if (!WStr::Icmp(args.Argv(1), L"all")) {
        int count = textureManager.textureHashMap.Count();

        for (int i = 0; i < count; i++) {
            const auto entry = textureManager.textureHashMap.GetByIndex(i);
            Texture *texture = entry->second;
            texture->Reload();
        }
    } else {
        Texture *texture = textureManager.FindTexture(WStr::ToStr(args.Argv(1)));
        if (!texture) {
            BE_WARNLOG(L"Couldn't find texture to reload \"%ls\"\n", args.Argv(1));
            return;
        }

        texture->Reload();
    }
}

void TextureManager::Cmd_ConvertNormalAR2RGB(const CmdArgs &args) {	
    char path[MaxAbsolutePath];
    
    if (args.Argc() != 3) {
        BE_LOG(L"convertNormalAR2RGB <rootdir> <filter>\n");
        return;
    }
    
    FileArray fileArray;
    int numFiles = fileSystem.ListFiles(WStr::ToStr(args.Argv(1)), WStr::ToStr(args.Argv(2)), fileArray);
    if (!numFiles) {
        BE_WARNLOG(L"no files found\n");
        return;
    }

    Image image1;
    Image image2;

    for (int i = 0; i < numFiles; i++) {
        Str::snPrintf(path, sizeof(path), "%ls/%hs", args.Argv(1), fileArray.GetFilename(i));
        image1.Load(path);
        if (image1.IsEmpty())
            continue;

        BE_LOG(L"converting '%hs'\n", path);

        if (image1.GetFormat() != Image::RGBA_8_8_8_8) {
            image1.ConvertFormatSelf(Image::RGBA_8_8_8_8);
        }

        image2.Create2D(image1.GetWidth(), image1.GetHeight(), 1, Image::RGB_8_8_8, nullptr, Image::LinearFlag);
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

    BE_LOG(L"all done\n");
}

BE_NAMESPACE_END
