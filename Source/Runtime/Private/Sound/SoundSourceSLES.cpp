#include "Precompiled.h"
#include "Sound/SoundSystem.h"

BE_NAMESPACE_BEGIN

SoundSource::SoundSource() {
    sound = nullptr;
    slPlayerObject = nullptr;
    slPlay = nullptr;
    slBufferQueue = nullptr;
    slVolume = nullptr;
    slSeek = nullptr;
    sl3DLocation = nullptr;
    hasPositionUpdated = false;
}

SoundSource::~SoundSource() {
    DestroyAudioPlayer();

    if (pcm.IsOpened()) {
        pcm.Close();
    }
}

// Callback that is registered if the source needs to loop
static void OpenSLBufferQueueCallback(SLAndroidSimpleBufferQueueItf bufferQueue, void *context) {
    SoundSource *soundSource = (SoundSource *)context;
    if (soundSource) {
        soundSource->OnRequeueBufferCallback(bufferQueue);
    }
}

void SoundSource::OnRequeueBufferCallback(SLAndroidSimpleBufferQueueItf bufferQueue) {
    SLresult result;
    if (!sound->isStream) {
        result = (*slBufferQueue)->Enqueue(slBufferQueue, sound->soundBuffer->buffers[0], sound->soundBuffer->bufferSize);
    }
}

bool SoundSource::CreateAudioPlayer(const Sound *sound) {
    // Data location
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq;
    loc_bufq.locatorType        = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    loc_bufq.numBuffers         = sound->isStream ? SoundBuffer::MaxStreamBuffers : (sound->looping ? 2 : 1);

    // PCM format info
    SLDataFormat_PCM format_pcm;
    format_pcm.formatType       = SL_DATAFORMAT_PCM;
    format_pcm.numChannels      = (SLuint32)(sound->numChannels);
    format_pcm.samplesPerSec    = (SLuint32)(sound->sampleRates * 1000);
    format_pcm.bitsPerSample    = sound->bitsWidth; // SL_PCMSAMPLEFORMAT_FIXED_16;
    format_pcm.containerSize    = sound->bitsWidth; // SL_PCMSAMPLEFORMAT_FIXED_16;
    format_pcm.channelMask      = sound->numChannels == 2 ? (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT) : SL_SPEAKER_FRONT_CENTER;
    format_pcm.endianness       = SL_BYTEORDER_LITTLEENDIAN;

    // Combine location and format into source
    SLDataSource audioSrc;
    audioSrc.pLocator           = &loc_bufq;
    audioSrc.pFormat            = &format_pcm;

    // Output location
    SLDataLocator_OutputMix loc_outmix;
    loc_outmix.locatorType      = SL_DATALOCATOR_OUTPUTMIX;
    loc_outmix.outputMix        = soundSystem.slOutputMixObject;

    // Data sink
    SLDataSink audioSink;
    audioSink.pLocator          = &loc_outmix;
    audioSink.pFormat           = nullptr;

    // Create audio player with source and sink
    const SLInterfaceID ids[]   = { SL_IID_PLAY, SL_IID_BUFFERQUEUE, SL_IID_VOLUME, /*, SL_IID_3DLOCATION*/ };
    const SLboolean req[]       = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE/*, SL_BOOLEAN_FALSE*/ };

    SLresult result = (*soundSystem.slEngine)->CreateAudioPlayer(soundSystem.slEngine, &slPlayerObject, &audioSrc, &audioSink, COUNT_OF(ids), ids, req);
    if (result != SL_RESULT_SUCCESS) {
        BE_WARNLOG(L"Failed OpenSL CreateAudioPlayer : 0x%x\n", result);
        return false;
    }

    // Realize the player
    result = (*slPlayerObject)->Realize(slPlayerObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        BE_WARNLOG(L"Failed OpenSL Realize : 0x%x\n", result);
        return false;
    }

    // Get the play interface
    result = (*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_PLAY, &slPlay);
    if (result != SL_RESULT_SUCCESS) {
        BE_WARNLOG(L"Failed OpenSL GetInterface SL_IID_PLAY : 0x%x\n", result);
        return false;
    }

    // Get the volume interface
    result = (*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_VOLUME, &slVolume);
    if (result != SL_RESULT_SUCCESS) {
        BE_WARNLOG(L"Failed OpenSL GetInterface SL_IID_VOLUME : 0x%x\n", result);
        return false;
    }

    // Get the buffer queue interface
    result = (*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_BUFFERQUEUE, &slBufferQueue);
    if (result != SL_RESULT_SUCCESS) {
        BE_WARNLOG(L"Failed OpenSL GetInterface SL_IID_BUFFERQUEUE : 0x%x\n", result);
        return false;
    }    

