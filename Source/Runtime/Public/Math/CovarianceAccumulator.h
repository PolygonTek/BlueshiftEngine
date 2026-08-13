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

#include "Math/Matrix2.h"
#include "Math/Matrix3.h"

/*
-------------------------------------------------------------------------------

    Covariance Accumulator

    Incrementally accumulates the mean and covariance of vector samples using
    Welford's method without storing the samples. Supports sequential sample
    addition and merging of partial accumulators, and provides population and
    sample covariance.

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

template <typename VectorType, typename MatrixType>
class CovarianceAccumulator {
public:
                        /// Initializes an empty accumulator with no samples.
    CovarianceAccumulator() { Clear(); }

                        /// Resets the accumulated sample count, mean, and covariance sum.
    void                Clear();
                        /// Adds one vector sample to the accumulated statistics.
    void                AddPoint(const VectorType &point);
                        /// Merges the sample statistics from another accumulator into this one.
    void                Merge(const CovarianceAccumulator &other);

                        /// Returns the number of accumulated samples.
    int                 Count() const { return count; }
                        /// Returns the mean of the accumulated samples.
    const VectorType &  Mean() const { return mean; }

                        /// Returns the unnormalized sum of deviation outer products.
    const MatrixType &  CovarianceSum() const { return covarianceSum; }
                        /// Returns the population covariance normalized by the sample count N.
    MatrixType          PopulationCovariance() const;
                        /// Returns the sample covariance normalized by N-1, or zero when fewer than two samples exist.
    MatrixType          SampleCovariance() const;

private:
    int                 count;
    VectorType          mean;
    MatrixType          covarianceSum;
};

template <typename VectorType, typename MatrixType>
BE_INLINE void CovarianceAccumulator<VectorType, MatrixType>::Clear() {
    count = 0;
    mean = VectorType::zero;
    covarianceSum = MatrixType::zero;
}

template <typename VectorType, typename MatrixType>
BE_INLINE void CovarianceAccumulator<VectorType, MatrixType>::AddPoint(const VectorType &point) {
    count++;

    VectorType delta = point - mean;
    float invCount = 1.0f / count;
    mean += delta * invCount;

    // Accumulate the symmetric outer product of deviations before and after the Welford mean update.
    covarianceSum += MatrixType::FromOuterProduct(delta, delta) * ((count - 1) * invCount);
}

template <typename VectorType, typename MatrixType>
BE_INLINE void CovarianceAccumulator<VectorType, MatrixType>::Merge(const CovarianceAccumulator &other) {
    if (other.count == 0) {
        return;
    }

    if (count == 0) {
        *this = other;
        return;
    }

    int mergedCount = count + other.count;
    VectorType delta = other.mean - mean;
    float otherWeight = (float)other.count / mergedCount;
    float covarianceWeight = (float)count * other.count / mergedCount;

    covarianceSum += other.covarianceSum + MatrixType::FromOuterProduct(delta, delta) * covarianceWeight;
    mean += delta * otherWeight;
    count = mergedCount;
}

template <typename VectorType, typename MatrixType>
BE_INLINE MatrixType CovarianceAccumulator<VectorType, MatrixType>::PopulationCovariance() const {
    if (count == 0) {
        return MatrixType::zero;
    }
    return covarianceSum * (1.0f / count);
}

template <typename VectorType, typename MatrixType>
BE_INLINE MatrixType CovarianceAccumulator<VectorType, MatrixType>::SampleCovariance() const {
    if (count <= 1) {
        return MatrixType::zero;
    }
    return covarianceSum * (1.0f / (count - 1));
}

using CovarianceAccumulator2 = CovarianceAccumulator<Vec2, Mat2>;
using CovarianceAccumulator3 = CovarianceAccumulator<Vec3, Mat3>;

BE_NAMESPACE_END
