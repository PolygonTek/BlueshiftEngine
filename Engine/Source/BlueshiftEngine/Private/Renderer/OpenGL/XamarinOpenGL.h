#pragma once

/*
===============================================================================

    OpenGL for iOS

===============================================================================
*/

// OpenGL ES Programming Guide for iOS - Adopting OpenGL ES 3.0
// https://developer.apple.com/library/ios/documentation/3DDrawing/Conceptual/OpenGLES_ProgrammingGuide/AdoptingOpenGLES3/AdoptingOpenGLES3.html

#include "OpenGLES3.h"

BE_NAMESPACE_BEGIN

class XamarinOpenGL : public OpenGLES3 {
public:
    static void             Init();
    
    static void             DrawBuffer(GLenum buffer);
    static void             BindDefaultFBO() { gglBindFramebuffer(GL_FRAMEBUFFER, 0); }
};

typedef XamarinOpenGL           OpenGL;

BE_NAMESPACE_END