#if 0
    // Get the 3d location interface
    result = (*slPlayerObject)->GetInterface(slPlayerObject, SL_IID_3DLOCATION, &sl3DLocation);
    if (result != SL_RESULT_SUCCESS) {
    }
#endif

    return true;
}

void SoundSource::DestroyAudioPlayer() {
    if (slPlayerObject) {
        (*slPlayerObject)->Destroy(slPlayerObject);
        slPlayerObject = nullptr;
        slPlay = nullptr;
        slBufferQueue = nullptr;
        slVolume = nullptr;
        slSeek = nullptr;
        sl3DLocation = nullptr;
    }
}

void SoundSource::Init(Sound *sound) {
    SLresult result;
    
    if (!CreateAudioPlayer(sound)) {
        BE_WARNLOG(L"Failed to create OpenSL audio player !\n");
        return;
    }

    if (sound->isStream) {
        lastBufferQueueIndex = 0;
    } else {
        // If looping, register a callback to requeue the buffer
        if (sound->looping) {
            //result = (*slSeek)->SetLoop(slSeek, SL_BOOLEAN_TRUE, 0, SL_TIME_UNKNOWN);
            result = (*slBufferQueue)->RegisterCallback(slBufferQueue, OpenSLBufferQueueCallback, (void *)this);
            if (result != SL_RESULT_SUCCESS) {
                BE_WARNLOG(L"Failed OpenSL RegisterCallback : 0x%x\n", result);
                DestroyAudioPlayer();
                return;
            }
        }

        int byteOffset = sound->ByteOffset();

        result = (*slBufferQueue)->Enqueue(slBufferQueue, sound->soundBuffer->buffers[0] + byteOffset, sound->soundBuffer->bufferSize - byteOffset);
        if (result != SL_RESULT_SUCCESS) {
            BE_WARNLOG(L"Failed OpenSL Enqueue : 0x%x\n", result);
            if (sound->looping) {
                result = (*slBufferQueue)->RegisterCallback(slBufferQueue, nullptr, nullptr);
            }
            DestroyAudioPlayer();
            return;
        }

        if (sound->looping) {
            (*slBufferQueue)->Enqueue(slBufferQueue, sound->soundBuffer->buffers[0], sound->soundBuffer->bufferSize);
        }
    }

    this->sound = sound;

    hasPositionUpdated = false;

    if (sound->NumChannels() > 1 || sound->localSound) {
        SetVolume(sound->volume * SoundSystem::s_volume.GetFloat());
    }

    Update();
}

void SoundSource::Update() {
    if (!sound) {
        return;
    }

    if (sound->isStream) {
        UpdateStream();
    }

#if 0
    if (!sound->localSound && sl3DLocation) {
        SLVec3D slVec = { (SLint32)sound->origin.x, (SLint32)sound->origin.y, (SLint32)sound->origin.z };
        SLresult result = (*sl3DLocation)->SetLocationCartesian(sl3DLocation, &slVec);
        assert(SL_RESULT_SUCCESS == result);
    }
#endif

    if (sound->NumChannels() == 1 && !sound->localSound) {
        // Do linear attenuation
        float distance = soundSystem.listenerPosition.Distance(sound->origin);
        Clamp(distance, sound->minDistance, sound->maxDistance);
        float gain = (1.0f - (distance - sound->minDistance) / (sound->maxDistance - sound->minDistance));
        SetVolume(sound->volume * gain * SoundSystem::s_volume.GetFloat());
    }
}

