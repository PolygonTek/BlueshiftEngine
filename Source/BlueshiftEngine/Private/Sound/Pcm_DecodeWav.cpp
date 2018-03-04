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

BE_NAMESPACE_BEGIN

#pragma pack(push, 4)

struct WaveFormatEx {
    uint16_t        format;
    uint16_t        channels;
    uint32_t        sampleRates;
    uint32_t        bytesPerSec;
    uint16_t        blockAlign;
    uint16_t        bitsWidth;
    int32_t         cbSize;
};

struct WavePcmFormat {
    uint16_t        format;
    uint16_t        channels;
    uint32_t        sampleRates;
    uint32_t        bytesPerSec;
    uint16_t        blockAlign;
    uint16_t        bitsWidth;
};

struct WaveImaAdpcmFormat {
    uint16_t        format;
    uint16_t        channels;
    uint32_t        sampleRates;
    uint32_t        bytesPerSec;
    uint16_t        blockAlign;
    uint16_t        bitsWidth;
    uint16_t        extraBytes;
    uint16_t        samplesPerBlock;
};

#pragma pack(pop)

//--------------------------------------------------------------------------------------------------
// RIFF wave chunk id
//--------------------------------------------------------------------------------------------------
static const uint32_t WAVE_FOURCC_RIFF  = MAKE_FOURCC('R', 'I', 'F', 'F');
static const uint32_t WAVE_FOURCC_WAVE  = MAKE_FOURCC('W', 'A', 'V', 'E');
static const uint32_t WAVE_FOURCC_fmt   = MAKE_FOURCC('f', 'm', 't', ' ');
static const uint32_t WAVE_FOURCC_fact  = MAKE_FOURCC('f', 'a', 'c', 't');
static const uint32_t WAVE_FOURCC_data  = MAKE_FOURCC('d', 'a', 't', 'a');

//--------------------------------------------------------------------------------------------------
// predefined IMA ADPCM data
//--------------------------------------------------------------------------------------------------
static const int32_t ima_index_adjust[8] = {
    -1, -1, -1, -1,     // 0 ~ 3, decrease the step size
     2,  4,  6,  8,     // 4 ~ 7, increase the step size
};

