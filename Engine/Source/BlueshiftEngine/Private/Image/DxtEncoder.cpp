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

#include "Precompiled.h"
#include "Core/Task.h"
#include "Math/Math.h"
#include "Image/DxtEncoder.h"
#include "Eigen/Eigen/Dense"

BE_NAMESPACE_BEGIN

#define INSET_COLOR_SHIFT       4       // inset the bounding box with ( range >> shift )
#define INSET_ALPHA_SHIFT       5       // inset alpha channel

#define C565_5_MASK             0xF8    // 0xFF minus last three bits
#define C565_6_MASK             0xFC    // 0xFF minus last two bits

static void SwapColors(byte *c1, byte *c2) {
    byte tm[3];
    memcpy(tm, c1, 3);
    memcpy(c1, c2, 3);
    memcpy(c2, tm, 3);
}

void DXTEncoder::GetMinMaxBBox(const byte *colorBlock, byte *minColor, byte *maxColor) {
    minColor[0] = minColor[1] = minColor[2] = minColor[3] = 255;
    maxColor[0] = maxColor[1] = maxColor[2] = maxColor[3] = 0;

    for (int i = 0; i < 16; i++) {
        if (colorBlock[i * 4 + 0] < minColor[0]) {
            minColor[0] = colorBlock[i * 4 + 0];
        }
        if (colorBlock[i * 4 + 1] < minColor[1]) {
            minColor[1] = colorBlock[i * 4 + 1];
        }
        if (colorBlock[i * 4 + 2] < minColor[2]) {
            minColor[2] = colorBlock[i * 4 + 2];
        }
        if (colorBlock[i * 4 + 3] < minColor[3]) {
            minColor[3] = colorBlock[i * 4 + 3];
        }
        if (colorBlock[i * 4 + 0] > maxColor[0]) {
            maxColor[0] = colorBlock[i * 4 + 0];
        }
        if (colorBlock[i * 4 + 1] > maxColor[1]) {
            maxColor[1] = colorBlock[i * 4 + 1];
        }
        if (colorBlock[i * 4 + 2] > maxColor[2]) {
            maxColor[2] = colorBlock[i * 4 + 2];
        }
        if (colorBlock[i * 4 + 3] > maxColor[3]) {
            maxColor[3] = colorBlock[i * 4 + 3];
        }
    }
}

void DXTEncoder::GetMinMaxAlpha(const byte *colorBlock, const int alphaOffset, byte *minColor, byte *maxColor) {
    byte alphaMin = 255;
    byte alphaMax = 0;

    // get alpha min / max
    for (int i = 0; i < 16; i++) {
        if (colorBlock[i * 4 + alphaOffset] < alphaMin) {
            alphaMin = colorBlock[i * 4 + alphaOffset];
        }
        if (colorBlock[i * 4 + alphaOffset] > alphaMax) {
            alphaMax = colorBlock[i * 4 + alphaOffset];
        }
    }

    minColor[alphaOffset] = (byte)alphaMin;
    maxColor[alphaOffset] = (byte)alphaMax;
}

static Vec3 GetMultiplicity1Evector(const float matrix[6], float evalue) {
    // compute A - e*I
    float m[6];
    m[0] = matrix[0] - evalue;
    m[1] = matrix[1];
    m[2] = matrix[2];
    m[3] = matrix[3] - evalue;
    m[4] = matrix[4];
    m[5] = matrix[5] - evalue;

    // compute cofactor matrix
    float u[6];
    u[0] = m[3] * m[5] - m[4] * m[4];
    u[1] = m[2] * m[4] - m[1] * m[5];
    u[2] = m[1] * m[4] - m[2] * m[3];
    u[3] = m[0] * m[5] - m[2] * m[2];
    u[4] = m[1] * m[2] - m[4] * m[0];
    u[5] = m[0] * m[3] - m[1] * m[1];

    // find the largest component
    float mc = std::fabs(u[0]);
    int mi = 0;
    for (int i = 1; i < 6; ++i) {
        float c = std::fabs(u[i]);
        if (c > mc) {
            mc = c;
            mi = i;
        }
    }

    // pick the column with this component
    switch (mi) {
    case 0:
        return Vec3(u[0], u[1], u[2]);
    case 1:
    case 3:
        return Vec3(u[1], u[3], u[4]);
    default:
        return Vec3(u[2], u[4], u[5]);
    }
}

static Vec3 GetMultiplicity2Evector(const float matrix[6], float evalue) {
    // compute A - e*I
    float m[6];
    m[0] = matrix[0] - evalue;
    m[1] = matrix[1];
    m[2] = matrix[2];
    m[3] = matrix[3] - evalue;
    m[4] = matrix[4];
    m[5] = matrix[5] - evalue;

    // find the largest component
    float mc = std::fabs(m[0]);
    int mi = 0;
    for (int i = 1; i < 6; ++i) {
        float c = std::fabs(m[i]);
        if (c > mc) {
            mc = c;
            mi = i;
        }
    }

    // pick the first eigenvector based on this index
    switch (mi) {
    case 0:
    case 1:
        return Vec3(-m[1], m[0], 0.0f);
    case 2:
        return Vec3(m[2], 0.0f, -m[0]);
    case 3:
    case 4:
        return Vec3(0.0f, -m[4], m[3]);
    default:
        return Vec3(0.0f, -m[5], m[4]);
    }
}

