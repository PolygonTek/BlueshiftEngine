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
#include "Core/Str.h"
#include "Math/Math.h"
#include "IO/FileSystem.h"
#include "Image/Image.h"
#include "ImageInternal.h"

BE_NAMESPACE_BEGIN

static constexpr uint32_t BTEX_MAGIC = MAKE_FOURCC('B', 'T', 'E', 'X');
static constexpr uint16_t BTEX_VERSION = 1;

#pragma pack(push, 1)
struct BTEXHeader {
    uint32_t    magic;
    uint16_t    version;
    uint16_t    headerSize;
    uint32_t    payloadFormat;
    uint32_t    imageFlags;
    uint64_t    payloadOffset;
    uint64_t    payloadSize;
};
#pragma pack(pop)

struct BTEXInfo {
    Image::PayloadFormat    payloadFormat = Image::PayloadFormat::Unknown;
    Image::Flag             flags = Image::Flag::None;
    size_t                  payloadOffset = 0;
    size_t                  payloadSize = 0;
};

static bool IsValidBTEXPayloadFormat(Image::PayloadFormat payloadFormat) {
    switch (payloadFormat) {
    case Image::PayloadFormat::DDS:
    case Image::PayloadFormat::PVR:
    case Image::PayloadFormat::TGA:
    case Image::PayloadFormat::PNG:
        return true;
    default:
        return false;
    }
}

static bool IsWritableBTEXPayloadFormat(Image::PayloadFormat payloadFormat) {
    switch (payloadFormat) {
    case Image::PayloadFormat::DDS:
    case Image::PayloadFormat::PVR:
    case Image::PayloadFormat::TGA:
    case Image::PayloadFormat::PNG:
        return true;
    default:
        return false;
    }
}

static bool LoadBTEXInfoFromHeader(const char *name, const BTEXHeader &header, size_t size, BTEXInfo *outInfo) {
    if (!outInfo) {
        return false;
    }

    if (header.magic != BTEX_MAGIC) {
        BE_WARNLOG("Image::LoadBTEXInfoFromMemory: bad BTEX magic '%s'\n", name ? name : "");
        return false;
    }
    if (header.version != BTEX_VERSION) {
        BE_WARNLOG("Image::LoadBTEXInfoFromMemory: unsupported BTEX version %u in '%s'\n", static_cast<unsigned int>(header.version), name ? name : "");
        return false;
    }
    if (header.headerSize < sizeof(BTEXHeader) || header.headerSize > size) {
        BE_WARNLOG("Image::LoadBTEXInfoFromMemory: bad BTEX header size %u in '%s'\n", static_cast<unsigned int>(header.headerSize), name ? name : "");
        return false;
    }
    if (!IsValidBTEXPayloadFormat(static_cast<Image::PayloadFormat>(header.payloadFormat))) {
        BE_WARNLOG("Image::LoadBTEXInfoFromMemory: unsupported BTEX payload format %u in '%s'\n", static_cast<unsigned int>(header.payloadFormat), name ? name : "");
        return false;
    }
    if (header.payloadOffset < header.headerSize || header.payloadOffset > size) {
        BE_WARNLOG("Image::LoadBTEXInfoFromMemory: bad BTEX payload offset in '%s'\n", name ? name : "");
        return false;
    }
    if (header.payloadSize == 0 || header.payloadSize > size - header.payloadOffset) {
        BE_WARNLOG("Image::LoadBTEXInfoFromMemory: bad BTEX payload size in '%s'\n", name ? name : "");
        return false;
    }

    BTEXInfo info;
    info.payloadFormat = static_cast<Image::PayloadFormat>(header.payloadFormat);
    info.flags = static_cast<Image::Flag>(header.imageFlags);
    info.payloadOffset = static_cast<size_t>(header.payloadOffset);
    info.payloadSize = static_cast<size_t>(header.payloadSize);
    *outInfo = info;

    return true;
}

static bool LoadBTEXInfoFromMemory(const char *name, const byte *data, size_t size, BTEXInfo *outInfo) {
    if (!outInfo || !data || size < sizeof(BTEXHeader)) {
        return false;
    }

    BTEXHeader header;
    memcpy(&header, data, sizeof(header));

    return LoadBTEXInfoFromHeader(name, header, size, outInfo);
}

static bool LoadBTEXInfoFromFile(const char *name, File *fp, size_t size, BTEXInfo *outInfo) {
    if (!outInfo || !fp || size < sizeof(BTEXHeader)) {
        return false;
    }

    BTEXHeader header;
    if (fp->Read(&header, sizeof(header)) != sizeof(header)) {
        return false;
    }

    return LoadBTEXInfoFromHeader(name, header, size, outInfo);
}

