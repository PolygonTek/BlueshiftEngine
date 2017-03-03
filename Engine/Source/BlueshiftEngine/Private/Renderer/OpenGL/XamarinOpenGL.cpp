#include "Precompiled.h"
#include "XamarinOpenGL.h"

BE_NAMESPACE_BEGIN

void XamarinOpenGL::Init() {
    OpenGLES3::Init();
}

void XamarinOpenGL::DrawBuffer(GLenum buffer) {
    if (buffer == GL_BACK) {
        buffer = GL_COLOR_ATTACHMENT0;
    }
    gglDrawBuffers(1, &buffer);
}

BE_NAMESPACE_END
