#include "Precompiled.h"
#include "Math/Math.h"

BE_NAMESPACE_BEGIN

const Mat3x4 Mat3x4::zero(Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0), Vec4(0, 0, 0, 0));
const Mat3x4 Mat3x4::identity(Vec4(1, 0, 0, 0), Vec4(0, 1, 0, 0), Vec4(0, 0, 1, 0));

Mat3x4 Mat3x4::operator*(const Mat3x4 &a) const {
    Mat3x4 dst;

    dst.mat[0][0] = mat[0][0] * a.mat[0][0] + mat[0][1] * a.mat[1][0] + mat[0][2] * a.mat[2][0];
    dst.mat[0][1] = mat[0][0] * a.mat[0][1] + mat[0][1] * a.mat[1][1] + mat[0][2] * a.mat[2][1];
    dst.mat[0][2] = mat[0][0] * a.mat[0][2] + mat[0][1] * a.mat[1][2] + mat[0][2] * a.mat[2][2];
    dst.mat[0][3] = mat[0][0] * a.mat[0][3] + mat[0][1] * a.mat[1][3] + mat[0][2] * a.mat[2][3] + mat[0][3];

    dst.mat[1][0] = mat[1][0] * a.mat[0][0] + mat[1][1] * a.mat[1][0] + mat[1][2] * a.mat[2][0];
    dst.mat[1][1] = mat[1][0] * a.mat[0][1] + mat[1][1] * a.mat[1][1] + mat[1][2] * a.mat[2][1];
    dst.mat[1][2] = mat[1][0] * a.mat[0][2] + mat[1][1] * a.mat[1][2] + mat[1][2] * a.mat[2][2];
    dst.mat[1][3] = mat[1][0] * a.mat[0][3] + mat[1][1] * a.mat[1][3] + mat[1][2] * a.mat[2][3] + mat[1][3];

    dst.mat[2][0] = mat[2][0] * a.mat[0][0] + mat[2][1] * a.mat[1][0] + mat[2][2] * a.mat[2][0];
    dst.mat[2][1] = mat[2][0] * a.mat[0][1] + mat[2][1] * a.mat[1][1] + mat[2][2] * a.mat[2][1];
    dst.mat[2][2] = mat[2][0] * a.mat[0][2] + mat[2][1] * a.mat[1][2] + mat[2][2] * a.mat[2][2];
    dst.mat[2][3] = mat[2][0] * a.mat[0][3] + mat[2][1] * a.mat[1][3] + mat[2][2] * a.mat[2][3] + mat[2][3];

    return dst;
}

Mat3x4 &Mat3x4::TransformSelf(const Mat3x4 &a) {
    float dst[3];

    dst[0] = mat[0][0] * a.mat[0][0] + mat[1][0] * a.mat[0][1] + mat[2][0] * a.mat[0][2];
    dst[1] = mat[0][0] * a.mat[1][0] + mat[1][0] * a.mat[1][1] + mat[2][0] * a.mat[1][2];
    dst[2] = mat[0][0] * a.mat[2][0] + mat[1][0] * a.mat[2][1] + mat[2][0] * a.mat[2][2];
    mat[0][0] = dst[0];
    mat[1][0] = dst[1];
    mat[2][0] = dst[2];

    dst[0] = mat[0][1] * a.mat[0][0] + mat[1][1] * a.mat[0][1] + mat[2][1] * a.mat[0][2];
    dst[1] = mat[0][1] * a.mat[1][0] + mat[1][1] * a.mat[1][1] + mat[2][1] * a.mat[1][2];
    dst[2] = mat[0][1] * a.mat[2][0] + mat[1][1] * a.mat[2][1] + mat[2][1] * a.mat[2][2];
    mat[0][1] = dst[0];
    mat[1][1] = dst[1];
    mat[2][1] = dst[2];

    dst[0] = mat[0][2] * a.mat[0][0] + mat[1][2] * a.mat[0][1] + mat[2][2] * a.mat[0][2];
    dst[1] = mat[0][2] * a.mat[1][0] + mat[1][2] * a.mat[1][1] + mat[2][2] * a.mat[1][2];
    dst[2] = mat[0][2] * a.mat[2][0] + mat[1][2] * a.mat[2][1] + mat[2][2] * a.mat[2][2];
    mat[0][2] = dst[0];
    mat[1][2] = dst[1];
    mat[2][2] = dst[2];

    dst[0] = mat[0][3] * a.mat[0][0] + mat[1][3] * a.mat[0][1] + mat[2][3] * a.mat[0][2];
    dst[1] = mat[0][3] * a.mat[1][0] + mat[1][3] * a.mat[1][1] + mat[2][3] * a.mat[1][2];
    dst[2] = mat[0][3] * a.mat[2][0] + mat[1][3] * a.mat[2][1] + mat[2][3] * a.mat[2][2];
    mat[0][3] = dst[0];
    mat[1][3] = dst[1];
    mat[2][3] = dst[2];

    mat[0][3] += a.mat[0][3];
    mat[1][3] += a.mat[1][3];
    mat[2][3] += a.mat[2][3];

    return *this;
}

