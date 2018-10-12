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
#include "Scripting/LuaVM.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterMat3x4(LuaCpp::Module &module) {
    LuaCpp::Selector _Mat3x4 = module["Mat3x4"];

    _Mat3x4.SetClass<Mat3x4>();
    _Mat3x4.AddClassCtor<Mat3x4, float, float, float, float, float, float, float, float, float, float, float, float>();
    _Mat3x4.AddClassMembers<Mat3x4>(
        "at", static_cast<Vec4&(Mat3x4::*)(int)>(&Mat3x4::At), // index start from zero
        "assign", static_cast<Mat3x4&(Mat3x4::*)(const Mat3x4&)>(&Mat3x4::operator=),
        "add", &Mat3x4::Add,
        "add_self", &Mat3x4::AddSelf,
        "sub", &Mat3x4::Sub,
        "sub_self", &Mat3x4::SubSelf,
        "mul", &Mat3x4::Mul,
        "mul_self", &Mat3x4::MulSelf,
        "mul_scalar", &Mat3x4::MulScalar,
        "mul_scalar_self", &Mat3x4::MulScalarSelf,
        "mul_vec3", static_cast<Vec3(Mat3x4::*)(const Vec3&)const>(&Mat3x4::MulVec),
        "mul_vec4", static_cast<Vec4(Mat3x4::*)(const Vec4&)const>(&Mat3x4::MulVec),
        "tmul_vec3", static_cast<Vec3(Mat3x4::*)(const Vec3&)const>(&Mat3x4::TransposedMulVec),
        "tmul_vec4", static_cast<Vec4(Mat3x4::*)(const Vec4&)const>(&Mat3x4::TransposedMulVec),
        "equals", static_cast<bool(Mat3x4::*)(const Mat3x4 &, const float)const>(&Mat3x4::Equals),
        "is_identity", &Mat3x4::IsIdentity,
        "set_zero", &Mat3x4::SetZero,
        "set_identity", &Mat3x4::SetIdentity,
        "inverse", &Mat3x4::Inverse,
        "inverse_self", &Mat3x4::InverseSelf,
        "to_mat4", &Mat3x4::ToMat4,
        "to_string", static_cast<const char*(Mat3x4::*)(void)const>(&Mat3x4::ToString)
    );
    _Mat3x4.AddClassMembers<Mat3x4>(
        "__tostring", static_cast<const char*(Mat3x4::*)(void)const>(&Mat3x4::ToString),
        "__unm", static_cast<Mat3x4(Mat3x4::*)(void)const>(&Mat3x4::operator-),
        "__add", &Mat3x4::Add,
        "__sub", &Mat3x4::Sub,
        "__mul", &Mat3x4::Mul,
        "__eq", static_cast<bool(Mat3x4::*)(const Mat3x4&)const>(&Mat3x4::operator==)
    );

    _Mat3x4["zero"] = Mat3x4::zero;
    _Mat3x4["identity"] = Mat3x4::identity;
}

BE_NAMESPACE_END
