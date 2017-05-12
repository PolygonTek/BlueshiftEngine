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

    Hermite();
    ~Hermite() = default;

                        /// Clears all data
    void                Clear();

                        /// Adds point with the given time t
    int                 AddPoint(float t, const T &value);

                        /// Removes point with the given index
    bool                RemoveIndex(int index);

                        /// Returns number of points
    int                 NumPoints() const { return points.Count(); }

                        /// Sets time of the point with the given index index
    void                SetTime(int index, float t) { times[index] = t; changed = true; }

                        /// Gets time of the point with the given index index
    float               GetTime(int index) const { return times[index]; }

                        /// Sets point value with the given index index
    void                SetPoint(int index, const T &value) { points[index] = value; changed = true; }

                        /// Gets point value with the given index index
    T                   GetPoint(int index) const { return points[index]; }

                        /// Gets minimum value of all points
    T                   GetMinPoint() const;

                        /// Gets maximum value of all points
    T                   GetMaxPoint() const;

                        /// Sets outgoing tangent of the point with the given index index
    void                SetOutgoingTangent(int index, const T &tangent) { outgoingTangents[index] = tangent; changed = true; }

                        /// Gets outgoing tangent of the point with the given index index
    T                   GetOutgoingTangent(int index) const { return outgoingTangents[index]; }

                        /// Sets incoming tangent of the point with the given index index
    void                SetIncomingTangent(int index, const T &tangent) { incomingTangents[index] = tangent; changed = true; }

                        /// Gets incoming tangent of the point with the given index index
    T                   GetIncomingTangent(int index) const { return incomingTangents[index]; }

    float               GetMinTime() const { return timeMinMax[0]; }
    void                SetMinTime(float time) { timeMinMax[0] = time; }
        
    float               GetMaxTime() const { return timeMinMax[1]; }
    void                SetMaxTime(float time) { timeMinMax[1] = time; }

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
    float               WrapTime(float t) const;
    T                   IntegrateSegment(float t) const;
    void                UpdateIntegrals() const;

    Array<float>        times;              // k + 1 times to arrive at each point
    Array<T>            points;             // k + 1 points
    Array<T>            outgoingTangents;   // k + 1 outgoing tangents on each segment (last one should be ignored)
    Array<T>            incomingTangents;   // k + 1 incoming tangents on each segment (first one should be ignored)
    mutable Array<T>    integrals;
    TimeWrapMode        timeWrapModes[2];
    float               timeMinMax[2];
    mutable int         currentIndex;       // cached index for fast lookup
    mutable bool        changed;
};

template <typename T>
BE_INLINE Hermite<T>::Hermite() {
    currentIndex = -1;
    changed = false;
    timeWrapModes[0] = TimeWrapMode::Clamp;
    timeWrapModes[1] = TimeWrapMode::Clamp;
    timeMinMax[0] = 0.0f;
    timeMinMax[1] = 1.0f;
}

template <typename T>
BE_INLINE void Hermite<T>::Clear() {
    timeWrapModes[0] = TimeWrapMode::Clamp;
    timeWrapModes[1] = TimeWrapMode::Clamp;
    timeMinMax[0] = 0.0f;
    timeMinMax[1] = 1.0f;
    times.Clear();
    points.Clear();
    outgoingTangents.Clear();
    incomingTangents.Clear();
    integrals.Clear();
    currentIndex = -1;
    changed = true;
}

template <typename T>
BE_INLINE int Hermite<T>::AddPoint(float t, const T &p) {
    int index = IndexForTime(t);
    T d = (index > 0 && index < times.Count() ? EvaluateDerivative(t) : T(0));
    times.Insert(t, index);
    points.Insert(p, index);
    outgoingTangents.Insert(d, index);
    incomingTangents.Insert(d, index);
    if (t < timeMinMax[0]) {
        timeMinMax[0] = t;
    } else if (t > timeMinMax[1]) {
        timeMinMax[1] = t;
    }
    changed = true;
    return index;
}

template <typename T>
BE_INLINE bool Hermite<T>::RemoveIndex(int index) {
    if (index < 0 || index >= times.Count()) {
        return false;
    }
    times.RemoveIndex(index);
    points.RemoveIndex(index);
    outgoingTangents.RemoveIndex(index);
    incomingTangents.RemoveIndex(index);
    changed = true;
    return true;
}

template <typename T>
BE_INLINE T Hermite<T>::GetMinPoint() const {
    T minPoint = T(FLT_MAX);
    const T *p = &points[0];

    for (int pointIndex = 0; pointIndex < points.Count(); pointIndex++, p++) {
        if (*p < minPoint) {
            minPoint = *p;
        }
    }
    return minPoint;
}

template <typename T>
BE_INLINE T Hermite<T>::GetMaxPoint() const {
    T maxPoint = T(FLT_MIN);
    const T *p = &points[0];

    for (int pointIndex = 0; pointIndex < points.Count(); pointIndex++, p++) {
        if (*p > maxPoint) {
            maxPoint = *p;
        }
    }
    return maxPoint;
}

