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

CVAR(physics_enable, "1", CVar::Flag::Bool, "");
CVAR(physics_showWireframe, "0", CVar::Flag::Integer, "show debug wireframe in physics engine");
CVAR(physics_showAABB, "0", CVar::Flag::Integer, "show debug AABB in physics engine");
CVAR(physics_showContactPoints, "0", CVar::Flag::Integer, "");
CVAR(physics_showNormals, "0", CVar::Flag::Integer, "");
CVAR(physics_showConstraints, "0", CVar::Flag::Integer, "");
CVAR(physics_noDeactivation, "0", CVar::Flag::Bool, "");
CVAR(physics_enableCCD, "1", CVar::Flag::Bool, "");

BE_NAMESPACE_END
