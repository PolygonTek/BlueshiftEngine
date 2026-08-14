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

static bool IsValidBestFitParameters(const Vec3 *points, int numPoints, int minimumPoints, float relativeEpsilon) {
    if (!points || numPoints < minimumPoints || relativeEpsilon <= 0.0f || !Math::IsFinite(relativeEpsilon)) {
        return false;
    }
    return true;
}

static bool CanonicalizeBestFitDirection(Vec3 &direction) {
    if (direction.Normalize() == 0.0f || !direction.IsFinite()) {
        return false;
    }

    int index = direction.MaxAbsComponentIndex();
    // Eigenvector signs are arbitrary, so choose one representative for deterministic results.
    if (direction[index] < 0.0f) {
        direction = -direction;
    }
    return true;
}

static bool ComputePointPrincipalComponents(const Vec3 *points, int numPoints, float relativeEpsilon, Vec3 &mean, Vec3 &eigenValues, Mat3 &eigenVectors) {
    CovarianceAccumulator3 covariance;
    for (int i = 0; i < numPoints; i++) {
        if (!points[i].IsFinite()) {
            return false;
        }
        covariance.AddPoint(points[i]);
    }

    Vec3 values;
    Mat3 vectors;
    // PCA diagonalizes the covariance; columns are sorted from greatest to least point variance.
    if (!covariance.CovarianceSum().EigenDecomposeSymmetric(values, vectors, relativeEpsilon)) {
        return false;
    }

    float eigenScale = values.MaxAbsComponent();
    if (!Math::IsFinite(eigenScale)) {
        return false;
    }
    float eigenTolerance = eigenScale * relativeEpsilon;

    // A covariance matrix is positive semidefinite; only round-off-sized negative values are valid.
    if (values.z < -eigenTolerance) {
        return false;
    }
    for (int i = 0; i < 3; i++) {
        if (values[i] < 0.0f) {
            values[i] = 0.0f;
        }
    }

    mean = covariance.Mean();
    eigenValues = values;
    eigenVectors = vectors;
    return mean.IsFinite();
}

bool BestFitLine(const Vec3 *points, int numPoints, Line &line, float relativeEpsilon) {
    if (!IsValidBestFitParameters(points, numPoints, 2, relativeEpsilon)) {
        return false;
    }

    Vec3 mean;
    Vec3 eigenValues;
    Mat3 eigenVectors;
    if (!ComputePointPrincipalComponents(points, numPoints, relativeEpsilon, mean, eigenValues, eigenVectors)) {
        return false;
    }

    // The maximum-variance axis minimizes squared orthogonal distance to a line. A spectral gap
    // is required because equal leading variances do not define a unique direction.
    float eigenTolerance = eigenValues.x * relativeEpsilon;
    if (eigenValues.x <= eigenTolerance || eigenValues.x - eigenValues.y <= eigenTolerance) {
        return false;
    }

    Vec3 direction = eigenVectors[0];
    if (!CanonicalizeBestFitDirection(direction)) {
        return false;
    }

    line = Line(mean, direction);
    return true;
}

bool BestFitPlane(const Vec3 *points, int numPoints, Plane &plane, float relativeEpsilon) {
    if (!IsValidBestFitParameters(points, numPoints, 3, relativeEpsilon)) {
        return false;
    }

    Vec3 mean;
    Vec3 eigenValues;
    Mat3 eigenVectors;
    if (!ComputePointPrincipalComponents(points, numPoints, relativeEpsilon, mean, eigenValues, eigenVectors)) {
        return false;
    }

    // The minimum-variance axis minimizes squared normal distance to a plane. It must be
    // separated from the next axis for the plane normal to be uniquely determined.
    float eigenTolerance = eigenValues.x * relativeEpsilon;
    if (eigenValues.y - eigenValues.z <= eigenTolerance) {
        return false;
    }

    Vec3 normal = eigenVectors[2];
    if (!CanonicalizeBestFitDirection(normal)) {
        return false;
    }

    plane = Plane(normal, mean);
    return true;
}

bool BestFitRigidTransform(const Vec3 *sourcePoints, const Vec3 *targetPoints, int numPoints, Mat3x4 &transform, float relativeEpsilon) {
    if (!IsValidBestFitParameters(sourcePoints, numPoints, 3, relativeEpsilon) || !targetPoints) {
        return false;
    }

    Vec3 sourceMean = Vec3::zero;
    Vec3 targetMean = Vec3::zero;
    Mat3 crossCovariance = Mat3::zero;

    // Accumulate both means and H = sum((source - sourceMean) * (target - targetMean)^T)
    // together, without forming large uncentered products.
    for (int i = 0; i < numPoints; i++) {
        const Vec3 &sourcePoint = sourcePoints[i];
        const Vec3 &targetPoint = targetPoints[i];
        if (!sourcePoint.IsFinite() || !targetPoint.IsFinite()) {
            return false;
        }

        Vec3 sourceDelta = sourcePoint - sourceMean;
        Vec3 targetDelta = targetPoint - targetMean;
        float invCount = 1.0f / (i + 1);

        sourceMean += sourceDelta * invCount;
        targetMean += targetDelta * invCount;

        if (i > 0) {
            // Paired Welford accumulation avoids loss of precision from subtracting large raw products.
            crossCovariance += Mat3::FromOuterProduct(sourceDelta, targetDelta) * (i * invCount);
        }
    }

    if (!sourceMean.IsFinite() || !targetMean.IsFinite()) {
        return false;
    }

    Mat3 leftVectors;
    Vec3 singularValues;
    Mat3 rightVectors;
    // For H = U * S * V^T, the Kabsch rotation mapping source to target is R = V * U^T.
    if (!crossCovariance.SingularValueDecompose(leftVectors, singularValues, rightVectors, relativeEpsilon)) {
        return false;
    }

    // Rank two is sufficient: a non-collinear point set determines a unique 3D rotation.
    if (singularValues.x <= 0.0f || singularValues.y <= singularValues.x * relativeEpsilon) {
        return false;
    }

    Mat3 leftVectorsTranspose = leftVectors.Transpose();
    Mat3 rotation = rightVectors * leftVectorsTranspose;
    float determinant = rotation.Determinant();
    if (!Math::IsFinite(determinant)) {
        return false;
    }

    if (determinant < 0.0f) {
        // Reflection rejection is unique only when there is one least-constrained singular direction.
        if (singularValues.y - singularValues.z <= singularValues.x * relativeEpsilon) {
            return false;
        }

        // Flip that direction to select the nearest proper rotation.
        rightVectors[2] = -rightVectors[2];
        rotation = rightVectors * leftVectorsTranspose;
        determinant = rotation.Determinant();
    }

    if (!rotation.IsFinite() || determinant <= 0.0f || !Math::IsFinite(determinant)) {
        return false;
    }

    // Align the original, uncentered point sets after the rotation has aligned their deviations.
    Vec3 translation = targetMean - rotation * sourceMean;
    if (!translation.IsFinite()) {
        return false;
    }

    transform = Mat3x4(rotation, translation);
    return true;
}

BE_NAMESPACE_END
