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

const Json::Value PropertySpec::ToJsonValue(PropertySpec::Type type, const Variant &var) {
    Json::Value value;

    switch (type) {
    case PropertySpec::StringType:
        value = var.As<Str>().c_str();
        break;
    case PropertySpec::FloatType:
        value = var.As<float>();
        break;
    case PropertySpec::IntType:
    case PropertySpec::EnumType:
        value = var.As<int>();
        break;
    case PropertySpec::ObjectType:
        value = var.As<Guid>().ToString();
        break;
    case PropertySpec::BoolType:
        value = var.As<bool>();
        break;
    case PropertySpec::PointType:
        value = var.As<Point>().ToString();
        break;
    case PropertySpec::RectType:
        value = var.As<Rect>().ToString();
        break;
    case PropertySpec::Vec2Type:
        value = var.As<Vec2>().ToString();
        break;
    case PropertySpec::Vec3Type:
    case PropertySpec::Color3Type:
        value = var.As<Vec3>().ToString();
        break;
    case PropertySpec::Vec4Type:
    case PropertySpec::Color4Type:
        value = var.As<Vec4>().ToString();
        break;
    case PropertySpec::AnglesType:
        value = var.As<Angles>().ToString();
        break;
    case PropertySpec::Mat3Type:
        value = var.As<Mat3>().ToString();
        break;
    default:
        assert(0);
        break;
    }

    return value;
}

const Variant PropertySpec::ToVariant(PropertySpec::Type type, const char *value) {
    Variant out;

    switch (type) {
    case PropertySpec::StringType:
        out = Str(value);
        break;
    case PropertySpec::FloatType:
        out = (float)atof(value);
        break;
    case PropertySpec::IntType:
    case PropertySpec::EnumType:
        out = atoi(value);
        break;
    case PropertySpec::ObjectType: {
        Guid guid = Guid::ParseString(value);
        out = guid;
        break; }
    case PropertySpec::BoolType:
        out = !Str::Cmp(value, "true");
        break;
    case PropertySpec::PointType: {
        Point point;
        sscanf(value, "%i %i", &point.x, &point.y);
        out = point;
        break; }
    case PropertySpec::RectType: {
        Rect rect;
        sscanf(value, "%i %i %i %i", &rect.x, &rect.y, &rect.w, &rect.h);
        out = rect;
        break; }
    case PropertySpec::Vec2Type: {
        Vec2 vec2;
        sscanf(value, "%f %f", &vec2.x, &vec2.y);
        out = vec2;
        break; }
    case PropertySpec::Vec3Type: 
    case PropertySpec::Color3Type: {
        Vec3 vec3;
        sscanf(value, "%f %f %f", &vec3.x, &vec3.y, &vec3.z);
        out = vec3;
        break; }
    case PropertySpec::Vec4Type: 
    case PropertySpec::Color4Type: {
        Vec4 vec4;
        sscanf(value, "%f %f %f %f", &vec4.x, &vec4.y, &vec4.z, &vec4.w);
        out = vec4;
        break; }
    case PropertySpec::AnglesType: {
        Angles angles;
        sscanf(value, "%f %f %f", &angles[0], &angles[1], &angles[2]);
        out = angles;
        break; }
    case PropertySpec::Mat3Type: {
        Mat3 mat3;
        sscanf(value, "%f %f %f %f %f %f %f %f %f", &mat3[0].x, &mat3[0].y, &mat3[0].z, &mat3[1].x, &mat3[1].y, &mat3[1].z, &mat3[2].x, &mat3[2].y, &mat3[2].z);
        out = mat3;
        break; }
    default:
        assert(0);
        break;
    }

    return out;
}

static PropertySpec::Type StringToType(const char *s) {
    if (!Str::Cmp(s, "string")) return PropertySpec::StringType;
    if (!Str::Cmp(s, "float")) return PropertySpec::FloatType;
    if (!Str::Cmp(s, "int")) return PropertySpec::IntType;
    if (!Str::Cmp(s, "bool")) return PropertySpec::BoolType;
    if (!Str::Cmp(s, "point")) return PropertySpec::PointType;
    if (!Str::Cmp(s, "rect")) return PropertySpec::RectType;
    if (!Str::Cmp(s, "vec2")) return PropertySpec::Vec2Type;
    if (!Str::Cmp(s, "vec3")) return PropertySpec::Vec3Type;
    if (!Str::Cmp(s, "vec4")) return PropertySpec::Vec4Type;
    if (!Str::Cmp(s, "color3")) return PropertySpec::Color3Type;
    if (!Str::Cmp(s, "color4")) return PropertySpec::Color4Type;
    if (!Str::Cmp(s, "angles")) return PropertySpec::AnglesType;
    if (!Str::Cmp(s, "mat3")) return PropertySpec::Mat3Type;
    if (!Str::Cmp(s, "enum")) return PropertySpec::EnumType;
    if (!Str::Cmp(s, "object")) return PropertySpec::ObjectType;
    return PropertySpec::BadType;
}