// symmetric 3x3 matrix
// m[0] m[1] m[2]
//      m[3] m[4]
//           m[5]
static Vec3 ComputePrincipalComponent(const float matrix[6]) {
    // compute the cubic coefficients
    // x^3 - c2 * x^2 + c1 * x + c0 = 0
    float c0 = matrix[0] * matrix[4] * matrix[4] + matrix[3] * matrix[2] * matrix[2] + matrix[5] * matrix[1] * matrix[1] - 
        matrix[0] * matrix[3] * matrix[5] - 2.0f * matrix[1] * matrix[2] * matrix[4];
    float c1 = matrix[0] * matrix[3] + matrix[0] * matrix[5] + matrix[3] * matrix[5] - 
        matrix[1] * matrix[1] - matrix[2] * matrix[2] - matrix[4] * matrix[4];
    float c2 = matrix[0] + matrix[3] + matrix[5];
    
    // compute the quadratic coefficients
    // x = t + c2 / 3
    // t^3 + p * t + q = 0
    float p = c1 - (1.0f / 3.0f) * c2 * c2;
    float q = (-2.0f / 27.0f) * c2 * c2 * c2 + (1.0f / 3.0f) * c1 * c2 + c0;

    // compute the root count check
    float d = 0.25f * q * q + (1.0f / 27.0f) * p * p * p;

    // test the multiplicity
    if (d > FLT_EPSILON) {
        // only one root, which implies we have a multiple of the identity
        return Vec3(1.0f);
    } else if (d < -FLT_EPSILON) {
        // three distinct roots
        float theta = std::atan2(std::sqrt(-d), -0.5f * q);
        float rho = std::sqrt(0.25f * q * q - d);

        float rt = std::pow(rho, 1.0f / 3.0f);
        float ct = std::cos(theta / 3.0f);
        float st = std::sin(theta / 3.0f);

        float l1 = (1.0f / 3.0f) * c2 + 2.0f * rt * ct;
        float l2 = (1.0f / 3.0f) * c2 - rt * (ct + (float)sqrt(3.0f) * st);
        float l3 = (1.0f / 3.0f) * c2 - rt * (ct - (float)sqrt(3.0f) * st);

        // pick the larger
        if (std::fabs(l2) > std::fabs(l1)) {
            l1 = l2;
        }
        if (std::fabs(l3) > std::fabs(l1)) {
            l1 = l3;
        }

        // get the eigenvector
        return GetMultiplicity1Evector(matrix, l1);
    } else { // if (d >= -FLT_EPSILON && d <= FLT_EPSILON) {
        // two roots
        float rt;
        if (q < 0.0f) {
            rt = -std::pow(-0.5f * q, 1.0f / 3.0f);
        } else {
            rt = std::pow(0.5f * q, 1.0f / 3.0f);
        }

        float l1 = (1.0f / 3.0f) * c2 + rt;		// repeated
        float l2 = (1.0f / 3.0f) * c2 - 2.0f * rt;

        // get the eigenvector
        if (std::fabs(l1) > std::fabs(l2)) {
            return GetMultiplicity2Evector(matrix, l1);
        } else {
            return GetMultiplicity1Evector(matrix, l2);
        }
    }
}

void DXTEncoder::GetMinMaxColorHQ(const byte *colorBlock, byte *minColor, byte *maxColor) {
    // Normalize colors and compute mean vector
    Vec3 points[16];
    Vec3 centroid = Vec3::zero;

    for (int i = 0; i < 16; i++) {
        points[i] = RGB888ToVec3(&colorBlock[i * 4]);

        centroid += points[i];
    }
    centroid /= 16.0f;

    // Compute symmetric covariance matrix
    float covMatrix[6] = { 0, };

    for (int i = 0; i < 16; i++) {
        Vec3 a = points[i] - centroid;

        covMatrix[0] += a.x * a.x;
        covMatrix[1] += a.x * a.y;
        covMatrix[2] += a.x * a.z;
        covMatrix[3] += a.y * a.y;
        covMatrix[4] += a.y * a.z;
        covMatrix[5] += a.z * a.z;
    }

    // Compute principal axis from covariance matrix
    // TODO: compare to https://en.wikipedia.org/wiki/Eigenvalue_algorithm
#if 1
    Vec3 principalAxis = ComputePrincipalComponent(covMatrix);
    float l = principalAxis.Normalize();
    if (l == 0.0f) {
        principalAxis.Set(1.0f, 1.0f, 1.0f);
        principalAxis.Normalize();
    }
#else
    Eigen::Matrix3f m;
    m << covMatrix[0], covMatrix[1], covMatrix[2],
        covMatrix[1], covMatrix[3], covMatrix[4],
        covMatrix[2], covMatrix[4], covMatrix[5];
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigSolver(m);
    Vec3 principalAxis(eigSolver.eigenvectors().col(2).data());
#endif

    // Compute bounds relative to principle axis
    float bounds[2] = { 99999.0f, -99999.0f };

    for (int i = 0; i < 16; i++) {
        float p = principalAxis.Dot(points[i] - centroid);
        if (p < bounds[0]) {
            bounds[0] = p;
        }
        if (p > bounds[1]) {
            bounds[1] = p;
        }
    }

    // Compute min/max points
    Vec3 minPoint = centroid + bounds[0] * principalAxis;
    Vec3 maxPoint = centroid + bounds[1] * principalAxis;

    minPoint.Clamp(Vec3::zero, Vec3::one);
    maxPoint.Clamp(Vec3::zero, Vec3::one);

    RGB888FromVec3(minPoint, minColor);
    RGB888FromVec3(maxPoint, maxColor);

    uint16_t minColor565 = RGB888To565(minColor);
    uint16_t maxColor565 = RGB888To565(maxColor);
    
    if (minColor565 > maxColor565) {
        SwapColors(minColor, maxColor);
    }
}