template <typename T>
BE_INLINE bool Hermite<T>::Initialize(BoundaryCondition boundaryCondition) {
    // TODO: implement this
}

template <typename T>
BE_INLINE float Hermite<T>::WrapTime(float t) const {
    if (t < times.First()) {
        if (timeWrapModes[0] == TimeWrapMode::Clamp) {
            return times.First();
        } else {
            float l = times.Last() - times.First();
            float s = Math::Ceil(times.First() / l) * l - times.First();

            if (timeWrapModes[0] == TimeWrapMode::Loop) {
                float a = (s + t) / l;
                return times.First() + t - (Math::Floor(a) * l - s);
            } else {
                float b = (times.First() - t) / l;
                float fb = Math::Floor(b);
                return times.First() + ((int)fb % 2 == 0 ? (b - fb) * l : (Math::Ceil(b) - b) * l);
            }
        }
    } else if (t > times.Last()) {
        if (timeWrapModes[1] == TimeWrapMode::Clamp) {
            return times.Last();
        } else {
            float l = times.Last() - times.First();
            float b = (t - times.Last()) / l;

            if (timeWrapModes[1] == TimeWrapMode::Loop) {
                return times.First() + (t - times.Last()) - Math::Floor(b) * l;
            } else {
                float fb = Math::Floor(b);
                return times.First() + ((int)fb % 2 == 0 ? Math::Ceil(b) * l - (t - times.Last()) : (t - times.Last()) - fb * l);
            }
        }
    }

    // t is not in boundary, so just return it
    return t;
}

template <typename T>
BE_INLINE T Hermite<T>::Evaluate(float t) const {
    if (points.Count() <= 0) {
        return T(0);
    }

    if (points.Count() == 1) {
        return points[0];
    }

    t = WrapTime(t);

    // find segment and parameter
    int index = Max(IndexForTime(t), 1) - 1;
    float t0 = times[index];
    float t1 = times[index + 1];
    float u = (t - t0) / (t1 - t0);

    // evaluate
    T a =  2.0f * points[index] - 2.0f * points[index + 1] + 1.0f * outgoingTangents[index] + 1.0f * incomingTangents[index + 1];
    T b = -3.0f * points[index] + 3.0f * points[index + 1] - 2.0f * outgoingTangents[index] - 1.0f * incomingTangents[index + 1];

    return points[index] + u * (outgoingTangents[index] + u * (b + u * a));
}

template <typename T>
BE_INLINE T Hermite<T>::EvaluateDerivative(float t) const {
    if (points.Count() < 2) {
        return T(0);
    }

    t = WrapTime(t);

    // find segment and parameter
    int index = Max(IndexForTime(t), 1) - 1;
    float t0 = times[index];
    float t1 = times[index + 1];
    float u = (t - t0) / (t1 - t0);

    // evaluate
    T a =  2.0f * points[index] - 2.0f * points[index + 1] + 1.0f * outgoingTangents[index] + 1.0f * incomingTangents[index + 1];
    T b = -3.0f * points[index] + 3.0f * points[index + 1] - 2.0f * outgoingTangents[index] - 1.0f * incomingTangents[index + 1];

    return outgoingTangents[index] + u * (2.0f * b + 3.0f * u * a);
}

template <typename T>
BE_INLINE T Hermite<T>::EvaluateSecondDerivative(float t) const {
    if (points.Count() < 2) {
        return T(0);
    }

    t = WrapTime(t);

    // find segment and parameter
    int index = Max(IndexForTime(t), 1) - 1;
    float t0 = times[index];
    float t1 = times[index + 1];
    float u = (t - t0) / (t1 - t0);

    // evaluate
    T a =  2.0f * points[index] - 2.0f * points[index + 1] + 1.0f * outgoingTangents[index] + 1.0f * incomingTangents[index + 1];
    T b = -3.0f * points[index] + 3.0f * points[index + 1] - 2.0f * outgoingTangents[index] - 1.0f * incomingTangents[index + 1];

    return 2.0f * b + 6.0f * u * a;
}

template <typename T>
BE_INLINE T Hermite<T>::IntegrateSegment(float t) const {
    assert(t >= times[0] && t <= times.Last());

    if (t == times[0]) {
        return 0;
    }

    // find segment and parameter
    int index = Max(IndexForTime(t), 1) - 1;
    float t0 = times[index];
    float t1 = times[index + 1];
    float u = (t - t0) / (t1 - t0);

    // evaluate
    T a =  2.0f * points[index] - 2.0f * points[index + 1] + 1.0f * outgoingTangents[index] + 1.0f * incomingTangents[index + 1];
    T b = -3.0f * points[index] + 3.0f * points[index + 1] - 2.0f * outgoingTangents[index] - 1.0f * incomingTangents[index + 1];

    return (t1 - t0) * (u * (points[index] + u * (0.5f * outgoingTangents[index] + u * (b / 3.0f + u * 0.25f * a))));
}

