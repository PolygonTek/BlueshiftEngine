#include "Precompiled.h"
#include "Renderer/RendererGL.h"
#include "RGLInternal.h"

BE_NAMESPACE_BEGIN

Renderer::Handle RendererGL::FenceSync() {
    GLSync *sync = new GLSync;

    int handle = syncList.FindNull();
    if (handle == -1) {
        handle = syncList.Append(sync);
    } else {
        syncList[handle] = sync;
    }

    sync->sync = gglFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    return (Handle)handle;
}

void RendererGL::DeleteSync(Handle syncHandle) {
    GLSync *sync = syncList[syncHandle];

    gglDeleteSync(sync->sync);

    delete syncList[syncHandle];
    syncList[syncHandle] = nullptr;
}

void RendererGL::WaitSync(Handle syncHandle) {
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
