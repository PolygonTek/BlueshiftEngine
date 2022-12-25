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
#include "Render/Render.h"
#include "Components/Transform/ComTransform.h"
#include "Components/Physics/ComSoftBody.h"
#include "Components/Renderable/ComMeshRenderer.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Soft Body", ComSoftBody, Component)
BEGIN_EVENTS(ComSoftBody)
END_EVENTS

void ComSoftBody::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("mass", "Mass", float, GetMass, SetMass, 20.f,
        "Mass (kg)", PropertyInfo::Flag::Editor).SetRange(0, 200, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("friction", "Friction", float, GetFriction, SetFriction, 0.8f,
        "", PropertyInfo::Flag::Editor).SetRange(0, 1, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("bendingConstraintDistance", "Bending Constraint Distance", int, GetBendingConstraintDistance, SetBendingConstraintDistance, 2,
        "", PropertyInfo::Flag::Editor).SetRange(1, 10, 1);
    REGISTER_ACCESSOR_PROPERTY("thickness", "Thickness", float, GetThickness, SetThickness, CmToUnit(1.0f),
        "", PropertyInfo::Flag::Editor).SetRange(CmToUnit(0.1f), CmToUnit(100.0f), CmToUnit(0.1f));
    REGISTER_ACCESSOR_PROPERTY("stiffness", "Stiffness", float, GetStiffness, SetStiffness, 0.025f,
        "", PropertyInfo::Flag::Editor).SetRange(0.001f, 1, 0.001f);
    REGISTER_MIXED_ACCESSOR_PROPERTY("windVelocity", "Wind Velocity", Vec3, GetWindVelocity, SetWindVelocity, Vec3::zero,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("solverIterationCount", "Solver Iteration Count", int, GetSolverIterationCount, SetSolverIterationCount, 5,
        "", PropertyInfo::Flag::Editor).SetRange(1, 30, 1);
    REGISTER_ACCESSOR_PROPERTY("ccd", "CCD", bool, IsCCDEnabled, SetCCDEnabled, true,
        "Continuous collision detection", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("selfCollision", "Self Collision", bool, IsSelfCollisionEnabled, SetSelfCollisionEnabled, false,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_ARRAY_PROPERTY("pointWeights", "Point Weights", float, GetPointWeight, SetPointWeight, GetPointWeightCount, SetPointWeightCount, 1.0f,
        "", PropertyInfo::Flag::Empty);
}

ComSoftBody::ComSoftBody() {
    updatable = true;
    memset(&physicsDesc, 0, sizeof(physicsDesc));
}

ComSoftBody::~ComSoftBody() {
    Purge(false);
}

void ComSoftBody::Purge(bool chainPurge) {
    if (body) {
        physicsSystem.DestroyCollidable(body);
        body = nullptr;
    }

    if (chainPurge) {
        Component::Purge();
    }
}

void ComSoftBody::Init() {
    Component::Init();

    physicsDesc.type = PhysCollidable::Type::SoftBody;

    ComMeshRenderer *meshRenderer = entity->GetComponent<ComMeshRenderer>();
    if (meshRenderer->IsInitialized()) {
        if (meshRenderer->renderObjectDef.mesh && !meshRenderer->renderObjectDef.mesh->IsDynamicMesh()) {
            meshRenderer->ReinstantiateMesh();
        }
    }

    ResetPoints();

    ComTransform *transform = GetEntity()->GetTransform();
    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComSoftBody::TransformUpdated, SignalObject::ConnectionType::Unique);

    // Mark as initialized
    SetInitialized(true);
}

void ComSoftBody::ResetPoints() {
    const ComTransform *transform = GetEntity()->GetTransform();
    ALIGN_AS32 Mat3x4 worldMatrix = transform->GetMatrix();

    const ComMeshRenderer *meshRenderer = entity->GetComponent<ComMeshRenderer>();
    const Mesh *mesh = meshRenderer->GetReferenceMesh();

    int numTotalVerts = 0;
    int numTotalIndexes = 0;

    for (int surfaceIndex = 0; surfaceIndex < mesh->NumSurfaces(); surfaceIndex++) {
        const MeshSurf *meshSurf = mesh->GetSurface(surfaceIndex);
        numTotalVerts += meshSurf->subMesh->NumVerts();
        numTotalIndexes += meshSurf->subMesh->NumIndexes();
    }

    graphicsToPhysicsVertexMapping.SetCount(numTotalVerts);

    HashIndex pointsHash;
    physicsDesc.points.Reserve(numTotalVerts);
    physicsDesc.points.SetCount(0);
    physicsDesc.pointIndexes.Reserve(numTotalIndexes);
    physicsDesc.pointIndexes.SetCount(0);

    int baseGraphicsIndex = 0;

    for (int surfaceIndex = 0; surfaceIndex < mesh->NumSurfaces(); surfaceIndex++) {
        const MeshSurf *meshSurf = mesh->GetSurface(surfaceIndex);
        const SubMesh *subMesh = meshSurf->subMesh;

        const VertexGenericLit *verts = subMesh->Verts();

        for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++) {
            const VertexGenericLit *vertex = &verts[vertexIndex];

            Vec3 localPosition = vertex->GetPosition();
            Vec3 worldPosition = worldMatrix.TransformPos(localPosition);

            bool foundSamePosition = false;
            int physicsIndex = -1;
            int hash = pointsHash.GenerateHash(worldPosition);
            for (int i = pointsHash.First(hash); i != -1; i = pointsHash.Next(i)) {
                foundSamePosition = physicsDesc.points[i].Equals(worldPosition, MmToUnit(0.1f));
                if (foundSamePosition) {
                    physicsIndex = i;
                    break;
                }
            }

            if (!foundSamePosition) {
                physicsIndex = physicsDesc.points.Append(worldPosition);
                pointsHash.Add(hash, physicsIndex);
            }
            int graphicsIndex = baseGraphicsIndex + vertexIndex;

            graphicsToPhysicsVertexMapping[graphicsIndex++] = physicsIndex;
        }

        baseGraphicsIndex += subMesh->NumVerts();
    }

    baseGraphicsIndex = 0;

    for (int surfaceIndex = 0; surfaceIndex < mesh->NumSurfaces(); surfaceIndex++) {
        const MeshSurf *meshSurf = mesh->GetSurface(surfaceIndex);
        const SubMesh *subMesh = meshSurf->subMesh;

        const VertIndex *indexes = subMesh->Indexes();

        for (int indexIndex = 0; indexIndex < subMesh->NumIndexes(); indexIndex++) {
            int graphicsIndex = baseGraphicsIndex + indexes[indexIndex];

            physicsDesc.pointIndexes.Append(graphicsToPhysicsVertexMapping[graphicsIndex]);
        }

        baseGraphicsIndex += subMesh->NumVerts();
    }

    physicsDesc.points.Squeeze();
    physicsDesc.pointIndexes.Squeeze();

    SetPropertyArrayCount("pointWeights", physicsDesc.points.Count());
}

void ComSoftBody::Awake() {
    if (!body) {
        CreateBody();
    }
}

void ComSoftBody::CreateBody() {
    if (body) {
        physicsSystem.DestroyCollidable(body);
        body = nullptr;
    }

    ComTransform *transform = GetEntity()->GetTransform();

    physicsDesc.origin = transform->GetOrigin();
    physicsDesc.axis = transform->GetAxis();

    body = static_cast<PhysSoftBody *>(physicsSystem.CreateCollidable(physicsDesc));
    body->SetUserPointer(this);
    body->SetCollisionFilterBit(entity->GetLayer());
    body->SetPositionSolverIterationCount(positionSolverIterationCount);

    if (IsActiveInHierarchy()) {
        body->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComSoftBody::Update() {
    if (!body) {
        return;
    }

    if (body->IsActive()) {
        UpdateMeshVertsFromPoints();
    }
}

void ComSoftBody::UpdateMeshVertsFromPoints() {
    const ComTransform *transform = GetEntity()->GetTransform();
    ALIGN_AS32 Mat3x4 inverseWorldMatrix = transform->GetMatrix().InverseOrthogonal();

    ComMeshRenderer *meshRenderer = entity->GetComponent<ComMeshRenderer>();
    Mesh *mesh = meshRenderer->renderObjectDef.mesh;

    int baseGraphicsIndex = 0;

    for (int surfaceIndex = 0; surfaceIndex < mesh->NumSurfaces(); surfaceIndex++) {
        const MeshSurf *meshSurf = mesh->GetSurface(surfaceIndex);
        const SubMesh *subMesh = meshSurf->subMesh;

        VertexGenericLit *verts = subMesh->Verts();

        for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++) {
            int graphicsIndex = baseGraphicsIndex + vertexIndex;
            int physicsIndex = graphicsToPhysicsVertexMapping[graphicsIndex];

            Vec3 nodePosition = body->GetNodePosition(physicsIndex);
            Vec3 nodeNormal = body->GetNodeNormal(physicsIndex);

            Vec3 localPosition = inverseWorldMatrix.TransformPos(nodePosition);
            Vec3 localNormal = inverseWorldMatrix.TransformDir(nodeNormal);

            verts[vertexIndex].SetPosition(localPosition);
            verts[vertexIndex].SetNormal(localNormal);
        }

        baseGraphicsIndex += subMesh->NumVerts();
    }

    // FIXME
    mesh->RecomputeTangents();

    AABB worldAabb;
    body->GetWorldAABB(worldAabb);

    meshRenderer->renderObjectDef.aabb.SetFromTransformedAABB(worldAabb, inverseWorldMatrix);
    meshRenderer->UpdateVisuals();
}

float ComSoftBody::GetMass() const {
    return body ? body->GetMass() : physicsDesc.mass;
}

void ComSoftBody::SetMass(float mass) {
    if (body) {
        body->SetMass(mass);
    } else {
        physicsDesc.mass = mass;
    }
}

float ComSoftBody::GetFriction() const {
    return body ? body->GetFriction() : physicsDesc.friction;
}

void ComSoftBody::SetFriction(float friction) {
    if (body) {
        body->SetFriction(friction);
    } else {
        physicsDesc.friction = friction;
    }
}

int ComSoftBody::GetBendingConstraintDistance() const {
    return body ? body->GetBendingConstraintDistance() : physicsDesc.bendingConstraintDistance;
}

void ComSoftBody::SetBendingConstraintDistance(int distance) {
    if (body) {
        body->SetBendingConstraintDistance(distance);
    } else {
        physicsDesc.bendingConstraintDistance = distance;
    }
}

float ComSoftBody::GetThickness() const {
    return body ? body->GetThickness() : physicsDesc.thickness;
}

void ComSoftBody::SetThickness(float thickness) {
    if (body) {
        body->SetThickness(thickness);
    } else {
        physicsDesc.thickness = thickness;
    }
}

float ComSoftBody::GetStiffness() const {
    return body ? body->GetStiffness() : physicsDesc.stiffness;
}

void ComSoftBody::SetStiffness(float stiffness) {
    if (body) {
        body->SetStiffness(stiffness);
    } else {
        physicsDesc.stiffness = stiffness;
    }
}

Vec3 ComSoftBody::GetWindVelocity() const {
    return body ? body->GetWindVelocity() : physicsDesc.windVelocity;
}

void ComSoftBody::SetWindVelocity(const Vec3 &windVelocity) {
    if (body) {
        body->SetWindVelocity(windVelocity);
    } else {
        physicsDesc.windVelocity = windVelocity;
    }
}

int ComSoftBody::GetSolverIterationCount() const {
    return body ? body->GetPositionSolverIterationCount() : positionSolverIterationCount;
}

void ComSoftBody::SetSolverIterationCount(int iterationCount) {
    if (body) {
        body->SetPositionSolverIterationCount(iterationCount);
    } else {
        positionSolverIterationCount = iterationCount;
    }
}

bool ComSoftBody::IsCCDEnabled() const {
    return body ? body->IsContinuousCollisionDetectionEnabled() : physicsDesc.enableCCD;
}

void ComSoftBody::SetCCDEnabled(bool enabled) {
    if (body) {
        body->SetContinuousCollisionDetectionEnabled(enabled);
    } else {
        physicsDesc.enableCCD = enabled;
    }
}

bool ComSoftBody::IsSelfCollisionEnabled() const {
    return body ? body->IsSelfCollisionEnabled() : physicsDesc.enableSelfCollision;
}

void ComSoftBody::SetSelfCollisionEnabled(bool enabled) {
    if (body) {
        body->SetSelfCollisionEnabled(enabled);
    } else {
        physicsDesc.enableSelfCollision = enabled;
    }
}

float ComSoftBody::GetPointWeight(int index) const {
    return physicsDesc.pointWeights[index];
}

void ComSoftBody::SetPointWeight(int index, float weight) {
    physicsDesc.pointWeights[index] = weight;
}

void ComSoftBody::SetPointWeightCount(int count) {
    int oldCount = physicsDesc.pointWeights.Count();

    physicsDesc.pointWeights.SetCount(count);

    if (count > oldCount) {
        for (int i = oldCount; i < count; i++) {
            physicsDesc.pointWeights[i] = 1.0f;
        }
    }
}

Vec3 ComSoftBody::GetPointPosition(int index) const {
    return body ? body->GetNodePosition(index) : physicsDesc.points[index];
}

void ComSoftBody::TransformUpdated(const ComTransform *transform) {
    ResetPoints();

    if (body) {
        body->SetTransform(Mat3x4(transform->GetAxis(), transform->GetOrigin()));
        body->Activate();

        UpdateMeshVertsFromPoints();
    }
}

#if WITH_EDITOR
void ComSoftBody::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    if (selected) {
        RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

        for (int nodeIndex = 0; nodeIndex < physicsDesc.points.Count(); nodeIndex++) {
            Vec3 worldPosition = GetPointPosition(nodeIndex);

            float viewScale = camera->CalcClampedViewScale(worldPosition);

            Color4 color = Math::Lerp(Color4::red, Color4::green, physicsDesc.pointWeights[nodeIndex]);

            renderWorld->SetDebugColor(Color4::white, color);
            renderWorld->DebugQuad(worldPosition, camera->GetState().axis[1], camera->GetState().axis[2], MeterToUnit(1.8f) * viewScale);
        }
    }
}
#endif

BE_NAMESPACE_END
