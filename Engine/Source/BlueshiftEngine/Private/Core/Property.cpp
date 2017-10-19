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
#include "Core/Property.h"
#include "Core/Object.h"
#include "Math/Math.h"
#include "Core/Lexer.h"

BE_NAMESPACE_BEGIN

const Json::Value PropertyInfo::ToJsonValue(PropertyInfo::Type type, const Variant &var) {
    Json::Value value;

    switch (type) {
    case PropertyInfo::StringType:
        value = var.As<Str>().c_str();
        break;
    case PropertyInfo::FloatType:
        value = var.As<float>();
        break;
    case PropertyInfo::IntType:
    case PropertyInfo::EnumType:
        value = var.As<int>();
        break;
    case PropertyInfo::BoolType:
        value = var.As<bool>();
        break;
    case PropertyInfo::PointType:
        value = var.As<Point>().ToString();
        break;
    case PropertyInfo::RectType:
        value = var.As<Rect>().ToString();
        break;
    case PropertyInfo::Vec2Type:
        value = var.As<Vec2>().ToString();
        break;
    case PropertyInfo::Vec3Type:
        value = var.As<Vec3>().ToString();
        break;
    case PropertyInfo::Vec4Type:
        value = var.As<Vec4>().ToString();
        break;
    case PropertyInfo::Color3Type:
        value = var.As<Color3>().ToString();
        break;
    case PropertyInfo::Color4Type:
        value = var.As<Color4>().ToString();
        break;
    case PropertyInfo::AnglesType:
        value = var.As<Angles>().ToString();
        break;
    case PropertyInfo::Mat3Type:
        value = var.As<Mat3>().ToString();
        break;
    case PropertyInfo::ObjectType:
        value = var.As<Guid>().ToString();
        break;
    default:
        assert(0);
        break;
    }

    return value;
}

Variant PropertyInfo::ToVariant(PropertyInfo::Type type, const char *value) {
    Variant out;

    switch (type) {
    case PropertyInfo::StringType:
        out = Str(value);
        break;
    case PropertyInfo::FloatType:
        out = (float)atof(value);
        break;
    case PropertyInfo::IntType:
    case PropertyInfo::EnumType:
        out = atoi(value);
        break;
    case PropertyInfo::BoolType:
        out = !Str::Cmp(value, "true");
        break;
    case PropertyInfo::PointType: {
        Point point;
        sscanf(value, "%i %i", &point.x, &point.y);
        out = point;
        break; }
    case PropertyInfo::RectType: {
        Rect rect;
        sscanf(value, "%i %i %i %i", &rect.x, &rect.y, &rect.w, &rect.h);
        out = rect;
        break; }
    case PropertyInfo::Vec2Type: {
        Vec2 vec2;
        sscanf(value, "%f %f", &vec2.x, &vec2.y);
        out = vec2;
        break; }
    case PropertyInfo::Vec3Type: {
        Vec3 vec3;
        sscanf(value, "%f %f %f", &vec3.x, &vec3.y, &vec3.z);
        out = vec3;
        break; }
    case PropertyInfo::Vec4Type: {
        Vec4 vec4;
        sscanf(value, "%f %f %f %f", &vec4.x, &vec4.y, &vec4.z, &vec4.w);
        out = vec4;
        break; }
    case PropertyInfo::Color3Type: {
        Color3 color3;
        sscanf(value, "%f %f %f", &color3.r, &color3.g, &color3.b);
        out = color3;
        break; }
    case PropertyInfo::Color4Type: {
        Color4 color4;
        sscanf(value, "%f %f %f %f", &color4.r, &color4.g, &color4.b, &color4.a);
        out = color4;
        break; }
    case PropertyInfo::AnglesType: {
        Angles angles;
        sscanf(value, "%f %f %f", &angles[0], &angles[1], &angles[2]);
        out = angles;
        break; }
    case PropertyInfo::Mat3Type: {
        Mat3 mat3;
        sscanf(value, "%f %f %f %f %f %f %f %f %f", &mat3[0].x, &mat3[0].y, &mat3[0].z, &mat3[1].x, &mat3[1].y, &mat3[1].z, &mat3[2].x, &mat3[2].y, &mat3[2].z);
        out = mat3;
        break; }
    case PropertyInfo::ObjectType: {
        Guid guid = Guid::ParseString(value);
        out = guid;
        break; }
    default:
        assert(0);
        break;
    }

    return out;
}

static PropertyInfo::Type StringToType(const char *s) {
    if (!Str::Cmp(s, "string")) return PropertyInfo::StringType;
    if (!Str::Cmp(s, "float")) return PropertyInfo::FloatType;
    if (!Str::Cmp(s, "int")) return PropertyInfo::IntType;
    if (!Str::Cmp(s, "bool")) return PropertyInfo::BoolType;
    if (!Str::Cmp(s, "point")) return PropertyInfo::PointType;
    if (!Str::Cmp(s, "rect")) return PropertyInfo::RectType;
    if (!Str::Cmp(s, "vec2")) return PropertyInfo::Vec2Type;
    if (!Str::Cmp(s, "vec3")) return PropertyInfo::Vec3Type;
    if (!Str::Cmp(s, "vec4")) return PropertyInfo::Vec4Type;
    if (!Str::Cmp(s, "color3")) return PropertyInfo::Color3Type;
    if (!Str::Cmp(s, "color4")) return PropertyInfo::Color4Type;
    if (!Str::Cmp(s, "angles")) return PropertyInfo::AnglesType;
    if (!Str::Cmp(s, "mat3")) return PropertyInfo::Mat3Type;
    if (!Str::Cmp(s, "enum")) return PropertyInfo::EnumType;
    if (!Str::Cmp(s, "object")) return PropertyInfo::ObjectType;
    return PropertyInfo::BadType;
}