static const Str TypeToString(PropertySpec::Type type) {
    Str s;
    switch (type) {
    case PropertySpec::StringType: s = "string"; break;
    case PropertySpec::FloatType: s = "float"; break;
    case PropertySpec::IntType: s = "int"; break;
    case PropertySpec::BoolType: s = "bool"; break;
    case PropertySpec::PointType: s = "point"; break;
    case PropertySpec::RectType: s = "rect"; break;
    case PropertySpec::Vec2Type: s = "vec2"; break;
    case PropertySpec::Vec3Type: s = "vec3"; break;
    case PropertySpec::Vec4Type: s = "vec4"; break;
    case PropertySpec::Color3Type: s = "color3"; break;
    case PropertySpec::Color4Type: s = "color4"; break;
    case PropertySpec::AnglesType: s = "angls"; break;
    case PropertySpec::Mat3Type: s = "mat3"; break;
    case PropertySpec::EnumType: s = "enum"; break;
    case PropertySpec::ObjectType: s = "object"; break;
    default: break;
    }
    return s;
}

char *PropertySpec::ToString() const {
    static char buffer[1024];
    char s1[256] = "";
    char s2[256] = "";
    char s3[256] = "";

    Str::snPrintf(s1, sizeof(s1), "%s(\"%s\") : %s", name.c_str(), label.c_str(), TypeToString(type).c_str());

    if (type == PropertySpec::EnumType) {
        Str enumSequence;
        for (int i = 0; i < enumeration.Count(); i++) {
            enumSequence += enumeration[i];
            if (i != enumeration.Count() - 1) {
                enumSequence += ";";
            }
        }
        Str::snPrintf(s2, sizeof(s2), " \"%s\"", enumSequence.c_str());
    } else if (type == PropertySpec::ObjectType) {
        Str::snPrintf(s2, sizeof(s2), " \"%s\"", metaObject->ClassName());
    } else if (type == PropertySpec::IntType || 
        type == PropertySpec::FloatType || 
        type == PropertySpec::Vec2Type || 
        type == PropertySpec::Vec3Type || 
        type == PropertySpec::Vec3Type || 
        type == PropertySpec::Vec4Type) {

        if (flags & PropertySpec::Ranged) {
            Str::snPrintf(s2, sizeof(s2), " range %.4f %.4f %.4f", range.minValue, range.maxValue, range.step);
        }
    }

    if (flags & PropertySpec::Hidden) {
        Str::Append(s3, sizeof(s3), "hidden ");
    }
    if (flags & PropertySpec::ShaderDefine) {
        Str::Append(s3, sizeof(s3), "shaderDefine ");
    }

    char *lastChar = &s3[Str::Length(s3) - 1];
    assert(*lastChar == ' ');
    *lastChar = '\0';

    Str::snPrintf(buffer, sizeof(buffer), "%s%s = \"%s\" (%s)", s1, s2, defaultValue.c_str(), s3);

    return buffer;
}

bool PropertySpec::ParseSpec(Lexer &lexer) {
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
    if (type == PropertySpec::EnumType) {
        if (!lexer.ExpectTokenType(TokenType::TT_STRING, &enumSequence)) {
            return false;
        }
        enumeration.Clear();
        SplitStringIntoList(enumeration, enumSequence, ";");
    } else if (type == PropertySpec::ObjectType) {
        if (!lexer.ReadToken(&metaObjectName, false)) {
            return false;
        }
        metaObject = Object::GetMetaObject(metaObjectName);
    } else if (type == PropertySpec::IntType || 
        type == PropertySpec::FloatType || 
        type == PropertySpec::Vec2Type || 
        type == PropertySpec::Vec3Type || 
        type == PropertySpec::Vec3Type || 
        type == PropertySpec::Vec4Type) {
        Str token;
        lexer.ReadToken(&token, false);

        if (token == "range") {
            flags |= PropertySpec::Ranged;
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
