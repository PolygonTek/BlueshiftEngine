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

#pragma once

/*
-------------------------------------------------------------------------------

    Hermite Spline

-------------------------------------------------------------------------------
*/

#include "Containers/Array.h"

BE_NAMESPACE_BEGIN

template <typename T> 
class Hermite {
public:
    enum BoundaryCondition {
        ClampedBoundary,
        NaturalBoundary,
        CyclicBoundary
    };

    enum TimeWrapMode {
        Clamp,
        Loop,
        PingPong
    };

    enum TangentMode {
        FreeSmooth      = 0,
        Auto            = 1,
        Broken          = 2,
        LeftLinear      = 4,
        LeftConstant    = 8,
        RightLinear     = 16,
        RightConstant   = 32
    };

    Hermite();
    ~Hermite() = default;

                        /// Compare with another
    bool                operator==(const Hermite<T> &rhs) const;
    bool                operator!=(const Hermite<T> &rhs) const;

                        /// Clears all data
    void                Clear();

                        /// Adds point with the given time t
    int                 AddPoint(float t, const T &value);

                        /// Removes point with the given key index index
    bool                RemoveIndex(int index);

                        /// Returns number of points
    int                 NumKeys() const { return keys.Count(); }

                        /// Sets time of the point with the given key index index
                        /// If key index changed, returns new index
    int                 SetTime(int index, float t);

                        /// Gets time of the point with the given key index index
    float               GetTime(int index) const { return keys[index].time; }

                        /// Sets point value with the given key index index
    void                SetPoint(int index, const T &value) { keys[index].point = value; changed = true; }

                        /// Moves all points with the given translation value
    void                MovePoints(const T &translation);

                        /// Gets point value with the given key index index
    T                   GetPoint(int index) const { return keys[index].point; }

                        /// Gets minimum value of all points
    T                   GetMinPoint() const;

                        /// Gets maximum value of all points
    T                   GetMaxPoint() const;

                        /// Sets outgoing slope of the point with the given key index index
    void                SetOutgoingSlope(int index, const T &slope) { keys[index].outSlope = slope; changed = true; }

                        /// Gets outgoing slope of the point with the given key index index
    T                   GetOutgoingSlope(int index) const { return keys[index].outSlope; }

                        /// Sets incoming slope of the point with the given key index index
    void                SetIncomingSlope(int index, const T &slope) { keys[index].inSlope = slope; changed = true; }

                        /// Gets incoming slope of the point with the given key index index
    T                   GetIncomingSlope(int index) const { return keys[index].inSlope; }

                        /// Sets tangent mode flags with the given key index index
    void                SetTangentModeFlags(int index, int flags) { keys[index].tangentModeFlags = flags; }

                        /// Gets tangent mode flags with the given key index 
    int                 GetTangentModeFlags(int index) const { return keys[index].tangentModeFlags; }

    TimeWrapMode        GetMinTimeWrapMode() const { return timeWrapModes[0]; }
    void                SetMinTimeWrapMode(TimeWrapMode timeWrapMode) { timeWrapModes[0] = timeWrapMode; }

    TimeWrapMode        GetMaxTimeWrapMode() const { return timeWrapModes[1]; }
    void                SetMaxTimeWrapMode(TimeWrapMode timeWrapMode) { timeWrapModes[1] = timeWrapMode; }

                        /// Initializes Hermite spline with given boundary condition
    bool                Initialize(BoundaryCondition boundaryCondition);

                        /// Evaluates point
    T                   Evaluate(float t) const;

                        /// Evaluates derivative
    T                   EvaluateDerivative(float t) const;

                        /// Evaluates second derivative
    T                   EvaluateSecondDerivative(float t) const;

                        /// Evaluates antiderivate from 0 to t
    T                   Integrate(float t) const;

                        /// Evaluates antiderivate from t0 to t1
    T                   Integrate(float t0, float t1) const { return Integrate(t1) - Integrate(t0); }