static const Str TypeToString(PropertyInfo::Type type) {
    Str s;
    switch (type) {
    case PropertyInfo::StringType: s = "string"; break;
    case PropertyInfo::FloatType: s = "float"; break;
    case PropertyInfo::IntType: s = "int"; break;
    case PropertyInfo::BoolType: s = "bool"; break;
    case PropertyInfo::PointType: s = "point"; break;
    case PropertyInfo::RectType: s = "rect"; break;
    case PropertyInfo::Vec2Type: s = "vec2"; break;
    case PropertyInfo::Vec3Type: s = "vec3"; break;
    case PropertyInfo::Vec4Type: s = "vec4"; break;
    case PropertyInfo::Color3Type: s = "color3"; break;
    case PropertyInfo::Color4Type: s = "color4"; break;
    case PropertyInfo::AnglesType: s = "angls"; break;
    case PropertyInfo::Mat3Type: s = "mat3"; break;
    case PropertyInfo::EnumType: s = "enum"; break;
    case PropertyInfo::ObjectType: s = "object"; break;
    default: break;
    }
    return s;
}

char *PropertyInfo::ToString() const {
    static char buffer[1024];
    char s1[256] = "";
    char s2[256] = "";
    char s3[256] = "";

    Str::snPrintf(s1, sizeof(s1), "%s(\"%s\") : %s", name.c_str(), label.c_str(), TypeToString(type).c_str());

    if (type == PropertyInfo::EnumType) {
        Str enumSequence;
        for (int i = 0; i < enumeration.Count(); i++) {
            enumSequence += enumeration[i];
            if (i != enumeration.Count() - 1) {
                enumSequence += ";";
            }
        }
        Str::snPrintf(s2, sizeof(s2), " \"%s\"", enumSequence.c_str());
    } else if (type == PropertyInfo::ObjectType) {
        Str::snPrintf(s2, sizeof(s2), " \"%s\"", metaObject->ClassName());
    } else if (type == PropertyInfo::IntType || 
        type == PropertyInfo::FloatType || 
        type == PropertyInfo::Vec2Type || 
        type == PropertyInfo::Vec3Type || 
        type == PropertyInfo::Vec4Type) {

        if (flags & PropertyInfo::Ranged) {
            Str::snPrintf(s2, sizeof(s2), " range %.4f %.4f %.4f", range.minValue, range.maxValue, range.step);
        }
    }

    if (flags & PropertyInfo::Hidden) {
        Str::Append(s3, sizeof(s3), "hidden ");
    }
    if (flags & PropertyInfo::ShaderDefine) {
        Str::Append(s3, sizeof(s3), "shaderDefine ");
    }

    char *lastChar = &s3[Str::Length(s3) - 1];
    assert(*lastChar == ' ');
    *lastChar = '\0';

    Str::snPrintf(buffer, sizeof(buffer), "%s%s = \"%s\" (%s)", s1, s2, defaultValue.c_str(), s3);

    return buffer;
}

bool PropertyInfo::ParseSpec(Lexer &lexer) {
    type = BadType;
    flags = Readable | Writable;
    range = Rangef(0, 0, 1);
    metaObject = nullptr;
    
    if (!lexer.ReadToken(&name, false)) {
        return false;
    }

    if (!lexer.ExpectPunctuation(P_PARENTHESESOPEN)) {
        return false;
    }

    if (!lexer.ExpectTokenType(TokenType::TT_STRING, &label)) {
        return false;
    }

    if (!lexer.ExpectPunctuation(P_PARENTHESESCLOSE)) {
        return false;
    }

    if (!lexer.ExpectPunctuation(P_COLON)) {
        return false;
    }

    Str typeStr;
    if (!lexer.ReadToken(&typeStr, false)) {
        return false;
    }

    type = StringToType(typeStr);

    Str enumSequence, metaObjectName;
    if (type == PropertyInfo::EnumType) {
        if (!lexer.ExpectTokenType(TokenType::TT_STRING, &enumSequence)) {
            return false;
        }
        enumeration.Clear();
        SplitStringIntoList(enumeration, enumSequence, ";");
    } else if (type == PropertyInfo::ObjectType) {
        if (!lexer.ReadToken(&metaObjectName, false)) {
            return false;
        }
        metaObject = Object::GetMetaObject(metaObjectName);
    } else if (type == PropertyInfo::IntType || 
        type == PropertyInfo::FloatType || 
        type == PropertyInfo::Vec2Type || 
        type == PropertyInfo::Vec3Type || 
        type == PropertyInfo::Vec4Type) {
        Str token;
        lexer.ReadToken(&token, false);

        if (token == "range") {
            flags |= PropertyInfo::Ranged;
            range.minValue = lexer.ParseNumber();
            range.maxValue = lexer.ParseNumber();
            range.step = lexer.ParseNumber();
        } else {
            lexer.UnreadToken(&token);
        }
    }

    if (!lexer.ExpectPunctuation(P_ASSIGN)) {
        return false;
    }

    if (!lexer.ExpectTokenType(TokenType::TT_STRING, &defaultValue)) {
        return false;
    }

    Str token;
    lexer.ReadToken(&token, false);
    if (token == "(") {
        while (lexer.ReadToken(&token, false)) {
            if (token == ")") {
                break;
            } else if (token == "hidden") {
                flags |= Hidden;
            } else if (token == "shaderDefine") {
                flags |= ShaderDefine;
            } else {
                return false;
            }
        }
    }
    
    return true;
}

BE_NAMESPACE_END
