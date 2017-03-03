#include "Precompiled.h"
#include "AndroidOpenGL.h"

BE_NAMESPACE_BEGIN

void AndroidOpenGL::Init() {
    OpenGLES3::Init();
}

void AndroidOpenGL::DrawBuffer(GLenum buffer) {
    if (buffer == GL_BACK) {
        buffer = GL_COLOR_ATTACHMENT0;
    }
    gglDrawBuffers(1, &buffer);
}

BE_NAMESPACE_END