void DXTEncoder::InsetColorsBBox(byte *minColor, byte *maxColor) {
    byte inset[4];

    inset[0] = (maxColor[0] - minColor[0]) >> INSET_COLOR_SHIFT;
    inset[1] = (maxColor[1] - minColor[1]) >> INSET_COLOR_SHIFT;
    inset[2] = (maxColor[2] - minColor[2]) >> INSET_COLOR_SHIFT;
    inset[3] = (maxColor[3] - minColor[3]) >> INSET_COLOR_SHIFT;

    minColor[0] = (minColor[0] + inset[0] <= 255) ? minColor[0] + inset[0] : 255;
    minColor[1] = (minColor[1] + inset[1] <= 255) ? minColor[1] + inset[1] : 255;
    minColor[2] = (minColor[2] + inset[2] <= 255) ? minColor[2] + inset[2] : 255;
    minColor[3] = (minColor[3] + inset[3] <= 255) ? minColor[3] + inset[3] : 255;

    maxColor[0] = (maxColor[0] - inset[0] >= 0) ? maxColor[0] - inset[0] : 0;
    maxColor[1] = (maxColor[1] - inset[1] >= 0) ? maxColor[1] - inset[1] : 0;
    maxColor[2] = (maxColor[2] - inset[2] >= 0) ? maxColor[2] - inset[2] : 0;
    maxColor[3] = (maxColor[3] - inset[3] >= 0) ? maxColor[3] - inset[3] : 0;
}

void DXTEncoder::InsetNormalsBBox3Dc(byte *minNormal, byte *maxNormal) {
    int inset[4];
    int mini[4];
    int maxi[4];

    inset[0] = (maxNormal[0] - minNormal[0]) - ((1 << (INSET_ALPHA_SHIFT - 1)) - 1);
    inset[1] = (maxNormal[1] - minNormal[1]) - ((1 << (INSET_ALPHA_SHIFT - 1)) - 1);

    mini[0] = ((minNormal[0] << INSET_ALPHA_SHIFT) + inset[0]) >> INSET_ALPHA_SHIFT;
    mini[1] = ((minNormal[1] << INSET_ALPHA_SHIFT) + inset[1]) >> INSET_ALPHA_SHIFT;

    maxi[0] = ((maxNormal[0] << INSET_ALPHA_SHIFT) - inset[0]) >> INSET_ALPHA_SHIFT;
    maxi[1] = ((maxNormal[1] << INSET_ALPHA_SHIFT) - inset[1]) >> INSET_ALPHA_SHIFT;

    mini[0] = (mini[0] >= 0) ? mini[0] : 0;
    mini[1] = (mini[1] >= 0) ? mini[1] : 0;

    maxi[0] = (maxi[0] <= 255) ? maxi[0] : 255;
    maxi[1] = (maxi[1] <= 255) ? maxi[1] : 255;

    minNormal[0] = (byte)mini[0];
    minNormal[1] = (byte)mini[1];

    maxNormal[0] = (byte)maxi[0];
    maxNormal[1] = (byte)maxi[1];
}

void DXTEncoder::GetMinMaxColorsMaxDist(const byte *colorBlock, byte *minColor, byte *maxColor) {
    int maxDistC = -1;
    int maxDistA = -1;

    for (int i = 0; i < 64 - 4; i += 4) {
        for (int j = i + 4; j < 64; j += 4) {
            int dc = ColorDistance(&colorBlock[i], &colorBlock[j]);
            if (dc > maxDistC) {
                maxDistC = dc;
                memcpy(minColor, colorBlock + i, 3);
                memcpy(maxColor, colorBlock + j, 3);
            }
            int da = AlphaDistance(colorBlock[i + 3], colorBlock[j + 3]);
            if (da > maxDistA) {
                maxDistA = da;
                minColor[3] = colorBlock[i + 3];
                maxColor[3] = colorBlock[j + 3];
            }
        }
    }

    if (maxColor[0] < minColor[0]) {
        SwapColors(minColor, maxColor);
    }
}

