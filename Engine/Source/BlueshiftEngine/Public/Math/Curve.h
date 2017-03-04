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

    Curve base template.

-------------------------------------------------------------------------------
*/

#include "Containers/Array.h"

BE_NAMESPACE_BEGIN

template <typename T>
class Curve {
public:
    Curve();
    virtual ~Curve();

                        // add a timed / value pair to the spline
                        // returns the index to the inserted pair
    virtual int         AddValue(const float time, const T &value);

    virtual void        RemoveIndex(const int index) { values.RemoveIndex(index); times.RemoveIndex(index); changed = true; }
    
    virtual void        Clear() { values.Clear(); times.Clear(); currentIndex = -1; changed = true; }

                        // get the value for the given time
    virtual T           GetCurrentValue(const float time) const;

                        // get the first derivative for the given time
    virtual T           GetCurrentFirstDerivative(const float time) const;

                        // get the second derivative for the given time
    virtual T           GetCurrentSecondDerivative(const float time) const;

    virtual bool        IsDone(const float time) const;

    int                 GetNumValues() const { return values.Count(); }
    void                SetValue(const int index, const T &value) { values[index] = value; changed = true; }
    T                   GetValue(const int index) const { return values[index]; }
    T *                 GetValueAddress(const int index) { return &values[index]; }
    void                SetTime(const int index, const float time) { times[index] = time; changed = true; }
    float               GetTime(const int index) const { return times[index]; }

    float               GetLengthForTime(const float time) const;
    float               GetTimeForLength(const float length, const float epsilon = 0.1f) const;
    float               GetLengthBetweenKnots(const int i0, const int i1) const;

    void                MakeUniform(const float totalTime);
    void                SetConstantSpeed(const float totalTime);
    void                ShiftTime(const float deltaTime);
    void                Translate(const T &translation);

protected:
                        // find the index for the first time greater than or equal to the given time
    int                 IndexForTime(const float time) const;

                        // get the value for the given time
    float               TimeForIndex(const int index) const;

                        // get the value for the given time
    T                   ValueForIndex(const int index) const;

    float               GetSpeed(const float time) const;
    float               RombergIntegral(const float t0, const float t1, const int order) const;

    Array<float>        times;          // knots
    Array<T>            values;         // knot values
    
    mutable int         currentIndex;   // cached index for fast lookup
    mutable bool        changed;        // set whenever the curve changes
};

template <typename T>
BE_INLINE Curve<T>::Curve() {
    currentIndex = -1;
    changed = false;
}

template <typename T>
BE_INLINE Curve<T>::~Curve() {
}

template <typename T>
BE_INLINE int Curve<T>::AddValue(const float time, const T &value) {
    int i = IndexForTime(time);
    times.Insert(time, i);
    values.Insert(value, i);
    changed = true;
    return i;
}

template <typename T>
BE_INLINE T Curve<T>::GetCurrentValue(const float time) const {
    int i = IndexForTime(time);
    if (i >= values.Count()) {
        return values[values.Count() - 1];
    } else {
        return values[i];
    }
}

template <typename T>
BE_INLINE T Curve<T>::GetCurrentFirstDerivative(const float time) const {
    return (values[0] - values[0]); //-V501
}

template <typename T>
BE_INLINE T Curve<T>::GetCurrentSecondDerivative(const float time) const {
    return (values[0] - values[0]); //-V501
}

template <typename T>
BE_INLINE bool Curve<T>::IsDone(const float time) const {
    return (time >= times[times.Count() - 1]);
}

template <typename T>
BE_INLINE float Curve<T>::GetSpeed(const float time) const {
    T value = GetCurrentFirstDerivative(time);
    float speed = 0.0f;
    for (int i = 0; i < value.GetDimension(); i++) {
        speed += value[i] * value[i];
    }
    return Math::Sqrt(speed);
}

template <typename T>
BE_INLINE float Curve<T>::RombergIntegral(const float t0, const float t1, const int order) const {
    float *temp[2];
    temp[0] = (float *)_alloca16(order * sizeof(float));
    temp[1] = (float *)_alloca16(order * sizeof(float));

    float delta = t1 - t0;
    temp[0][0] = 0.5f * delta * (GetSpeed(t0) + GetSpeed(t1));

    int m = 1;
    for (int i = 2; i <= order; i++, m *= 2, delta *= 0.5f) {
        // approximate using the trapezoid rule
        float sum = 0.0f;
        for (int j = 1; j <= m; j++) {
            sum += GetSpeed(t0 + delta * (j - 0.5f));
        }

        // Richardson extrapolation
        temp[1][0] = 0.5f * (temp[0][0] + delta * sum);
        int n = 4;
        for (int k = 1; k < i; k++, n *= 4) {
            temp[1][k] = (n * temp[1][k - 1] - temp[0][k - 1]) / (n - 1);
        }

        for (int j = 0; j < i; j++) {
            temp[0][j] = temp[1][j];
        }
    }
    return temp[0][order - 1];
}

template <typename T>
BE_INLINE float Curve<T>::GetLengthBetweenKnots(const int i0, const int i1) const {
    float length = 0.0f;
    for (int i = i0; i < i1; i++) {
        length += RombergIntegral(times[i], times[i + 1], 5);
    }
    return length;
}

template <typename T>
BE_INLINE float Curve<T>::GetLengthForTime(const float time) const {
    float length = 0.0f;
    int index = IndexForTime(time);
    for (int i = 0; i < index; i++) {
        length += RombergIntegral(times[i], times[i + 1], 5);
    }
    length += RombergIntegral(times[index], time, 5);
    return length;
}

template <typename T>
BE_INLINE float Curve<T>::GetTimeForLength(const float length, const float epsilon) const {
    if (length <= 0.0f) {
        return times[0];
    }

    float *accumLength = (float *)_alloca16(values.Count() * sizeof(float));
    float totalLength = 0.0f;
    int index;
    for (index = 0; index < values.Count() - 1; index++) {
        totalLength += GetLengthBetweenKnots(index, index + 1);
        accumLength[index] = totalLength;
        if (length < accumLength[index]) {
            break;
        }
    }

    if (index >= values.Count() - 1) {
        return times[times.Count() - 1];
    }

    float len0, len1;
    if (index == 0) {
        len0 = length;
        len1 = accumLength[0];
    } else {
        len0 = length - accumLength[index - 1];
        len1 = accumLength[index] - accumLength[index - 1];
    }

    // invert the arc length integral using Newton's method
    float t = (times[index + 1] - times[index]) * len0 / len1;
    for (int i = 0; i < 32; i++) {
        float diff = RombergIntegral(times[index], times[index] + t, 5) - len0;
        if (Math::Fabs(diff) <= epsilon) {
            return times[index] + t;
        }
        t -= diff / GetSpeed(times[index] + t);
    }
    return times[index] + t;
}

template <typename T>
BE_INLINE void Curve<T>::MakeUniform(const float totalTime) {
    int n = times.Count() - 1;
    for (int i = 0; i <= n; i++) {
        times[i] = i * totalTime / n;
    }
    changed = true;
}

