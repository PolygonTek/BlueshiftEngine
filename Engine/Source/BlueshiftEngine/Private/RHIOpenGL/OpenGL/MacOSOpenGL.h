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
