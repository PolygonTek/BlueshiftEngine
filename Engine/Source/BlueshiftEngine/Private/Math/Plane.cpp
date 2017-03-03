#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

int Plane::GetType() const {
    if (Normal()[0] == 0.0f) {
        if (Normal()[1] == 0.0f) {
            return Normal()[2] > 0.0f ? Type::Z : Type::NegZ;
        } else if (Normal()[2] == 0.0f) {
            return Normal()[1] > 0.0f ? Type::Y : Type::NegY;
        } else {
            return Type::ZeroX;
        }
    } else if (Normal()[1] == 0.0f) {
        if (Normal()[2] == 0.0f) {
            return Normal()[0] > 0.0f ? Type::X : Type::NegX;
        } else {
            return Type::ZeroY;
        }
    } else if (Normal()[2] == 0.0f) {
        return Type::ZeroZ;
    } else {
        return Type::NonAxial;
    }
}

const char *Plane::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), 4, precision);
}

BE_NAMESPACE_END
