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

void LuaVM::RegisterMat2(LuaCpp::Module &module) {
    LuaCpp::Selector _Mat2 = module["Mat2"];

    _Mat2.SetClass<Mat2>();
    _Mat2.AddClassCtor<Mat2, float, float, float, float>();
    _Mat2.AddClassMembers<Mat2>(
        "at", static_cast<Vec2&(Mat2::*)(int)>(&Mat2::At), // index start from zero
        "assign", static_cast<Mat2&(Mat2::*)(const Mat2&)>(&Mat2::operator=),
        "add", static_cast<Mat2(Mat2::*)(const Mat2&)const>(&Mat2::operator+),
        "add_self", static_cast<Mat2&(Mat2::*)(const Mat2&)>(&Mat2::operator+=),
        "subtract", static_cast<Mat2(Mat2::*)(const Mat2&)const>(&Mat2::operator-),
        "subtract_self", static_cast<Mat2&(Mat2::*)(const Mat2&)>(&Mat2::operator-=),
        "multiply", static_cast<Mat2(Mat2::*)(const Mat2&)const>(&Mat2::operator*),
        "multiply_self", static_cast<Mat2&(Mat2::*)(const Mat2&)>(&Mat2::operator*=),
        "multiply_comp", static_cast<Mat2(Mat2::*)(const float)const>(&Mat2::operator*),
        "multiply_comp_self", static_cast<Mat2&(Mat2::*)(const float)>(&Mat2::operator*=),
        "multiply_vec2", static_cast<Vec2(Mat2::*)(const Vec2&)const>(&Mat2::operator*),
        "equals", static_cast<bool(Mat2::*)(const Mat2&, const float)const>(&Mat2::Equals),
        "is_identity", &Mat2::IsIdentity,
        "is_symmetric", &Mat2::IsSymmetric,
        "is_diagonal", &Mat2::IsDiagonal,
        "is_singular", &Mat2::IsSingular,
        "set_zero", &Mat2::SetZero,
        "set_identity", &Mat2::SetIdentity,
        "trace", &Mat2::Trace,
        "determinant", &Mat2::Determinant,
        "transpose", &Mat2::Transpose,
        "transpose_self", &Mat2::TransposeSelf,
        "inverse", &Mat2::Inverse,
        "inverse_self", &Mat2::InverseSelf,
        "to_string", static_cast<const char*(Mat2::*)()const>(&Mat2::ToString)
    );
    _Mat2.AddClassMembers<Mat2>(
        "__tostring", static_cast<const char*(Mat2::*)(void)const>(&Mat2::ToString),
        "__unm", static_cast<Mat2(Mat2::*)(void)const>(&Mat2::operator-),
        "__add", &Mat2::Add,
        "__sub", &Mat2::Sub,
        "__mul", &Mat2::Mul,
        "__eq", static_cast<bool(Mat2::*)(const Mat2&)const>(&Mat2::operator==)
    );

    _Mat2["zero"] = Mat2::zero;
    _Mat2["identity"] = Mat2::identity;
}

BE_NAMESPACE_END
