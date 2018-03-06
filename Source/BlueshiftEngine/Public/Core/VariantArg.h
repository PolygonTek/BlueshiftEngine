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

#include "Core/Str.h"
#include "Core/WStr.h"

BE_NAMESPACE_BEGIN

class Point;
class Rect;
class Vec3;
class Mat3;
class Mat4;
class Guid;

class BE_API VariantArg {
public:
    static const char VoidType = '\0';
    static const char IntType = 'i';
    static const char FloatType = 'f';
    static const char PointerType = 'a';
    static const char PointType = 'p';
    static const char RectType = 'r';
    static const char Vec3Type = 'v';
    static const char Mat3x3Type = 'm';
    static const char Mat4x4Type = 'M';
    static const char StringType = 's';
    static const char WStringType = 'w';
    static const char GuidType = 'g';

    VariantArg() { type = IntType; pointer = 0; };
    VariantArg(const int data) { type = IntType; intValue = data; };
    VariantArg(const float data) { type = FloatType; floatValue = data; };
    VariantArg(const void *data) { type = PointerType; pointer = reinterpret_cast<intptr_t>(data); };
    VariantArg(const char *data) { type = StringType; pointer = reinterpret_cast<intptr_t>(data); };
    VariantArg(const wchar_t *data) { type = WStringType; pointer = reinterpret_cast<intptr_t>(data); };
    VariantArg(const Point &data) { type = PointType; pointer = reinterpret_cast<intptr_t>(&data); };
    VariantArg(const Rect &data) { type = RectType; pointer = reinterpret_cast<intptr_t>(&data); };
    VariantArg(const Vec3 &data) { type = Vec3Type; pointer = reinterpret_cast<intptr_t>(&data); };
    VariantArg(const Mat3 &data) { type = Mat3x3Type; pointer = reinterpret_cast<intptr_t>(&data); };
    VariantArg(const Mat4 &data) { type = Mat4x4Type; pointer = reinterpret_cast<intptr_t>(&data); };
    VariantArg(const Guid &data) { type = GuidType; pointer = reinterpret_cast<intptr_t>(&data); }
    VariantArg(const Str &data) { type = StringType; pointer = reinterpret_cast<intptr_t>(data.c_str()); };
    VariantArg(const WStr &data) { type = WStringType; pointer = reinterpret_cast<intptr_t>(data.c_str()); };

    int                     type;
    union {
        intptr_t            pointer;
        int                 intValue;
        float               floatValue;
    };
};

BE_NAMESPACE_END