template <typename T>
BE_INLINE void Curve<T>::SetConstantSpeed(const float totalTime) {
    float *length = (float *)_alloca16(values.Count() * sizeof(float));
    float totalLength = 0.0f;
    for (int i = 0; i < values.Count() - 1; i++) {
        length[i] = GetLengthBetweenKnots(i, i + 1);
        totalLength += length[i];
    }
    float scale = totalTime / totalLength;
    float t = 0.0f;
    for (int i = 0; i < times.Count() - 1; i++) {
        times[i] = t;
        t += scale * length[i];
    }
    times[times.Count() - 1] = totalTime;
    changed = true;
}

template <typename T>
BE_INLINE void Curve<T>::ShiftTime(const float deltaTime) {
    for (int i = 0; i < times.Count(); i++) {
        times[i] += deltaTime;
    }
    changed = true;
}

template <typename T>
BE_INLINE void Curve<T>::Translate(const T &translation) {
    for (int i = 0; i < values.Count(); i++) {
        values[i] += translation;
    }
    changed = true;
}

template <typename T>
BE_INLINE int Curve<T>::IndexForTime(const float time) const {
    if (currentIndex >= 0 && currentIndex <= times.Count()) {
        // use the cached index if it is still valid
        if (currentIndex == 0) {
            if (time <= times[currentIndex]) {
                return currentIndex;
            }
        } else if (currentIndex == times.Count()) {
            if (time > times[currentIndex - 1]) {
                return currentIndex;
            }
        } else if (time > times[currentIndex - 1] && time <= times[currentIndex]) {
            return currentIndex;
        } else if (time > times[currentIndex] && (currentIndex + 1 == times.Count() || time <= times[currentIndex + 1])) {
            // use the next index
            currentIndex++;
            return currentIndex;
        }
    }

    // use binary search to find the index for the given time
    int len = times.Count();
    int mid = len;
    int offset = 0;
    int res = 0;
    while (mid > 0) {
        mid = len >> 1;
        if (time == times[offset + mid]) {
            return offset + mid;
        } else if (time > times[offset + mid]) {
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

template <typename T>
BE_INLINE T Curve<T>::ValueForIndex(const int index) const {
    int n = values.Count() - 1;

    if (index < 0) {
        return values[0] + index * (values[1] - values[0]);
    } else if (index > n) {
        return values[n] + (index - n) * (values[n] - values[n - 1]);
    }
    return values[index];
}

template <typename T>
BE_INLINE float Curve<T>::TimeForIndex(const int index) const {
    int n = times.Count() - 1;

    if (index < 0) {
        return times[0] + index * (times[1] - times[0]);
    } else if (index > n) {
        return times[n] + (index - n) * (times[n] - times[n - 1]);
    }
    return times[index];
}

/*
-------------------------------------------------------------------------------

    Bezier Curve template.
    The degree of the polynomial equals the number of knots minus one.

-------------------------------------------------------------------------------
*/

template <typename T>
class Curve_Bezier : public Curve<T> {
public:
    Curve_Bezier();

                        // get the value for the given time
    virtual T           GetCurrentValue(const float time) const override;

                        // get the first derivative for the given time
    virtual T           GetCurrentFirstDerivative(const float time) const override;

                        // get the second derivative for the given time
    virtual T           GetCurrentSecondDerivative(const float time) const override;

protected:
                        // bezier basis functions
    void                Basis(const int order, const float t, float *bvals) const;

                        // first derivative of bezier basis functions
    void                BasisFirstDerivative(const int order, const float t, float *bvals) const;
    
                        // second derivative of bezier basis functions
    void                BasisSecondDerivative(const int order, const float t, float *bvals) const;
};

template <typename T>
BE_INLINE Curve_Bezier<T>::Curve_Bezier() {
}

template <typename T>
BE_INLINE T Curve_Bezier<T>::GetCurrentValue(const float time) const {
    float *bvals = (float *)_alloca16(this->values.Count() * sizeof(float));

    Basis(this->values.Count(), time, bvals);
    T v = bvals[0] * this->values[0];
    for (int i = 1; i < this->values.Count(); i++) {
        v += bvals[i] * this->values[i];
    }
    return v;
}

template <typename T>
BE_INLINE T Curve_Bezier<T>::GetCurrentFirstDerivative(const float time) const {
    float *bvals = (float *)_alloca16(this->values.Count() * sizeof(float));

    BasisFirstDerivative(this->values.Count(), time, bvals);
    T v = bvals[0] * this->values[0];
    for (int i = 1; i < this->values.Count(); i++) {
        v += bvals[i] * this->values[i];
    }
    float d = (this->times[this->times.Count() - 1] - this->times[0]);
    return ((float)(this->values.Count() - 1) / d) * v;
}

template <typename T>
BE_INLINE T Curve_Bezier<T>::GetCurrentSecondDerivative(const float time) const {
    float *bvals = (float *)_alloca16(this->values.Count() * sizeof(float));

    BasisSecondDerivative(this->values.Count(), time, bvals);
    T v = bvals[0] * this->values[0];
    for (int i = 1; i < this->values.Count(); i++) {
        v += bvals[i] * this->values[i];
    }
    float d = (this->times[this->times.Count() - 1] - this->times[0]);
    return ((float)(this->values.Count() - 2) * (this->values.Count() - 1) / (d * d)) * v;
}

template <typename T>
BE_INLINE void Curve_Bezier<T>::Basis(const int order, const float t, float *bvals) const {
    bvals[0] = 1.0f;
    int d = order - 1;
    if (d <= 0) {
        return;
    }

    float *c = (float *)_alloca16((d + 1) * sizeof(float));
    float s = (float)(t - this->times[0]) / (this->times[this->times.Count() - 1] - this->times[0]);
    float o = 1.0f - s;
    float ps = s;
    float po = o;

    for (int i = 1; i < d; i++) {
        c[i] = 1.0f;
    }
    for (int i = 1; i < d; i++) {
        c[i - 1] = 0.0f;
        float c1 = c[i];
        c[i] = 1.0f;
        for (int j = i + 1; j <= d; j++) {
            float c2 = c[j];
            c[j] = c1 + c[j - 1];
            c1 = c2;
        }
        bvals[i] = c[d] * ps;
        ps *= s;
    }
    for (int i = d - 1; i >= 0; i--) {
        bvals[i] *= po;
        po *= o;
    }
    bvals[d] = ps;
}

template <typename T>
BE_INLINE void Curve_Bezier<T>::BasisFirstDerivative(const int order, const float t, float *bvals) const {
    Basis(order - 1, t, bvals + 1);
    bvals[0] = 0.0f;
    for (int i = 0; i < order - 1; i++) {
        bvals[i] -= bvals[i + 1];
    }
}

template <typename T>
BE_INLINE void Curve_Bezier<T>::BasisSecondDerivative(const int order, const float t, float *bvals) const {
    BasisFirstDerivative(order - 1, t, bvals + 1);
    bvals[0] = 0.0f;
    for (int i = 0; i < order - 1; i++) {
        bvals[i] -= bvals[i + 1];
    }
}

/*
-------------------------------------------------------------------------------

    Quadratic Bezier Curve template.
    Should always have exactly three knots.

-------------------------------------------------------------------------------
*/

template <typename T>
class Curve_QuadraticBezier : public Curve<T> {
public:
    Curve_QuadraticBezier();

                        // get the value for the given time
    virtual T           GetCurrentValue(const float time) const override;

                        // get the first derivative for the given time
    virtual T           GetCurrentFirstDerivative(const float time) const override;

                        // get the second derivative for the given time
    virtual T           GetCurrentSecondDerivative(const float time) const override;

protected:
                        // quadratic bezier basis functions
    void                Basis(const float t, float *bvals) const;

                        // first derivative of quadratic bezier basis functions
    void                BasisFirstDerivative(const float t, float *bvals) const;

                        // second derivative of quadratic bezier basis functions
    void                BasisSecondDerivative(const float t, float *bvals) const;
};

template <typename T>
BE_INLINE Curve_QuadraticBezier<T>::Curve_QuadraticBezier() {
}

template <typename T>
BE_INLINE T Curve_QuadraticBezier<T>::GetCurrentValue(const float time) const {
    float bvals[3];
    assert(this->values.Count() == 3);
    Basis(time, bvals);
    return (bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2]);
}

template <typename T>
BE_INLINE T Curve_QuadraticBezier<T>::GetCurrentFirstDerivative(const float time) const {
    float bvals[3];
    assert(this->values.Count() == 3);
    BasisFirstDerivative(time, bvals);
    float d = (this->times[2] - this->times[0]);
    return (bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2]) / d;
}

template <typename T>
BE_INLINE T Curve_QuadraticBezier<T>::GetCurrentSecondDerivative(const float time) const {
    float bvals[3];
    assert(this->values.Count() == 3);
    BasisSecondDerivative(time, bvals);
    float d = (this->times[2] - this->times[0]);
    return (bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2]) / (d * d);
}

