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

    OpenGL 4.3

===============================================================================
*/

#include "OpenGL3.h"

BE_NAMESPACE_BEGIN

class OpenGL4 : public OpenGL3 {
public:
    static const int        GLSL_VERSION = 430;
    static const char *     GLSL_VERSION_STRING;

    static void             Init();

    static void             DrawElementsIndirect(GLenum mode, GLenum type, const void *indirect) { gglDrawElementsIndirect(mode, type, indirect); }
    static void             MultiDrawElementsIndirect(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride) { gglMultiDrawElementsIndirect(mode, type, indirect, drawcount, stride); }
};

BE_NAMESPACE_END