void DXTEncoder::ComputeAlphaIndicesFast(const byte *colorBlock, const int alphaOffset, const byte maxAlpha, const byte minAlpha, byte *out) {
    assert(maxAlpha >= minAlpha);
    const int ALPHA_RANGE = 7;

    byte ab1, ab2, ab3, ab4, ab5, ab6, ab7;
    ALIGN16(byte indexes[16]);

    ab1 = (13 * maxAlpha +  1 * minAlpha + ALPHA_RANGE) / (ALPHA_RANGE * 2);
    ab2 = (11 * maxAlpha +  3 * minAlpha + ALPHA_RANGE) / (ALPHA_RANGE * 2);
    ab3 = ( 9 * maxAlpha +  5 * minAlpha + ALPHA_RANGE) / (ALPHA_RANGE * 2);
    ab4 = ( 7 * maxAlpha +  7 * minAlpha + ALPHA_RANGE) / (ALPHA_RANGE * 2);
    ab5 = ( 5 * maxAlpha +  9 * minAlpha + ALPHA_RANGE) / (ALPHA_RANGE * 2);
    ab6 = ( 3 * maxAlpha + 11 * minAlpha + ALPHA_RANGE) / (ALPHA_RANGE * 2);
    ab7 = ( 1 * maxAlpha + 13 * minAlpha + ALPHA_RANGE) / (ALPHA_RANGE * 2);

    colorBlock += alphaOffset;

    for (int i = 0; i < 16; i++) {
        byte a = colorBlock[i * 4];
        int b1 = (a >= ab1);
        int b2 = (a >= ab2);
        int b3 = (a >= ab3);
        int b4 = (a >= ab4);
        int b5 = (a >= ab5);
        int b6 = (a >= ab6);
        int b7 = (a >= ab7);
        int index = (8 - b1 - b2 - b3 - b4 - b5 - b6 - b7) & 7;
        indexes[i] = byte(index ^ (2 > index));
    }

    out[0] = (indexes[0] >> 0) | (indexes[1] << 3) | (indexes[2] << 6);
    out[1] = (indexes[2] >> 2) | (indexes[3] << 1) | (indexes[4] << 4) | (indexes[5] << 7);
    out[2] = (indexes[5] >> 1) | (indexes[6] << 2) | (indexes[7] << 5);

    out[3] = (indexes[8] >> 0) | (indexes[9] << 3) | (indexes[10] << 6);
    out[4] = (indexes[10] >> 2) | (indexes[11] << 1) | (indexes[12] << 4) | (indexes[13] << 7);
    out[5] = (indexes[13] >> 1) | (indexes[14] << 2) | (indexes[15] << 5);
}

int DXTEncoder::ComputeAlphaIndices(const byte *colorBlock, const int alphaOffset, const byte alpha0, const byte alpha1, byte *rindexes) {
    byte alphas[8];
    alphas[0] = alpha0;
    alphas[1] = alpha1;

    if (alpha0 > alpha1) {
        alphas[2] = (6 * alpha0 + 1 * alpha1) / 7;
        alphas[3] = (5 * alpha0 + 2 * alpha1) / 7;
        alphas[4] = (4 * alpha0 + 3 * alpha1) / 7;
        alphas[5] = (3 * alpha0 + 4 * alpha1) / 7;
        alphas[6] = (2 * alpha0 + 5 * alpha1) / 7;
        alphas[7] = (1 * alpha0 + 6 * alpha1) / 7;
    } else {
        alphas[2] = (4 * alpha0 + 1 * alpha1) / 5;
        alphas[3] = (3 * alpha0 + 2 * alpha1) / 5;
        alphas[4] = (2 * alpha0 + 3 * alpha1) / 5;
        alphas[5] = (1 * alpha0 + 4 * alpha1) / 5;
        alphas[6] = 0;
        alphas[7] = 255;
    }

    unsigned int indexes[16];
    int error = 0;

    for (int i = 0; i < 16; i++) {
        unsigned int bestError = 256 * 256;
        byte a = colorBlock[i * 4 + alphaOffset];
        for (int j = 0; j < 8; j++) {
            unsigned int d = AlphaDistance(a, alphas[j]);
            if (d < bestError) {
                bestError = d;
                indexes[i] = j;
            }
        }
        error += bestError;
    }

    rindexes[0] = byte((indexes[0] >> 0) | (indexes[1] << 3) | (indexes[2] << 6));
    rindexes[1] = byte((indexes[2] >> 2) | (indexes[3] << 1) | (indexes[4] << 4) | (indexes[5] << 7));
    rindexes[2] = byte((indexes[5] >> 1) | (indexes[6] << 2) | (indexes[7] << 5));

    rindexes[3] = byte((indexes[8] >> 0) | (indexes[9] << 3) | (indexes[10] << 6));
    rindexes[4] = byte((indexes[10] >> 2) | (indexes[11] << 1) | (indexes[12] << 4) | (indexes[13] << 7));
    rindexes[5] = byte((indexes[13] >> 1) | (indexes[14] << 2) | (indexes[15] << 5));

    return error;
}

void DXTEncoder::Compute4BitsAlpha(const byte *colorBlock, const int alphaOffset, uint16_t *out) {
    colorBlock += alphaOffset;

    out[0] = ((int)colorBlock[0] >> 4) | ((int)colorBlock[1] & 0x0F) | (((int)colorBlock[2] & 0x0F) << 8) | (((int)colorBlock[3] & 0xF0) << 12);
    out[1] = ((int)colorBlock[4] >> 4) | ((int)colorBlock[5] & 0x0F) | (((int)colorBlock[6] & 0x0F) << 8) | (((int)colorBlock[7] & 0xF0) << 12);
    out[2] = ((int)colorBlock[8] >> 4) | ((int)colorBlock[9] & 0x0F) | (((int)colorBlock[10] & 0x0F) << 8) | (((int)colorBlock[11] & 0xF0) << 12);
    out[3] = ((int)colorBlock[12] >> 4) | ((int)colorBlock[13] & 0x0F) | (((int)colorBlock[14] & 0x0F) << 8) | (((int)colorBlock[15] & 0xF0) << 12);
}

