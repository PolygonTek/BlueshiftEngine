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
#include "Cholesky.h"
#include "LU.h"
#include "QRCP.h"
#include "JacobiSVD.h"
#include "SymmetricPSDSolver.h"

BE_NAMESPACE_BEGIN

ALIGN_AS16 const Mat2 Mat2::zero(Vec2(0, 0), Vec2(0, 0));
ALIGN_AS16 const Mat2 Mat2::identity(Vec2(1, 0), Vec2(0, 1));

Mat2 Mat2::operator-() const & {
#if defined(ENABLE_SIMD4_INTRIN)
    ALIGN_AS16 Mat2 dst;

    simd4f r0 = loadu_ps(mat[0]);

    store_ps(-r0, dst[0]);
#else
    Mat2 dst;

    dst[0][0] = -mat[0][0];
    dst[0][1] = -mat[0][1];

    dst[1][0] = -mat[1][0];
    dst[1][1] = -mat[1][1];
#endif
    return dst;
}

Mat2 &&Mat2::operator-() && {
#if defined(ENABLE_SIMD4_INTRIN)
    simd4f r0 = loadu_ps(mat[0]);

    storeu_ps(-r0, mat[0]);
#else
    mat[0][0] = -mat[0][0];
    mat[0][1] = -mat[0][1];

    mat[1][0] = -mat[1][0];
    mat[1][1] = -mat[1][1];
#endif
    return std::move(*this);
}

Mat2 &Mat2::operator*=(const Mat2 &rhs) {
#if defined(ENABLE_SIMD4_INTRIN)
    simd4f a = loadu_ps(mat[0]);
    simd4f b = loadu_ps(rhs.mat[0]);

    simd4f result = shuffle_ps<0, 0, 2, 2>(a) * shuffle_ps<0, 1, 0, 1>(b);
    result = madd_ps(shuffle_ps<1, 1, 3, 3>(a), shuffle_ps<2, 3, 2, 3>(b), result);

    storeu_ps(result, mat[0]);
#else
    float dst[2];

    dst[0] = mat[0][0] * rhs.mat[0][0] + mat[0][1] * rhs.mat[1][0];
    dst[1] = mat[0][0] * rhs.mat[0][1] + mat[0][1] * rhs.mat[1][1];
    mat[0][0] = dst[0];
    mat[0][1] = dst[1];

    dst[0] = mat[1][0] * rhs.mat[0][0] + mat[1][1] * rhs.mat[1][0];
    dst[1] = mat[1][0] * rhs.mat[0][1] + mat[1][1] * rhs.mat[1][1];
    mat[1][0] = dst[0];
    mat[1][1] = dst[1];
#endif
    return *this;
}

bool Mat2::InverseSelf() {
#if 1
    // 2+4 = 6 multiplications
    //       1 division
    double det, invDet, a;

    det = mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];

    if (Math::Abs(det) < MATRIX_INVERSE_EPSILON) {
        return false;
    }

    invDet = 1.0f / det;

    a = mat[0][0];
    mat[0][0] =   mat[1][1] * invDet;
    mat[0][1] = - mat[0][1] * invDet;
    mat[1][0] = - mat[1][0] * invDet;
    mat[1][1] =   a * invDet;

    return true;
#else
    // 2*4 = 8 multiplications
    //       2 division
    float *mat = reinterpret_cast<float *>(this);
    double d, di;
    float s;

    di = mat[0];
    s = di;
    mat[0*2+0] = d = 1.0f / di;
    mat[0*2+1] *= d;
    d = -d;
    mat[1*2+0] *= d;
    d = mat[1*2+0] * di;
    mat[1*2+1] += mat[0*2+1] * d;
    di = mat[1*2+1];
    s *= di;
    mat[1*2+1] = d = 1.0f / di;
    mat[1*2+0] *= d;
    d = -d;
    mat[0*2+1] *= d;
    d = mat[0*2+1] * di;
    mat[0*2+0] += mat[1*2+0] * d;

    return (s != 0.0f && !FLOAT_IS_NAN(s));
#endif
}

bool Mat2::Solve(const Vec2 &rightHandSide, Vec2 &solution, float tolerance) const {
    return LU::Solve<2>(*this, rightHandSide, solution, tolerance);
}

bool Mat2::CholeskyDecompose(Mat2 &lowerTriangular, float tolerance) const {
    float input[2][2];
    for (int row = 0; row < 2; row++) {
        for (int column = 0; column < 2; column++) {
            input[row][column] = mat[row][column];
        }
    }

    float output[2][2];
    if (!Cholesky::Decompose<2>(input, output, tolerance)) {
        return false;
    }

    Mat2 lower(
        Vec2(output[0][0], output[0][1]),
        Vec2(output[1][0], output[1][1]));

    lowerTriangular = lower;
    return true;
}

bool Mat2::SolveSymmetricPSD(const Vec2 &rightHandSide, Vec2 &solution, float tolerance) const {
    return SymmetricPSDSolver::Solve<2>(*this, rightHandSide, solution, tolerance);
}

