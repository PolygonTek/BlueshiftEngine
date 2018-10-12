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

void LuaVM::RegisterMath(LuaCpp::Module &module) {
    LuaCpp::Selector _Math = module["Math"];

    _Math["pi"] = Math::Pi;
    _Math["e"] = Math::E;
    _Math["inf"] = Math::Infinity;
    _Math["inv_sqrt"].SetFunc(Math::InvSqrt);
    _Math["sqrt"].SetFunc(Math::Sqrt);
    _Math["sin"].SetFunc(Math::Sin);
    _Math["cos"].SetFunc(Math::Cos);
    _Math["tan"].SetFunc(Math::Tan);
    _Math["asin"].SetFunc(Math::ASin);
    _Math["acos"].SetFunc(Math::ACos);
    _Math["atan"].SetFunc(static_cast<float(*)(float)>(&Math::ATan));
    _Math["atan2"].SetFunc(static_cast<float(*)(float, float)>(&Math::ATan));
    _Math["pow"].SetFunc(Math::Pow);
    _Math["exp"].SetFunc(Math::Exp);
    _Math["log"].SetFunc(static_cast<float (*)(float)>(&Math::Log));
    _Math["is_power_of_two"].SetFunc(Math::IsPowerOfTwo);
    _Math["abs"].SetFunc(Math::Abs);
    _Math["fabs"].SetFunc(Math::Fabs);
    _Math["floor"].SetFunc(Math::Floor);
    _Math["ceil"].SetFunc(Math::Ceil);
    _Math["rint"].SetFunc(Math::Rint);
    _Math["ftoi"].SetFunc(Math::Ftoi);
    _Math["ftob"].SetFunc(Math::Ftob);
    _Math["factorial"].SetFunc(Math::Factorial);
    _Math["to_radian"].SetFunc([](float degree) -> float { return DEG2RAD(degree); });
    _Math["to_degree"].SetFunc([](float radian) -> float { return RAD2DEG(radian); });
    _Math["angle_normalize_360"].SetFunc(Math::AngleNormalize360);
    _Math["angle_normalize_180"].SetFunc(Math::AngleNormalize180);
    _Math["angle_delta"].SetFunc(Math::AngleDelta);
    _Math["random"].SetFunc(Math::Random);
    _Math["lerp"].SetFunc(Lerp<float>);
}

BE_NAMESPACE_END