bool Image::LoadBTEXFromMemory(const char *name, const byte *data, size_t size) {
    BTEXInfo info;
    if (!LoadBTEXInfoFromMemory(name, data, size, &info)) {
        return false;
    }

    const byte *payload = data + info.payloadOffset;
    const size_t payloadSize = info.payloadSize;

    bool loaded = false;
    switch (info.payloadFormat) {
    case PayloadFormat::DDS:
        loaded = LoadDDSFromMemory(name, payload, payloadSize);
        break;
    case PayloadFormat::PVR:
        loaded = LoadPVRFromMemory(name, payload, payloadSize);
        break;
    case PayloadFormat::TGA:
        loaded = LoadTGAFromMemory(name, payload, payloadSize);
        break;
    case PayloadFormat::PNG:
        loaded = LoadPNGFromMemory(name, payload, payloadSize);
        break;
    default:
        assert(0);
        break;
    }

    if (!loaded) {
        return false;
    }

    flags |= info.flags;
    return true;
}

bool Image::LoadBTEXDDSMipRange(const char *filename, int32_t firstMip, int32_t mipCount, MipRange *outInfo) {
    size_t fileSize = 0;
    File *fp = fileSystem.OpenFileRead(filename, false, &fileSize);
    if (!fp) {
        return false;
    }

    BTEXInfo info;
    if (!LoadBTEXInfoFromFile(filename, fp, fileSize, &info)) {
        fileSystem.CloseFile(fp);
        return false;
    }

    if (info.payloadFormat != PayloadFormat::DDS) {
        fileSystem.CloseFile(fp);
        return false;
    }

    if (fp->Seek(static_cast<int64_t>(info.payloadOffset)) < 0) {
        fileSystem.CloseFile(fp);
        return false;
    }

    if (!LoadDDSMipRange(fp, firstMip, mipCount, outInfo)) {
        fileSystem.CloseFile(fp);
        return false;
    }

    flags |= info.flags;

    fileSystem.CloseFile(fp);
    return true;
}

bool Image::LoadBTEXDDSMipRangeForMaxDimension(const char *filename, int32_t maxDimension, MipRange *outInfo) {
    size_t fileSize = 0;
    File *fp = fileSystem.OpenFileRead(filename, false, &fileSize);
    if (!fp) {
        return false;
    }

    BTEXInfo info;
    if (!LoadBTEXInfoFromFile(filename, fp, fileSize, &info)) {
        fileSystem.CloseFile(fp);
        return false;
    }

    if (info.payloadFormat != PayloadFormat::DDS) {
        fileSystem.CloseFile(fp);
        return false;
    }

    if (fp->Seek(static_cast<int64_t>(info.payloadOffset)) < 0) {
        fileSystem.CloseFile(fp);
        return false;
    }

    if (!LoadDDSMipRangeForMaxDimension(fp, maxDimension, outInfo)) {
        fileSystem.CloseFile(fp);
        return false;
    }

    flags |= info.flags;

    fileSystem.CloseFile(fp);
    return true;
}

bool Image::WriteBTEX(const char *filename, PayloadFormat payloadFormat) const {
    if (!filename || filename[0] == 0) {
        return false;
    }
    if (!IsWritableBTEXPayloadFormat(payloadFormat)) {
        BE_WARNLOG("Image::WriteBTEX: unsupported BTEX payload format %u\n", static_cast<unsigned int>(payloadFormat));
        return false;
    }

    File *fp = fileSystem.OpenFile(filename, File::Mode::Write);
    if (!fp) {
        BE_WARNLOG("Image::WriteBTEX: file open error\n");
        return false;
    }

    BTEXHeader header;
    memset(&header, 0, sizeof(header));
    header.magic = BTEX_MAGIC;
    header.version = BTEX_VERSION;
    header.headerSize = static_cast<uint16_t>(sizeof(BTEXHeader));
    header.payloadFormat = static_cast<uint32_t>(payloadFormat);
    header.imageFlags = static_cast<uint32_t>(flags);
    header.payloadOffset = static_cast<uint64_t>(sizeof(BTEXHeader));

    if (!fp->Write(&header, sizeof(header))) {
        fileSystem.CloseFile(fp);
        return false;
    }

    const int payloadOffset = fp->Tell();
    bool wrotePayload = false;
    switch (payloadFormat) {
    case PayloadFormat::DDS:
        wrotePayload = WriteDDS(fp);
        break;
    case PayloadFormat::PVR:
        wrotePayload = WritePVR(fp);
        break;
    case PayloadFormat::TGA:
        wrotePayload = WriteTGA(fp);
        break;
    case PayloadFormat::PNG:
        wrotePayload = WritePNG(fp);
        break;
    default:
        assert(0);
        break;
    }

    if (payloadOffset < 0 || !wrotePayload) {
        fileSystem.CloseFile(fp);
        return false;
    }

    const int payloadEnd = fp->Tell();
    if (payloadEnd < payloadOffset) {
        fileSystem.CloseFile(fp);
        return false;
    }

    header.payloadOffset = static_cast<uint64_t>(payloadOffset);
    header.payloadSize = static_cast<uint64_t>(payloadEnd - payloadOffset);

    if (fp->Seek(0) < 0 || !fp->Write(&header, sizeof(header))) {
        fileSystem.CloseFile(fp);
        return false;
    }

    fileSystem.CloseFile(fp);
    return true;
}

