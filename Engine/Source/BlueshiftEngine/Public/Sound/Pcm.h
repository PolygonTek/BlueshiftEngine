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

BE_NAMESPACE_BEGIN

class File;
struct WaveFormatEx;

class Pcm {
public:
    enum FileType {
        UnknownFile,
        WavFile,
        OggFile
    };

    enum Format {
        MS_PCM          = 0x0001,
        MS_ADPCM        = 0x0002,
        IMA_ADPCM       = 0x0011,
    };

    Pcm();
    ~Pcm();

    bool                IsOpened() const;
    bool                IsLoaded() const;

    Pcm::FileType       GetFileType() const { return fileType; }
    int                 NumChannels() const { return channels; }
    int                 SampleRates() const { return sampleRates; }
    int                 BitsWidth() const { return bitsWidth; }
    int                 BytesPerSecond() const { return channels * sampleRates * (bitsWidth >> 3); }
    float               Duration() const { return (float)size / BytesPerSecond(); }
    int                 DataSize() const { return size; }
    byte *              GetData() const { return data; }

    void                Purge();

    bool                Create(int numChannels, int sampleRates, int bitsWidth, int size, const byte *data);

    bool                Load(const char *filename);
    void                Free();

    bool                Open(const char *filename);
    void                Close();
    int                 Read(int size, byte *buffer);

    void                Seek(int offset);

private:
                        // decode Wav file
    bool                BeginDecodeFile_Wav();
    void                EndDecodeFile_Wav();
    int                 DecodeFile_Wav(byte *buffer, int len);
    int                 DecodeFile_PcmWave(byte *buffer, int len);
    int                 DecodeFile_ImaAdpcmWave(byte *buffer, int len);
    bool                SeekFile_Wav(int offset);
                        // decode Wav memory
    bool                DecodeMemory_Wav(byte *base, size_t fileSize);

                        // decode Ogg file
    bool                BeginDecodeFile_OggVorbis();
    void                EndDecodeFile_OggVorbis();
    int                 DecodeFile_OggVorbis(byte *buffer, int len);
    bool                SeekFile_OggVorbis(int offset);
                        // decode Ogg memory
    bool                DecodeMemory_OggVorbis(byte *base, size_t fileSize);

    FileType            fileType;
    WaveFormatEx *      waveFormat;         ///< Used only for wave file type
    int                 channels;           ///< Number of channels
    int                 sampleRates;        ///< Samples per second
    int                 bitsWidth;          ///< Bits per sample

    byte *              data;               ///< Wave PCM data (available when Load() called)
    int                 size;               ///< Size of PCM data
    int                 dataPos;            ///< 
    int                 readPos;            ///< Current read offset

    File *              fp;
    void *              vf;                 ///< OggVorbis_File *
};

BE_INLINE Pcm::Pcm() {
    fileType = FileType::UnknownFile;
    waveFormat = nullptr;
    data = nullptr;
    fp = nullptr;
}

BE_INLINE Pcm::~Pcm() {
    Purge();
}

BE_NAMESPACE_END