template <typename T>
BE_INLINE void Curve_QuadraticBezier<T>::Basis(const float t, float *bvals) const {
    float s1 = (float)(t - this->times[0]) / (this->times[2] - this->times[0]);
    float s2 = s1 * s1;
    bvals[0] = s2 - 2.0f * s1 + 1.0f;
    bvals[1] = -2.0f * s2 + 2.0f * s1;
    bvals[2] = s2;
}

template <typename T>
BE_INLINE void Curve_QuadraticBezier<T>::BasisFirstDerivative(const float t, float *bvals) const {
    float s1 = (float)(t - this->times[0]) / (this->times[2] - this->times[0]);
    bvals[0] = 2.0f * s1 - 2.0f;
    bvals[1] = -4.0f * s1 + 2.0f;
    bvals[2] = 2.0f * s1;
}

template <typename T>
BE_INLINE void Curve_QuadraticBezier<T>::BasisSecondDerivative(const float t, float *bvals) const {
    float s1 = (float)(t - this->times[0]) / (this->times[2] - this->times[0]);
    bvals[0] = 2.0f;
    bvals[1] = -4.0f;
    bvals[2] = 2.0f;
}

/*
-------------------------------------------------------------------------------

    Cubic Bezier Curve template.
    Should always have exactly four knots.

-------------------------------------------------------------------------------
*/

template <typename T>
class Curve_CubicBezier : public Curve<T> {
public:
    Curve_CubicBezier();

                        // get the value for the given time
    virtual T           GetCurrentValue(const float time) const override;

                        // get the first derivative for the given time
    virtual T           GetCurrentFirstDerivative(const float time) const override;

                        // get the second derivative for the given time
    virtual T           GetCurrentSecondDerivative(const float time) const override;

protected:
                        // cubic bezier basis functions
    void                Basis(const float t, float *bvals) const;

                        // first derivative of cubic bezier basis functions
    void                BasisFirstDerivative(const float t, float *bvals) const;

                        // second derivative of cubic bezier basis functions
    void                BasisSecondDerivative(const float t, float *bvals) const;
};

template <typename T>
BE_INLINE Curve_CubicBezier<T>::Curve_CubicBezier() {
}

template <typename T>
BE_INLINE T Curve_CubicBezier<T>::GetCurrentValue(const float time) const {
    float bvals[4];
    assert(this->values.Count() == 4);
    Basis(time, bvals);
    return (bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] + bvals[3] * this->values[3]);
}

template <typename T>
BE_INLINE T Curve_CubicBezier<T>::GetCurrentFirstDerivative(const float time) const {
    float bvals[4];
    assert(this->values.Count() == 4);
    BasisFirstDerivative(time, bvals);
    float d = (this->times[3] - this->times[0]);
    return (bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] + bvals[3] * this->values[3]) / d;
}

template <typename T>
BE_INLINE T Curve_CubicBezier<T>::GetCurrentSecondDerivative(const float time) const {
    float bvals[4];
    assert(this->values.Count() == 4);
    BasisSecondDerivative(time, bvals);
    float d = (this->times[3] - this->times[0]);
    return (bvals[0] * this->values[0] + bvals[1] * this->values[1] + bvals[2] * this->values[2] + bvals[3] * this->values[3]) / (d * d);
}

template <typename T>
BE_INLINE void Curve_CubicBezier<T>::Basis(const float t, float *bvals) const {
    float s1 = (float)(t - this->times[0]) / (this->times[3] - this->times[0]);
    float s2 = s1 * s1;
    float s3 = s2 * s1;
    bvals[0] = -s3 + 3.0f * s2 - 3.0f * s1 + 1.0f;
    bvals[1] = 3.0f * s3 - 6.0f * s2 + 3.0f * s1;
    bvals[2] = -3.0f * s3 + 3.0f * s2;
    bvals[3] = s3;
}

template <typename T>
BE_INLINE void Curve_CubicBezier<T>::BasisFirstDerivative(const float t, float *bvals) const {
    float s1 = (float)(t - this->times[0]) / (this->times[3] - this->times[0]);
    float s2 = s1 * s1;
    bvals[0] = -3.0f * s2 + 6.0f * s1 - 3.0f;
    bvals[1] = 9.0f * s2 - 12.0f * s1 + 3.0f;
    bvals[2] = -9.0f * s2 + 6.0f * s1;
    bvals[3] = 3.0f * s2;
}

template <typename T>
BE_INLINE void Curve_CubicBezier<T>::BasisSecondDerivative(const float t, float *bvals) const {
    float s1 = (float)(t - this->times[0]) / (this->times[3] - this->times[0]);
    bvals[0] = -6.0f * s1 + 6.0f;
    bvals[1] = 18.0f * s1 - 12.0f;
    bvals[2] = -18.0f * s1 + 6.0f;
    bvals[3] = 6.0f * s1;
}

/*
-------------------------------------------------------------------------------

    Spline base template.

-------------------------------------------------------------------------------
*/

template <typename T>
class Curve_Spline : public Curve<T> {
public:
    enum Boundary { 
        FreeBoundary, 
        ClampedBoundary, 
        ClosedBoundary 
    };

    Curve_Spline();

    virtual bool        IsDone(const float time) const override;

    virtual void        SetBoundaryType(const Boundary bt) { boundaryType = bt; this->changed = true; }
    virtual Boundary    GetBoundaryType() const { return boundaryType; }