bool Mat2::QRDecomposeColumnPivoted(Mat2 &orthogonalFactor, Mat2 &upperTriangularFactor, int columnPermutation[2], int &rank, float tolerance) const {
    float input[2][2];
    for (int row = 0; row < 2; row++) {
        for (int column = 0; column < 2; column++) {
            input[row][column] = mat[row][column];
        }
    }

    float outputQ[2][2];
    float outputR[2][2];
    int outputPermutation[2];
    int outputRank;
    if (!QRCP::Decompose<2>(input, outputQ, outputR, outputPermutation, outputRank, tolerance)) {
        return false;
    }

    Mat2 q(
        Vec2(outputQ[0][0], outputQ[0][1]),
        Vec2(outputQ[1][0], outputQ[1][1]));
    Mat2 r(
        Vec2(outputR[0][0], outputR[0][1]),
        Vec2(outputR[1][0], outputR[1][1]));

    orthogonalFactor = q;
    upperTriangularFactor = r;
    for (int i = 0; i < 2; i++) {
        columnPermutation[i] = outputPermutation[i];
    }
    rank = outputRank;
    return true;
}

bool Mat2::EigenDecomposeSymmetric(Vec2 &eigenValues, Mat2 &eigenVectors, float epsilon) const {
    if (epsilon <= 0.0f || !Math::IsFinite(epsilon)) {
        return false;
    }

    for (int row = 0; row < 2; row++) {
        for (int column = 0; column < 2; column++) {
            float value = mat[row][column];
            if (!Math::IsFinite(value)) {
                return false;
            }
        }
    }

    if (!IsSymmetric(epsilon)) {
        return false;
    }

    float diagonal0 = mat[0][0];
    float diagonal1 = mat[1][1];
    float offDiagonal = 0.5f * mat[0][1] + 0.5f * mat[1][0];
    float halfTrace = 0.5f * diagonal0 + 0.5f * diagonal1;
    float halfDifference = 0.5f * diagonal0 - 0.5f * diagonal1;

    // Compute hypot(halfDifference, offDiagonal) without squaring unscaled values.
    float absHalfDifference = Math::Abs(halfDifference);
    float absOffDiagonal = Math::Abs(offDiagonal);
    float scale = absHalfDifference > absOffDiagonal ? absHalfDifference : absOffDiagonal;
    float radius = 0.0f;
    if (scale > 0.0f) {
        float scaledHalfDifference = halfDifference / scale;
        float scaledOffDiagonal = offDiagonal / scale;
        radius = scale * Math::Sqrt(scaledHalfDifference * scaledHalfDifference + scaledOffDiagonal * scaledOffDiagonal);
    }

    Vec2 values(halfTrace + radius, halfTrace - radius);
    if (!Math::IsFinite(radius) || !Math::IsFinite(values.x) || !Math::IsFinite(values.y)) {
        return false;
    }

    Mat2 vectors;
    if (radius == 0.0f) {
        vectors = Mat2::identity;
    } else {
        // Select the form that avoids cancellation when constructing the major eigenvector.
        Vec2 majorVector = halfDifference >= 0.0f ? Vec2(radius + halfDifference, offDiagonal) : Vec2(offDiagonal, radius - halfDifference);

        float absX = Math::Abs(majorVector.x);
        float absY = Math::Abs(majorVector.y);
        float vectorScale = absX > absY ? absX : absY;
        if (vectorScale == 0.0f || !Math::IsFinite(vectorScale)) {
            return false;
        }

        majorVector *= 1.0f / vectorScale;
        float lengthSqr = majorVector.LengthSqr();
        if (lengthSqr <= 0.0f || !Math::IsFinite(lengthSqr)) {
            return false;
        }
        majorVector *= Math::InvSqrt(lengthSqr);

        // Mat2 is row-major, so explicitly place the orthonormal vectors in columns.
        vectors = Mat2(
            majorVector.x, -majorVector.y,
            majorVector.y,  majorVector.x);
    }

    eigenValues = values;
    eigenVectors = vectors;
    return true;
}

bool Mat2::SingularValueDecompose(Mat2 &leftVectors, Vec2 &singularValues, Mat2 &rightVectors, float tolerance) const {
    float input[2][2];
    for (int row = 0; row < 2; row++) {
        for (int column = 0; column < 2; column++) {
            input[row][column] = mat[row][column];
        }
    }

    float outputU[2][2];
    float outputSingularValues[2];
    float outputV[2][2];
    if (!JacobiSVD::Decompose<2>(input, outputU, outputSingularValues, outputV, tolerance)) {
        return false;
    }

    Mat2 u(
        outputU[0][0], outputU[0][1],
        outputU[1][0], outputU[1][1]);
    Mat2 v(
        outputV[0][0], outputV[0][1],
        outputV[1][0], outputV[1][1]);

    leftVectors = u;
    singularValues.Set(outputSingularValues[0], outputSingularValues[1]);
    rightVectors = v;
    return true;
}

bool Mat2::ConditionNumber(float &conditionNumber, float tolerance) const {
    return JacobiSVD::ComputeConditionNumber<2, Mat2, Vec2>(*this, conditionNumber, tolerance);
}

bool Mat2::PseudoInverse(Mat2 &pseudoInverse, float tolerance) const {
    return JacobiSVD::ComputePseudoInverse<2, Mat2, Vec2>(*this, pseudoInverse, tolerance);
}

Mat2 Mat2::FromString(const char *str) {
    Mat2 m;
    int count = sscanf(str, "%f %f %f %f", &m[0].x, &m[0].y, &m[1].x, &m[1].y);
    assert(count == m.GetDimension());
    return m;
}

BE_NAMESPACE_END
