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
#include "Script/LuaVM.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterMat3(LuaCpp::Module &module) {
    LuaCpp::Selector _Mat3 = module["Mat3"];

    _Mat3.SetClass<Mat3>();
    _Mat3.AddClassCtor<Mat3, float, float, float, float, float, float, float, float, float>();
    _Mat3.AddClassMembers<Mat3>(
        "at", static_cast<Vec3&(Mat3::*)(int)>(&Mat3::At), // index start from zero
        "assign", static_cast<Mat3&(Mat3::*)(const Mat3&)>(&Mat3::operator=),
        "add", &Mat3::Add,
        "add_self", &Mat3::AddSelf,
        "sub", &Mat3::Sub,
        "sub_self", &Mat3::SubSelf,
        "mul", &Mat3::Mul,
        "mul_self", &Mat3::MulSelf,
        "mul_scalar", &Mat3::MulScalar,
        "mul_scalar_self", &Mat3::MulScalarSelf,
        "mul_vec", static_cast<Vec3(Mat3::*)(const Vec3&)const>(&Mat3::MulVec),
        "tmul", &Mat3::TransposedMul,
        "tmul_vec", static_cast<Vec3(Mat3::*)(const Vec3&)const>(&Mat3::TransposedMulVec),
        "equals", static_cast<bool(Mat3::*)(const Mat3 &, const float)const>(&Mat3::Equals),
        "is_identity", &Mat3::IsIdentity,
        "is_symmetric", &Mat3::IsSymmetric,
        "is_diagonal", &Mat3::IsDiagonal,
        "is_singular", &Mat3::IsSingular,
        "set_zero", &Mat3::SetZero,
        "set_identity", &Mat3::SetIdentity,
        "fix_degeneracies", &Mat3::FixDegeneracies,
        "fix_denormals", &Mat3::FixDenormals,
        "trace", &Mat3::Trace,
        "determinant", &Mat3::Determinant,
        "transpose", &Mat3::Transpose,
        "transpose_self", &Mat3::TransposeSelf,
        "inverse", &Mat3::Inverse,
        "inverse_self", &Mat3::InverseSelf,
        "orthonormalize", &Mat3::OrthoNormalize,
        "orthonormalize_self", &Mat3::OrthoNormalizeSelf,
        "rotate_x", &Mat3::RotateX,
        "rotate_y", &Mat3::RotateY,
        "rotate_z", &Mat3::RotateZ,
        "rotate", &Mat3::Rotate,
        "scale", &Mat3::Scale,
        "to_angles", &Mat3::ToAngles,
        "to_rotation", &Mat3::ToRotation,
        "to_quat", &Mat3::ToQuat,
        "to_mat4", &Mat3::ToMat4,
        "to_string", static_cast<const char*(Mat3::*)(void)const>(&Mat3::ToString)
    );
    _Mat3.AddClassMembers<Mat3>(
        "__tostring", static_cast<const char*(Mat3::*)(void)const>(&Mat3::ToString),
        "__unm", static_cast<Mat3(Mat3::*)(void)const>(&Mat3::operator-),
        "__add", &Mat3::Add,
        "__sub", &Mat3::Sub,
        "__mul", &Mat3::Mul,
        "__eq", static_cast<bool(Mat3::*)(const Mat3&)const>(&Mat3::operator==)
    );

    _Mat3["zero"] = Mat3::zero;
    _Mat3["identity"] = Mat3::identity;
}

BE_NAMESPACE_END