void DXTEncoder::ComputeColorIndicesFast(const byte *colorBlock, const byte *maxColor, const byte *minColor, uint32_t *out) {
    ALIGN16(uint16_t colors[4][4]);
    uint32_t result = 0;

    colors[0][0] = (maxColor[0] & C565_5_MASK) | (maxColor[0] >> 5);
    colors[0][1] = (maxColor[1] & C565_6_MASK) | (maxColor[1] >> 6);
    colors[0][2] = (maxColor[2] & C565_5_MASK) | (maxColor[2] >> 5);
    colors[0][3] = 0;
    colors[1][0] = (minColor[0] & C565_5_MASK) | (minColor[0] >> 5);
    colors[1][1] = (minColor[1] & C565_6_MASK) | (minColor[1] >> 6);
    colors[1][2] = (minColor[2] & C565_5_MASK) | (minColor[2] >> 5);
    colors[1][3] = 0;
    colors[2][0] = (2 * colors[0][0] + 1 * colors[1][0]) / 3;
    colors[2][1] = (2 * colors[0][1] + 1 * colors[1][1]) / 3;
    colors[2][2] = (2 * colors[0][2] + 1 * colors[1][2]) / 3;
    colors[2][3] = 0;
    colors[3][0] = (1 * colors[0][0] + 2 * colors[1][0]) / 3;
    colors[3][1] = (1 * colors[0][1] + 2 * colors[1][1]) / 3;
    colors[3][2] = (1 * colors[0][2] + 2 * colors[1][2]) / 3;
    colors[3][3] = 0;

    // uses sum of absolute differences instead of squared distance to find the best match
    for (int i = 15; i >= 0; i--) {
        int c0, c1, c2, c3, m, d0, d1, d2, d3;

        c0 = colorBlock[i * 4 + 0];
        c1 = colorBlock[i * 4 + 1];
        c2 = colorBlock[i * 4 + 2];
        c3 = colorBlock[i * 4 + 3];

        m = colors[0][0] - c0;
        d0 = abs(m);
        m = colors[1][0] - c0;
        d1 = abs(m);
        m = colors[2][0] - c0;
        d2 = abs(m);
        m = colors[3][0] - c0;
        d3 = abs(m);

        m = colors[0][1] - c1;
        d0 += abs(m);
        m = colors[1][1] - c1;
        d1 += abs(m);
        m = colors[2][1] - c1;
        d2 += abs(m);
        m = colors[3][1] - c1;
        d3 += abs(m);

        m = colors[0][2] - c2;
        d0 += abs(m);
        m = colors[1][2] - c2;
        d1 += abs(m);
        m = colors[2][2] - c2;
        d2 += abs(m);
        m = colors[3][2] - c2;
        d3 += abs(m);

#if 0
        int b0 = d0 > d2;
        int b1 = d1 > d3;
        int b2 = d0 > d3;
        int b3 = d1 > d2;
        int b4 = d0 > d1;
        int b5 = d2 > d3;

        result |= ((!b3 & b4) | (b2 & b5) | (((b0 & b3) | (b1 & b2)) << 1)) << (i << 1);
#else
        bool b0 = d0 > d3;
        bool b1 = d1 > d2;
        bool b2 = d0 > d2;
        bool b3 = d1 > d3;
        bool b4 = d2 > d3;

        int x0 = b1 & b2;
        int x1 = b0 & b3;
        int x2 = b0 & b4;

        result |= (x2 | ((x0 | x1) << 1)) << (i << 1);
#endif
    }

    *out = result;
}

int DXTEncoder::ComputeColorIndices(const byte *colorBlock, const uint16_t color0, const uint16_t color1, uint32_t &result) {
    byte colors[4][4];

    RGB888From565(color0, colors[0]);
    RGB888From565(color1, colors[1]);

    if (color0 > color1) {
        colors[2][0] = (2 * colors[0][0] + 1 * colors[1][0]) / 3;
        colors[2][1] = (2 * colors[0][1] + 1 * colors[1][1]) / 3;
        colors[2][2] = (2 * colors[0][2] + 1 * colors[1][2]) / 3;
        colors[3][0] = (1 * colors[0][0] + 2 * colors[1][0]) / 3;
        colors[3][1] = (1 * colors[0][1] + 2 * colors[1][1]) / 3;
        colors[3][2] = (1 * colors[0][2] + 2 * colors[1][2]) / 3;
    } else {
        colors[2][0] = (1 * colors[0][0] + 1 * colors[1][0]) / 2;
        colors[2][1] = (1 * colors[0][1] + 1 * colors[1][1]) / 2;
        colors[2][2] = (1 * colors[0][2] + 1 * colors[1][2]) / 2;
        colors[3][0] = 0;
        colors[3][1] = 0;
        colors[3][2] = 0;
    }

    uint32_t indexes[16];

    int error = 0;
    for (int i = 0; i < 16; i++) {
        unsigned int bestError = 256 * 256 * 4;
        for (int j = 0; j < 4; j++) {
            unsigned int d = ColorDistance(&colorBlock[i * 4], &colors[j][0]);
            if (d < bestError) {
                bestError = d;
                indexes[i] = j;
            }
        }
        error += bestError;
    }

    result = 0;
    for (int i = 0; i < 16; i++) {
        result |= (indexes[i] << (uint32_t)(i << 1));
    }

    return error;
}

