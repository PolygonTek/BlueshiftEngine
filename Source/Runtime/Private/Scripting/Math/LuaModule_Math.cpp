// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
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
    _Math["big_number"] = Math::BigNumber;
    _Math["inv_sqrt"].SetFunc(static_cast<float (*)(float)>(&Math::InvSqrt));
    _Math["sqrt"].SetFunc(static_cast<float (*)(float)>(&Math::Sqrt));
    _Math["sin"].SetFunc(static_cast<float (*)(float)>(&Math::Sin));
    _Math["cos"].SetFunc(static_cast<float (*)(float)>(&Math::Cos));
    _Math["tan"].SetFunc(static_cast<float (*)(float)>(&Math::Tan));
    _Math["asin"].SetFunc(static_cast<float (*)(float)>(&Math::ASin));
    _Math["acos"].SetFunc(static_cast<float (*)(float)>(&Math::ACos));
    _Math["atan"].SetFunc(static_cast<float(*)(float)>(&Math::ATan));
    _Math["atan2"].SetFunc(static_cast<float(*)(float, float)>(&Math::ATan2));
    _Math["exp"].SetFunc(static_cast<float (*)(float)>(&Math::Exp));
    _Math["log"].SetFunc(static_cast<float (*)(float)>(&Math::Log));
    _Math["log2"].SetFunc(static_cast<float (*)(float)>(&Math::Log2));
    _Math["log10"].SetFunc(static_cast<float (*)(float)>(&Math::Log10));
    _Math["pow"].SetFunc(static_cast<float (*)(float, float)>(&Math::Pow));
    _Math["is_power_of_two"].SetFunc(Math::IsPowerOfTwo);
    _Math["abs"].SetFunc(Math::Abs);
    _Math["fabs"].SetFunc(Math::Fabs);
    _Math["floor"].SetFunc(Math::Floor);
    _Math["ceil"].SetFunc(Math::Ceil);
    _Math["round"].SetFunc(Math::Round);
    _Math["ftoi"].SetFunc(Math::Ftoi);
    _Math["ftob"].SetFunc(Math::Ftob);
    _Math["factorial"].SetFunc(Math::Factorial);
    _Math["to_radian"].SetFunc([](float degree) -> float { return DEG2RAD(degree); });
    _Math["to_degree"].SetFunc([](float radian) -> float { return RAD2DEG(radian); });
    _Math["angle_normalize_360"].SetFunc(Math::AngleNormalize360);
    _Math["angle_normalize_180"].SetFunc(Math::AngleNormalize180);
    _Math["angle_delta"].SetFunc(Math::AngleDelta);
    _Math["random"].SetFunc(static_cast<float (*)(float, float)>(&RNG::GetRandom));
    _Math["lerp"].SetFunc(Math::Lerp<float>);
}

BE_NAMESPACE_END
