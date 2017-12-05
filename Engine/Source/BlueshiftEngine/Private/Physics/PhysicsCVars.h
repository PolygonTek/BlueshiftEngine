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

#pragma once

#include "Core/CVars.h"

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Physics Console Variables

-------------------------------------------------------------------------------
*/

extern CVar         physics_enable;
extern CVar         physics_showWireframe;
extern CVar         physics_showAABB;
extern CVar         physics_showContactPoints;
extern CVar         physics_showNormals;
extern CVar         physics_showConstraints;
extern CVar         physics_noDeactivation;
extern CVar         physics_enableCCD;

BE_NAMESPACE_END