    virtual void        SetCloseTime(const float t) { closeTime = t; this->changed = true; }
    virtual float       GetCloseTime() { return boundaryType == ClosedBoundary ? closeTime : 0.0f; }

protected:
                        // get the value for the given time
    T                   ValueForIndex(const int index) const;

                        // get the value for the given time
    float               TimeForIndex(const int index) const;
    
                        // return the clamped time based on the boundary type
    float               ClampedTime(const float t) const;

    Boundary            boundaryType;
    float               closeTime;
};

template <typename T>
BE_INLINE Curve_Spline<T>::Curve_Spline() {
    boundaryType = FreeBoundary;
    closeTime = 0.0f;
}

template <typename T>
BE_INLINE T Curve_Spline<T>::ValueForIndex(const int index) const {
    int n = this->values.Count() - 1;

    if (index < 0) {
        if (boundaryType == ClosedBoundary) {
            return this->values[this->values.Count() + index % this->values.Count()];
        } else {
            return this->values[0] + index * (this->values[1] - this->values[0]);
        }
    } else if (index > n) {
        if (boundaryType == ClosedBoundary) {
            return this->values[index % this->values.Count()];
        } else {
            return this->values[n] + (index - n) * (this->values[n] - this->values[n - 1]);
        }
    }
    return this->values[index];
}

template <typename T>
BE_INLINE float Curve_Spline<T>::TimeForIndex(const int index) const {
    int n = this->times.Count() - 1;

    if (index < 0) {
        if (boundaryType == ClosedBoundary) {
            return (index / this->times.Count()) * (this->times[n] + closeTime) - (this->times[n] + closeTime - this->times[this->times.Count() + index % this->times.Count()]);
        } else {
            return this->times[0] + index * (this->times[1] - this->times[0]);
        }
    } else if (index > n) {
        if (boundaryType == ClosedBoundary) {
            return (index / this->times.Count()) * (this->times[n] + closeTime) + this->times[index % this->times.Count()];
        } else {
            return this->times[n] + (index - n) * (this->times[n] - this->times[n - 1]);
        }
    }
    return this->times[index];
}

template <typename T>
BE_INLINE float Curve_Spline<T>::ClampedTime(const float t) const {
    if (boundaryType == ClampedBoundary) {
        if (t < this->times[0]) {
            return this->times[0];
        } else if (t >= this->times[this->times.Count() - 1]) {
            return this->times[this->times.Count() - 1];
        }
    }
    return t;
}

template <typename T>
BE_INLINE bool Curve_Spline<T>::IsDone(const float time) const {
    return (boundaryType != ClosedBoundary && time >= this->times[this->times.Count() - 1]);
}

/*
-------------------------------------------------------------------------------

    Uniform Cubic Interpolating Spline template.
    The curve goes through all the knots.

-------------------------------------------------------------------------------
*/

template <typename T>
class Curve_CatmulRomSpline : public Curve_Spline<T> {
public:
    Curve_CatmulRomSpline();

                        // get the value for the given time
    virtual T           GetCurrentValue(const float time) const override;

                        // get the first derivative for the given time
    virtual T           GetCurrentFirstDerivative(const float time) const override;

                        // get the second derivative for the given time
    virtual T           GetCurrentSecondDerivative(const float time) const override;

protected:
                        // spline basis functions
    void                Basis(const int index, const float t, float *bvals) const;

                        // first derivative of spline basis functions
    void                BasisFirstDerivative(const int index, const float t, float *bvals) const;

                        // second derivative of spline basis functions
    void                BasisSecondDerivative(const int index, const float t, float *bvals) const;
};

template <typename T>
BE_INLINE Curve_CatmulRomSpline<T>::Curve_CatmulRomSpline() {
}

