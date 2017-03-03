#include "Precompiled.h"
#include "IOSOpenGL.h"

BE_NAMESPACE_BEGIN

void IOSOpenGL::Init() {
    OpenGLES3::Init();
}

void IOSOpenGL::DrawBuffer(GLenum buffer) {
    if (buffer == GL_BACK) {
        buffer = GL_COLOR_ATTACHMENT0;
    }
    gglDrawBuffers(1, &buffer);
}

BE_NAMESPACE_END
