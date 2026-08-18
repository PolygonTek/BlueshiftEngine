// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Octahedral Encoding

    Maps a unit vector onto an octahedron and unfolds its surface into a square.
    The vector is first projected onto the octahedron using its L1 norm. The
    lower hemisphere is then folded across the octahedron edges so the entire
    sphere can be represented by two components. Decoding reverses the fold and
    normalizes the reconstructed vector.

-------------------------------------------------------------------------------
*/

class Octahedral {
public:
                        /// Encodes a normalized, non-zero vector into [-1, 1]^2.
    static Vec2         Encode(const Vec3 &unitVector);
                        /// Decodes a value from [-1, 1]^2 into a normalized vector.
    static Vec3         Decode(const Vec2 &encoded);

private:
    static float        SignNotZero(float value);
};

BE_INLINE Vec2 Octahedral::Encode(const Vec3 &unitVector) {
    const float invL1Norm = 1.0f / (Math::Fabs(unitVector.x) + Math::Fabs(unitVector.y) + Math::Fabs(unitVector.z));
    const float x = unitVector.x * invL1Norm;
    const float y = unitVector.y * invL1Norm;

    if (unitVector.z >= 0.0f) {
        return Vec2(x, y);
    }

    return Vec2((1.0f - Math::Fabs(y)) * SignNotZero(x),
                (1.0f - Math::Fabs(x)) * SignNotZero(y));
}

BE_INLINE Vec3 Octahedral::Decode(const Vec2 &encoded) {
    Vec3 unitVector(encoded.x, encoded.y, 1.0f - Math::Fabs(encoded.x) - Math::Fabs(encoded.y));
    const float fold = unitVector.z < 0.0f ? -unitVector.z : 0.0f;

    unitVector.x += unitVector.x >= 0.0f ? -fold : fold;
    unitVector.y += unitVector.y >= 0.0f ? -fold : fold;
    unitVector.Normalize();

    return unitVector;
}

BE_INLINE float Octahedral::SignNotZero(float value) {
    return value >= 0.0f ? 1.0f : -1.0f;
}

BE_NAMESPACE_END