void SoundSource::UpdateStream() {
    if (!pcm.IsOpened()) {
        pcm.Open(sound->hashName);
    }

    if (IsFinished()) {
        pcm.Seek(sound->ByteOffset());

        bufferUnqueueIndex = 0;

        for (int index = 0; index < sound->soundBuffer->bufferCount; index++) {
            int readSize = pcm.Read(sound->soundBuffer->bufferSize, sound->soundBuffer->buffers[index]);
            if (!readSize && !sound->looping) {
                break;
            }

            if (sound->looping) {
                while (readSize < sound->soundBuffer->bufferSize) {
                    pcm.Seek(0);
                    readSize += pcm.Read(sound->soundBuffer->bufferSize - readSize, sound->soundBuffer->buffers[index] + readSize);
                }
            } else {
                if (readSize < sound->soundBuffer->bufferSize) {
                    memset(sound->soundBuffer->buffers[index] + readSize, 0, sound->soundBuffer->bufferSize - readSize);
                }
            }

            SLresult result = (*slBufferQueue)->Enqueue(slBufferQueue, sound->soundBuffer->buffers[index], sound->soundBuffer->bufferSize);
            assert(SL_RESULT_SUCCESS == result);
        }
    } else {
        SLAndroidSimpleBufferQueueState st;
        (*slBufferQueue)->GetState(slBufferQueue, &st);

        if (st.count == 0) {
            SLresult result = (*slPlay)->SetPlayState(slPlay, SL_PLAYSTATE_STOPPED);
            assert(SL_RESULT_SUCCESS == result);
            return;
        }

        int processedBuffers = st.index - lastBufferQueueIndex;
        lastBufferQueueIndex = st.index;

        while (processedBuffers > 0) {
            int readSize = pcm.Read(sound->soundBuffer->bufferSize, sound->soundBuffer->buffers[bufferUnqueueIndex]);
            if (!readSize && !sound->looping) {
                break;
            }

            if (sound->looping) {
                while (readSize < sound->soundBuffer->bufferSize) {
                    pcm.Seek(0);
                    readSize += pcm.Read(sound->soundBuffer->bufferSize - readSize, sound->soundBuffer->buffers[bufferUnqueueIndex] + readSize);
                }
            } else {
                if (readSize < sound->soundBuffer->bufferSize) {
                    memset(sound->soundBuffer->buffers[bufferUnqueueIndex] + readSize, 0, sound->soundBuffer->bufferSize - readSize);
                }
            }

            SLresult result = (*slBufferQueue)->Enqueue(slBufferQueue, sound->soundBuffer->buffers[bufferUnqueueIndex], sound->soundBuffer->bufferSize);
            assert(SL_RESULT_SUCCESS == result);

            bufferUnqueueIndex = (bufferUnqueueIndex + 1) % sound->soundBuffer->bufferCount;

            processedBuffers--;
        }
    }
}

bool SoundSource::IsFinished() {
    if (!sound) {
        return true;
    }

    if (!slPlay) {
        return true;
    }

    SLuint32 playState;
    SLresult result = (*slPlay)->GetPlayState(slPlay, &playState);
    assert(SL_RESULT_SUCCESS == result);
    if (playState == SL_PLAYSTATE_STOPPED) {
        return true;
    }

    if (!sound->isStream) {
        SLmillisecond positionMs;
        SLmillisecond durationMs;

        result = (*slPlay)->GetPosition(slPlay, &positionMs);
        assert(SL_RESULT_SUCCESS == result);

        result = (*slPlay)->GetDuration(slPlay, &durationMs);
        assert(SL_RESULT_SUCCESS == result);

        // on some android devices, the value for GetPosition wraps back to 0 when the playback is done, however it's very possible
        // for us to try to check for IsSourceFinished when the Position is genuinely "0". Therefore, we'll flip hasPositionUpdated once
        // we've actually started the sound to denote a wrap-back 0 position versus a real 0 position
        if ((durationMs != SL_TIME_UNKNOWN && positionMs == durationMs) || (positionMs == 0 && hasPositionUpdated)) {
            return true;
        } else {
            hasPositionUpdated = true;
        }
    }

    return false;
}

