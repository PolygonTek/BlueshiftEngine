#include "Precompiled.h"
#include "Render/Render.h"
#include "Components/ComTransform.h"
#include "Components/ComMeshRenderer.h"
#include "Game/Entity.h"
#include "Asset/Asset.h"
#include "Asset/GuidMapper.h"

BE_NAMESPACE_BEGIN

ABSTRACT_DECLARATION("Mesh Renderer", ComMeshRenderer, ComRenderable)
BEGIN_EVENTS(ComMeshRenderer)
END_EVENTS
BEGIN_PROPERTIES(ComMeshRenderer)
    PROPERTY_OBJECT("mesh", "Mesh", "mesh", GuidMapper::defaultMeshGuid.ToString(), MeshAsset::metaObject, PropertySpec::ReadWrite),
    PROPERTY_OBJECT("materials", "Materials", "List of materials to use when rendering.", GuidMapper::defaultMaterialGuid.ToString(), MaterialAsset::metaObject, PropertySpec::ReadWrite | PropertySpec::IsArray),
    PROPERTY_BOOL("useLightProbe", "Use Light Probe", "", "true", PropertySpec::ReadWrite),
    PROPERTY_BOOL("castShadows", "Cast Shadows", "", "true", PropertySpec::ReadWrite),
    PROPERTY_BOOL("receiveShadows", "Receive Shadows", "", "true", PropertySpec::ReadWrite),
END_PROPERTIES

void ComMeshRenderer::RegisterProperties() {
    //REGISTER_ACCESSOR_PROPERTY("Mesh", MeshAsset, GetMesh, SetMesh, GuidMapper::defaultMeshGuid.ToString(), "", PropertySpec::ReadWrite);
    //REGISTER_ACCESSOR_PROPERTY("Materials", MaterialAsset, GetMaterial, SetMaterial, Guid::zero.ToString(), "", PropertySpec::ReadWrite);
    //REGISTER_ACCESSOR_PROPERTY("Use Light Probe", bool, IsUseLightProbe, SetUseLightProbe, "", PropertySpec::ReadWrite);
    //REGISTER_ACCESSOR_PROPERTY("Cast Shadows", bool, IsCastShadows, SetCastShadows, "", PropertySpec::ReadWrite);
    //REGISTER_ACCESSOR_PROPERTY("Receive Shadows", bool, IsReceiveShadows, SetReceiveShadows, "", PropertySpec::ReadWrite);
}

ComMeshRenderer::ComMeshRenderer() {
    meshAsset = nullptr;
    referenceMesh = nullptr;

    Connect(&SIG_PropertyChanged, this, (SignalCallback)&ComMeshRenderer::PropertyChanged);
}

ComMeshRenderer::~ComMeshRenderer() {
    Purge(false);
}

void ComMeshRenderer::Purge(bool chainPurge) {
    if (sceneEntity.mesh) {
        meshManager.ReleaseMesh(sceneEntity.mesh);
        sceneEntity.mesh = nullptr;
    }

    if (referenceMesh) {
        meshManager.ReleaseMesh(referenceMesh);
        referenceMesh = nullptr;
    }

    if (chainPurge) {
        ComRenderable::Purge();
    }
}

void ComMeshRenderer::Init() {
    ComRenderable::Init();

    ChangeMesh(props->Get("mesh").As<Guid>());

    // Set SceneEntity parameters
    sceneEntity.layer          = GetEntity()->GetLayer();
    sceneEntity.mesh           = nullptr;
    sceneEntity.aabb           = referenceMesh->GetAABB();
    sceneEntity.customSkin     = nullptr;
    sceneEntity.castShadows    = props->Get("castShadows").As<bool>();
    sceneEntity.receiveShadows = props->Get("receiveShadows").As<bool>();
}

void ComMeshRenderer::ChangeMesh(const Guid &meshGuid) {
    // Disconnect from old mesh asset
    if (meshAsset) {
        meshAsset->Disconnect(&SIG_Reloaded, this);
    }

    // Release the previous used instantiated mesh
    if (sceneEntity.mesh) {
        meshManager.ReleaseMesh(sceneEntity.mesh);
        sceneEntity.mesh = nullptr;
    }

    // Release the previous used reference mesh
    if (referenceMesh) {
        meshManager.ReleaseMesh(referenceMesh);
        referenceMesh = nullptr;
    }

    // Release previous used materials
    for (int i = 0; i < sceneEntity.customMaterials.Count(); i++) {
        materialManager.ReleaseMaterial(sceneEntity.customMaterials[i]);
    }

    // Get the new reference mesh
    const Str meshPath = resourceGuidMapper.Get(meshGuid);
    referenceMesh = meshManager.GetMesh(meshPath);

    // Get number of materials
    BE1::Array<int> materialIndexArray;
    for (int i = 0; i < referenceMesh->NumSurfaces(); i++) {
        materialIndexArray.AddUnique(referenceMesh->GetSurface(i)->materialIndex);
    }
    int numMaterials = materialIndexArray.Count();

    // Resize material slots
    sceneEntity.customMaterials.SetCount(numMaterials);

    props->SetNumElements("materials", numMaterials);

    // Get all of the new materials
    for (int i = 0; i < numMaterials; i++) {
        Str name = va("materials[%i]", i);

        //const MeshSurf *surf = referenceMesh->GetSurface(i);
        const Guid materialGuid = props->Get(name).As<Guid>();
        const Str materialPath = resourceGuidMapper.Get(materialGuid);

        sceneEntity.customMaterials[i] = materialManager.GetMaterial(materialPath);
    }

    // Need to mesh asset to be reloaded in Editor
    meshAsset = (MeshAsset *)MeshAsset::FindInstance(meshGuid);
    if (meshAsset) {
        meshAsset->Connect(&SIG_Reloaded, this, (SignalCallback)&ComMeshRenderer::MeshReloaded, SignalObject::Queued);
    }
}

