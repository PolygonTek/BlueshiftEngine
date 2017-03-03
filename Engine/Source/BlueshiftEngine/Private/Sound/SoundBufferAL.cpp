#include "Precompiled.h"
#include "Core/Heap.h"
#include "Sound/SoundSystem.h"

BE_NAMESPACE_BEGIN

static ALenum ToALFormat(int numChannels, int bitsWidth) {
    bool stereo = numChannels > 1;

    if (stereo) {
        if (bitsWidth == 8) {
            return AL_FORMAT_STEREO8;
        } else if (bitsWidth == 16) {
            return AL_FORMAT_STEREO16;
        }
    } else {
        if (bitsWidth == 8) {
            return AL_FORMAT_MONO8;
        } else if (bitsWidth == 16) {
            return AL_FORMAT_MONO16;
        }
    }
    return 0;
}

SoundBuffer::SoundBuffer() {
    duplicated = false;
    bufferCount = 0;
}

SoundBuffer::~SoundBuffer() {
    Purge();
}

void SoundBuffer::Purge() {
    if (!duplicated) {
        if (bufferCount > 0) {
            alDeleteBuffers(bufferCount, alBufferIds);
            bufferCount = 0;
        }
    }
}

bool SoundBuffer::CreateStaticBuffer(int numChannels, int bitsWidth, int sampleRates, int size, const byte *data) {
    Purge();

    duplicated = false;

    this->format = ToALFormat(numChannels, bitsWidth);
    if (!format) {
        return false;
    }

    this->sampleRates = sampleRates;

    bufferCount = 1;

    alGenBuffers(1, &alBufferIds[0]);

    alBufferData(alBufferIds[0], format, data, size, sampleRates);

    return true;
}

bool SoundBuffer::CreateStreamBuffer(int numChannels, int bitsWidth, int sampleRates) {
    Purge();

    duplicated = false;

    this->format = ToALFormat(numChannels, bitsWidth);
    if (!format) {
        return false;
    }

    this->sampleRates = sampleRates;

    bufferCount = MaxStreamBuffers;

    alGenBuffers(bufferCount, alBufferIds);

    return true;
}

bool SoundBuffer::DuplicateBuffer(SoundBuffer *originalSoundBuffer) {
    duplicated = true;

    memcpy(alBufferIds, originalSoundBuffer->alBufferIds, sizeof(alBufferIds));
    bufferCount = originalSoundBuffer->bufferCount;
    format = originalSoundBuffer->format;
    sampleRates = originalSoundBuffer->sampleRates;

    return true;
}

BE_NAMESPACE_END