static const int16_t ima_step_size[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230,
    253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,
    1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
    3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
    11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

//--------------------------------------------------------------------------------------------------
// IMA ADPCM decode functions
//--------------------------------------------------------------------------------------------------
static void IMA_ADPCM_DecodeBlock(const uint32_t **in_ptr, int16_t *out, int channels, int samples) {
    struct {
        int16_t predictor;
        byte stepIndex;
        byte pad;
    } channelData[2];
    uint32_t fourBytes;
    int i, j, k;

    const uint32_t *in = *in_ptr;
    int16_t *out_ptr = out;

    // 채널 별로 4 바이트 초기값 (preamble) 을 읽는다
    for (i = 0; i < channels; i++) {
        fourBytes = *in++;
        channelData[i].predictor = out_ptr[i] = fourBytes & 0xffff;
        channelData[i].stepIndex = (fourBytes >> 16) & 0xff;
    }

    // 채널 별로 4 바이트씩 (채널 별로 8 개의 샘플) 읽어서 처리한다
    for (; i < samples; i += 8 * channels) {
        for (j = 0; j < channels; j++) {
            fourBytes = *in++;

            int32_t predictor = channelData[j].predictor;
            int16_t stepIndex = channelData[j].stepIndex;

            out_ptr = out + i + j;

            for (k = 0; k < 8; k++, fourBytes >>= 4, out_ptr += channels) {
                int nibble = fourBytes & 0xf;

                // 스텝 값을 구한다
                int step = ima_step_size[stepIndex];

                // 다음 스텝 인덱스 보정
                stepIndex += ima_index_adjust[nibble & 7];
                Clamp<int16_t>(stepIndex, 0, 88);

                // 차이값을 구한다 (diff = (nibble + 0.5) * step / 4)
                int diff = step >> 3;
                if (nibble & 1) {
                    diff += step >> 2;
                }
                if (nibble & 2) {
                    diff += step >> 1;
                }
                if (nibble & 4) {
                    diff += step;
                }

                // 차이값을 더해 산정한다
                if (nibble & 8) {
                    predictor -= diff;
                    if (predictor < -32768) {
                        predictor = -32768;
                    }
                } else {
                    predictor += diff;
                    if (predictor > 32767) {
                        predictor = 32767;
                    }
                }

                // 디코딩 된 최종값 (signed 16 bit) 저장
                *out_ptr = predictor;
            }

            channelData[j].predictor = predictor;
            channelData[j].stepIndex = stepIndex;
        }
    }

    *in_ptr = in;
}

static void IMA_ADPCM_Decode(byte *in, int size, int blockSize, int channels, int samplesPerBlock, byte *out) {
    int numBlocks = (size + blockSize - 1) / blockSize;
    int samples = (size - numBlocks * channels * 4) * 2 + numBlocks * channels;

    samplesPerBlock *= channels;

    const uint32_t *in_ptr = (const uint32_t *)in;
    int16_t *out_ptr = (int16_t *)out;

    for (int i = 0; i < samples; i += samplesPerBlock) {
        IMA_ADPCM_DecodeBlock(&in_ptr, &out_ptr[i], channels, samples > samplesPerBlock ? samplesPerBlock : samples);
    }
}

//----------------------------------------------------------------------------------------------------------------------

int Pcm::DecodeFile_PcmWave(byte *buffer, int len) {
    int read = (int)fp->Read(buffer, (size_t)len);
    if (readPos + read > size) {
        read = size - readPos;
    }

    return read;
}

int Pcm::DecodeFile_ImaAdpcmWave(byte *buffer, int len) {
    static int  last[4096];
    static int  lastBytes = 0;
    int         read;

    if (lastBytes > 0 && readPos > 0) {
        simdProcessor->Memcpy(buffer, last, lastBytes);
        read = lastBytes;
        lastBytes = 0;
    } else {
        read = 0;
    }

    // 한 블럭의 크기
    int blockBytes = ((((WaveImaAdpcmFormat *)waveFormat)->samplesPerBlock - 1) / 2 + 4) * channels;
    byte compressed[4096];
    byte extracted[4096];

    do {
        int length = (int)fp->Read(compressed, blockBytes);
        if (!length) {
            break;
        }

        int readbytes = (length - 4 * channels) * 4 + channels * 2;

        if (readPos + read + readbytes > this->size) {
            readbytes = this->size - read - readPos;
            length = ((readbytes / (2 * channels) - 1) / 2 + 4) * channels;
        }

        IMA_ADPCM_Decode(compressed, length, waveFormat->blockAlign, channels, ((WaveImaAdpcmFormat *)waveFormat)->samplesPerBlock, extracted);

        if (read + readbytes > len) { // 읽을 크기 보다 실제로 읽은 크기가 더 커져 버린 경우
            // 초과된 량은 버퍼에 저장해 놓는다
            lastBytes = read + readbytes - len;
            simdProcessor->Memcpy(last, extracted + readbytes - lastBytes, lastBytes);
            readbytes = len - read;
        }

        simdProcessor->Memcpy(buffer + read, extracted, readbytes);
        read += readbytes;
    } while (read < len);

    return read;
}

static bool FindChunkInFile(File *fp, uint32_t chunkId, uint32_t *chunkSize) {
    uint32_t id, length;
    byte *tempBuffer = nullptr;
    byte *tempMem = nullptr;
    int tempMemSize = 0;

    while (1) {
        fp->ReadUInt32(id);
        fp->ReadUInt32(length);

        if (id == chunkId) {
            *chunkSize = length;
            return true;
        }

        if (length <= 0x8000) {
            if (!tempBuffer) {
                tempBuffer = (byte *)_alloca16(0x8000);
            }

            fp->Read(tempBuffer, length);
        } else {
            if (tempMem && length > tempMemSize) {
                Mem_AlignedFree(tempMem);
                tempMem = nullptr;
            }

            if (!tempMem) {
                tempMemSize = length;
                tempMem = (byte *)Mem_Alloc16(tempMemSize);
            }

            fp->Read(tempMem, length);
        }
    }

    if (tempMem) {
        Mem_AlignedFree(tempMem);
    }
    return false;
}

bool Pcm::BeginDecodeFile_Wav() {
    uint32_t id, length;

    fp->ReadUInt32(id);
    fp->ReadUInt32(length);
    
    if (id != WAVE_FOURCC_RIFF) {
        BE_WARNLOG(L"Missing RIFF chunk\n");
        return false;
    }

    fp->ReadUInt32(id);

    if (id != WAVE_FOURCC_WAVE) {
        BE_WARNLOG(L"Missing WAVE form type\n");
        return false;
    }

    if (!FindChunkInFile(fp, WAVE_FOURCC_fmt, &length)) {
        BE_WARNLOG(L"Missing fmt chunk\n");
        return false;
    }

    if (length < sizeof(WavePcmFormat)) {
        BE_WARNLOG(L"Invalid fmt chunk\n");
        return false;
    }

    this->waveFormat = (WaveFormatEx *)Mem_Alloc(length);
    fp->Read(this->waveFormat, length);

    if (waveFormat->format != Format::MS_PCM && waveFormat->format != Format::IMA_ADPCM) {
        BE_WARNLOG(L"Unsupported wave format\n");
        return false;
    }

    this->channels      = waveFormat->channels;
    this->sampleRates   = waveFormat->sampleRates;
    this->bitsWidth     = waveFormat->format == Format::IMA_ADPCM ? 16 : waveFormat->bitsWidth;

    if (!FindChunkInFile(fp, WAVE_FOURCC_data, &length)) {
        BE_WARNLOG(L"Missing data chunk\n");
        return false;
    }

    if (waveFormat->format == Format::MS_PCM) {
        size = length;
    } else if (waveFormat->format == Format::IMA_ADPCM) {
        int numBlocks = (length + waveFormat->blockAlign - 1) / waveFormat->blockAlign;
        size = ((length - 4 * channels * numBlocks) * 2 + channels * numBlocks) * 2;
    }

    data = nullptr;
    dataPos = fp->Tell();
    readPos = 0;

    return true;
}

void Pcm::EndDecodeFile_Wav() {
    Mem_Free(waveFormat);
    waveFormat = nullptr;
}

int Pcm::DecodeFile_Wav(byte *buffer, int len) {
    if (waveFormat->format == Format::MS_PCM) {
        return DecodeFile_PcmWave(buffer, len);
    } else if (waveFormat->format == Format::IMA_ADPCM) {
        return DecodeFile_ImaAdpcmWave(buffer, len);
    }

    return 0;
}

bool Pcm::SeekFile_Wav(int byteOffset) {
    if (waveFormat->format == Format::MS_PCM) {
        fp->Seek(dataPos + AlignUp(byteOffset, bitsWidth));
    } else if (waveFormat->format == Format::IMA_ADPCM) {
        // TODO: IMPLEMENT THIS !
    }
    return true;
}

static bool FindChunkInMemory(byte **ptr, const byte *endPtr, uint32_t chunkId, uint32_t *chunkSize) {
    while (*ptr < endPtr) {
        uint32_t id = *(uint32_t *)*ptr;
        *ptr += 4;

        uint32_t length = *(uint32_t *)*ptr;
        *ptr += 4;

        if (id == chunkId) {
            *chunkSize = length; 
            return true;
        }

        *ptr += length;
    }

    return false;
}

bool Pcm::DecodeMemory_Wav(byte *base, size_t fileSize) {
    byte *ptr = base;

    // skip 'RIFF'
    ptr += 4;

    // read wave length (파일 크기에서 8 을 뺀값)
    uint32_t length = *((uint32_t *)ptr);
    ptr += 4;

    if (*(uint32_t *)ptr != WAVE_FOURCC_WAVE) {
        BE_WARNLOG(L"Missing WAVE form type\n");
        return false;
    }

    // skip 'WAVE' signature
    ptr += 4;

    if (!FindChunkInMemory(&ptr, base + fileSize, WAVE_FOURCC_fmt, &length)) {
        BE_WARNLOG(L"Missing fmt chunk\n");
        return false;
    }    

    if (length < sizeof(WavePcmFormat)) {
        BE_WARNLOG(L"Invalid wave format header\n");
        return false;
    }

    WaveFormatEx *fmt = (WaveFormatEx *)ptr;
    if (fmt->format != Format::MS_PCM && fmt->format != Format::IMA_ADPCM) {
        BE_WARNLOG(L"Unsupported wave format\n");
        return false;
    }

    this->channels      = fmt->channels;
    this->sampleRates   = fmt->sampleRates;
    this->bitsWidth     = fmt->format == Format::IMA_ADPCM ? 16 : fmt->bitsWidth;

    ptr += length;

    if (!FindChunkInMemory(&ptr, base + fileSize, WAVE_FOURCC_data, &length)) {
        BE_WARNLOG(L"Missing data chunk\n");
        return false;
    }

    if (fmt->format == Format::MS_PCM) {
        size = length;
        data = (byte *)Mem_Alloc16(size);

        simdProcessor->Memcpy(data, ptr, size);
    } else if (fmt->format == Format::IMA_ADPCM) {
        int numBlocks = (length + fmt->blockAlign - 1) / fmt->blockAlign;

        size = ((length - 4 * channels * numBlocks) * 2 + channels * numBlocks) * 2;
        data = (byte *)Mem_Alloc16(size);

        IMA_ADPCM_Decode(ptr, length, fmt->blockAlign, channels, ((WaveImaAdpcmFormat *)fmt)->samplesPerBlock, data);
    }

    ptr += length;

    return true;
}

BE_NAMESPACE_END
