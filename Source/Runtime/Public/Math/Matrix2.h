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

#include "Core/Str.h"

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Mat2 - 2x2 Matrix (row major)

-------------------------------------------------------------------------------
*/

/// A 2-by-2 matrix for linear transformations of 2D geometry.
class BE_API Mat2 {
public:
    /// Specifies the height of this matrix.
    enum { Rows = 2 };

    /// Specifies the width of this matrix.
    enum { Cols = 2 };

    /// The default constructor does not initialize any members of this class.
    Mat2() = default;
    /// Constructs a Mat2 by explicitly specifying the two row vectors.
    Mat2(const Vec2 &row0, const Vec2 &row1);
    /// Constructs a Mat2 by explicitly specifying all the matrix elements.
    Mat2(float _00, float _01, float _10, float _11);
    /// Constructs a Mat2 from a C array, to the value.
    explicit Mat2(const float data[2][2]);
    explicit Mat2(const float *data);

                        /// Casts this Mat2 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
    const float *       Ptr() const { return (const float *)&mat[0]; }
    float *             Ptr() { return (float *)&mat[0]; }
                        /// Casts this Mat2 to a C array.
                        /// This function simply returns a C pointer view to this data structure.
                        /// This function is identical to the member function Ptr().
                        operator const float *() const { return (const float *)&mat[0]; }
                        operator float *() { return (float *)&mat[0]; }

                        /// Accesses an element of this matrix.
    Vec2 &              At(int index) { return (*this)[index]; }
                        /// Accesses an element of this matrix using array notation.
    const Vec2 &        operator[](int index) const;
    Vec2 &              operator[](int index);

                        /// Unary operator + allows this structure to be used in an expression '+m'.
    const Mat2 &        operator+() const { return *this; }

                        /// Performs an unary negation of this matrix.
    Mat2                Negate() const & { return -(*this); }
    Mat2 &&             Negate() && { return -std::move(*this); }
                        /// Performs an unary negation of this matrix.
                        /// This function is identical to the member function Negate().
    Mat2                operator-() const &;
    Mat2 &&             operator-() &&;

                        /// Adds a matrix to this matrix.
    Mat2                Add(const Mat2 &m) const & { return *this + m; }
    Mat2 &&             Add(const Mat2 &m) && { *this += m; return std::move(*this); }
                        /// Adds a matrix to this matrix.
    Mat2                operator+(const Mat2 &rhs) const &;
    Mat2 &&             operator+(const Mat2 &rhs) && { *this += rhs; return std::move(*this); }

                        /// Subtracts a matrix from this matrix.
    Mat2                Sub(const Mat2 &m) const & { return *this - m; }
    Mat2 &&             Sub(const Mat2 &m) && { *this -= m; return std::move(*this); }
                        /// Subtracts a matrix from this matrix.
    Mat2                operator-(const Mat2 &rhs) const &;
    Mat2 &&             operator-(const Mat2 &rhs) && { *this -= rhs; return std::move(*this); }

                        /// Multiplies a matrix to this matrix.
    Mat2                Mul(const Mat2 &m) const & { return *this * m; }
    Mat2 &&             Mul(const Mat2 &m) && { *this *= m; return std::move(*this); }
                        /// Multiplies a matrix to this matrix.
                        /// This function is identical to the member function Mul().
    Mat2                operator*(const Mat2 &rhs) const &;
    Mat2 &&             operator*(const Mat2 &rhs) && { *this *= rhs; return std::move(*this); }

                        /// Returns this->Transpose() * m
    Mat2                TransposedMul(const Mat2 &m) const;

                        /// Multiplies this matrix by a scalar.
    Mat2                MulScalar(float s) const & { return *this * s; }
    Mat2 &&             MulScalar(float s) && { *this *= s; return std::move(*this); }
                        /// Multiplies this matrix by a scalar.
                        /// This function is identical to the member function MulScalar().
    Mat2                operator*(float rhs) const &;
    Mat2 &&             operator*(float rhs) && { *this *= rhs; return std::move(*this); }
                        /// Multiplies the given matrix by a scalar.
    friend Mat2         operator*(float lhs, const Mat2 &rhs) { return rhs * lhs; }
    friend Mat2 &&      operator*(float lhs, Mat2 &&rhs) { rhs *= lhs; return std::move(rhs); }

                        /// Transforms the given vector by this matrix
    Vec2                MulVec(const Vec2 &v) const { return *this * v; }
                        /// Transforms the given vector by this matrix.
                        /// This function is identical to the member function MulVec().
    Vec2                operator*(const Vec2 &rhs) const;