template <typename T>
BE_INLINE void Hermite<T>::UpdateIntegrals() const {
    integrals.SetCount(points.Count());
    if (points.Count() == 0) {
        return;
    }

    integrals[0] = 0;

    T sum = T(0);

    for (int i = 1; i < times.Count(); i++) {
        sum += IntegrateSegment(times[i]);
        integrals[i] = sum;
    }

    integrals[0] = Integrate(times[0]);
    
    for (int i = 1; i < times.Count(); i++) {
        integrals[i] += integrals[0];
    }
}

template <typename T>
BE_INLINE T Hermite<T>::Integrate(float t) const {
    if (changed) {
        changed = false;
        UpdateIntegrals();
    }

    if (points.Count() <= 0) {
        return T(0);
    }

    if (points.Count() == 1) {
        return t * points[0];
    }

    // handle boundary
    if (t <= times.First()) {
        if (timeWrapModes[0] == TimeWrapMode::Clamp) {
            return t * points[0];
        } else {
            float l = times.Last() - times[0];
            float s = Math::Ceil(times[0] / l) * l - times[0];

            if (timeWrapModes[0] == TimeWrapMode::Loop) {
                float a = (s + t) / l;
                float fa = Math::Floor(a);

                // wrap time t, s
                t = times[0] + t - (fa * l - s);
                s += times[0];

                T intS = (s <= times[0] ? 0 : IntegrateSegment(s) + integrals[IndexForTime(s) - 1] - integrals[0]);
                T intT = (t <= times[0] ? 0 : IntegrateSegment(t) + integrals[IndexForTime(t) - 1] - integrals[0]);
                T intSegs = integrals.Last() - integrals[0];

                return fa * intSegs + intT - intS;
            } else { // TimeWrapMode::PingPong
                float a = times[0] / l;
                float b = (times[0] - t) / l;
                float fa = Math::Floor(a);
                float fb = Math::Floor(b);
                float cb = Math::Ceil(b);
                bool ra = (int)fa % 2 == 0;
                bool rb = (int)fb % 2 == 0;

                // wrap time t, s
                t = times[0] + (rb ? (b - fb) * l : (cb - b) * l);
                s += times[0];

                T intS = (s <= times[0] ? 0 : IntegrateSegment(s) + integrals[IndexForTime(s) - 1] - integrals[0]);
                T intT = (t <= times[0] ? 0 : IntegrateSegment(t) + integrals[IndexForTime(t) - 1] - integrals[0]);
                T intSegs = integrals.Last() - integrals[0];

                T intT0 = (ra ? fa * intSegs + intS : Math::Ceil(a) * intSegs - intS);
                T intTr = (rb ? fb * intSegs + intT : cb * intSegs - intT);

                return intT0 - intTr;
            }
        }
    } else if (t > times.Last()) {
        if (timeWrapModes[1] == TimeWrapMode::Clamp) {
            return (t - times.Last()) * points.Last();
        } else {
            float l = times.Last() - times[0];
            float b = (t - times.Last()) / l;

            if (timeWrapModes[1] == TimeWrapMode::Loop) {
                float fb = Math::Floor(b);

                // wrap time t
                t = times[0] + (t - times.Last()) - fb * l;

                T intT = (t <= times[0] ? 0 : IntegrateSegment(t) + integrals[IndexForTime(t) - 1] - integrals[0]);
                T intSegs = integrals.Last() - integrals[0];

                return integrals.Last() + fb * intSegs + intT;
            } else { // TimeWrapMode::PingPong
                float fb = Math::Floor(b);
                float cb = Math::Ceil(b);
                bool rb = (int)fb % 2 == 0;

                // wrap time t
                t = times[0] + (rb ? cb * l - (t - times.Last()) : (t - times.Last()) - fb * l);

                T intT = (t <= times[0] ? 0 : IntegrateSegment(t) + integrals[IndexForTime(t) - 1] - integrals[0]);
                T intSegs = integrals.Last() - integrals[0];

                return integrals.Last() + (rb ? cb * intSegs - intT : fb * intSegs + intT);
            } 
        }
    }

    return IntegrateSegment(t) + (t <= times[0] ? 0 : integrals[IndexForTime(t) - 1]);
}

template <typename T>
BE_INLINE int Hermite<T>::IndexForTime(float t) const {
    if (currentIndex >= 0 && currentIndex <= times.Count()) {
        // use the cached index if it is still valid
        if (currentIndex == 0) {
            if (t <= times[currentIndex]) {
                return currentIndex;
            }
        } else if (currentIndex == times.Count()) {
            if (t > times[currentIndex - 1]) {
                return currentIndex;
            }
        } else if (t > times[currentIndex - 1] && t <= times[currentIndex]) {
            return currentIndex;
        } else if (t > times[currentIndex] && (currentIndex + 1 == times.Count() || t <= times[currentIndex + 1])) {
            // use the next index
            currentIndex++;
            return currentIndex;
        }
    }

    // use binary search to find the index for the given t
    int len = times.Count();
    int mid = len;
    int offset = 0;
    int res = 0;
    while (mid > 0) {
        mid = len >> 1;
        if (t == times[offset + mid]) {
            return offset + mid;
        } else if (t > times[offset + mid]) {
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