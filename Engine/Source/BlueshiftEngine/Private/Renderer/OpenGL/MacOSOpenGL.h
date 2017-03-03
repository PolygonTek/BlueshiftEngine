#pragma once

/*
===============================================================================

    OpenGL for OS X

===============================================================================
*/

#include "OpenGL3.h"

BE_NAMESPACE_BEGIN

class MacOSOpenGL : public OpenGL3 {
public:
    static bool             SupportPolygonMode() { return true; }
    static bool             SupportPackedFloat() { return true; }
    static bool             SupportDepthBufferFloat() { return true; }
    static bool             SupportPixelBufferObject() { return true; }
    static bool             SupportTextureRectangle() { return true; }
    static bool             SupportTextureArray() { return true; }
    static bool             SupportTextureBufferObject() { return true; }
    static bool             SupportTextureCompressionLATC() { return true; }
};

typedef MacOSOpenGL         OpenGL;

BE_NAMESPACE_END