void DXTEncoder::EncodeDXT1BlockFast(const byte *colorBlock, byte **dstPtr) {
    ALIGN16(byte minColor[4]);
    ALIGN16(byte maxColor[4]);
    ALIGN16(DXTBlock::ColorBlock dxtColorBlock);

    GetMinMaxBBox(colorBlock, minColor, maxColor);
    InsetColorsBBox(minColor, maxColor);

    dxtColorBlock.color0 = RGB888To565(maxColor);
    dxtColorBlock.color1 = RGB888To565(minColor);

    ComputeColorIndicesFast(colorBlock, maxColor, minColor, &dxtColorBlock.indexes);

    memcpy(*dstPtr, &dxtColorBlock, sizeof(dxtColorBlock));
    *dstPtr += sizeof(dxtColorBlock);
}

void DXTEncoder::EncodeDXT3BlockFast(const byte *colorBlock, byte **dstPtr) {
    ALIGN16(byte minColor[4]);
    ALIGN16(byte maxColor[4]);
    ALIGN16(DXTBlock::ColorBlock dxtColorBlock);
    ALIGN16(DXTBlock::AlphaExplicitBlock dxtAlphaBlock);

    GetMinMaxBBox(colorBlock, minColor, maxColor);
    InsetColorsBBox(minColor, maxColor);

    Compute4BitsAlpha(colorBlock, 3, dxtAlphaBlock.row);

    memcpy(*dstPtr, &dxtAlphaBlock, sizeof(dxtAlphaBlock));
    *dstPtr += sizeof(dxtAlphaBlock);

    dxtColorBlock.color0 = RGB888To565(maxColor);
    dxtColorBlock.color1 = RGB888To565(minColor);

    ComputeColorIndicesFast(colorBlock, maxColor, minColor, &dxtColorBlock.indexes);

    memcpy(*dstPtr, &dxtColorBlock, sizeof(dxtColorBlock));
    *dstPtr += sizeof(dxtColorBlock);
}

void DXTEncoder::EncodeDXT5BlockFast(const byte *colorBlock, byte **dstPtr) {
    ALIGN16(byte minColor[4]);
    ALIGN16(byte maxColor[4]);
    ALIGN16(DXTBlock::ColorBlock dxtColorBlock);
    ALIGN16(DXTBlock::AlphaBlock dxtAlphaBlock);

    GetMinMaxBBox(colorBlock, minColor, maxColor);
    InsetColorsBBox(minColor, maxColor);

    dxtAlphaBlock.alpha0 = maxColor[3];
    dxtAlphaBlock.alpha1 = minColor[3];

    ComputeAlphaIndicesFast(colorBlock, 3, maxColor[3], minColor[3], dxtAlphaBlock.indexes);

    memcpy(*dstPtr, &dxtAlphaBlock, sizeof(dxtAlphaBlock));
    *dstPtr += sizeof(dxtAlphaBlock);

    dxtColorBlock.color0 = RGB888To565(maxColor);
    dxtColorBlock.color1 = RGB888To565(minColor);

    ComputeColorIndicesFast(colorBlock, maxColor, minColor, &dxtColorBlock.indexes);

    memcpy(*dstPtr, &dxtColorBlock, sizeof(dxtColorBlock));
    *dstPtr += sizeof(dxtColorBlock);
}

void DXTEncoder::EncodeDXN2BlockFast(const byte *colorBlock, byte **dstPtr) {
    ALIGN16(byte minNormal[4]);
    ALIGN16(byte maxNormal[4]);
    ALIGN16(DXTBlock::AlphaBlock dxtAlphaBlock);

    GetMinMaxBBox(colorBlock, minNormal, maxNormal);
    InsetNormalsBBox3Dc(minNormal, maxNormal);

    for (int i = 0; i < 2; i++) {
        dxtAlphaBlock.alpha0 = maxNormal[i];
        dxtAlphaBlock.alpha1 = minNormal[i];

        ComputeAlphaIndicesFast(colorBlock, i, maxNormal[i], minNormal[i], dxtAlphaBlock.indexes);

        memcpy(*dstPtr, &dxtAlphaBlock, sizeof(dxtAlphaBlock));
        *dstPtr += sizeof(dxtAlphaBlock);
    }
}

void DXTEncoder::EncodeDXT1BlockHQ(const byte *colorBlock, byte **dstPtr) {
    ALIGN16(byte minColor[4]);
    ALIGN16(byte maxColor[4]);
    ALIGN16(DXTBlock::ColorBlock dxtColorBlock);

    bool transparent = false;
    for (int i = 0; i < 16; i++) {
        if (colorBlock[3] < 128) {
            transparent = true;
            break;
        }
    }

    GetMinMaxColorHQ(colorBlock, minColor, maxColor);

    uint16_t maxColor565 = RGB888To565(maxColor);
    uint16_t minColor565 = RGB888To565(minColor);

    if (!transparent) {
        dxtColorBlock.color0 = maxColor565;
        dxtColorBlock.color1 = minColor565;
    } else {
        dxtColorBlock.color0 = minColor565;
        dxtColorBlock.color1 = maxColor565;
    }

    ComputeColorIndices(colorBlock, dxtColorBlock.color0, dxtColorBlock.color1, dxtColorBlock.indexes);

    memcpy(*dstPtr, &dxtColorBlock, sizeof(dxtColorBlock));
    *dstPtr += sizeof(DXTBlock::ColorBlock);
}

