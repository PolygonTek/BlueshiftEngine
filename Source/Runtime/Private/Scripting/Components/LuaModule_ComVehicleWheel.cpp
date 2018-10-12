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
#include "Components/ComVehicleWheel.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterVehicleWheelComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComVehicleWheel = module["ComVehicleWheel"];

    _ComVehicleWheel.SetClass<ComVehicleWheel>(module["Component"]);
    _ComVehicleWheel.AddClassMembers<ComVehicleWheel>(
        "local_angles", &ComVehicleWheel::GetLocalAngles,
        "set_local_angles", &ComVehicleWheel::SetLocalAngles,
        "local_origin", &ComVehicleWheel::GetLocalOrigin,
        "set_local_origin", &ComVehicleWheel::SetLocalOrigin,
        "steering_angle", &ComVehicleWheel::GetSteeringAngle,
        "set_steering_angle", &ComVehicleWheel::SetSteeringAngle,
        "torque", &ComVehicleWheel::GetTorque,
        "set_torque", &ComVehicleWheel::SetTorque,
        "braking_torque", &ComVehicleWheel::GetBrakingTorque,
        "set_braking_torque", &ComVehicleWheel::SetBrakingTorque,
        "skid_info", &ComVehicleWheel::GetSkidInfo,
        "suspension_relative_velocity", &ComVehicleWheel::GetSuspensionRelativeVelocity);

    _ComVehicleWheel["meta_object"] = ComVehicleWheel::metaObject;
}

BE_NAMESPACE_END