                        /// Returns this->Transpose() * v
    Vec2                TransposedMulVec(const Vec2 &v) const;
                        /// Transforms the given vector by the given matrix in the order v * M (!= M * v).
    friend Vec2         operator*(const Vec2 &lhs, const Mat2 &rhs) { return rhs.TransposedMulVec(lhs); }

                        /// Assign from another matrix.
    Mat2 &              operator=(const Mat2 &rhs);

                        /// Adds a matrix to this matrix, in-place.
    Mat2 &              AddSelf(const Mat2 &m) { *this += m; return *this; }
                        /// Adds a matrix to this matrix, in-place.
                        /// This function is identical to the member function AddSelf().
    Mat2 &              operator+=(const Mat2 &rhs);

                        /// Subtracts a matrix from this matrix, in-place.
    Mat2 &              SubSelf(const Mat2 &m) { *this -= m; return *this; }
                        /// Subtracts a matrix from this matrix, in-place.
                        /// This function is identical to the member function SubSelf().
    Mat2 &              operator-=(const Mat2 &rhs);
    
                        /// Multiplies this matrix with the given matrix, in-place.
    Mat2 &              MulSelf(const Mat2 &m) { *this *= m; return *this; }
                        /// Multiplies this matrix with the given matrix, in-place.
                        /// This function is identical to the member function MulSelf().
    Mat2 &              operator*=(const Mat2 &rhs);
    
                        /// Multiplies this matrix with the given scalar, in-place.
    Mat2 &              MulScalarSelf(float s) { *this *= s; return *this; }
                        /// Multiplies this matrix with the given scalar, in-place.
                        /// This function is identical to the member function MulScalarSelf().
    Mat2 &              operator*=(float rhs);
    
                        /// Exact compare, no epsilon.
    bool                Equals(const Mat2 &m) const;
                        /// Compare with epsilon.
    bool                Equals(const Mat2 &m, const float epsilon) const;
                        /// Exact compare, no epsilon.
    bool                operator==(const Mat2 &rhs) const { return Equals(rhs); }
                        /// Exact compare, no epsilon.
    bool                operator!=(const Mat2 &rhs) const { return !Equals(rhs); }

