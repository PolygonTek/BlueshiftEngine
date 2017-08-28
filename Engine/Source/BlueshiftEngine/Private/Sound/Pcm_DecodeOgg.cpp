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
#include "Core/Heap.h"
#include "Simd/Simd.h"
#include "File/FileSystem.h"
#include "Sound/Pcm.h"
#include "vorbis/vorbisfile.h"

BE_NAMESPACE_BEGIN

//--------------------------------------------------------------------------------------------------
// Vorbis file callback
//--------------------------------------------------------------------------------------------------

static size_t VorbisFileRead(void *ptr, size_t size, size_t nmemb, void *datasource) {
    size_t readBytes = size * nmemb;
    if (!readBytes) {
        return 0;
    }

    return ((File *)datasource)->Read(ptr, readBytes);
}

static int VorbisFileSeek(void *datasource, ogg_int64_t offset, int whence) {
    switch (whence) {
    case SEEK_SET:
        return ((File *)datasource)->Seek(offset);
    case SEEK_CUR:
        return ((File *)datasource)->Seek(((File *)datasource)->Tell() + offset);
    case SEEK_END:
        return ((File *)datasource)->SeekFromEnd(offset);
    }

    return -1;
}

static int VorbisFileClose(void *datasource) {
    return 0;
}

static long VorbisFileTell(void *datasource) {
    return ((File *)datasource)->Tell();
}

//--------------------------------------------------------------------------------------------------------

bool Pcm::BeginDecodeFile_OggVorbis() {
    static ov_callbacks vorbisFileCallbacks = { VorbisFileRead, VorbisFileSeek, VorbisFileClose, VorbisFileTell };

    vf = Mem_Alloc(sizeof(OggVorbis_File));

    int ret = ov_open_callbacks(fp, (OggVorbis_File *)vf, nullptr, 0, vorbisFileCallbacks);
    if (ret < 0) {
        Mem_Free(vf);
        return false;
    }

    vorbis_info *vi = ov_info((OggVorbis_File *)vf, -1);
    if (!vi) {
        ov_clear((OggVorbis_File *)vf);
        Mem_Free(vf);
        return false;
    }

    this->channels      = (int32_t)vi->channels;
    this->sampleRates   = (int32_t)vi->rate;
    this->bitsWidth     = 16;
    this->size          = (int32_t)ov_pcm_total((OggVorbis_File *)vf, -1) * (this->bitsWidth >> 3) * this->channels;
    this->data          = nullptr;
    this->readPos       = 0;

    return true;
}

void Pcm::EndDecodeFile_OggVorbis() {
    ov_clear((OggVorbis_File *)vf);

    Mem_Free(vf);
}

int Pcm::DecodeFile_OggVorbis(byte *outBuffer, int len) {
    byte *dstPtr = outBuffer;
    char temp[4096];
    long lastReadBytes;
    long readBytes = 0;
    int currentSection;

    do {
        int length = (int)(len - readBytes);
        if (!length) {
            break;
        }

        // 최소 읽기 가능 크기 4
        Clamp<int>(length, 4, sizeof(temp));
        if (length > sizeof(temp)) {
            length = sizeof(temp);
        }

        lastReadBytes = ov_read((OggVorbis_File *)vf, temp, length, 0, bitsWidth >> 3, 1, &currentSection);
        if (lastReadBytes <= 0) {
            break;
        }

        // 읽을 크기 보다 실제로 읽은 크기가 더 커져 버린 경우
        long overBytes = readBytes + lastReadBytes - len;
        if (overBytes > 0) {
            ogg_int64_t offset = ov_raw_tell((OggVorbis_File *)vf);
            ov_raw_seek((OggVorbis_File *)vf, offset - overBytes);

            lastReadBytes -= overBytes;
        }

        simdProcessor->Memcpy(dstPtr, temp, (int)lastReadBytes);

        readBytes += lastReadBytes;
        dstPtr += lastReadBytes;
    } while (readBytes < len);

    return (int)readBytes;
}

bool Pcm::SeekFile_OggVorbis(int byteOffset) {
    double time = (double)byteOffset / BytesPerSecond();

    if (ov_time_seek((OggVorbis_File *)this->vf, time) != 0) {
        return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
// Vorbis memory file callback
//--------------------------------------------------------------------------------------------------

struct VorbisMemoryFile {
    byte *  data;
    size_t  length;
    int64_t offset;
};

static size_t VorbisMemoryFileRead(void *ptr, size_t size, size_t nmemb, void *datasource) {
    VorbisMemoryFile *vmf = (VorbisMemoryFile *)datasource;

    size_t readPos = size * nmemb;
    if (!readPos) {
        return 0;
    }

    if (vmf->length - vmf->offset < readPos) {
        readPos = vmf->length - vmf->offset;
    }

    memcpy(ptr, vmf->data + vmf->offset, readPos);
    vmf->offset += readPos;

    return readPos;
}

static int VorbisMemoryFileSeek(void *datasource, ogg_int64_t offset, int whence) {
    VorbisMemoryFile *vmf = (VorbisMemoryFile *)datasource;

    if (whence == SEEK_SET) {
        vmf->offset = offset;
    } else if (whence == SEEK_CUR) {
        vmf->offset = vmf->offset + offset;
    } else if (whence == SEEK_END) {
        vmf->offset = vmf->length - offset;
    }

    if (vmf->offset < 0 || vmf->offset > vmf->length - 1) {
        return -1;
    }

    return 0;
}

static int VorbisMemoryFileClose(void *datasource) {
    return 0;
}

static long VorbisMemoryFileTell(void *datasource) {
    return ((VorbisMemoryFile *)datasource)->offset;
}

//--------------------------------------------------------------------------------------------------------

bool Pcm::DecodeMemory_OggVorbis(byte *base, size_t fileSize) {
    static ov_callbacks vorbisMemoryFileCallbacks = { VorbisMemoryFileRead, VorbisMemoryFileSeek, VorbisMemoryFileClose, VorbisMemoryFileTell };
    char temp[4096];
    long length;

    VorbisMemoryFile vmf;
    vmf.data = base;
    vmf.length = fileSize;
    vmf.offset = 0;

    OggVorbis_File vf;
    if (ov_open_callbacks(&vmf, &vf, nullptr, 0, vorbisMemoryFileCallbacks) < 0) {
        return false;
    }

    vorbis_info *vi = ov_info(&vf, -1);
    if (!vi) {
        ov_clear(&vf);
        return false;
    }

    this->channels      = (int32_t)vi->channels;
    this->sampleRates   = (int32_t)vi->rate;
    this->bitsWidth     = 16;
    this->size          = (int32_t)ov_pcm_total(&vf, -1) * (this->bitsWidth >> 3) * this->channels;
    this->data          = (byte *)Mem_Alloc16(this->size);
    this->readPos       = 0;

    byte *ptr = this->data;

    do {
        int currentSection;
        length = ov_read(&vf, temp, sizeof(temp), 0, (this->bitsWidth >> 3), 1, &currentSection);

        if (length > 0) {
            if (ptr - this->data + length > this->size) {
                break;
            }

            simdProcessor->Memcpy(ptr, temp, (int)length);
            ptr += length;
        }
    } while (length > 0 && ptr - this->data < this->size);

    ov_clear(&vf);

    return true;
}

BE_NAMESPACE_END