bool Image::LoadMipRange(const char *filename, int32_t firstMip, int32_t mipCount, MipRange *outInfo) {
    if (!filename || filename[0] == 0) {
        return false;
    }

    Str name = filename;
    if (name.CheckExtension(".dds")) {
        return LoadDDSMipRange(filename, firstMip, mipCount, outInfo);
    }
    if (name.CheckExtension(".btex")) {
        return LoadBTEXDDSMipRange(filename, firstMip, mipCount, outInfo);
    }

    return false;
}

bool Image::LoadMipRangeForMaxDimension(const char *filename, int32_t maxDimension, MipRange *outInfo) {
    if (!filename || filename[0] == 0) {
        return false;
    }

    Str name = filename;
    if (name.CheckExtension(".dds")) {
        return LoadDDSMipRangeForMaxDimension(filename, maxDimension, outInfo);
    }
    if (name.CheckExtension(".btex")) {
        return LoadBTEXDDSMipRangeForMaxDimension(filename, maxDimension, outInfo);
    }

    return false;
}

bool Image::Load(const char *filename) {
    if (!filename || filename[0] == 0) {
        return false;
    }

    Str name = filename;

    byte *data;
    size_t size = fileSystem.LoadFile(name, true, (void **)&data);
    if (data) {
        // Call image loading function by cheking file extension.
        if (name.CheckExtension(".btex")) {
            LoadBTEXFromMemory(name, data, size);
        } else if (name.CheckExtension(".dds")) {
            LoadDDSFromMemory(name, data, size);
        } else if (name.CheckExtension(".pvr")) {
            LoadPVRFromMemory(name, data, size);
        } else if (name.CheckExtension(".tga")) {
            LoadTGAFromMemory(name, data, size);
        } else if (name.CheckExtension(".jpg")) {
            LoadJPGFromMemory(name, data, size);
        } else if (name.CheckExtension(".png")) {
            LoadPNGFromMemory(name, data, size);
        } else if (name.CheckExtension(".bmp")) {
            LoadBMPFromMemory(name, data, size);
        } else if (name.CheckExtension(".pcx")) {
            LoadPCXFromMemory(name, data, size);
        } else if (name.CheckExtension(".hdr")) {
            LoadHDRFromMemory(name, data, size);
        } else if (name.CheckExtension(".exr")) {
            LoadEXRFromMemory(name, data, size);
        }

        fileSystem.FreeFile(data);

        if (pic) {
            return true;
        }
    }

    return false;
}

bool Image::Write(const char *filename) const {
    if (!filename || filename[0] == 0) {
        return false;
    }

    bool ret = false;
    Str name = filename;
    Str extension;
    name.ExtractFileExtension(extension);

    if (!extension.Icmp("btex")) {
        ret = WriteBTEX(filename);
    } else if (!extension.Icmp("dds")) {
        ret = WriteDDS(filename);
    } else if (!extension.Icmp("pvr")) {
        ret = WritePVR(filename);
    } else if (!extension.Icmp("tga")) {
        ret = WriteTGA(filename);
    } else if (!extension.Icmp("jpg")) {
        ret = WriteJPG(filename, 100);
    } else if (!extension.Icmp("png")) {
        ret = WritePNG(filename);
    } else if (!extension.Icmp("bmp")) {
        ret = WriteBMP(filename);
    } else if (!extension.Icmp("pcx")) {
        ret = WritePCX(filename);
    } else if (!extension.Icmp("hdr")) {
        ret = WriteHDR(filename);
    }

    return ret;
}

BE_NAMESPACE_END