                        /// Gets index at time t
    int                 IndexForTime(float t) const;
    
private:
    struct Key {
        float           time;               // times to arrive at each point
        T               point;              // geometry sample point
        T               inSlope;            // incoming slope on each segment (first one should be ignored)
        T               outSlope;           // outgoing slope on each segment (last one should be ignored)
        int             tangentModeFlags;

        bool operator==(const Key &rhs) const {
            if (time == rhs.time && point == rhs.point && inSlope == rhs.inSlope && outSlope == rhs.outSlope && tangentModeFlags == rhs.tangentModeFlags) {
                return true;
            }
            return false;
        }

        bool operator!=(const Key &rhs) const {
            if (time == rhs.time && point == rhs.point && inSlope == rhs.inSlope && outSlope == rhs.outSlope && tangentModeFlags == rhs.tangentModeFlags) {
                return false;
            }
            return true;
        }
    };

    float               WrapTime(float t) const;
    T                   IntegrateSegment(float t) const;
    void                UpdateIntegrals() const;

    Array<Key>          keys;
    mutable Array<T>    integrals;
    TimeWrapMode        timeWrapModes[2];
    mutable int         currentIndex;       // cached index for fast lookup
    mutable bool        changed;
};

template <typename T>
BE_INLINE Hermite<T>::Hermite() {
    currentIndex = -1;
    changed = false;
    timeWrapModes[0] = TimeWrapMode::Clamp;
    timeWrapModes[1] = TimeWrapMode::Clamp;
}

template <typename T>
BE_INLINE bool Hermite<T>::operator==(const Hermite<T> &rhs) const {
    if (keys == rhs.keys && timeWrapModes[0] == rhs.timeWrapModes[0] && timeWrapModes[1] == timeWrapModes[1]) {
        return true;
    }
    return false;
}

template <typename T>
BE_INLINE bool Hermite<T>::operator!=(const Hermite<T> &rhs) const {
    if (keys == rhs.keys && timeWrapModes[0] == rhs.timeWrapModes[0] && timeWrapModes[1] == timeWrapModes[1]) {
        return false;
    }
    return true;
}

template <typename T>
BE_INLINE void Hermite<T>::Clear() {
    timeWrapModes[0] = TimeWrapMode::Clamp;
    timeWrapModes[1] = TimeWrapMode::Clamp;
    keys.Clear();
    integrals.Clear();
    currentIndex = -1;
    changed = true;
}

template <typename T>
BE_INLINE int Hermite<T>::AddPoint(float time, const T &point) {
    int index = IndexForTime(time);
    T slope = T(0);
    if (index > 0 && index < keys.Count()) {
        slope = EvaluateDerivative(time);
    }
    Key key;
    key.time = time;
    key.point = point;
    key.outSlope = slope;
    key.inSlope = slope;
    key.tangentModeFlags = TangentMode::FreeSmooth;
    keys.Insert(key, index);
    changed = true;
    return index;
}

template <typename T>
BE_INLINE bool Hermite<T>::RemoveIndex(int index) {
    if (index < 0 || index >= keys.Count()) {
        return false;
    }
    keys.RemoveIndex(index);
    changed = true;
    return true;
}

template <typename T>
BE_INLINE int Hermite<T>::SetTime(int index, float t) { 
    if ((index > 0 && t <= keys[index - 1].time) || (index < keys.Count() - 1 && t > keys[index + 1].time)) {
        Key key;
        key = keys[index];
        key.time = t;

        RemoveIndex(index);

        int newIndex = IndexForTime(t);
        keys.Insert(key, newIndex);
        changed = true;
        return newIndex;
    }

    keys[index].time = t;
    changed = true;
    return index;
}

template <typename T>
BE_INLINE void Hermite<T>::MovePoints(const T &translation) {
    Key *key = &keys[0];

    for (int keyIndex = 0; keyIndex < keys.Count(); keyIndex++, key++) {
        key->point += translation;
    }
    changed = true;
}

template <typename T>
BE_INLINE T Hermite<T>::GetMinPoint() const {
    T minPoint = T(FLT_MAX);
    const Key *key = &keys[0];

    for (int keyIndex = 0; keyIndex < keys.Count(); keyIndex++, key++) {
        if (key->point < minPoint) {
            minPoint = key->point;
        }
    }
    return minPoint;
}

