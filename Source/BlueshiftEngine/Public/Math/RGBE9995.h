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

BE_NAMESPACE_BEGIN

/// 32-bits shared exponent float color.
class RGBE9995 {
public:
    RGBE9995(uint32_t value);
    RGBE9995(float r, float g, float b);

                        operator const uint32_t() const { return value; }
                        operator uint32_t() { return value; }

    void                SetFromColor3(float r, float g, float b) { *this = FromColor3(r, g, b); }

    static RGBE9995     FromColor3(float r, float g, float b);
    
    Color3              ToColor3() const;

    uint32_t            value;
};

BE_INLINE RGBE9995::RGBE9995(uint32_t value) {
    this->value = value;
}

BE_INLINE RGBE9995::RGBE9995(float r, float g, float b) {
    SetFromColor3(r, g, b);
}

BE_NAMESPACE_END
