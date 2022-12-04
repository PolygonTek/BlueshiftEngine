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
#include "Game/CastResult.h"
#include "Components/Physics/ComSensor.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterSensorComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComSensor = module["ComSensor"];

    _ComSensor.SetClass<ComSensor>(module["Component"]);

    _ComSensor["meta_object"] = ComSensor::metaObject;

    _ComSensor["from_cast_result"].SetFunc(&GetSensorFromCastResult);
}

BE_NAMESPACE_END
