#include "Precompiled.h"
#include "Math/Math.h"
#include "Simd/Simd.h"
#include "Core/CVars.h"
#include "Sound/SoundSystem.h"

BE_NAMESPACE_BEGIN

static const int MaxSources = 32;

static CVar s_khz(L"s_khz", L"44", CVar::Integer | CVar::Archive, L"");
static CVar s_doppler(L"s_doppler", L"1.0", CVar::Float | CVar::Archive, L"");
static CVar s_rolloff(L"s_rolloff", L"2.0", CVar::Float | CVar::Archive, L"");

bool SoundSystem::InitDevice(void *windowHandle) {
    BE_LOG(L"Initializing OpenAL...\n");
    
    // Verify that a given extension is available for the current context
    ALboolean enumeration = alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT");
    if (enumeration == AL_FALSE) {
        BE_WARNLOG(L"OpenAL enumeration extension not available\n");
    }

    // Retrieve a list of available devices
    // Each device name is separated by a single nullptr character
    // and the list is terminated with 2 nullptr characters
    const ALCchar *deviceNameList = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
    const ALCchar *deviceNamePtr = deviceNameList;
    BE_LOG(L"OpenAL devices: \n");
    while (1) {
        BE_LOG(L"...%hs\n", deviceNamePtr);
        
        size_t nextOffset = strlen(deviceNamePtr);
        if (!deviceNamePtr[nextOffset]) {
            break;
        }
        
        deviceNamePtr += nextOffset;
    }
    
    // Retrieve the default device name
    const ALCchar *defaultDeviceName = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);

    // Reset the error stack
    alGetError();

    // Open the default device
    BE_LOG(L"...opening OpenAL device: ");
    alDevice = alcOpenDevice(defaultDeviceName);
    if (!alDevice) {
        BE_LOG(L"failed\n");
        return false;
    }
    BE_LOG(L"ok\n");
    
    // Get the OpenAL version
    ALCint majorVersion, minorVersion;
    alcGetIntegerv(alDevice, ALC_MAJOR_VERSION, 1, &majorVersion);
    alcGetIntegerv(alDevice, ALC_MINOR_VERSION, 1, &minorVersion);

    // Create context
    BE_LOG(L"...creating OpenAL context: ");
#if defined(__WIN32__)
    int attrs[] = {
        ALC_FREQUENCY, 44100,
        ALC_STEREO_SOURCES, 4,
        0, 0
    };
    alContext = alcCreateContext(alDevice, attrs);
#else
    alContext = alcCreateContext(alDevice, 0);
#endif
    if (!alContext) {
        alcCloseDevice(alDevice);
        BE_LOG(L"failed\n");
        return false;
    }
    BE_LOG(L"ok\n");

    // Set active context
    BE_LOG(L"...make current context: ");
    if (!alcMakeContextCurrent(alContext)) {
        alcDestroyContext(alContext);
        alcCloseDevice(alDevice);
        BE_LOG(L"failed\n");
        return false;
    }
    BE_LOG(L"ok\n");
    
    BE_LOG(L"OpenAL vendor: %hs\n", alGetString(AL_VENDOR));
    BE_LOG(L"OpenAL renderer: %hs\n", alGetString(AL_RENDERER));
    BE_LOG(L"OpenAL version: %hs\n", alGetString(AL_VERSION));
    BE_LOG(L"OpenAL extensions: %hs\n", alGetString(AL_EXTENSIONS));

    ALCint freq, monoSources, stereoSources, refresh, sync;
    alcGetIntegerv(alDevice, ALC_FREQUENCY, 1, &freq);
    alcGetIntegerv(alDevice, ALC_MONO_SOURCES, 1, &monoSources);
    alcGetIntegerv(alDevice, ALC_STEREO_SOURCES, 1, &stereoSources);
    alcGetIntegerv(alDevice, ALC_REFRESH, 1, &refresh);
    alcGetIntegerv(alDevice, ALC_SYNC, 1, &sync);

    alListenerf(AL_GAIN, 0.99f);
    
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
    //alDopplerFactor(1.0); // 1.2 = exaggerate the pitch shift by 20%
    //alDopplerVelocity(343.0f); // m/s this may need to be scaled at some point

    for (int sourceIndex = 0; sourceIndex < MaxSources; sourceIndex++) {
        ALuint alSourceId;
        alGenSources((ALuint)1, &alSourceId);

        ALenum error = alGetError();
        if (error == AL_NO_ERROR) {
            SoundSource *source = new SoundSource;
            source->alSourceId = alSourceId;
            sources.Append(source);
            freeSources.Append(source);
        } else {
            break;
        }
    }
    
    return true;
}

void SoundSystem::ShutdownDevice() {
    BE_LOG(L"Shutting down OpenAL...\n");

    for (int sourceIndex = 0; sourceIndex < sources.Count(); sourceIndex++) {
        SoundSource *source = sources[sourceIndex];
        alSourceStop(source->alSourceId);
        alDeleteSources(1, &source->alSourceId);
        delete source;
    }

    sources.Clear();
    freeSources.Clear();

    alcMakeContextCurrent(nullptr);
    
    alcDestroyContext(alContext);

    alcCloseDevice(alDevice);
}

void SoundSystem::PlaceListenerInternal(const Vec3 &origin, const Vec3 &forward, const Vec3 &up) {
    ALfloat listenerOri[] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
    ALfloat listenerVel[] = { 0, 0, 0 };

    alListener3f(AL_POSITION, origin.x, origin.y, origin.z);
    alListenerfv(AL_ORIENTATION, listenerOri);
    alListenerfv(AL_VELOCITY, listenerVel);
}

BE_NAMESPACE_END
