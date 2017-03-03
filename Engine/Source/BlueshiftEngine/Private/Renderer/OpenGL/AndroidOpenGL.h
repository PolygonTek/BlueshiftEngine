#pragma once

/*
===============================================================================

    OpenGL for Android

===============================================================================
*/

#include "OpenGLES3.h"

BE_NAMESPACE_BEGIN

class AndroidOpenGL : public OpenGLES3 {
public:
    static void             Init();
    
    static void             DrawBuffer(GLenum buffer);
    static void             BindDefaultFBO() { gglBindFramebuffer(GL_FRAMEBUFFER, 0); }
};

typedef AndroidOpenGL       OpenGL;

BE_NAMESPACE_END
