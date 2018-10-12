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

void LuaVM::RegisterMat4(LuaCpp::Module &module) {
    LuaCpp::Selector _Mat4 = module["Mat4"];

    _Mat4.SetClass<Mat4>();
    _Mat4.AddClassCtor<Mat4, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float>();
    _Mat4.AddClassMembers<Mat4>(
        "at", static_cast<Vec4&(Mat4::*)(int)>(&Mat4::At), // index start from zero
        "assign", static_cast<Mat4&(Mat4::*)(const Mat4&)>(&Mat4::operator=),
        "add", &Mat4::Add,
        "add_self", &Mat4::AddSelf,
        "sub", &Mat4::Sub,
        "sub_self", &Mat4::SubSelf,
        "mul", static_cast<Mat4(Mat4::*)(const Mat4&)const>(&Mat4::Mul),
        "mul_self", static_cast<Mat4&(Mat4::*)(const Mat4&)>(&Mat4::MulSelf),
        "mul_scalar", &Mat4::MulScalar,
        "mul_scalar_self", &Mat4::MulScalarSelf,
        "mul_vec4", static_cast<Vec4(Mat4::*)(const Vec4&)const>(&Mat4::MulVec),
        "mul_vec3", static_cast<Vec3(Mat4::*)(const Vec3&)const>(&Mat4::MulVec),
        "tmul_vec4", static_cast<Vec4(Mat4::*)(const Vec4&)const>(&Mat4::TransposedMulVec),
        "tmul_vec3", static_cast<Vec3(Mat4::*)(const Vec3&)const>(&Mat4::TransposedMulVec),
        "transform_normal", &Mat4::TransformNormal,
        "equals", static_cast<bool(Mat4::*)(const Mat4&, const float)const>(&Mat4::Equals),
        "is_identity", &Mat4::IsIdentity,
        "is_symmetric", &Mat4::IsSymmetric,
        "is_diagonal", &Mat4::IsDiagonal,
        "is_singular", &Mat4::IsSingular,
        "is_affine", &Mat4::IsAffine,
        "set_zero", &Mat4::SetZero,
        "set_identity", &Mat4::SetIdentity,
        "set_translation", &Mat4::SetTranslation,
        "set_frustum", static_cast<void(Mat4::*)(float, float, float, float, float, float)>(&Mat4::SetFrustum),
        "set_perspective", static_cast<void(Mat4::*)(float, float, float, float)>(&Mat4::SetPerspective),
        "set_ortho", &Mat4::SetOrtho,
        "set_reflect", &Mat4::SetReflect,
        "set_trs", &Mat4::SetTRS,
        "trace", &Mat4::Trace,
        "determinant", &Mat4::Determinant,
        "transpose", &Mat4::Transpose,
        "transpose_self", &Mat4::TransposeSelf,
        "inverse", &Mat4::Inverse,
        "inverse_self", &Mat4::InverseSelf,
        "affine_inverse", &Mat4::AffineInverse,
        "affine_inverse_self", &Mat4::AffineInverseSelf,
        "euclidean_inverse", &Mat4::EuclideanInverse,
        "euclidean_inverse_self", &Mat4::EuclideanInverseSelf,
        "translate", static_cast<Mat4 &(Mat4::*)(float, float, float)>(&Mat4::Translate),
        "scale", static_cast<Mat4 &(Mat4::*)(float, float, float)>(&Mat4::Scale),
        "uniform_scale", &Mat4::UniformScale,
        "to_mat3", &Mat4::ToMat3,
        "to_translation_vec3", &Mat4::ToTranslationVec3,
        "to_string", static_cast<const char*(Mat4::*)(void)const>(&Mat4::ToString)
    );
    _Mat4.AddClassMembers<Mat4>(
        "__tostring", static_cast<const char *(Mat4::*)(void) const>(&Mat4::ToString),
        "__unm", static_cast<Mat4(Mat4::*)(void)const>(&Mat4::operator-),
        "__add", &Mat4::Add,
        "__sub", &Mat4::Sub,
        "__mul", static_cast<Mat4(Mat4::*)(const Mat4 &)const>(&Mat4::Mul),
        "__eq", static_cast<bool(Mat4::*)(const Mat4&)const>(&Mat4::operator==)
    );
    
    _Mat4["zero"] = Mat4::zero;
    _Mat4["identity"] = Mat4::identity;
}

BE_NAMESPACE_END
