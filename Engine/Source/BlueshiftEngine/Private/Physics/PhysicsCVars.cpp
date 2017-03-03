#include "Precompiled.h"
#include "Core/CVars.h"

BE_NAMESPACE_BEGIN

CVAR(physics_enable, L"1", CVar::Bool, L"");
CVAR(physics_frameRate, L"60", CVar::Float, L"");
CVAR(physics_showWireframe, L"0", CVar::Integer, L"show debug wireframe in physics engine");
CVAR(physics_showAABB, L"0", CVar::Integer, L"show debug AABB in physics engine");
CVAR(physics_showContactPoints, L"0", CVar::Integer, L"");
CVAR(physics_showNormals, L"0", CVar::Integer, L"");
CVAR(physics_showConstraints, L"0", CVar::Integer, L"");
CVAR(physics_noDeactivation, L"0", CVar::Bool, L"");
CVAR(physics_enableCCD, L"1", CVar::Bool, L"");

BE_NAMESPACE_END