                        /// Tests if every element is finite.
    bool                IsFinite() const { return mat[0].IsFinite() && mat[1].IsFinite(); }
                        /// Tests if this is the identity matrix, up to the given epsilon.
    bool                IsIdentity(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the upper triangular matrix, up to the given epsilon.
    bool                IsUpperTriangular(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the lower triangular matrix, up to the given epsilon.
    bool                IsLowerTriangular(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the symmetric matrix, up to the given epsilon.
    bool                IsSymmetric(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the diagonal matrix, up to the given epsilon.
    bool                IsDiagonal(const float epsilon = MATRIX_EPSILON) const;
                        /// Tests if this is the singular matrix.
    bool                IsSingular() const;

                        /// Returns the minimum component.
    float               MinComponent() const { return Min(mat[0].MinComponent(), mat[1].MinComponent()); }
                        /// Returns the maximum component.
    float               MaxComponent() const { return Max(mat[0].MaxComponent(), mat[1].MaxComponent()); }
                        /// Returns the minimum absolute component.
    float               MinAbsComponent() const { return Min(mat[0].MinAbsComponent(), mat[1].MinAbsComponent()); }
                        /// Returns the maximum absolute component.
    float               MaxAbsComponent() const { return Max(mat[0].MaxAbsComponent(), mat[1].MaxAbsComponent()); }

                        /// Returns the squared Frobenius norm of this matrix.
    float               FrobeniusNormSqr() const;
                        /// Returns the Frobenius norm of this matrix.
    float               FrobeniusNorm() const;

                        /// Sets all the element of this matrix to zero.
    void                SetZero();
                        /// Sets this matrix to equal the identity.
    void                SetIdentity();

                        /// Returns the sum of the diagonal elements of this matrix.
                        /// Mathematically, this means sum of all eigenvalues.
    float               Trace() const;
                        /// Returns the determinant of this matrix.
                        /// Mathematically, this means multiplication of all eigenvalues.
    float               Determinant() const;

                        /// Transposes this matrix.
                        /// This operation swaps all elements with respect to the diagonal.
    Mat2                Transpose() const &;
    Mat2 &&             Transpose() && { return std::move(TransposeSelf()); }
                        /// Transposes this matrix, in-place.
    Mat2 &              TransposeSelf();

                        /// Returns the symmetric part, 0.5 * (A + A^T), of this matrix.
    Mat2                Symmetrize() const &;
    Mat2                Symmetrize() && { SymmetrizeSelf(); return std::move(*this); }
                        /// Replaces this matrix with its symmetric part, 0.5 * (A + A^T).
    Mat2 &              SymmetrizeSelf();

                        /// Returns the skew-symmetric part, 0.5 * (A - A^T), of this matrix.
    Mat2                SkewSymmetrize() const &;
    Mat2                SkewSymmetrize() && { SkewSymmetrizeSelf(); return std::move(*this); }
                        /// Replaces this matrix with its skew-symmetric part, 0.5 * (A - A^T).
    Mat2 &              SkewSymmetrizeSelf();

                        /// Inverts this matrix.
    Mat2                Inverse() const &;
    Mat2 &&             Inverse() && { InverseSelf(); return std::move(*this); }
                        /// Inverts this matrix, in-place.
                        /// @return False if determinant is zero.
    bool                InverseSelf();

                        /// Solves A * x = b using Doolittle LU decomposition with partial pivoting.
                        /// The matrix must be finite and numerically nonsingular relative to its
                        /// largest-magnitude element and the requested tolerance.
                        /// On failure, solution is left unchanged. rightHandSide may alias solution.
                        /// @return False if the inputs are invalid or no finite solution is representable.
    bool                Solve(const Vec2 &rightHandSide, Vec2 &solution, float tolerance = MATRIX_EPSILON) const;

                        /// Computes A = L * L^T, where L is lower triangular.
                        /// The matrix must be finite, symmetric, and numerically positive definite
                        /// relative to its largest-magnitude element and the requested tolerance.
                        /// On failure, lowerTriangular is left unchanged.
                        /// @return False if the input requirements are not met.
    bool                CholeskyDecompose(Mat2 &lowerTriangular, float tolerance = MATRIX_EPSILON) const;

                        /// Solves A * x = b for this symmetric positive semi-definite matrix.
                        /// Positive-definite matrices use Cholesky factorization. Numerically
                        /// rank-deficient matrices return the minimum-norm least-squares solution
                        /// using a symmetric eigendecomposition fallback.
                        /// On failure, solution is left unchanged. rightHandSide may alias solution.
                        /// @return False if the inputs are non-finite, tolerance is invalid, this
                        /// matrix is non-symmetric or indefinite, or no finite solution is representable.
    bool                SolveSymmetricPSD(const Vec2 &rightHandSide, Vec2 &solution, float tolerance = MATRIX_EPSILON) const;

                        /// Computes the column-pivoted QR factorization A * P = Q * R.
                        /// Q is orthogonal, and R is upper triangular with non-negative diagonal entries.
                        /// columnPermutation[j] is the original column represented by pivoted column j.
                        /// rank is the numerical rank relative to the largest diagonal entry of R.
                        /// Rank-deficient matrices are successfully decomposed.
                        /// On failure, all output parameters are left unchanged.
                        /// @return False if this matrix is not finite or the factors cannot be
                        /// represented by finite floats.
    bool                QRDecomposeColumnPivoted(Mat2 &orthogonalFactor, Mat2 &upperTriangularFactor, int columnPermutation[2], int &rank, float tolerance = MATRIX_EPSILON) const;

                        /// Decomposes this symmetric matrix into eigenvalues and eigenvectors.
                        /// Eigenvalues are sorted in descending order and the corresponding
                        /// orthonormal eigenvectors are stored in the columns of eigenVectors.
                        /// On failure, the output parameters are left unchanged.
                        /// @return False if this matrix is non-finite or non-symmetric.
    bool                EigenDecomposeSymmetric(Vec2 &eigenValues, Mat2 &eigenVectors, float epsilon = MATRIX_EPSILON) const;

                        /// Computes A = U * diag(singularValues) * V^T using one-sided Jacobi SVD.
                        /// Singular values are non-negative and sorted in descending order.
                        /// Numerically zero singular values may be clamped to zero and their
                        /// corresponding left singular vectors complete an orthonormal basis.
                        /// On failure, the output parameters are left unchanged.
    bool                SingularValueDecompose(Mat2 &leftVectors, Vec2 &singularValues, Mat2 &rightVectors, float tolerance = MATRIX_EPSILON) const;

                        /// Computes the spectral condition number sigmaMax / sigmaMin.
                        /// Singular values at or below tolerance * sigmaMax are treated as zero.
                        /// A numerically rank-deficient matrix returns positive infinity.
                        /// On failure, conditionNumber is left unchanged.
    bool                ConditionNumber(float &conditionNumber, float tolerance = MATRIX_EPSILON) const;

                        /// Computes the Moore-Penrose pseudoinverse using SVD.
                        /// Singular values at or below tolerance * sigmaMax are treated as zero.
                        /// On failure, pseudoInverse is left unchanged.
    bool                PseudoInverse(Mat2 &pseudoInverse, float tolerance = MATRIX_EPSILON) const;

    static Mat2         FromOuterProduct(const Vec2 &a, const Vec2 &b);

                        /// Returns "_00 _01 _10 _11".
    const char *        ToString() const { return ToString(4); }
                        /// Returns "_00 _01 _10 _11" with the given precision.
    const char *        ToString(int precision) const;

                        /// Creates from the string.
    static Mat2         FromString(const char *str);

                        /// Returns dimension of this type.
    constexpr int       GetDimension() const { return Rows * Cols; }

    ALIGN_AS16 static const Mat2 zero;
    ALIGN_AS16 static const Mat2 identity;

    Vec2                mat[Rows];
};

BE_INLINE Mat2::Mat2(const Vec2 &row0, const Vec2 &row1) {
    mat[0] = row0;
    mat[1] = row1;
}

BE_INLINE Mat2::Mat2(float _00, float _01, float _10, float _11) {
    mat[0][0] = _00; 
    mat[0][1] = _01;

    mat[1][0] = _10;
    mat[1][1] = _11;
}

BE_INLINE Mat2::Mat2(const float data[2][2]) {
    mat[0][0] = data[0][0];
    mat[0][1] = data[0][1];
    mat[1][0] = data[1][0];
    mat[1][1] = data[1][1];
}

BE_INLINE Mat2::Mat2(const float *data) {
    mat[0][0] = data[0];
    mat[0][1] = data[1];
    mat[1][0] = data[2];
    mat[1][1] = data[2];
}

BE_INLINE const Vec2 &Mat2::operator[](int index) const {
    assert((index >= 0) && (index < Rows));
    return mat[index];
}

BE_INLINE Vec2 &Mat2::operator[](int index) {
    assert((index >= 0) && (index < Rows));
    return mat[index];
}

BE_INLINE Mat2 Mat2::operator+(const Mat2 &a) const & {
    return Mat2(
        mat[0].x + a[0].x, mat[0].y + a[0].y, 
        mat[1].x + a[1].x, mat[1].y + a[1].y);
}
    
BE_INLINE Mat2 Mat2::operator-(const Mat2 &a) const & {
    return Mat2(
        mat[0].x - a[0].x, mat[0].y - a[0].y, 
        mat[1].x - a[1].x, mat[1].y - a[1].y);
}

BE_INLINE Mat2 Mat2::operator*(const Mat2 &a) const & {
    return Mat2(
        mat[0].x * a[0].x + mat[0].y * a[1].x,
        mat[0].x * a[0].y + mat[0].y * a[1].y,
        mat[1].x * a[0].x + mat[1].y * a[1].x,
        mat[1].x * a[0].y + mat[1].y * a[1].y);
}

BE_INLINE Mat2 Mat2::TransposedMul(const Mat2 &a) const {
    return Mat2(
        mat[0].x * a[0].x + mat[1].x * a[1].x,
        mat[0].x * a[0].y + mat[1].x * a[1].y,
        mat[0].y * a[0].x + mat[1].y * a[1].x,
        mat[0].y * a[0].y + mat[1].y * a[1].y);
}

BE_INLINE Mat2 Mat2::operator*(float a) const & {
    return Mat2(
        mat[0].x * a, mat[0].y * a, 
        mat[1].x * a, mat[1].y * a);
}

BE_INLINE Vec2 Mat2::operator*(const Vec2 &vec) const {
    return Vec2(
        mat[0].x * vec.x + mat[0].y * vec.y, 
        mat[1].x * vec.x + mat[1].y * vec.y);
}

BE_INLINE Vec2 Mat2::TransposedMulVec(const Vec2 &vec) const {
    return Vec2(
        mat[0].x * vec.x + mat[1].x * vec.y,
        mat[0].y * vec.x + mat[1].y * vec.y);
}

BE_INLINE Mat2 &Mat2::operator=(const Mat2 &rhs) {
    mat[0][0] = rhs[0][0];
    mat[0][1] = rhs[0][1];

    mat[1][0] = rhs[1][0];
    mat[1][1] = rhs[1][1];

    return *this;
}

BE_INLINE Mat2 &Mat2::operator+=(const Mat2 &rhs) {
    mat[0].x += rhs[0].x;
    mat[0].y += rhs[0].y;

    mat[1].x += rhs[1].x;
    mat[1].y += rhs[1].y;

    return *this;
}

BE_INLINE Mat2 &Mat2::operator-=(const Mat2 &rhs) {
    mat[0].x -= rhs[0].x;
    mat[0].y -= rhs[0].y;

    mat[1].x -= rhs[1].x;
    mat[1].y -= rhs[1].y;

    return *this;
}

BE_INLINE Mat2 &Mat2::operator*=(float a) {
    mat[0].x *= a;
    mat[0].y *= a;

    mat[1].x *= a;
    mat[1].y *= a;

    return *this;
}

BE_INLINE bool Mat2::Equals(const Mat2 &a) const {
    if (mat[0].Equals(a[0]) && 
        mat[1].Equals(a[1])) {
        return true;
    }
    return false;
}

BE_INLINE bool Mat2::Equals(const Mat2 &a, const float epsilon) const {
    if (mat[0].Equals(a[0], epsilon) && 
        mat[1].Equals(a[1], epsilon)) {
        return true;
    }
    return false;
}

BE_INLINE void Mat2::SetZero() {
    *this = Mat2::zero;
}

BE_INLINE void Mat2::SetIdentity() {
    *this = Mat2::identity;
}

BE_INLINE bool Mat2::IsIdentity(const float epsilon) const {
    return Equals(Mat2::identity, epsilon);
}

BE_INLINE bool Mat2::IsUpperTriangular(const float epsilon) const {
    return (Math::Abs(mat[1][0]) <= epsilon);
}

BE_INLINE bool Mat2::IsLowerTriangular(const float epsilon) const {
    return (Math::Abs(mat[0][1]) <= epsilon);
}

BE_INLINE bool Mat2::IsSymmetric(const float epsilon) const {
    return (Math::Abs(mat[0][1] - mat[1][0]) <= epsilon);
}

BE_INLINE bool Mat2::IsDiagonal(const float epsilon) const {
    if (Math::Abs(mat[0][1]) > epsilon || Math::Abs(mat[1][0]) > epsilon) {
        return false;
    }
    return true;
}

BE_INLINE bool Mat2::IsSingular() const {
    return Determinant() == 0 ? true : false;
}

BE_INLINE float Mat2::FrobeniusNormSqr() const {
    return mat[0].LengthSqr() + mat[1].LengthSqr();
}

BE_INLINE float Mat2::FrobeniusNorm() const {
    return Math::Sqrt(FrobeniusNormSqr());
}

BE_INLINE float Mat2::Trace() const {
    return mat[0][0] + mat[1][1];
};

BE_INLINE float Mat2::Determinant() const {
    return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
}

BE_INLINE Mat2 Mat2::Transpose() const & {
    return Mat2(
        mat[0][0], mat[1][0], 
        mat[0][1], mat[1][1]);
}

BE_INLINE Mat2 &Mat2::TransposeSelf() {
    float tmp = mat[0][1];
    mat[0][1] = mat[1][0];
    mat[1][0] = tmp;
    return *this;
}

BE_INLINE Mat2 Mat2::Symmetrize() const & {
    Mat2 symmetric = *this;
    symmetric.SymmetrizeSelf();
    return symmetric;
}

BE_INLINE Mat2 &Mat2::SymmetrizeSelf() {
    float xy = 0.5f * (mat[0][1] + mat[1][0]);
    mat[0][1] = mat[1][0] = xy;
    return *this;
}

BE_INLINE Mat2 Mat2::SkewSymmetrize() const & {
    Mat2 skewSymmetric = *this;
    skewSymmetric.SkewSymmetrizeSelf();
    return skewSymmetric;
}

BE_INLINE Mat2 &Mat2::SkewSymmetrizeSelf() {
    float xy = 0.5f * (mat[0][1] - mat[1][0]);

    mat[0][0] = mat[1][1] = 0.0f;
    mat[0][1] = xy;
    mat[1][0] = -xy;

    return *this;
}

BE_INLINE Mat2 Mat2::Inverse() const & {
    Mat2 invMat = *this;
    bool r = invMat.InverseSelf();
    assert(r);
    return invMat;
}

BE_INLINE Mat2 Mat2::FromOuterProduct(const Vec2 &a, const Vec2 &b) {
    return Mat2(
        a[0] * b,
        a[1] * b);
}

BE_INLINE const char *Mat2::ToString(int precision) const {
    return Str::FloatArrayToString((const float *)(*this), GetDimension(), precision);
}

BE_NAMESPACE_END
