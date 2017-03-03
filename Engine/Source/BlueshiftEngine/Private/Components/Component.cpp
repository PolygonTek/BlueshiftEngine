#include "Precompiled.h"
#include "Components/ComTransform.h"
#include "Components/Component.h"
#include "Game/Entity.h"

BE_NAMESPACE_BEGIN

const SignalDef     SIG_UpdateUI("updateUI");
    
ABSTRACT_DECLARATION("Component", Component, Object)
BEGIN_EVENTS(Component)
END_EVENTS
BEGIN_PROPERTIES(Component)
    PROPERTY_OBJECT("guid", "GUID", "component GUID", Guid::zero.ToString(), Component::metaObject, PropertySpec::ReadWrite | PropertySpec::Hidden),
    PROPERTY_BOOL("enabled", "Enabled", "is enabled component ?", "true", PropertySpec::ReadWrite | PropertySpec::Hidden),
END_PROPERTIES

void Component::RegisterProperties() {
    //REGISTER_PROPERTY("GUID", Component, guid, Guid::zero.ToString(), PropertySpec::ReadWrite | PropertySpec::Hidden);
    //REGISTER_ACCESSOR_PROPERTY("Enabled", bool, IsEnabled, SetEnabled, "true", PropertySpec::ReadWrite | PropertySpec::Hidden);
}

Component::Component() {
    entity = nullptr;
    enabled = true;
    initialized = false;
}

Component::~Component() {
    Purge(false);
}

void Component::Purge(bool chainPurge) {
    initialized = false;
}

void Component::Event_ImmediateDestroy() {
    if (entity) {
        if (!entity->components.Remove(this)) {
            assert(0);
            return;
        }

        entity->EmitSignal(&SIG_ComponentRemoved, this);
    }

    Object::Event_ImmediateDestroy();
}

const Str Component::ToString() const { 
    return entity->ToString(); 
}

GameWorld *Component::GetGameWorld() const {
    if (entity) {
        return entity->GetGameWorld();
    }
    return nullptr;
}

void Component::Init() {
    SetInitialized(true);

    //
    enabled = props->Get("enabled").As<bool>();
    //
}

void Component::Reload() {
    Purge();

    Init();
}

void Component::PropertyChanged(const char *classname, const char *propName) {
    if (!Str::Cmp(propName, "enabled")) {
        Enable(props->Get("enabled").As<bool>());
        return;
    }
}

BE_NAMESPACE_END
