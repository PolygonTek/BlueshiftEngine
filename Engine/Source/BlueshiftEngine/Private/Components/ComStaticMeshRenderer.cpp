#include "Precompiled.h"
#include "Render/Render.h"
#include "Components/ComTransform.h"
#include "Components/ComSkinnedMeshRenderer.h"
#include "Components/ComStaticMeshRenderer.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Static Mesh Renderer", ComStaticMeshRenderer, ComMeshRenderer)
BEGIN_EVENTS(ComStaticMeshRenderer)
END_EVENTS
BEGIN_PROPERTIES(ComStaticMeshRenderer)
    PROPERTY_BOOL("occluder", "Occluder", "", "false", PropertySpec::ReadWrite),
END_PROPERTIES

void ComStaticMeshRenderer::RegisterProperties() {
    //REGISTER_ACCESSOR_PROPERTY("Occluder", bool, IsOccluder, SetOccluder, "false", PropertySpec::ReadWrite);
}

ComStaticMeshRenderer::ComStaticMeshRenderer() {
    Connect(&SIG_PropertyChanged, this, (SignalCallback)&ComStaticMeshRenderer::PropertyChanged);
}

ComStaticMeshRenderer::~ComStaticMeshRenderer() {
    Purge(false);
}

bool ComStaticMeshRenderer::IsConflictComponent(const MetaObject &componentClass) const {
    if (componentClass.IsTypeOf(ComSkinnedMeshRenderer::metaObject)) {
        return true;
    }
    return false;
}

void ComStaticMeshRenderer::Purge(bool chainPurge) {
    if (chainPurge) {
        ComMeshRenderer::Purge();
    }
}

void ComStaticMeshRenderer::Init() {
    ComMeshRenderer::Init();

    // Set SceneEntity parameters
    sceneEntity.mesh = referenceMesh->InstantiateMesh(Mesh::StaticMesh);
    sceneEntity.occluder = props->Get("occluder").As<bool>();

    UpdateVisuals();
}

void ComStaticMeshRenderer::Update() {
}

void ComStaticMeshRenderer::MeshUpdated() {
    sceneEntity.mesh = referenceMesh->InstantiateMesh(Mesh::StaticMesh);
    sceneEntity.aabb = referenceMesh->GetAABB();
    // temp code
    renderWorld->RemoveEntity(sceneEntityHandle);
    sceneEntityHandle = -1;
    // temp code

    UpdateVisuals();
}

void ComStaticMeshRenderer::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "occluder")) {
        sceneEntity.occluder = props->Get("occluder").As<bool>();
        UpdateVisuals();
        return;
    }

    ComMeshRenderer::PropertyChanged(classname, propName);
}

BE_NAMESPACE_END
