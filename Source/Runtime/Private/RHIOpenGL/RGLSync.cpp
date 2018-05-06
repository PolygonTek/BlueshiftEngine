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
#include "RHI/RHIOpenGL.h"
#include "RGLInternal.h"

BE_NAMESPACE_BEGIN

RHI::Handle OpenGLRHI::CreateSync() {
    GLSync *sync = new GLSync;

    int handle = syncList.FindNull();
    if (handle == -1) {
        handle = syncList.Append(sync);
    } else {
        syncList[handle] = sync;
    }

    sync->sync = nullptr;

    return (Handle)handle;
}

void OpenGLRHI::DestroySync(Handle syncHandle) {
    GLSync *sync = syncList[syncHandle];

    if (gglIsSync(sync->sync)) {
        gglDeleteSync(sync->sync);
        sync->sync = nullptr;
    }

    delete syncList[syncHandle];
    syncList[syncHandle] = nullptr;
}

bool OpenGLRHI::IsSync(Handle syncHandle) const {
    GLSync *sync = syncList[syncHandle];

    if (syncHandle == NullSync || !sync->sync || !gglIsSync(sync->sync)) {
        return false;
    }
    return true;
}

void OpenGLRHI::FenceSync(Handle syncHandle) {
    GLSync *sync = syncList[syncHandle];

    sync->sync = gglFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void OpenGLRHI::DeleteSync(Handle syncHandle) {
    GLSync *sync = syncList[syncHandle];

    gglDeleteSync(sync->sync);
}

void OpenGLRHI::WaitSync(Handle syncHandle) {
    if (syncHandle == NullSync) {
        return;
    }

    GLSync *sync = syncList[syncHandle];

    while (1) {
        GLenum result = gglClientWaitSync(sync->sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
        if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
            return;
        }
    }
}

BE_NAMESPACE_END