Mat3x4 &Mat3x4::UntransformSelf(const Mat3x4 &a) {
    float dst[3];

    mat[0][3] -= a.mat[0][3];
    mat[1][3] -= a.mat[1][3];
    mat[2][3] -= a.mat[2][3];

    dst[0] = mat[0][0] * a.mat[0][0] + mat[1][0] * a.mat[1][0] + mat[2][0] * a.mat[2][0];
    dst[1] = mat[0][0] * a.mat[0][1] + mat[1][0] * a.mat[1][1] + mat[2][0] * a.mat[2][1];
    dst[2] = mat[0][0] * a.mat[0][2] + mat[1][0] * a.mat[1][2] + mat[2][0] * a.mat[2][2];
    mat[0][0] = dst[0];
    mat[1][0] = dst[1];
    mat[2][0] = dst[2];

    dst[0] = mat[0][1] * a.mat[0][0] + mat[1][1] * a.mat[1][0] + mat[2][1] * a.mat[2][0];
    dst[1] = mat[0][1] * a.mat[0][1] + mat[1][1] * a.mat[1][1] + mat[2][1] * a.mat[2][1];
    dst[2] = mat[0][1] * a.mat[0][2] + mat[1][1] * a.mat[1][2] + mat[2][1] * a.mat[2][2];
    mat[0][1] = dst[0];
    mat[1][1] = dst[1];
    mat[2][1] = dst[2];

    dst[0] = mat[0][2] * a.mat[0][0] + mat[1][2] * a.mat[1][0] + mat[2][2] * a.mat[2][0];
    dst[1] = mat[0][2] * a.mat[0][1] + mat[1][2] * a.mat[1][1] + mat[2][2] * a.mat[2][1];
    dst[2] = mat[0][2] * a.mat[0][2] + mat[1][2] * a.mat[1][2] + mat[2][2] * a.mat[2][2];
    mat[0][2] = dst[0];
    mat[1][2] = dst[1];
    mat[2][2] = dst[2];

    dst[0] = mat[0][3] * a.mat[0][0] + mat[1][3] * a.mat[1][0] + mat[2][3] * a.mat[2][0];
    dst[1] = mat[0][3] * a.mat[0][1] + mat[1][3] * a.mat[1][1] + mat[2][3] * a.mat[2][1];
    dst[2] = mat[0][3] * a.mat[0][2] + mat[1][3] * a.mat[1][2] + mat[2][3] * a.mat[2][2];
    mat[0][3] = dst[0];
    mat[1][3] = dst[1];
    mat[2][3] = dst[2];

    return *this;
}

BE_NAMESPACE_END
