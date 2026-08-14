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

#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

template <typename VectorType>
static bool CanonicalizePrincipalDirection(VectorType &direction) {
    if (direction.Normalize() == 0.0f) {
        return false;
    }

    int index = direction.MaxAbsComponentIndex();
    if (direction[index] < 0.0f) {
        direction *= -1.0f;
    }
    return true;
}

static bool BuildStablePrincipalAxes(const Vec2 &eigenValues, const Mat2 &eigenVectors, float eigenTolerance, Mat2 &axes) {
    if (eigenValues[0] <= eigenTolerance || eigenValues[0] - eigenValues[1] <= eigenTolerance) {
        // No direction is uniquely determined by an effectively zero or isotropic covariance.
        axes.SetIdentity();
        return true;
    }

    Vec2 principalAxis(eigenVectors[0][0], eigenVectors[1][0]);
    if (!CanonicalizePrincipalDirection(principalAxis)) {
        return false;
    }

    // Mat2 is row-major, so explicitly place the right-handed orthonormal axes in columns.
    axes = Mat2(
         principalAxis.x, -principalAxis.y,
         principalAxis.y,  principalAxis.x);
    return true;
}

static bool BuildStablePrincipalAxes(const Vec3 &eigenValues, const Mat3 &eigenVectors, float eigenTolerance, Mat3 &axes) {
    if (eigenValues[0] <= eigenTolerance) {
        axes.SetIdentity();
        return true;
    }

    if (eigenValues[0] - eigenValues[2] <= eigenTolerance) {
        // No direction is uniquely determined by an effectively isotropic covariance.
        axes.SetIdentity();
        return true;
    }

    if (eigenValues[0] - eigenValues[1] <= eigenTolerance) {
        // The smallest component is unique; choose a deterministic basis for its plane.
        axes[2] = eigenVectors[2];
        if (!CanonicalizePrincipalDirection(axes[2])) {
            return false;
        }
        axes[2].OrthogonalBasis(axes[0], axes[1]);
        return true;
    }

    if (eigenValues[1] - eigenValues[2] <= eigenTolerance) {
        // The largest component is unique; choose a deterministic perpendicular basis.
        axes[0] = eigenVectors[0];
        if (!CanonicalizePrincipalDirection(axes[0])) {
            return false;
        }
        axes[0].OrthogonalBasis(axes[1], axes[2]);
        return true;
    }

    axes[0] = eigenVectors[0];
    if (!CanonicalizePrincipalDirection(axes[0])) {
        return false;
    }

    axes[1] = eigenVectors[1] - axes[0] * eigenVectors[1].Dot(axes[0]);
    if (!CanonicalizePrincipalDirection(axes[1])) {
        return false;
    }

    axes[2].SetFromCross(axes[0], axes[1]);
    return axes[2].Normalize() != 0.0f;
}

template <typename VectorType, typename MatrixType>
static void ClearPrincipalComponents(VectorType &eigenValues, MatrixType &axes, int &rank) {
    eigenValues = VectorType::zero;
    axes = MatrixType::identity;
    rank = 0;
}

template <typename VectorType, typename MatrixType>
static bool SetPrincipalComponentsFromCovariance(const MatrixType &covariance, float relativeEpsilon,
    VectorType &eigenValues, MatrixType &axes, int &rank) {
    if (relativeEpsilon <= 0.0f || !Math::IsFinite(relativeEpsilon)) {
        return false;
    }

    VectorType values;
    MatrixType vectors;
    if (!covariance.EigenDecomposeSymmetric(values, vectors)) {
        return false;
    }

    float eigenScale = values.MaxAbsComponent();
    float eigenTolerance = eigenScale * relativeEpsilon;

    // Covariance matrices are positive semidefinite. Tolerate only round-off-sized negatives.
    if (values[MatrixType::Rows - 1] < -eigenTolerance) {
        return false;
    }

    for (int i = 0; i < MatrixType::Rows; i++) {
        if (values[i] < 0.0f) {
            values[i] = 0.0f;
        }
    }

    int componentRank = 0;
    for (int i = 0; i < MatrixType::Rows; i++) {
        if (values[i] > eigenTolerance) {
            componentRank++;
        }
    }

    MatrixType stableAxes;
    if (!BuildStablePrincipalAxes(values, vectors, eigenTolerance, stableAxes)) {
        return false;
    }

    eigenValues = values;
    axes = stableAxes;
    rank = componentRank;
    return true;
}

template <typename VectorType, typename MatrixType>
void PrincipalComponents<VectorType, MatrixType>::Clear() {
    ClearPrincipalComponents(eigenValues, axes, rank);
}

template <typename VectorType, typename MatrixType>
bool PrincipalComponents<VectorType, MatrixType>::SetFromCovariance(const MatrixType &covariance, float relativeEpsilon) {
    return SetPrincipalComponentsFromCovariance(covariance, relativeEpsilon, eigenValues, axes, rank);
}

template struct PrincipalComponents<Vec2, Mat2>;
template struct PrincipalComponents<Vec3, Mat3>;

BE_NAMESPACE_END
