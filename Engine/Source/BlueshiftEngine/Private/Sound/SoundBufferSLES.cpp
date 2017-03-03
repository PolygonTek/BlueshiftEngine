#include "Precompiled.h"
#include "Core/Heap.h"
#include "Simd/Simd.h"
#include "Sound/SoundSystem.h"

BE_NAMESPACE_BEGIN

SoundBuffer::SoundBuffer() {
    duplicated = false;
    bufferSize = 0;
    bufferCount = 0;
}

SoundBuffer::~SoundBuffer() {
    Purge();
}

void SoundBuffer::Purge() {
    if (!duplicated) {
        for (int i = 0; i < bufferCount; i++) {
            Mem_AlignedFree(buffers[i]);
        }
        bufferCount = 0;
    }
}

bool SoundBuffer::CreateStaticBuffer(int numChannels, int bitsWidth, int sampleRates, int size, const byte *data) {
    Purge();

    duplicated = false;
       
    bufferSize = size;
    bufferCount = 1;
    buffers[0] = (byte *)Mem_Alloc16(bufferSize);
    simdProcessor->Memcpy(buffers[0], data, bufferSize);
    
    return true;
}

bool SoundBuffer::CreateStreamBuffer(int numChannels, int bitsWidth, int sampleRates) {
    duplicated = false;

    bufferSize = numChannels * sampleRates * (bitsWidth >> 3) * StreamBufferSeconds;
    bufferCount = MaxStreamBuffers;

    // The buffers that are queued in a player object are used in place and
    // are not required to be copied by the device, although this may be 
    // implementation dependent.T he application developer should be aware 
    // that modifying the content of a buffer after it has been queued is 
    // undefined and can cause audio corruption.
    for (int i = 0; i < bufferCount; i++) {
        buffers[i] = (byte *)Mem_Alloc16(bufferSize);
    }

    return true;
}

bool SoundBuffer::DuplicateBuffer(SoundBuffer *originalSoundBuffer) {
    duplicated = true;

    bufferSize = originalSoundBuffer->bufferSize;
    bufferCount = originalSoundBuffer->bufferCount;
    for (int i = 0; i < bufferCount; i++) {
        buffers[i] = originalSoundBuffer->buffers[i];
    }

    return true;
}

BE_NAMESPACE_END