void DXTEncoder::EncodeDXT3BlockHQ(const byte *colorBlock, byte **dstPtr) {
    ALIGN16(byte minColor[4]);
    ALIGN16(byte maxColor[4]);
    ALIGN16(DXTBlock::ColorBlock dxtColorBlock);
    ALIGN16(DXTBlock::AlphaExplicitBlock dxtAlphaBlock);

    Compute4BitsAlpha(colorBlock, 3, dxtAlphaBlock.row);

    memcpy(*dstPtr, &dxtAlphaBlock, sizeof(dxtAlphaBlock));
    *dstPtr += sizeof(dxtAlphaBlock);

    GetMinMaxColorHQ(colorBlock, minColor, maxColor);

    dxtColorBlock.color0 = RGB888To565(maxColor);
    dxtColorBlock.color1 = RGB888To565(minColor);

    ComputeColorIndices(colorBlock, dxtColorBlock.color0, dxtColorBlock.color1, dxtColorBlock.indexes);

    memcpy(*dstPtr, &dxtColorBlock, sizeof(dxtColorBlock));
    *dstPtr += sizeof(dxtColorBlock);
}

void DXTEncoder::EncodeDXT5BlockHQ(const byte *colorBlock, byte **dstPtr) {
    ALIGN16(byte minColor[4]);
    ALIGN16(byte maxColor[4]);
    ALIGN16(DXTBlock::ColorBlock dxtColorBlock);
    ALIGN16(DXTBlock::AlphaBlock dxtAlphaBlock1);
    ALIGN16(DXTBlock::AlphaBlock dxtAlphaBlock2);

    GetMinMaxAlpha(colorBlock, 3, minColor, maxColor);
    GetMinMaxColorHQ(colorBlock, minColor, maxColor);
    
    dxtAlphaBlock1.alpha0 = maxColor[3];
    dxtAlphaBlock1.alpha1 = minColor[3]; 
    
    dxtAlphaBlock2.alpha0 = minColor[3];
    dxtAlphaBlock2.alpha1 = maxColor[3];

    int error1 = ComputeAlphaIndices(colorBlock, 3, dxtAlphaBlock1.alpha0, dxtAlphaBlock1.alpha1, dxtAlphaBlock1.indexes);
    int error2 = ComputeAlphaIndices(colorBlock, 3, dxtAlphaBlock2.alpha0, dxtAlphaBlock2.alpha1, dxtAlphaBlock2.indexes);

    if (error1 < error2) {
        memcpy(*dstPtr, &dxtAlphaBlock1, sizeof(dxtAlphaBlock1));
    } else {
        memcpy(*dstPtr, &dxtAlphaBlock2, sizeof(dxtAlphaBlock2));
    }

    *dstPtr += sizeof(DXTBlock::AlphaBlock);

    dxtColorBlock.color0 = RGB888To565(maxColor);
    dxtColorBlock.color1 = RGB888To565(minColor);

    ComputeColorIndices(colorBlock, dxtColorBlock.color0, dxtColorBlock.color1, dxtColorBlock.indexes);

    memcpy(*dstPtr, &dxtColorBlock, sizeof(dxtColorBlock));
    *dstPtr += sizeof(dxtColorBlock);
}

void DXTEncoder::EncodeDXN2BlockHQ(const byte *colorBlock, byte **dstPtr) {
    ALIGN16(byte minNormal[4]);
    ALIGN16(byte maxNormal[4]);
    ALIGN16(DXTBlock::AlphaBlock dxtAlphaBlock1);
    ALIGN16(DXTBlock::AlphaBlock dxtAlphaBlock2);

    for (int i = 0; i < 2; i++) {
        GetMinMaxAlpha(colorBlock, i, minNormal, maxNormal);

        dxtAlphaBlock1.alpha0 = maxNormal[i];
        dxtAlphaBlock1.alpha1 = minNormal[i];

        dxtAlphaBlock2.alpha0 = minNormal[i];
        dxtAlphaBlock2.alpha1 = maxNormal[i];

        int error1 = ComputeAlphaIndices(colorBlock, i, dxtAlphaBlock1.alpha0, dxtAlphaBlock1.alpha1, dxtAlphaBlock1.indexes);
        int error2 = ComputeAlphaIndices(colorBlock, i, dxtAlphaBlock2.alpha0, dxtAlphaBlock2.alpha1, dxtAlphaBlock2.indexes);

        if (error1 < error2) {
            memcpy(*dstPtr, &dxtAlphaBlock1, sizeof(dxtAlphaBlock1));
        } else {
            memcpy(*dstPtr, &dxtAlphaBlock2, sizeof(dxtAlphaBlock2));
        }

        *dstPtr += sizeof(DXTBlock::AlphaBlock);
    }
}