bool SoundSource::Stop() {
    if (!sound) {
        return false;
    }

    if (slPlay) {
        // set the player's state to stopped
        SLresult result = (*slPlay)->SetPlayState(slPlay, SL_PLAYSTATE_STOPPED);
        assert(SL_RESULT_SUCCESS == result);

        if (slBufferQueue) {
            (*slBufferQueue)->Clear(slBufferQueue);
        }

        if (!sound->isStream && sound->looping) {
            SLresult result = (*slBufferQueue)->RegisterCallback(slBufferQueue, nullptr, nullptr);
        }

        lastBufferQueueIndex = 0;

        DestroyAudioPlayer();
    }

    if (pcm.IsOpened()) {
        pcm.Close();
    }

    return true;
}

void SoundSource::Pause() {
    if (!sound) {
        return;
    }
    // set the player's state to paused
    SLresult result = (*slPlay)->SetPlayState(slPlay, SL_PLAYSTATE_PAUSED);
    assert(SL_RESULT_SUCCESS == result);
}

void SoundSource::Play() {
    if (!sound) {
        return;
    }
    // set the player's state to playing
    SLresult result = (*slPlay)->SetPlayState(slPlay, SL_PLAYSTATE_PLAYING);
    assert(SL_RESULT_SUCCESS == result);
}

uint32_t SoundSource::GetCurrentOffset() const {
    if (!sound) {
        return 0;
    }
    SLmillisecond positionMs;
    SLmillisecond durationMs;

    SLresult result = (*slPlay)->GetPosition(slPlay, &positionMs);
    assert(SL_RESULT_SUCCESS == result);

    result = (*slPlay)->GetDuration(slPlay, &durationMs);
    assert(SL_RESULT_SUCCESS == result);

    if (durationMs == SL_TIME_UNKNOWN) {
        return 0;
    }
    return sound->bytes * ((float)positionMs / durationMs);
}

void SoundSource::SetCurrentOffset(uint32_t offset) {
    if (!sound) {
        return;
    }

    if (sound->isStream) {
        if (!IsFinished()) {
            SLresult result = (*slPlay)->SetPlayState(slPlay, SL_PLAYSTATE_STOPPED);
            assert(SL_RESULT_SUCCESS == result);

            if (slBufferQueue) {
                (*slBufferQueue)->Clear(slBufferQueue);
            }

            lastBufferQueueIndex = 0;

            Update();

            Play();
        }
    }
#if 0
    else {
        if (slSeek) {
            SLresult result = (*slSeek)->SetPosition(slSeek, (SLmillisecond)(sound->duration * ((float)offset / sound->bytes)), SL_SEEKMODE_FAST);
            assert(SL_RESULT_SUCCESS == result);
        }
    }
#endif
}

void SoundSource::SetVolume(float volume) {
    if (!sound) {
        return;
    }

    static const int64_t minVolumeMillibel = -12000;
    SLmillibel volumeMillibel;
    if (volume <= 0) {
        volumeMillibel = minVolumeMillibel;
    } else if (volume >= 1.0f) {
        volumeMillibel = 0;
    } else {
        volumeMillibel = Clamp<int64_t>((int64_t)(2000.0f * Math::Log(10, volume)), minVolumeMillibel, 0);
    }
    SLresult result = (*slVolume)->SetVolumeLevel(slVolume, volumeMillibel);
    assert(SL_RESULT_SUCCESS == result);
}

BE_NAMESPACE_END