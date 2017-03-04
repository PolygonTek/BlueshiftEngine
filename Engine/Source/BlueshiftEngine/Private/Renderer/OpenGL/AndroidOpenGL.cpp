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
