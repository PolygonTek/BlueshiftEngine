#include "Precompiled.h"
#include "Asset/Resource.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("glTF", GltfResource, Resource)
BEGIN_EVENTS(GltfResource)
END_EVENTS

void GltfResource::RegisterProperties() {}

GltfResource::GltfResource() {}

GltfResource::~GltfResource() {}

void GltfResource::Rename(const Str &newName) {}

bool GltfResource::Reload() {
    return false;
}

bool GltfResource::Save() {
    return false;
}

BE_NAMESPACE_END