template <typename T>
BE_INLINE T Hermite<T>::GetMaxPoint() const {
    T maxPoint = T(-FLT_MAX);
    const Key *key = &keys[0];

    for (int keyIndex = 0; keyIndex < keys.Count(); keyIndex++, key++) {
        if (key->point > maxPoint) {
            maxPoint = key->point;
        }
    }
    return maxPoint;
}

template <typename T>
BE_INLINE bool Hermite<T>::Initialize(BoundaryCondition boundaryCondition) {
    // TODO: implement this
    return false;
}

template <typename T>
BE_INLINE float Hermite<T>::WrapTime(float t) const {
    const Key &k0 = keys.First();
    const Key &kn = keys.Last();

    if (t < k0.time) {
        if (timeWrapModes[0] == TimeWrapMode::Clamp) {
            return k0.time;
        } else {
            float l = kn.time - k0.time;
            float s = Math::Ceil(k0.time / l) * l - k0.time;

            if (timeWrapModes[0] == TimeWrapMode::Loop) {
                float a = (s + t) / l;
                return k0.time + t - (Math::Floor(a) * l - s);
            } else {
                float b = (k0.time - t) / l;
                float fb = Math::Floor(b);
                return k0.time + ((int)fb % 2 == 0 ? (b - fb) * l : (Math::Ceil(b) - b) * l);
            }
        }
    } else if (t > kn.time) {
        if (timeWrapModes[1] == TimeWrapMode::Clamp) {
            return kn.time;
        } else {
            float l = kn.time - k0.time;
            float b = (t - kn.time) / l;

            if (timeWrapModes[1] == TimeWrapMode::Loop) {
                return k0.time + (t - kn.time) - Math::Floor(b) * l;
            } else {
                float fb = Math::Floor(b);
                return k0.time + ((int)fb % 2 == 0 ? Math::Ceil(b) * l - (t - kn.time) : (t - kn.time) - fb * l);
            }
        }
    }

    // t is not in boundary, so just return it
    return t;
}

template <typename T>
BE_INLINE T Hermite<T>::Evaluate(float t) const {
    if (keys.Count() <= 0) {
        return T(0);
    }

    if (keys.Count() == 1) {
        return keys[0].point;
    }

    t = WrapTime(t);

    // find segment and parameter
    int index = Max(IndexForTime(t), 1) - 1;
    const Key &k0 = keys[index];
    const Key &k1 = keys[index + 1];

    // early out for constant tangent mode
    if (Math::Fabs(k0.outSlope) >= 100 || Math::Fabs(k1.inSlope) >= 100) {
        return k0.point;
    }

    float t0 = k0.time;
    float t1 = k1.time;
    float dt = t1 - t0;
    float u = (t - t0) / dt;

    float outTangent = k0.outSlope * dt;
    float inTangent = k1.inSlope * dt;

    // evaluate
    T a =  2.0f * k0.point - 2.0f * k1.point + outTangent + inTangent;
    T b = -3.0f * k0.point + 3.0f * k1.point - 2.0f * outTangent - inTangent;

    return k0.point + u * (outTangent + u * (b + u * a));
}

template <typename T>
BE_INLINE T Hermite<T>::EvaluateDerivative(float t) const {
    if (keys.Count() < 2) {
        return T(0);
    }

    t = WrapTime(t);

    // find segment and parameter
    int index = Max(IndexForTime(t), 1) - 1;
    const Key &k0 = keys[index];
    const Key &k1 = keys[index + 1];

    // early out for constant tangent mode
    if (Math::Fabs(k0.outSlope) >= 100 || Math::Fabs(k1.inSlope) >= 100) {
        return T(0);
    }

    float t0 = k0.time;
    float t1 = k1.time;
    float dt = t1 - t0;
    float invDt = 1.0f / dt;
    float u = (t - t0) * invDt;

    float outTangent = k0.outSlope * dt;
    float inTangent = k1.inSlope * dt;

    // evaluate
    T a =  2.0f * k0.point - 2.0f * k1.point + outTangent + inTangent;
    T b = -3.0f * k0.point + 3.0f * k1.point - 2.0f * outTangent - inTangent;

    return (outTangent + u * (2.0f * b + 3.0f * u * a)) * invDt;
}

