#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

Angles CQuat::ToAngles() const {
    return ToQuat().ToAngles();
}

Rotation CQuat::ToRotation() const {
    return ToQuat().ToRotation();
}

Mat3 CQuat::ToMat3() const {
    return ToQuat().ToMat3();
}

Mat4 CQuat::ToMat4() const {
    return ToQuat().ToMat4();
}

BE_NAMESPACE_END
