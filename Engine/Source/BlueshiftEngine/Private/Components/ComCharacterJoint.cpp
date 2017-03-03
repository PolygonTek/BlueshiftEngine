#include "Precompiled.h"
#include "Render/Render.h"
#include "Components/ComTransform.h"
#include "Components/ComRigidBody.h"
#include "Components/ComCharacterJoint.h"
#include "Game/Entity.h"
#include "Game/GameWorld.h"

BE_NAMESPACE_BEGIN

OBJECT_DECLARATION("Character Joint", ComCharacterJoint, ComJoint)
BEGIN_EVENTS(ComCharacterJoint)
END_EVENTS
BEGIN_PROPERTIES(ComCharacterJoint)
    PROPERTY_VEC3("anchor", "Anchor", "", "0 0 0", PropertySpec::ReadWrite),
    PROPERTY_ANGLES("angles", "Angles", "", "0 0 0", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("swing1LowerLimit", "X Swing Lower Limit", "", Rangef(-180, 0, 1), "-45", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("swing1UpperLimit", "X Swing Upper Limit", "", Rangef(0, 180, 1), "45", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("swing1Stiffness", "X Swing Stiffness", "", "0", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("swing1Damping", "X Swing Damping", "", Rangef(0, 1, 0.01f), "0.2", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("swing2LowerLimit", "Y Swing Lower Limit", "", Rangef(-180, 0, 1), "-45", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("swing2UpperLimit", "Y Swing Upper Limit", "", Rangef(0, 180, 1), "45", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("swing2Stiffness", "Y Swing Stiffness", "", "0", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("swing2Damping", "Y Swing Damping", "", Rangef(0, 1, 0.01f), "0.2", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("twistLowerLimit", "Twist Lower Limit", "", Rangef(-180, 0, 1), "-45", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("twistUpperLimit", "Twist Upper Limit", "", Rangef(0, 180, 1), "45", PropertySpec::ReadWrite),
    PROPERTY_FLOAT("twistStiffness", "Twist Stiffness", "", "0", PropertySpec::ReadWrite),
    PROPERTY_RANGED_FLOAT("twistDamping", "Twist Damping", "", Rangef(0, 1, 0.01), "0.2", PropertySpec::ReadWrite),
END_PROPERTIES

void ComCharacterJoint::RegisterProperties() {
    //REGISTER_ACCESSOR_PROPERTY("Anchor", Vec3, GetAnchor, SetAnchor, "0 0 0", PropertySpec::ReadWrite);
    //REGISTER_ACCESSOR_PROPERTY("Angles", Angles, GetAngles, SetAngles, "0 0 0", PropertySpec::ReadWrite);
    //REGISTER_ACCESSOR_PROPERTY("X Swing Lower Limit", float, GetSwing1LowerLimit, SetSwing1LowerLimit, "-45", PropertySpec::ReadWrite).SetRange(-180, 0, 1);
    //REGISTER_ACCESSOR_PROPERTY("X Swing Upper Limit", float, GetSwing1UpperLimit, SetSwing1UpperLimit, "45", PropertySpec::ReadWrite).SetRange(0, 180, 1);
    //REGISTER_ACCESSOR_PROPERTY("X Swing Stiffness", float, GetSwing1Stiffness, SetSwing1Stiffness, "0", PropertySpec::ReadWrite);
    //REGISTER_ACCESSOR_PROPERTY("X Swing Damping", float, GetSwing1Damping, SetSwing1Damping, "0.2", PropertySpec::ReadWrite).SetRange(0, 1, 0.01f);
    //REGISTER_ACCESSOR_PROPERTY("Y Swing Lower Limit", float, GetSwing2LowerLimit, SetSwing2LowerLimit, "-45", PropertySpec::ReadWrite).SetRange(-180, 0, 1);
    //REGISTER_ACCESSOR_PROPERTY("Y Swing Upper Limit", float, GetSwing2UpperLimit, SetSwing2UpperLimit, "45", PropertySpec::ReadWrite).SetRange(0, 180, 1);
    //REGISTER_ACCESSOR_PROPERTY("Y Swing Stiffness", float, GetSwing2Stiffness, SetSwing2Stiffness, "0", PropertySpec::ReadWrite);
    //REGISTER_ACCESSOR_PROPERTY("Y Swing Damping", float, GetSwing2Damping, SetSwing2Damping, "0.2", PropertySpec::ReadWrite).SetRange(0, 1, 0.01f);
    //REGISTER_ACCESSOR_PROPERTY("Twist Lower Limit", float, GetTwistLowerLimit, SetTwistLowerLimit, "-45", PropertySpec::ReadWrite).SetRange(-180, 0, 1);
    //REGISTER_ACCESSOR_PROPERTY("Twist Upper Limit", float, GetTwistUpperLimit, SetTwistUpperLimit, "45", PropertySpec::ReadWrite).SetRange(0, 180, 1);
    //REGISTER_ACCESSOR_PROPERTY("Twist Stiffness", float, GetTwistStiffness, SetTwistStiffness, "0", PropertySpec::ReadWrite);
    //REGISTER_ACCESSOR_PROPERTY("Twist Damping", float, GetTwistDamping, SetTwistDamping, "0.2", PropertySpec::ReadWrite).SetRange(0, 1, 0.01f);
}

ComCharacterJoint::ComCharacterJoint() {
    Connect(&SIG_PropertyChanged, this, (SignalCallback)&ComCharacterJoint::PropertyChanged);
}

ComCharacterJoint::~ComCharacterJoint() {
}

void ComCharacterJoint::Init() {
    ComJoint::Init();

    anchor = props->Get("anchor").As<Vec3>();
    axis = props->Get("angles").As<Angles>().ToMat3();
    axis.FixDegeneracies();

    lowerLimit.x = props->Get("swing1LowerLimit").As<float>();
    upperLimit.x = props->Get("swing1UpperLimit").As<float>();
    stiffness.x = props->Get("swing1Stiffness").As<float>();
    damping.x = props->Get("swing1Damping").As<float>();

    lowerLimit.y = props->Get("swing2LowerLimit").As<float>();
    upperLimit.y = props->Get("swing2UpperLimit").As<float>();
    stiffness.y = props->Get("swing2Stiffness").As<float>();
    damping.y = props->Get("swing2Damping").As<float>();

    lowerLimit.z = props->Get("twistLowerLimit").As<float>();
    upperLimit.z = props->Get("twistUpperLimit").As<float>();
    stiffness.z = props->Get("twistStiffness").As<float>();
    damping.z = props->Get("twistDamping").As<float>();
}

void ComCharacterJoint::Start() {
    ComJoint::Start();

    const ComTransform *transform = GetEntity()->GetTransform();
    const ComRigidBody *rigidBody = GetEntity()->GetComponent<ComRigidBody>();
    assert(rigidBody);

    PhysConstraintDesc desc;
    desc.type           = PhysConstraint::GenericSpring;
    desc.bodyA          = rigidBody->GetBody();
    desc.axisInA        = axis;
    desc.anchorInA      = transform->GetScale() * anchor;

    if (connectedBody) {
        Vec3 worldAnchor = desc.bodyA->GetOrigin() + desc.bodyA->GetAxis() * desc.anchorInA;

        desc.bodyB      = connectedBody->GetBody();
        desc.axisInB    = axis;
        desc.anchorInB  = connectedBody->GetBody()->GetAxis().TransposedMulVec(worldAnchor - connectedBody->GetBody()->GetOrigin());
    } else {
        desc.bodyB      = nullptr;
    }

    desc.collision      = collisionEnabled;
    desc.breakImpulse   = breakImpulse;
    constraint = physicsSystem.CreateConstraint(&desc);

    PhysGenericSpringConstraint *genericSpringConstraint = static_cast<PhysGenericSpringConstraint *>(constraint);
    genericSpringConstraint->SetAngularLowerLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    genericSpringConstraint->SetAngularUpperLimit(Vec3(DEG2RAD(upperLimit.x), DEG2RAD(upperLimit.y), DEG2RAD(upperLimit.z)));
    genericSpringConstraint->SetAngularStiffness(stiffness);
    genericSpringConstraint->SetAngularDamping(damping);

    if (IsEnabled()) {
        genericSpringConstraint->AddToWorld(GetGameWorld()->GetPhysicsWorld());
    }
}

void ComCharacterJoint::DrawGizmos(const SceneView::Parms &sceneView, bool selected) {
    RenderWorld *renderWorld = GetGameWorld()->GetRenderWorld();

    const ComTransform *transform = GetEntity()->GetTransform();
    Vec3 worldOrigin = transform->GetWorldMatrix() * anchor;
    Mat3 worldAxis = transform->GetAxis() * axis;
    
    renderWorld->SetDebugColor(Color4::red, Color4::zero);
    renderWorld->DebugLine(worldOrigin - worldAxis[0] * CentiToUnit(5), worldOrigin + worldAxis[0] * CentiToUnit(5), 1);
    renderWorld->DebugLine(worldOrigin - worldAxis[1] * CentiToUnit(5), worldOrigin + worldAxis[1] * CentiToUnit(5), 1);
    renderWorld->DebugLine(worldOrigin - worldAxis[2] * CentiToUnit(5), worldOrigin + worldAxis[2] * CentiToUnit(5), 1);
}

void ComCharacterJoint::PropertyChanged(const char *classname, const char *propName) {
    if (!IsInitalized()) {
        return;
    }
    
    if (!Str::Cmp(propName, "anchor")) {
        SetAnchor(props->Get("anchor").As<Vec3>());
        return;
    }
    
    if (!Str::Cmp(propName, "angles")) {
        axis = props->Get("angles").As<Angles>().ToMat3();
        return;
    }

    if (!Str::Cmp(propName, "swing1LowerLimit")) {
        SetSwing1LowerLimit(props->Get("swing1LowerLimit").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "swing1UpperLimit")) {
        SetSwing1UpperLimit(props->Get("swing1UpperLimit").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "swing1Stiffness")) {
        SetSwing1Stiffness(props->Get("swing1Stiffness").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "swing1Damping")) {
        SetSwing1Damping(props->Get("swing1Damping").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "swing2LowerLimit")) {
        SetSwing2LowerLimit(props->Get("swing2LowerLimit").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "swing2UpperLimit")) {
        SetSwing2UpperLimit(props->Get("swing2UpperLimit").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "swing2Stiffness")) {
        SetSwing2Stiffness(props->Get("swing2Stiffness").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "swing2Damping")) {
        SetSwing2Damping(props->Get("swing2Damping").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "twistLowerLimit")) {
        SetTwistLowerLimit(props->Get("twistLowerLimit").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "twistUpperLimit")) {
        SetTwistUpperLimit(props->Get("twistUpperLimit").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "twistStiffness")) {
        SetTwistStiffness(props->Get("twistStiffness").As<float>());
        return;
    }

    if (!Str::Cmp(propName, "twistDamping")) {
        SetTwistDamping(props->Get("twistDamping").As<float>());
        return;
    }

    ComJoint::PropertyChanged(classname, propName);
}

const Vec3 &ComCharacterJoint::GetAnchor() const {
    return anchor;
}

void ComCharacterJoint::SetAnchor(const Vec3 &anchor) {
    this->anchor = anchor;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, axis);
    }
}

const Angles ComCharacterJoint::GetAngles() const {
    return axis.ToAngles();
}

void ComCharacterJoint::SetAngles(const Angles &angles) {
    this->axis = angles.ToMat3();
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetFrameA(anchor, axis);
    }
}

const float ComCharacterJoint::GetSwing1LowerLimit() const {
    return lowerLimit.x;   
}

void ComCharacterJoint::SetSwing1LowerLimit(const float limit) {
    this->lowerLimit.x = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

const float ComCharacterJoint::GetSwing1UpperLimit() const {
    return upperLimit.x;
}

void ComCharacterJoint::SetSwing1UpperLimit(const float limit) {
    this->upperLimit.x = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

const float ComCharacterJoint::GetSwing1Stiffness() const {
    return stiffness.x;
}

void ComCharacterJoint::SetSwing1Stiffness(const float stiffness) {
    this->stiffness.x = stiffness;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularStiffness(this->stiffness);
    }
}

const float ComCharacterJoint::GetSwing1Damping() const {
    return damping.x;
}

void ComCharacterJoint::SetSwing1Damping(const float damping) {
    this->damping.x = damping;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularDamping(this->damping);
    }
}

const float ComCharacterJoint::GetSwing2LowerLimit() const {
    return lowerLimit.y;
}

void ComCharacterJoint::SetSwing2LowerLimit(const float limit) {
    this->lowerLimit.y = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

const float ComCharacterJoint::GetSwing2UpperLimit() const {
    return upperLimit.y;
}

void ComCharacterJoint::SetSwing2UpperLimit(const float limit) {
    this->upperLimit.y = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

const float ComCharacterJoint::GetSwing2Stiffness() const {
    return stiffness.y;
}

void ComCharacterJoint::SetSwing2Stiffness(const float stiffness) {
    this->stiffness.y = stiffness;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularStiffness(this->stiffness);
    }
}

const float ComCharacterJoint::GetSwing2Damping() const {
    return damping.y;
}

void ComCharacterJoint::SetSwing2Damping(const float damping) {
    this->damping.y = damping;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularDamping(this->damping);
    }
}

const float ComCharacterJoint::GetTwistLowerLimit() const {
    return lowerLimit.z;
}

void ComCharacterJoint::SetTwistLowerLimit(const float limit) {
    this->lowerLimit.z = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularLowerLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

const float ComCharacterJoint::GetTwistUpperLimit() const {
    return upperLimit.z;
}

void ComCharacterJoint::SetTwistUpperLimit(const float limit) {
    this->upperLimit.z = limit;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularUpperLimit(Vec3(DEG2RAD(lowerLimit.x), DEG2RAD(lowerLimit.y), DEG2RAD(lowerLimit.z)));
    }
}

const float ComCharacterJoint::GetTwistStiffness() const {
    return stiffness.z;
}

void ComCharacterJoint::SetTwistStiffness(const float stiffness) {
    this->stiffness.z = stiffness;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularStiffness(this->stiffness);
    }
}

const float ComCharacterJoint::GetTwistDamping() const {
    return damping.z;
}

void ComCharacterJoint::SetTwistDamping(const float damping) {
    this->damping.z = damping;
    if (constraint) {
        ((PhysGenericSpringConstraint *)constraint)->SetAngularDamping(this->damping);
    }
}

BE_NAMESPACE_END
