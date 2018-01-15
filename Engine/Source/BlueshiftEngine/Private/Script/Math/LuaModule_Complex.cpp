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

void LuaVM::RegisterComplex(LuaCpp::Module &module) {
    LuaCpp::Selector _Complex2 = module["Complex"];

    _Complex2.SetClass<Complex>();
    _Complex2.AddClassCtor<Complex, float, float>();
    _Complex2.AddClassMembers<Complex>(
        "re", &Complex::re,
        "im", &Complex::im,
        "element", static_cast<float&(Complex::*)(int)>(&Complex::operator[]), // index start from zero
        "assign", static_cast<Complex&(Complex::*)(const Complex&)>(&Complex::operator=),
        "set", &Complex::Set,
        "set_zero", &Complex::SetZero,
        "equals", static_cast<bool(Complex::*)(const Complex&, const float)const>(&Complex::Equals),
        "to_string", static_cast<const char*(Complex::*)()const>(&Complex::ToString),
        "add", static_cast<Complex(Complex::*)(const Complex&)const>(&Complex::operator+),
        "add_self", static_cast<Complex&(Complex::*)(const Complex&)>(&Complex::operator+=),
        "subtract", static_cast<Complex(Complex::*)(const Complex&)const>(&Complex::operator-),
        "subtract_self", static_cast<Complex&(Complex::*)(const Complex&)>(&Complex::operator-=),
        "multiply", static_cast<Complex(Complex::*)(const Complex&)const>(&Complex::operator*),
        "multiply_self", static_cast<Complex&(Complex::*)(const Complex&)>(&Complex::operator*=),
        "divide", static_cast<Complex(Complex::*)(const Complex&)const>(&Complex::operator/),
        "divide_self", static_cast<Complex&(Complex::*)(const Complex&)>(&Complex::operator/=),
        "conjugate", &Complex::Conjugate,
        "reciprocal", &Complex::Reciprocal,
        "sqrt", &Complex::Sqrt,
        "abs", &Complex::Abs
    );
    _Complex2.AddClassMembers<Complex>(
        "__tostring", static_cast<const char*(Complex::*)(void)const>(&Complex::ToString),
        "__unm", static_cast<Complex(Complex::*)(void)const>(&Complex::operator-),
        "__add", static_cast<Complex(Complex::*)(const Complex&)const>(&Complex::operator+),
        "__sub", static_cast<Complex(Complex::*)(const Complex&)const>(&Complex::operator-),
        "__mul", static_cast<Complex(Complex::*)(const Complex&)const>(&Complex::operator*),
        "__div", static_cast<Complex(Complex::*)(const Complex&)const>(&Complex::operator/),
        "__eq", static_cast<bool(Complex::*)(const Complex&)const>(&Complex::operator==)
    );

    _Complex2["origin"] = Complex::origin;
    _Complex2["zero"] = Complex::zero;
}

BE_NAMESPACE_END