void ComMeshRenderer::ChangeMaterial(const char *materialName) {
    int index;
    sscanf(materialName, "materials[%i]", &index);

    if (index < props->NumElements("materials")) {
        // Release the previous used material
        if (sceneEntity.customMaterials[index]) {
            materialManager.ReleaseMaterial(sceneEntity.customMaterials[index]);
        }

        // Get the new material
        //const MeshSurf *surf = referenceMesh->GetSurface(index);
        const Guid materialGuid = props->Get(materialName).As<Guid>();
        const Str materialPath = resourceGuidMapper.Get(materialGuid);
        sceneEntity.customMaterials[index] = materialManager.GetMaterial(materialPath);
    }
}

void ComMeshRenderer::MeshReloaded() {
    SetMesh(props->Get("mesh").As<Guid>());
}

void ComMeshRenderer::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }

    if (!Str::Cmp(propName, "mesh")) {
        SetMesh(props->Get("mesh").As<Guid>());
        return;
    }

    if (!Str::Cmpn(propName, "materials", 9)) {
        ChangeMaterial(propName);
        UpdateVisuals();
        return;
    }

    if (!Str::Cmp(propName, "useLightProbe")) {
        SetUseLightProbe(props->Get("useLightProbe").As<bool>());
        return;
    }
    
    if (!Str::Cmp(propName, "castShadows")) {
        SetCastShadows(props->Get("castShadows").As<bool>());
        return;
    }
    
    if (!Str::Cmp(propName, "receiveShadows")) {
        SetReceiveShadows(props->Get("receiveShadows").As<bool>());
        return;
    }

    ComRenderable::PropertyChanged(classname, propName);
}

Guid ComMeshRenderer::GetMesh() const {
    const Str meshPath = referenceMesh->GetHashName();
    return resourceGuidMapper.Get(meshPath);
}

void ComMeshRenderer::SetMesh(const Guid &guid) {
    ChangeMesh(guid);

    MeshUpdated();
}

bool ComMeshRenderer::IsUseLightProbe() const {
    return sceneEntity.useLightProbe;
}

void ComMeshRenderer::SetUseLightProbe(bool useLightProbe) {
    sceneEntity.useLightProbe = useLightProbe;
    UpdateVisuals();
}

bool ComMeshRenderer::IsCastShadows() const {
    return sceneEntity.castShadows;
}

void ComMeshRenderer::SetCastShadows(bool castShadows) {
    sceneEntity.castShadows = castShadows;
    UpdateVisuals();
}

bool ComMeshRenderer::IsReceiveShadows() const {
    return sceneEntity.receiveShadows;
}

void ComMeshRenderer::SetReceiveShadows(bool receiveShadows) {
    sceneEntity.receiveShadows = receiveShadows;
    UpdateVisuals();
}

bool ComMeshRenderer::GetClosestVertex(const SceneView *view, const Point &mousePixelLocation, Vec3 &closestVertex, float &closestDistance) const {
    const float initialClosestDistance = closestDistance;

    const ComTransform *transform = GetEntity()->GetTransform();

    for (int surfaceIndex = 0; surfaceIndex < sceneEntity.mesh->NumSurfaces(); surfaceIndex++) {
        const SubMesh *subMesh = sceneEntity.mesh->GetSurface(surfaceIndex)->subMesh;
        const VertexLightingGeneric *v = subMesh->Verts();

        for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++, v++) {
            Vec3 localPosition = v->GetPosition();
            Vec3 localNormal = v->GetNormal();

            Vec3 worldPosition = transform->GetWorldMatrix() * localPosition;
            Vec3 worldNormal = transform->GetWorldMatrix().TransformNormal(localNormal);

            bool isBackface;
            // Ignore backface vertices 
            if (!view->parms.orthogonal && !worldNormal.IsZero() && worldNormal.Dot(view->parms.origin - worldPosition) < 0) {
                isBackface = true;
            } else {
                isBackface = false;
            }

            if (!isBackface) {
                Point pixelLocation;

                if (view->WorldToPixel(worldPosition, pixelLocation)) {
                    float dist = pixelLocation.DistanceSqr(mousePixelLocation);

                    if (dist < closestDistance) {
                        closestDistance = dist;
                        closestVertex = worldPosition;
                    }
                }
            }
        }
    }

    return initialClosestDistance != closestDistance;
}

BE_NAMESPACE_END