template <typename T>
BE_INLINE T Hermite<T>::EvaluateSecondDerivative(float t) const {
    if (keys.Count() < 2) {
        return T(0);
    }

    t = WrapTime(t);

    // find segment and parameter
    int index = Max(IndexForTime(t), 1) - 1;
    const Key &k0 = keys[index];
    const Key &k1 = keys[index + 1];

    // early out for constant tangent mode
    if (Math::Fabs(k0.outSlope) >= 100 || Math::Fabs(k1.inSlope) >= 100) {
        return T(0);
    }

    float t0 = k0.time;
    float t1 = k1.time;
    float dt = t1 - t0;
    float invDt = 1.0f / dt;
    float u = (t - t0) * invDt;

    float outTangent = k0.outSlope * dt;
    float inTangent = k1.inSlope * dt;

    // evaluate
    T a =  2.0f * k0.point - 2.0f * k1.point + outTangent + inTangent;
    T b = -3.0f * k0.point + 3.0f * k1.point - 2.0f * outTangent - inTangent;

    return (2.0f * b + 6.0f * u * a) * invDt;
}

template <typename T>
BE_INLINE T Hermite<T>::IntegrateSegment(float t) const {
    assert(t >= keys[0].time && t <= keys.Last().time);

    if (t == keys[0].time) {
        return 0;
    }

    // find segment and parameter
    int index = Max(IndexForTime(t), 1) - 1;
    const Key &k0 = keys[index];
    const Key &k1 = keys[index + 1];

    // early out for constant tangent mode
    if (Math::Fabs(k0.outSlope) >= 100 || Math::Fabs(k1.inSlope) >= 100) {
        return (t - k0.time) * k0.point;
    }
   
    float t0 = k0.time;
    float t1 = k1.time;
    float dt = t1 - t0;
    float u = (t - t0) / dt;

    float outTangent = k0.outSlope * dt;
    float inTangent = k1.inSlope * dt;

    // evaluate
    T a =  2.0f * k0.point - 2.0f * k1.point + outTangent + inTangent;
    T b = -3.0f * k0.point + 3.0f * k1.point - 2.0f * outTangent - inTangent;

    return (t1 - t0) * (u * (k0.point + u * (0.5f * outTangent + u * (b / 3.0f + u * 0.25f * a))));
}

template <typename T>
BE_INLINE void Hermite<T>::UpdateIntegrals() const {
    integrals.SetCount(keys.Count());
    if (keys.Count() == 0) {
        return;
    }

    integrals[0] = 0;

    T sum = T(0);

    for (int i = 1; i < keys.Count(); i++) {
        sum += IntegrateSegment(keys[i].time);
        integrals[i] = sum;
    }

    integrals[0] = Integrate(keys[0].time);
    
    for (int i = 1; i < keys.Count(); i++) {
        integrals[i] += integrals[0];
    }
}