template <typename T>
BE_INLINE T Curve_CatmulRomSpline<T>::GetCurrentValue(const float time) const {
    if (this->times.Count() == 1) {
        return this->values[0];
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    float bvals[4];
    Basis(i - 1, clampedTime, bvals);
    T v = this->values[0] - this->values[0]; //-V501
    for (int j = 0; j < 4; j++) {
        int k = i + j - 2;
        v += bvals[j] * this->ValueForIndex(k);
    }
    return v;
}

template <typename T>
BE_INLINE T Curve_CatmulRomSpline<T>::GetCurrentFirstDerivative(const float time) const {
    if (this->times.Count() == 1) {
        return (this->values[0] - this->values[0]); //-V501
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    float bvals[4];
    BasisFirstDerivative(i - 1, clampedTime, bvals);
    T v = this->values[0] - this->values[0]; //-V501
    for (int j = 0; j < 4; j++) {
        int k = i + j - 2;
        v += bvals[j] * this->ValueForIndex(k);
    }
    float d = (this->TimeForIndex(i) - this->TimeForIndex(i - 1));
    return v / d;
}

template <typename T>
BE_INLINE T Curve_CatmulRomSpline<T>::GetCurrentSecondDerivative(const float time) const {
    if (this->times.Count() == 1) {
        return (this->values[0] - this->values[0]); //-V501
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    float bvals[4];
    BasisSecondDerivative(i - 1, clampedTime, bvals);
    T v = this->values[0] - this->values[0]; //-V501
    for (int j = 0; j < 4; j++) {
        int k = i + j - 2;
        v += bvals[j] * this->ValueForIndex(k);
    }
    float d = (this->TimeForIndex(i) - this->TimeForIndex(i - 1));
    return v / (d * d);
}

template <typename T>
BE_INLINE void Curve_CatmulRomSpline<T>::Basis(const int index, const float t, float *bvals) const {
    float s = (float)(t - this->TimeForIndex(index)) / (this->TimeForIndex(index + 1) - this->TimeForIndex(index));
    bvals[0] = ((-s + 2.0f) * s - 1.0f) * s * 0.5f;             // -0.5 * s * s * s + s * s - 0.5 * s
    bvals[1] = (((3.0f * s - 5.0f) * s) * s + 2.0f) * 0.5f;     //  1.5 * s * s * s - 2.5 * s * s + 1
    bvals[2] = ((-3.0f * s + 4.0f) * s + 1.0f) * s * 0.5f;      // -1.5 * s * s * s - 2.0 * s * s + 0.5 * s
    bvals[3] = ((s - 1.0f) * s * s) * 0.5f;                     //  0.5 * s * s * s - 0.5 * s * s
}

template <typename T>
BE_INLINE void Curve_CatmulRomSpline<T>::BasisFirstDerivative(const int index, const float t, float *bvals) const {
    float s = (float)(t - this->TimeForIndex(index)) / (this->TimeForIndex(index + 1) - this->TimeForIndex(index));
    bvals[0] = (-1.5f * s + 2.0f) * s - 0.5f;                   // -1.5 * s * s + 2 * s - 0.5
    bvals[1] = (4.5f * s - 5.0f) * s;                           //  4.5 * s * s - 5 * s
    bvals[2] = (-4.5 * s + 4.0f) * s + 0.5f;                    // -4.5 * s * s + 4 * s + 0.5
    bvals[3] = 1.5f * s * s - s;                                //  1.5 * s * s - s
}

template <typename T>
BE_INLINE void Curve_CatmulRomSpline<T>::BasisSecondDerivative(const int index, const float t, float *bvals) const {
    float s = (float)(t - this->TimeForIndex(index)) / (this->TimeForIndex(index + 1) - this->TimeForIndex(index));
    bvals[0] = -3.0f * s + 2.0f;
    bvals[1] = 9.0f * s - 5.0f;
    bvals[2] = -9.0f * s + 4.0f;
    bvals[3] = 3.0f * s - 1.0f;
}

/*
-------------------------------------------------------------------------------

    Cubic Interpolating Spline template.
    The curve goes through all the knots.
    The curve becomes the Catmull-Rom spline if the tension,
    continuity and bias are all set to zero.

-------------------------------------------------------------------------------
*/

template <typename T>
class Curve_KochanekBartelsSpline : public Curve_Spline<T> {
public:
    Curve_KochanekBartelsSpline();

                        // add a timed / value pair to the spline
                        // returns the index to the inserted pair
    virtual int         AddValue(const float time, const T &value) override;

                        // add a timed / value pair to the spline
                        // returns the index to the inserted pair
    virtual int         AddValue(const float time, const T &value, const float tension, const float continuity, const float bias) override;
    virtual void        RemoveIndex(const int index) override { this->values.RemoveIndex(index); this->times.RemoveIndex(index); tension.RemoveIndex(index); continuity.RemoveIndex(index); bias.RemoveIndex(index); }
    virtual void        Clear() override { this->values.Clear(); this->times.Clear(); tension.Clear(); continuity.Clear(); bias.Clear(); this->currentIndex = -1; }

                        // get the value for the given time
    virtual T           GetCurrentValue(const float time) const override;

                        // get the first derivative for the given time
    virtual T           GetCurrentFirstDerivative(const float time) const override;

                        // get the second derivative for the given time
    virtual T           GetCurrentSecondDerivative(const float time) const override;

protected:
    void                TangentsForIndex(const int index, T &t0, T &t1) const;

                        // spline basis functions
    void                Basis(const int index, const float t, float *bvals) const;

                        // first derivative of spline basis functions
    void                BasisFirstDerivative(const int index, const float t, float *bvals) const;

                        // second derivative of spline basis functions
    void                BasisSecondDerivative(const int index, const float t, float *bvals) const;

    Array<float>        tension;
    Array<float>        continuity;
    Array<float>        bias;
};

template <typename T>
BE_INLINE Curve_KochanekBartelsSpline<T>::Curve_KochanekBartelsSpline() {
}

template <typename T>
BE_INLINE int Curve_KochanekBartelsSpline<T>::AddValue(const float time, const T &value) {
    int i = this->IndexForTime(time);
    this->times.Insert(time, i);
    this->values.Insert(value, i);
    tension.Insert(0.0f, i);
    continuity.Insert(0.0f, i);
    bias.Insert(0.0f, i);
    return i;
}

template <typename T>
BE_INLINE int Curve_KochanekBartelsSpline<T>::AddValue(const float time, const T &value, const float tension, const float continuity, const float bias) {
    int i = this->IndexForTime(time);
    this->times.Insert(time, i);
    this->values.Insert(value, i);
    this->tension.Insert(tension, i);
    this->continuity.Insert(continuity, i);
    this->bias.Insert(bias, i);
    return i;
}

template <typename T>
BE_INLINE T Curve_KochanekBartelsSpline<T>::GetCurrentValue(const float time) const {
    float bvals[4];
    T t0, t1;

    if (this->times.Count() == 1) {
        return this->values[0];
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    TangentsForIndex(i - 1, t0, t1);
    Basis(i - 1, clampedTime, bvals);
    T v = bvals[0] * this->ValueForIndex(i - 1);
    v += bvals[1] * this->ValueForIndex(i);
    v += bvals[2] * t0;
    v += bvals[3] * t1;
    return v;
}

template <typename T>
BE_INLINE T Curve_KochanekBartelsSpline<T>::GetCurrentFirstDerivative(const float time) const {
    float bvals[4];
    T t0, t1;

    if (this->times.Count() == 1) {
        return (this->values[0] - this->values[0]); //-V501
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    TangentsForIndex(i - 1, t0, t1);
    BasisFirstDerivative(i - 1, clampedTime, bvals);
    T v = bvals[0] * this->ValueForIndex(i - 1);
    v += bvals[1] * this->ValueForIndex(i);
    v += bvals[2] * t0;
    v += bvals[3] * t1;
    float d = (this->TimeForIndex(i) - this->TimeForIndex(i - 1));
    return v / d;
}

template <typename T>
BE_INLINE T Curve_KochanekBartelsSpline<T>::GetCurrentSecondDerivative(const float time) const {
    float bvals[4];
    T t0, t1;

    if (this->times.Count() == 1) {
        return (this->values[0] - this->values[0]); //-V501
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    TangentsForIndex(i - 1, t0, t1);
    BasisSecondDerivative(i - 1, clampedTime, bvals);
    T v = bvals[0] * this->ValueForIndex(i - 1);
    v += bvals[1] * this->ValueForIndex(i);
    v += bvals[2] * t0;
    v += bvals[3] * t1;
    float d = (this->TimeForIndex(i) - this->TimeForIndex(i - 1));
    return v / (d * d);
}

template <typename T>
BE_INLINE void Curve_KochanekBartelsSpline<T>::TangentsForIndex(const int index, T &t0, T &t1) const {
    T delta = this->ValueForIndex(index + 1) - this->ValueForIndex(index);
    float dt = this->TimeForIndex(index + 1) - this->TimeForIndex(index);

    float omt = 1.0f - tension[index];
    float omc = 1.0f - continuity[index];
    float opc = 1.0f + continuity[index];
    float omb = 1.0f - bias[index];
    float opb = 1.0f + bias[index];
    float adj = 2.0f * dt / (this->TimeForIndex(index + 1) - this->TimeForIndex(index - 1));
    float s0 = 0.5f * adj * omt * opc * opb;
    float s1 = 0.5f * adj * omt * omc * omb;

    // outgoing tangent at first point
    t0 = s1 * delta + s0 * (this->ValueForIndex(index) - this->ValueForIndex(index - 1));

    omt = 1.0f - tension[index + 1];
    omc = 1.0f - continuity[index + 1];
    opc = 1.0f + continuity[index + 1];
    omb = 1.0f - bias[index + 1];
    opb = 1.0f + bias[index + 1];
    adj = 2.0f * dt / (this->TimeForIndex(index + 2) - this->TimeForIndex(index));
    s0 = 0.5f * adj * omt * omc * opb;
    s1 = 0.5f * adj * omt * opc * omb;

    // incoming tangent at second point
    t1 = s1 * (this->ValueForIndex(index + 2) - this->ValueForIndex(index + 1)) + s0 * delta;
}

template <typename T>
BE_INLINE void Curve_KochanekBartelsSpline<T>::Basis(const int index, const float t, float *bvals) const {
    float s = (float)(t - this->TimeForIndex(index)) / (this->TimeForIndex(index + 1) - this->TimeForIndex(index));
    bvals[0] = ((2.0f * s - 3.0f) * s) * s + 1.0f;  //  2 * s * s * s - 3 * s * s + 1
    bvals[1] = ((-2.0f * s + 3.0f) * s) * s;        // -2 * s * s * s + 3 * s * s
    bvals[2] = ((s - 2.0f) * s) * s + s;            //  s * s * s - 2 * s * s + s
    bvals[3] = ((s - 1.0f) * s) * s;                //  s * s * s - s * s
}

template <typename T>
BE_INLINE void Curve_KochanekBartelsSpline<T>::BasisFirstDerivative(const int index, const float t, float *bvals) const {
    float s = (float)(t - this->TimeForIndex(index)) / (this->TimeForIndex(index + 1) - this->TimeForIndex(index));
    bvals[0] = (6.0f * s - 6.0f) * s;               //  6 * s * s - 6 * s
    bvals[1] = (-6.0f * s + 6.0f) * s;              // -6 * s * s + 6 * s
    bvals[2] = (3.0f * s - 4.0f) * s + 1.0f;        //  3 * s * s - 4 * s + 1
    bvals[3] = (3.0f * s - 2.0f) * s;               //  3 * s * s - 2 * s
}

template <typename T>
BE_INLINE void Curve_KochanekBartelsSpline<T>::BasisSecondDerivative(const int index, const float t, float *bvals) const {
    float s = (float)(t - this->TimeForIndex(index)) / (this->TimeForIndex(index + 1) - this->TimeForIndex(index));
    bvals[0] = 12.0f * s - 6.0f;
    bvals[1] = -12.0f * s + 6.0f;
    bvals[2] = 6.0f * s - 4.0f;
    bvals[3] = 6.0f * s - 2.0f;
}

/*
-------------------------------------------------------------------------------

    B-Spline base template. Uses recursive definition and is slow.
    Use Curve_UniformCubicBSpline or Curve_NonUniformBSpline instead.

-------------------------------------------------------------------------------
*/

template <typename T>
class Curve_BSpline : public Curve_Spline<T> {
public:
    Curve_BSpline();

    virtual int         GetOrder() const { return order; }
    virtual void        SetOrder(const int i) { assert(i > 0 && i < 10); order = i; }

                        // get the value for the given time
    virtual T           GetCurrentValue(const float time) const override;
                        
                        // get the first derivative for the given time
    virtual T           GetCurrentFirstDerivative(const float time) const override;

                        // get the second derivative for the given time
    virtual T           GetCurrentSecondDerivative(const float time) const override;

protected:
                        // spline basis function
    float               Basis(const int index, const int order, const float t) const;

                        // first derivative of spline basis function
    float               BasisFirstDerivative(const int index, const int order, const float t) const;

                        // second derivative of spline basis function
    float               BasisSecondDerivative(const int index, const int order, const float t) const;

    int                 order;
};

template <typename T>
BE_INLINE Curve_BSpline<T>::Curve_BSpline() {
    order = 4;  // default to cubic
}

template <typename T>
BE_INLINE T Curve_BSpline<T>::GetCurrentValue(const float time) const {
    if (this->times.Count() == 1) {
        return this->values[0];
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    T v = this->values[0] - this->values[0]; //-V501
    for (int j = 0; j < order; j++) {
        int k = i + j - (order >> 1);
        v += Basis(k - 2, order, clampedTime) * this->ValueForIndex(k);
    }
    return v;
}

template <typename T>
BE_INLINE T Curve_BSpline<T>::GetCurrentFirstDerivative(const float time) const {
    if (this->times.Count() == 1) {
        return this->values[0];
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    T v = this->values[0] - this->values[0]; //-V501
    for (int j = 0; j < order; j++) {
        int k = i + j - (order >> 1);
        v += BasisFirstDerivative(k - 2, order, clampedTime) * this->ValueForIndex(k);
    }
    return v;
}

template <typename T>
BE_INLINE T Curve_BSpline<T>::GetCurrentSecondDerivative(const float time) const {
    if (this->times.Count() == 1) {
        return this->values[0];
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    T v = this->values[0] - this->values[0]; //-V501
    for (int j = 0; j < order; j++) {
        int k = i + j - (order >> 1);
        v += BasisSecondDerivative(k - 2, order, clampedTime) * this->ValueForIndex(k);
    }
    return v;
}

template <typename T>
BE_INLINE float Curve_BSpline<T>::Basis(const int index, const int order, const float t) const {
    if (order <= 1) {
        if (this->TimeForIndex(index) < t && t <= this->TimeForIndex(index + 1)) {
            return 1.0f;
        } else {
            return 0.0f;
        }
    } else {
        float sum = 0.0f;
        float d1 = this->TimeForIndex(index + order - 1) - this->TimeForIndex(index);
        if (d1 != 0.0f) {
            sum += (float)(t - this->TimeForIndex(index)) * Basis(index, order - 1, t) / d1;
        }

        float d2 = this->TimeForIndex(index + order) - this->TimeForIndex(index + 1);
        if (d2 != 0.0f) {
            sum += (float)(this->TimeForIndex(index + order) - t) * Basis(index + 1, order - 1, t) / d2;
        }
        return sum;
    }
}

template <typename T>
BE_INLINE float Curve_BSpline<T>::BasisFirstDerivative(const int index, const int order, const float t) const {
    return (Basis(index, order - 1, t) - Basis(index + 1, order - 1, t)) * 
        (float)(order - 1) / (this->TimeForIndex(index + (order - 1) - 2) - this->TimeForIndex(index - 2));
}

template <typename T>
BE_INLINE float Curve_BSpline<T>::BasisSecondDerivative(const int index, const int order, const float t) const {
    return (BasisFirstDerivative(index, order - 1, t) - BasisFirstDerivative(index + 1, order - 1, t)) *
        (float)(order - 1) / (this->TimeForIndex(index + (order - 1) - 2) - this->TimeForIndex(index - 2));
}

/*
-------------------------------------------------------------------------------

    Uniform Non-Rational Cubic B-Spline template.

-------------------------------------------------------------------------------
*/

template <typename T>
class Curve_UniformCubicBSpline : public Curve_BSpline<T> {
public:
    Curve_UniformCubicBSpline();

                        // get the value for the given time
    virtual T           GetCurrentValue(const float time) const override;

                        // get the first derivative for the given time
    virtual T           GetCurrentFirstDerivative(const float time) const override;

                        // get the second derivative for the given time
    virtual T           GetCurrentSecondDerivative(const float time) const override;

protected:
                        // spline basis functions
    void                Basis(const int index, const float t, float *bvals) const;

                        // first derivative of spline basis functions
    void                BasisFirstDerivative(const int index, const float t, float *bvals) const;

                        // second derivative of spline basis functions
    void                BasisSecondDerivative(const int index, const float t, float *bvals) const;
};

template <typename T>
BE_INLINE Curve_UniformCubicBSpline<T>::Curve_UniformCubicBSpline() {
    this->order = 4;    // always cubic
}

template <typename T>
BE_INLINE T Curve_UniformCubicBSpline<T>::GetCurrentValue(const float time) const {
    float bvals[4];

    if (this->times.Count() == 1) {
        return this->values[0];
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    Basis(i - 1, clampedTime, bvals);
    T v = this->values[0] - this->values[0]; //-V501
    for (int j = 0; j < 4; j++) {
        int k = i + j - 2;
        v += bvals[j] * this->ValueForIndex(k);
    }
    return v;
}

template <typename T>
BE_INLINE T Curve_UniformCubicBSpline<T>::GetCurrentFirstDerivative(const float time) const {
    float bvals[4];

    if (this->times.Count() == 1) {
        return (this->values[0] - this->values[0]); //-V501
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    BasisFirstDerivative(i - 1, clampedTime, bvals);
    T v = this->values[0] - this->values[0]; //-V501
    for (int j = 0; j < 4; j++) {
        int k = i + j - 2;
        v += bvals[j] * this->ValueForIndex(k);
    }
    float d = (this->TimeForIndex(i) - this->TimeForIndex(i - 1));
    return v / d;
}

template <typename T>
BE_INLINE T Curve_UniformCubicBSpline<T>::GetCurrentSecondDerivative(const float time) const {
    float bvals[4];

    if (this->times.Count() == 1) {
        return (this->values[0] - this->values[0]); //-V501
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    BasisSecondDerivative(i - 1, clampedTime, bvals);
    T v = this->values[0] - this->values[0]; //-V501
    for (int j = 0; j < 4; j++) {
        int k = i + j - 2;
        v += bvals[j] * this->ValueForIndex(k);
    }
    float d = (this->TimeForIndex(i) - this->TimeForIndex(i - 1));
    return v / (d * d);
}

template <typename T>
BE_INLINE void Curve_UniformCubicBSpline<T>::Basis(const int index, const float t, float *bvals) const {
    float s = (float)(t - this->TimeForIndex(index)) / (this->TimeForIndex(index + 1) - this->TimeForIndex(index));
    bvals[0] = (((-s + 3.0f) * s - 3.0f) * s + 1.0f) * (1.0f / 6.0f);
    bvals[1] = (((3.0f * s - 6.0f) * s) * s + 4.0f) * (1.0f / 6.0f);
    bvals[2] = (((-3.0f * s + 3.0f) * s + 3.0f) * s + 1.0f) * (1.0f / 6.0f);
    bvals[3] = (s * s * s) * (1.0f / 6.0f);
}

template <typename T>
BE_INLINE void Curve_UniformCubicBSpline<T>::BasisFirstDerivative(const int index, const float t, float *bvals) const {
    float s = (float)(t - this->TimeForIndex(index)) / (this->TimeForIndex(index + 1) - this->TimeForIndex(index));
    bvals[0] = -0.5f * s * s + s - 0.5f;
    bvals[1] = 1.5f * s * s - 2.0f * s;
    bvals[2] = -1.5f * s * s + s + 0.5f;
    bvals[3] = 0.5f * s * s;
}

template <typename T>
BE_INLINE void Curve_UniformCubicBSpline<T>::BasisSecondDerivative(const int index, const float t, float *bvals) const {
    float s = (float)(t - this->TimeForIndex(index)) / (this->TimeForIndex(index + 1) - this->TimeForIndex(index));
    bvals[0] = -s + 1.0f;
    bvals[1] = 3.0f * s - 2.0f;
    bvals[2] = -3.0f * s + 1.0f;
    bvals[3] = s;
}

/*
-------------------------------------------------------------------------------

    Non-Uniform Non-Rational B-Spline (NUBS) template.

-------------------------------------------------------------------------------
*/

template <typename T>
class Curve_NonUniformBSpline : public Curve_BSpline<T> {
public:
    Curve_NonUniformBSpline();

                        // get the value for the given time
    virtual T           GetCurrentValue(const float time) const override;

                        // get the first derivative for the given time
    virtual T           GetCurrentFirstDerivative(const float time) const override;

                        // get the second derivative for the given time
    virtual T           GetCurrentSecondDerivative(const float time) const override;

protected:
                        // spline basis functions
    void                Basis(const int index, const int order, const float t, float *bvals) const;

                        // first derivative of spline basis functions
    void                BasisFirstDerivative(const int index, const int order, const float t, float *bvals) const;

                        // second derivative of spline basis functions
    void                BasisSecondDerivative(const int index, const int order, const float t, float *bvals) const;
};

template <typename T>
BE_INLINE Curve_NonUniformBSpline<T>::Curve_NonUniformBSpline() {
}

template <typename T>
BE_INLINE T Curve_NonUniformBSpline<T>::GetCurrentValue(const float time) const {
    float *bvals = (float *)_alloca16(this->order * sizeof(float));

    if (this->times.Count() == 1) {
        return this->values[0];
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    Basis(i - 1, this->order, clampedTime, bvals);
    T v = this->values[0] - this->values[0]; //-V501
    for (int j = 0; j < this->order; j++) {
        int k = i + j - (this->order >> 1);
        v += bvals[j] * this->ValueForIndex(k);
    }
    return v;
}

template <typename T>
BE_INLINE T Curve_NonUniformBSpline<T>::GetCurrentFirstDerivative(const float time) const {
    float *bvals = (float *)_alloca16(this->order * sizeof(float));

    if (this->times.Count() == 1) {
        return (this->values[0] - this->values[0]); //-V501
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    BasisFirstDerivative(i - 1, this->order, clampedTime, bvals);
    T v = this->values[0] - this->values[0]; //-V501
    for (int j = 0; j < this->order; j++) {
        int k = i + j - (this->order >> 1);
        v += bvals[j] * this->ValueForIndex(k);
    }
    return v;
}

template <typename T>
BE_INLINE T Curve_NonUniformBSpline<T>::GetCurrentSecondDerivative(const float time) const {
    float *bvals = (float *)_alloca16(this->order * sizeof(float));

    if (this->times.Count() == 1) {
        return (this->values[0] - this->values[0]); //-V501
    }

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    BasisSecondDerivative(i - 1, this->order, clampedTime, bvals);
    T v = this->values[0] - this->values[0]; //-V501
    for (int j = 0; j < this->order; j++) {
        int k = i + j - (this->order >> 1);
        v += bvals[j] * this->ValueForIndex(k);
    }
    return v;
}

template <typename T>
BE_INLINE void Curve_NonUniformBSpline<T>::Basis(const int index, const int order, const float t, float *bvals) const {
    bvals[order - 1] = 1.0f;
    for (int r = 2; r <= order; r++) {
        int i = index - r + 1;
        bvals[order - r] = 0.0f;
        for (int s = order - r + 1; s < order; s++) {
            i++;
            float omega = (float)(t - this->TimeForIndex(i)) / (this->TimeForIndex(i + r - 1) - this->TimeForIndex(i));
            bvals[s - 1] += (1.0f - omega) * bvals[s];
            bvals[s] *= omega;
        }
    }
}

template <typename T>
BE_INLINE void Curve_NonUniformBSpline<T>::BasisFirstDerivative(const int index, const int order, const float t, float *bvals) const {
    int i;
    Basis(index, order - 1, t, bvals + 1);
    bvals[0] = 0.0f;
    for (i = 0; i < order - 1; i++) {
        bvals[i] -= bvals[i + 1];
        bvals[i] *= (float)(order - 1) / (this->TimeForIndex(index + i + (order - 1) - 2) - this->TimeForIndex(index + i - 2));
    }
    bvals[i] *= (float)(order - 1) / (this->TimeForIndex(index + i + (order - 1) - 2) - this->TimeForIndex(index + i - 2));
}

template <typename T>
BE_INLINE void Curve_NonUniformBSpline<T>::BasisSecondDerivative(const int index, const int order, const float t, float *bvals) const {
    int i;
    BasisFirstDerivative(index, order - 1, t, bvals + 1);
    bvals[0] = 0.0f;
    for (i = 0; i < order - 1; i++) {
        bvals[i] -= bvals[i + 1];
        bvals[i] *= (float)(order - 1) / (this->TimeForIndex(index + i + (order - 1) - 2) - this->TimeForIndex(index + i - 2));
    }
    bvals[i] *= (float)(order - 1) / (this->TimeForIndex(index + i + (order - 1) - 2) - this->TimeForIndex(index + i - 2));
}

/*
-------------------------------------------------------------------------------

    Non-Uniform Rational B-Spline (NURBS) template.

-------------------------------------------------------------------------------
*/

template <typename T>
class Curve_NURBS : public Curve_NonUniformBSpline<T> {
public:
    Curve_NURBS();

                        // add a timed/value pair to the spline
                        // returns the index to the inserted pair
    virtual int         AddValue(const float time, const T &value) override;

                        // add a timed/value pair to the spline
                        // returns the index to the inserted pair
    virtual int         AddValue(const float time, const T &value, const float weight);
    virtual void        RemoveIndex(const int index) override { this->values.RemoveIndex(index); this->times.RemoveIndex(index); weights.RemoveIndex(index); }
    virtual void        Clear() override { this->values.Clear(); this->times.Clear(); weights.Clear(); this->currentIndex = -1; }

                        // get the value for the given time
    virtual T           GetCurrentValue(const float time) const override;

                        // get the first derivative for the given time
    virtual T           GetCurrentFirstDerivative(const float time) const override;

                        // get the second derivative for the given time
    virtual T           GetCurrentSecondDerivative(const float time) const override;

protected:
                        // get the weight for the given index
    float               WeightForIndex(const int index) const;

    Array<float>        weights;
};

template <typename T>
BE_INLINE Curve_NURBS<T>::Curve_NURBS() {
}

template <typename T>
BE_INLINE int Curve_NURBS<T>::AddValue(const float time, const T &value) {
    int i = this->IndexForTime(time);
    this->times.Insert(time, i);
    this->values.Insert(value, i);
    weights.Insert(1.0f, i);
    return i;
}

template <typename T>
BE_INLINE int Curve_NURBS<T>::AddValue(const float time, const T &value, const float weight) {
    int i = this->IndexForTime(time);
    this->times.Insert(time, i);
    this->values.Insert(value, i);
    weights.Insert(weight, i);
    return i;
}

template <typename T>
BE_INLINE T Curve_NURBS<T>::GetCurrentValue(const float time) const {
    if (this->times.Count() == 1) {
        return this->values[0];
    }

    float *bvals = (float *)_alloca16(this->order * sizeof(float));

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    this->Basis(i - 1, this->order, clampedTime, bvals);
    T v = this->values[0] - this->values[0]; //-V501
    float w = 0.0f;
    for (int j = 0; j < this->order; j++) {
        int k = i + j - (this->order >> 1);
        float b = bvals[j] * WeightForIndex(k);
        w += b;
        v += b * this->ValueForIndex(k);
    }
    return v / w;
}

template <typename T>
BE_INLINE T Curve_NURBS<T>::GetCurrentFirstDerivative(const float time) const {
    float wd1;
    T vd1;

    if (this->times.Count() == 1) {
        return this->values[0];
    }

    float *bvals = (float *)_alloca16(this->order * sizeof(float));
    float *d1vals = (float *)_alloca16(this->order * sizeof(float));

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    this->Basis(i - 1, this->order, clampedTime, bvals);
    this->BasisFirstDerivative(i - 1, this->order, clampedTime, d1vals);
    T vb = vd1 = this->values[0] - this->values[0]; //-V501
    float wb = wd1 = 0.0f;
    for (int j = 0; j < this->order; j++) {
        int k = i + j - (this->order >> 1);
        float w = WeightForIndex(k);
        float b = bvals[j] * w;
        float d1 = d1vals[j] * w;
        wb += b;
        wd1 += d1;
        T v = this->ValueForIndex(k);
        vb += b * v;
        vd1 += d1 * v;
    }
    return (wb * vd1 - vb * wd1) / (wb * wb);
}

template <typename T>
BE_INLINE T Curve_NURBS<T>::GetCurrentSecondDerivative(const float time) const {
    float wd1, wd2;
    T vd1, vd2;

    if (this->times.Count() == 1) {
        return this->values[0];
    }

    float *bvals = (float *)_alloca16(this->order * sizeof(float));
    float *d1vals = (float *)_alloca16(this->order * sizeof(float));
    float *d2vals = (float *)_alloca16(this->order * sizeof(float));

    float clampedTime = this->ClampedTime(time);
    int i = this->IndexForTime(clampedTime);
    this->Basis(i - 1, this->order, clampedTime, bvals);
    this->BasisFirstDerivative(i - 1, this->order, clampedTime, d1vals);
    this->BasisSecondDerivative(i - 1, this->order, clampedTime, d2vals);
    T vb = vd1 = vd2 = this->values[0] - this->values[0]; //-V501
    float wb = wd1 = wd2 = 0.0f;
    for (int j = 0; j < this->order; j++) {
        int k = i + j - (this->order >> 1);
        float w = WeightForIndex(k);
        float b = bvals[j] * w;
        float d1 = d1vals[j] * w;
        float d2 = d2vals[j] * w;
        wb += b;
        wd1 += d1;
        wd2 += d2;
        T v = this->ValueForIndex(k);
        vb += b * v;
        vd1 += d1 * v;
        vd2 += d2 * v;
    }
    return ((wb * wb) * (wb * vd2 - vb * wd2) - (wb * vd1 - vb * wd1) * 2.0f * wb * wd1) / (wb * wb * wb * wb);
}

template <typename T>
BE_INLINE float Curve_NURBS<T>::WeightForIndex(const int index) const {
    int n = weights.Count() - 1;

    if (index < 0) {
        if (this->boundaryType == Curve_Spline<T>::ClosedBoundary) {
            return weights[weights.Count() + index % weights.Count()];
        } else {
            return weights[0] + index * (weights[1] - weights[0]);
        }
    } else if (index > n) {
        if (this->boundaryType == Curve_Spline<T>::ClosedBoundary) {
            return weights[index % weights.Count()];
        } else {
            return weights[n] + (index - n) * (weights[n] - weights[n - 1]);
        }
    }
    return weights[index];
}

BE_NAMESPACE_END
