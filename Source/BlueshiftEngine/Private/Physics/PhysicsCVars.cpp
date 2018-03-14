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
#include "Core/CVars.h"

BE_NAMESPACE_BEGIN

CVAR(physics_enable, L"1", CVar::Bool, L"");
CVAR(physics_showWireframe, L"0", CVar::Integer, L"show debug wireframe in physics engine");
CVAR(physics_showAABB, L"0", CVar::Integer, L"show debug AABB in physics engine");
CVAR(physics_showContactPoints, L"0", CVar::Integer, L"");
CVAR(physics_showNormals, L"0", CVar::Integer, L"");
CVAR(physics_showConstraints, L"0", CVar::Integer, L"");
CVAR(physics_noDeactivation, L"0", CVar::Bool, L"");
CVAR(physics_enableCCD, L"1", CVar::Bool, L"");

BE_NAMESPACE_END