BE_INLINE void DXTEncoder::ExtractBlock(const byte *src, int srcPitch, int blockWidth, int blockHeight, byte *colorBlock) {
    for (int by = 0; by < 4; by++) {
        const byte *srcPtrY = src + srcPitch * (by % blockHeight);

        for (int bx = 0; bx < 4; bx++) {
            const byte *srcPtrX = srcPtrY + 4 * (bx % blockWidth);

            for (int i = 0; i < 4; i++) {
                *colorBlock++ = srcPtrX[i];
            }
        }
    }
}

void DXTEncoder::CompressImageDXT1Fast(const byte *src, const int width, const int height, const int depth, byte *dst) {
    ALIGN16(byte colorBlock[4 * 16]);
    byte *dstPtr = dst;
    
    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y += 4, src += width * 4 * 4) {
            for (int x = 0; x < width; x += 4) {
                int bw = Min(4, width - x);
                int bh = Min(4, height - y);

                ExtractBlock(src + 4 * x, 4 * width, bw, bh, colorBlock);

                EncodeDXT1BlockFast(colorBlock, &dstPtr);
            }
        }
    }
}

void DXTEncoder::CompressImageDXT1HQ(const byte *src, const int width, const int height, const int depth, byte *dst) {
    ALIGN16(byte colorBlock[4 * 16]);
    byte *dstPtr = dst;

    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y += 4, src += width * 4 * 4) {
            for (int x = 0; x < width; x += 4) {
                int bw = Min(4, width - x);
                int bh = Min(4, height - y);

                ExtractBlock(src + 4 * x, 4 * width, bw, bh, colorBlock);

                EncodeDXT1BlockHQ(colorBlock, &dstPtr);
            }
        }
    }
}

void DXTEncoder::CompressImageDXT3Fast(const byte *src, const int width, const int height, const int depth, byte *dst) {
    ALIGN16(byte colorBlock[4 * 16]);
    byte *dstPtr = dst;

    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y += 4, src += width * 4 * 4) {
            for (int x = 0; x < width; x += 4) {
                int bw = Min(4, width - x);
                int bh = Min(4, height - y);

                ExtractBlock(src + 4 * x, 4 * width, bw, bh, colorBlock);

                EncodeDXT3BlockFast(colorBlock, &dstPtr);
            }
        }
    }
}

void DXTEncoder::CompressImageDXT3HQ(const byte *src, const int width, const int height, const int depth, byte *dst) {
    ALIGN16(byte colorBlock[4 * 16]);
    byte *dstPtr = dst;

    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y += 4, src += width * 4 * 4) {
            for (int x = 0; x < width; x += 4) {
                int bw = Min(4, width - x);
                int bh = Min(4, height - y);

                ExtractBlock(src + 4 * x, 4 * width, bw, bh, colorBlock);

                EncodeDXT3BlockHQ(colorBlock, &dstPtr);
            }
        }
    }
}

void DXTEncoder::CompressImageDXT5Fast(const byte *src, const int width, const int height, const int depth, byte *dst) {
    ALIGN16(byte colorBlock[4 * 16]);
    byte *dstPtr = dst;

    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y += 4, src += width * 4 * 4) {
            for (int x = 0; x < width; x += 4) {
                int bw = Min(4, width - x);
                int bh = Min(4, height - y);

                ExtractBlock(src + 4 * x, 4 * width, bw, bh, colorBlock);

                EncodeDXT5BlockFast(colorBlock, &dstPtr);
            }
        }
    }
}

void DXTEncoder::CompressImageDXT5HQ(const byte *src, const int width, const int height, const int depth, byte *dst) {
    ALIGN16(byte colorBlock[4 * 16]);
    byte *dstPtr = dst;

    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y += 4, src += width * 4 * 4) {
            for (int x = 0; x < width; x += 4) {
                int bw = Min(4, width - x);
                int bh = Min(4, height - y);

                ExtractBlock(src + 4 * x, 4 * width, bw, bh, colorBlock);

                EncodeDXT5BlockHQ(colorBlock, &dstPtr);
            }
        }
    }
}

void DXTEncoder::CompressImageDXN2Fast(const byte *src, const int width, const int height, const int depth, byte *dst) {
    ALIGN16(byte colorBlock[4 * 16]);
    byte *dstPtr = dst;

    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y += 4, src += width * 4 * 4) {
            for (int x = 0; x < width; x += 4) {
                int bw = Min(4, width - x);
                int bh = Min(4, height - y);

                ExtractBlock(src + 4 * x, 4 * width, bw, bh, colorBlock);

                EncodeDXN2BlockFast(colorBlock, &dstPtr);
            }
        }
    }
}

void DXTEncoder::CompressImageDXN2HQ(const byte *src, const int width, const int height, const int depth, byte *dst) {
    ALIGN16(byte colorBlock[4 * 16]);
    byte *dstPtr = dst;

    for (int z = 0; z < depth; z++) {
        for (int y = 0; y < height; y += 4, src += width * 4 * 4) {
            for (int x = 0; x < width; x += 4) {
                int bw = Min(4, width - x);
                int bh = Min(4, height - y);

                ExtractBlock(src + 4 * x, 4 * width, bw, bh, colorBlock);

                EncodeDXN2BlockHQ(colorBlock, &dstPtr);
            }
        }
    }
}

BE_NAMESPACE_END
