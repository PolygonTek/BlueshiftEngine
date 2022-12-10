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
#include "Components/Transform/ComTransform.h"
#include "Components/Physics/ComSoftBody.h"
#include "Components/Renderable/ComMeshRenderer.h"
#include "Render/Mesh.h"
#include "Render/SubMesh.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Soft Body", ComSoftBody, Component)
BEGIN_EVENTS(ComSoftBody)
END_EVENTS

void ComSoftBody::RegisterProperties() {
    REGISTER_ACCESSOR_PROPERTY("mass", "Mass", float, GetMass, SetMass, 1.f,
        "Mass (kg)", PropertyInfo::Flag::Editor).SetRange(0, 200, 0.01f);
    REGISTER_ACCESSOR_PROPERTY("friction", "Friction", float, GetFriction, SetFriction, 0.5f,
        "", PropertyInfo::Flag::Editor).SetRange(0, 1, 0.01f);
    REGISTER_PROPERTY("stretchingStiffness", "Stretching Stiffness", float, stretchingStiffness, 1.0f,
        "", PropertyInfo::Flag::Editor);
    REGISTER_ACCESSOR_PROPERTY("solverIterationCount", "Solver Iteration Count", int, GetSolverIterationCount, SetSolverIterationCount, 5,
        "", PropertyInfo::Flag::Editor).SetRange(1, 30, 1);
    REGISTER_ACCESSOR_PROPERTY("ccd", "CCD", bool, IsCCD, SetCCD, true,
        "Continuous collision detection", PropertyInfo::Flag::Editor);
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

    InitPoints();

    // Mark as initialized
    SetInitialized(true);
}

void ComSoftBody::Awake() {
    if (!body) {
        CreateBody();
    }
}

void ComSoftBody::Update() {
    if (!body || !IsActiveInHierarchy()) {
        return;
    }

    if (body->IsActive()) {
        ComMeshRenderer *meshRenderer = entity->GetComponent<ComMeshRenderer>();
        Mesh *mesh = meshRenderer->GetRenderObjectDef().mesh;

        int nodeIndex = 0;

        for (int surfaceIndex = 0; surfaceIndex < mesh->NumSurfaces(); surfaceIndex++) {
            const MeshSurf *meshSurf = mesh->GetSurface(surfaceIndex);
            const SubMesh *subMesh = meshSurf->subMesh;

            VertexGenericLit *verts = subMesh->Verts();

            for (int vertexIndex = 0; vertexIndex < subMesh->NumVerts(); vertexIndex++) {
                Vec3 position = body->GetNodePosition(nodeIndex++);

                verts[vertexIndex].SetPosition(position);
            }
        }
    }
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

bool ComSoftBody::IsCCD() const {
    return body ? body->IsCCD() : physicsDesc.ccd;
}

void ComSoftBody::SetCCD(bool enableCcd) {
    if (body) {
        body->SetCCD(enableCcd);
    } else {
        physicsDesc.ccd = enableCcd;
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

void ComSoftBody::InitPoints() {
    const ComTransform *transform = GetEntity()->GetTransform();
    ALIGN_AS32 Mat3x4 worldMatrix = transform->GetMatrix();

    const ComMeshRenderer *meshRenderer = entity->GetComponent<ComMeshRenderer>();
    //meshRenderer->Reinstantiate();

    const Mesh *mesh = meshRenderer->GetReferenceMesh();

    int numVerts = 0;
    int numIndexes = 0;

    for (int surfaceIndex = 0; surfaceIndex < mesh->NumSurfaces(); surfaceIndex++) {
        const MeshSurf *meshSurf = mesh->GetSurface(surfaceIndex);
        numVerts += meshSurf->subMesh->NumOriginalVerts();
        numIndexes += meshSurf->subMesh->NumIndexes();
    }

    physicsDesc.points.SetCount(numVerts);
    physicsDesc.pointIndexes.SetCount(numIndexes);

    int nodeIndex = 0;
    int nodeIndexIndex = 0;

    for (int surfaceIndex = 0; surfaceIndex < mesh->NumSurfaces(); surfaceIndex++) {
        const MeshSurf *meshSurf = mesh->GetSurface(surfaceIndex);
        const SubMesh *subMesh = meshSurf->subMesh;

        const VertexGenericLit *verts = subMesh->Verts();
        const VertIndex *indexes = subMesh->Indexes();

        for (int vertexIndex = 0; vertexIndex < subMesh->NumOriginalVerts(); vertexIndex++) {
            const VertexGenericLit *vertex = &verts[vertexIndex];

            Vec3 worldPosition = worldMatrix.TransformPos(vertex->GetPosition());
            //Vec3 worldNormal = worldMatrix.TransformDir(vertex->GetNormal());

            physicsDesc.points[nodeIndex++] = worldPosition;
        }

        for (int indexIndex = 0; indexIndex < subMesh->NumIndexes(); indexIndex++) {
            int index = indexes[indexIndex];

            physicsDesc.pointIndexes[nodeIndexIndex++] = index;
        }
    }

    SetPropertyArrayCount("pointWeights", numVerts);
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

    transform->Connect(&ComTransform::SIG_TransformUpdated, this, (SignalCallback)&ComSoftBody::TransformUpdated, SignalObject::ConnectionType::Unique);
}

void ComSoftBody::TransformUpdated(const ComTransform *transform) {
    if (body) {
        body->SetTransform(Mat3x4(transform->GetAxis(), transform->GetOrigin()));
        body->Activate();
    }
}

#if WITH_EDITOR
void ComSoftBody::DrawGizmos(const RenderCamera *camera, bool selected, bool selectedByParent) {
    if (selected) {
        // Draw points
    }
}
#endif

BE_NAMESPACE_END