template <typename T>
BE_INLINE T Hermite<T>::Integrate(float t) const {
    if (changed) {
        changed = false;
        UpdateIntegrals();
    }

    if (keys.Count() <= 0) {
        return T(0);
    }

    if (keys.Count() == 1) {
        return t * keys[0].point;
    }

    const Key &k0 = keys.First();
    const Key &kn = keys.Last();

    // handle boundary
    if (t <= k0.time) {
        if (timeWrapModes[0] == TimeWrapMode::Clamp) {
            return t * k0.point;
        } else {
            float l = kn.time - k0.time;
            if (l == 0.0f) {
                return 0.0f;
            }

            float s = Math::Ceil(k0.time / l) * l - k0.time;

            if (timeWrapModes[0] == TimeWrapMode::Loop) {
                float a = (s + t) / l;
                float fa = Math::Floor(a);

                // wrap time t, s
                t = k0.time + t - (fa * l - s);
                s += k0.time;

                T intS = (s <= k0.time ? 0 : IntegrateSegment(s) + integrals[IndexForTime(s) - 1] - integrals[0]);
                T intT = (t <= k0.time ? 0 : IntegrateSegment(t) + integrals[IndexForTime(t) - 1] - integrals[0]);
                T intSegs = integrals.Last() - integrals[0];

                return fa * intSegs + intT - intS;
            } else { // TimeWrapMode::PingPong
                float a = k0.time / l;
                float b = (k0.time - t) / l;
                float fa = Math::Floor(a);
                float fb = Math::Floor(b);
                float cb = Math::Ceil(b);
                bool ra = (int)fa % 2 == 0;
                bool rb = (int)fb % 2 == 0;

                // wrap time t, s
                t = k0.time + (rb ? (b - fb) * l : (cb - b) * l);
                s += k0.time;

                T intS = (s <= k0.time ? 0 : IntegrateSegment(s) + integrals[IndexForTime(s) - 1] - integrals[0]);
                T intT = (t <= k0.time ? 0 : IntegrateSegment(t) + integrals[IndexForTime(t) - 1] - integrals[0]);
                T intSegs = integrals.Last() - integrals[0];

                T intT0 = (ra ? fa * intSegs + intS : Math::Ceil(a) * intSegs - intS);
                T intTr = (rb ? fb * intSegs + intT : cb * intSegs - intT);

                return intT0 - intTr;
            }
        }
    } else if (t > kn.time) {
        if (timeWrapModes[1] == TimeWrapMode::Clamp) {
            return (t - kn.time) * kn.point;
        } else {
            float l = kn.time - k0.time;
            if (l == 0.0f) {
                return 0.0f;
            }

            float b = (t - kn.time) / l;

            if (timeWrapModes[1] == TimeWrapMode::Loop) {
                float fb = Math::Floor(b);

                // wrap time t
                t = k0.time + (t - kn.time) - fb * l;

                T intT = (t <= k0.time ? 0 : IntegrateSegment(t) + integrals[IndexForTime(t) - 1] - integrals[0]);
                T intSegs = integrals.Last() - integrals[0];

                return integrals.Last() + fb * intSegs + intT;
            } else { // TimeWrapMode::PingPong
                float fb = Math::Floor(b);
                float cb = Math::Ceil(b);
                bool rb = (int)fb % 2 == 0;

                // wrap time t
                t = k0.time + (rb ? cb * l - (t - kn.time) : (t - kn.time) - fb * l);

                T intT = (t <= k0.time ? 0 : IntegrateSegment(t) + integrals[IndexForTime(t) - 1] - integrals[0]);
                T intSegs = integrals.Last() - integrals[0];

                return integrals.Last() + (rb ? cb * intSegs - intT : fb * intSegs + intT);
            } 
        }
    }

    return IntegrateSegment(t) + (t <= k0.time ? 0 : integrals[IndexForTime(t) - 1]);
}

template <typename T>
BE_INLINE int Hermite<T>::IndexForTime(float t) const {
    if (currentIndex >= 0 && currentIndex <= keys.Count()) {
        // use the cached index if it is still valid
        if (currentIndex == 0) {
            if (t <= keys[currentIndex].time) {
                return currentIndex;
            }
        } else if (currentIndex == keys.Count()) {
            if (t > keys[currentIndex - 1].time) {
                return currentIndex;
            }
        } else if (t > keys[currentIndex - 1].time && t <= keys[currentIndex].time) {
            return currentIndex;
        } else if (t > keys[currentIndex].time && (currentIndex + 1 == keys.Count() || t <= keys[currentIndex + 1].time)) {
            // use the next index
            currentIndex++;
            return currentIndex;
        }
    }

    // use binary search to find the index for the given t
    int len = keys.Count();
    int mid = len;
    int offset = 0;
    int res = 0;
    while (mid > 0) {
        mid = len >> 1;
        if (t == keys[offset + mid].time) {
            return offset + mid;
        } else if (t > keys[offset + mid].time) {
            offset += mid;
            len -= mid;
            res = 1;
        } else {
            len -= mid;
            res = 0;
        }
    }
    currentIndex = offset + res;
    return currentIndex;
}

BE_NAMESPACE_END