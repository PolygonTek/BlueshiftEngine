#include "Precompiled.h"
#include "Sound/SoundSystem.h"

BE_NAMESPACE_BEGIN

SoundSource::SoundSource() {
    sound = nullptr;
}

SoundSource::~SoundSource() {
    if (pcm.IsOpened()) {
        pcm.Close();
    }
}

void SoundSource::Init(Sound *sound) {
    this->sound = sound;

    if (!sound->isStream) {
        alSourcei(alSourceId, AL_BUFFER, sound->soundBuffer->alBufferIds[0]);
        alSourcei(alSourceId, AL_LOOPING, sound->looping ? AL_TRUE : AL_FALSE);
        
        if (sound->playingTime > 0) {
            alSourcei(alSourceId, AL_BYTE_OFFSET, sound->ByteOffset());
        }
    } else {
        alSourcei(alSourceId, AL_LOOPING, AL_FALSE);
    }

    if (sound->NumChannels() == 1 && !sound->localSound) {
        alSourcei(alSourceId, AL_SOURCE_RELATIVE, AL_FALSE);
        alSourcef(alSourceId, AL_ROLLOFF_FACTOR, 1.0f);

        // The distance that the source will be the loudest (if the listener is
        // closer, it won't be any louder than if they were at this distance)
        alSourcef(alSourceId, AL_REFERENCE_DISTANCE, sound->minDistance);
        // The distance that the source will be the quietest (if the listener is
        // farther, it won't be any quieter than if they were at this distance)
        alSourcef(alSourceId, AL_MAX_DISTANCE, sound->maxDistance);

        alSource3f(alSourceId, AL_POSITION, sound->origin.x, sound->origin.y, sound->origin.z);
    } else {
        alSourcei(alSourceId, AL_SOURCE_RELATIVE, AL_TRUE);
        alSourcef(alSourceId, AL_ROLLOFF_FACTOR, 0);
        
        alSource3f(alSourceId, AL_POSITION, 0, 0, 0);
    }

    Update();
}

void SoundSource::Update() {
    if (sound->isStream) {
        UpdateStream();        
    }

    if (sound->NumChannels() == 1 && !sound->localSound) {
        alSource3f(alSourceId, AL_POSITION, sound->origin.x, sound->origin.y, sound->origin.z);
    }
}

void SoundSource::UpdateStream() {
    if (!pcm.IsOpened()) {
        pcm.Open(sound->hashName);
    }

    int pcmBufferSize = pcm.BytesPerSecond() * SoundBuffer::StreamBufferSeconds;
    byte *pcmBuffer = (byte *)_alloca16(pcmBufferSize);

    if (IsFinished()) {
        pcm.Seek(sound->ByteOffset());

        for (int bufferIndex = 0; bufferIndex < sound->soundBuffer->bufferCount; bufferIndex++) {
            int readSize = pcm.Read(pcmBufferSize, pcmBuffer);
            if (!readSize && !sound->looping) {
                break;
            }

            if (sound->looping) {
                while (readSize < pcmBufferSize) {
                    pcm.Seek(0);
                    readSize += pcm.Read(pcmBufferSize - readSize, pcmBuffer + readSize);
                }
            } else {
                if (readSize < pcmBufferSize) {
                    memset(pcmBuffer + readSize, 0, pcmBufferSize - readSize);
                }
            }

            ALuint buffer = sound->soundBuffer->alBufferIds[bufferIndex];
            alBufferData(buffer, sound->soundBuffer->format, pcmBuffer, pcmBufferSize, sound->soundBuffer->sampleRates);
            alSourceQueueBuffers(alSourceId, 1, &buffer);
        }
    } else {
        ALint buffersProcessed;
        alGetSourcei(alSourceId, AL_BUFFERS_PROCESSED, &buffersProcessed);

        while (buffersProcessed > 0) {
            int readSize = pcm.Read(pcmBufferSize, pcmBuffer);
            if (!readSize && !sound->looping) {
                break;
            }

            if (sound->looping) {
                while (readSize < pcmBufferSize) {
                    pcm.Seek(0);
                    readSize += pcm.Read(pcmBufferSize - readSize, pcmBuffer + readSize);
                }
            } else {
                if (readSize < pcmBufferSize) {
                    memset(pcmBuffer + readSize, 0, pcmBufferSize - readSize);
                }
            }

            ALuint buffer;
            alSourceUnqueueBuffers(alSourceId, 1, &buffer);
            alBufferData(buffer, sound->soundBuffer->format, pcmBuffer, pcmBufferSize, sound->soundBuffer->sampleRates);
            alSourceQueueBuffers(alSourceId, 1, &buffer);

            buffersProcessed--;
        }
    }
}

bool SoundSource::IsFinished() {
    if (!sound) {
        return true;
    }
    ALint state;
    alGetSourcei(alSourceId, AL_SOURCE_STATE, &state);
    if (state == AL_PLAYING || state == AL_PAUSED) {
        return false;
    }
    return true;
}

bool SoundSource::Stop() {
    if (!sound) {
        return false;
    }
    alSourceStop(alSourceId);
    alSourcei(alSourceId, AL_BUFFER, 0);

    if (pcm.IsOpened()) {
        pcm.Close();
    }

    return true;
}

void SoundSource::Pause() {
    if (!sound) {
        return;
    }
    alSourcePause(alSourceId);
}

void SoundSource::Play() {
    if (!sound) {
        return;
    }
    alSourcePlay(alSourceId);
}

uint32_t SoundSource::GetCurrentOffset() const {
    if (!sound) {
        return 0;
    }
    ALint offset;
    alGetSourcei(alSourceId, AL_BYTE_OFFSET, &offset);
    return offset;
}

void SoundSource::SetCurrentOffset(uint32_t offset) {
    if (!sound) {
        return;
    }

    if (sound->isStream) {
        if (!IsFinished()) {
            Stop();
            Update();
            Play();
        }
    } else {
        alSourcei(alSourceId, AL_BYTE_OFFSET, offset);
    }
}

void SoundSource::SetVolume(float volume) {
    if (!sound) {
        return;
    }
    alSourcef(alSourceId, AL_GAIN, volume);
}

BE_NAMESPACE_END