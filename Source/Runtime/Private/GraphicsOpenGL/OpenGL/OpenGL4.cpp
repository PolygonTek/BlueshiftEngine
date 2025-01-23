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
#include "OpenGL4.h"

BE_NAMESPACE_BEGIN

const char *OpenGL4::GLSL_VERSION_STRING = "430";

void OpenGL4::Init() {
    OpenGL3::Init();
}

bool OpenGL4::ImageFormatToGLFormat(Image::Format::Enum imageFormat, bool isSRGB, GLenum *glFormat, GLenum *glType, GLenum *glInternal) {
    switch (imageFormat) {
    case Image::Format::RGB_8_ETC2:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB8_ETC2 : GL_COMPRESSED_RGB8_ETC2;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB8_ETC2 : GL_COMPRESSED_RGB8_ETC2;
        return true;
    case Image::Format::RGBA_8_1_ETC2:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 : GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 : GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2;
        return true;
    case Image::Format::RGBA_8_8_ETC2:
        if (glFormat)   *glFormat = isSRGB ? GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC : GL_COMPRESSED_RGBA8_ETC2_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = isSRGB ? GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC : GL_COMPRESSED_RGBA8_ETC2_EAC;
        return true;
    case Image::Format::R_11_EAC:
        if (glFormat)   *glFormat = GL_COMPRESSED_R11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_R11_EAC;
        return true;
    case Image::Format::SignedR_11_EAC:
        if (glFormat)   *glFormat = GL_COMPRESSED_SIGNED_R11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_SIGNED_R11_EAC;
        return true;
    case Image::Format::RG_11_11_EAC:
        if (glFormat)   *glFormat = GL_COMPRESSED_RG11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_RG11_EAC;
        return true;
    case Image::Format::SignedRG_11_11_EAC:
        if (glFormat)   *glFormat = GL_COMPRESSED_SIGNED_RG11_EAC;
        if (glType)     *glType = 0;
        if (glInternal) *glInternal = GL_COMPRESSED_SIGNED_RG11_EAC;
        return true;
    default:
        return OpenGL3::ImageFormatToGLFormat(imageFormat, isSRGB, glFormat, glType, glInternal);
    }

    return false;
}

BE_NAMESPACE_END
