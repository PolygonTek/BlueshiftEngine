#include "Precompiled.h"
#include "Core/Vec4Color.h"

BE_NAMESPACE_BEGIN

const Color4 Vec4Color::table[16] = {
    Color4(0.7f, 0.7f, 0.7f, 1.0f),   // LIGHT GRAY
    Color4(1.0f, 0.0f, 0.0f, 1.0f),   // RED
    Color4(0.0f, 1.0f, 0.0f, 1.0f),   // GREEN
    Color4(1.0f, 1.0f, 0.0f, 1.0f),   // YELLOW
    Color4(0.0f, 0.0f, 1.0f, 1.0f),   // BLUE
    Color4(0.0f, 1.0f, 1.0f, 1.0f),   // CYAN
    Color4(1.0f, 0.0f, 1.0f, 1.0f),   // MAGENTA
    Color4(1.0f, 1.0f, 1.0f, 1.0f),   // WHITE
    Color4(0.5f, 0.5f, 0.5f, 1.0f),   // GRAY
    Color4(0.0f, 0.0f, 0.0f, 1.0f),   // BLACK
    Color4(1.0f, 0.5f, 0.0f, 1.0f),   // ORANGE
    Color4(1.0f, 0.0f, 0.5f, 1.0f),   // PINK
    Color4(0.5f, 1.0f, 0.0f, 1.0f),   // LAWN
    Color4(0.0f, 1.0f, 0.5f, 1.0f),   // MINT
    Color4(0.5f, 0.0f, 1.0f, 1.0f),   // VIOLET
    Color4(0.3f, 0.5f, 0.6f, 1.0f)    // TEAL
};

BE_NAMESPACE_END
