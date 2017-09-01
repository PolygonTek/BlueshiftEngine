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
#include "Render/Render.h"
#include "RenderUtils.h"

BE_NAMESPACE_BEGIN

void R_CubeMapFaceToAxis(RHI::CubeMapFace face, Mat3 &axis) {
    // FIXME
    switch (face) {
    case RHI::PositiveX:
        axis[0] = Vec3(1, 0, 0);
        axis[1] = Vec3(0, 0, 1);
        axis[2] = Vec3(0, -1, 0);
        break;
    case RHI::NegativeX:
        axis[0] = Vec3(-1, 0, 0);
        axis[1] = Vec3(0, 0, -1);
        axis[2] = Vec3(0, -1, 0);
        break;
    case RHI::PositiveY:
        axis[0] = Vec3(0, 1, 0);
        axis[1] = Vec3(-1, 0, 0);
        axis[2] = Vec3(0, 0, 1);
        break;
    case RHI::NegativeY:
        axis[0] = Vec3(0, -1, 0);
        axis[1] = Vec3(-1, 0, 0);
        axis[2] = Vec3(0, 0, -1);
        break;
    case RHI::PositiveZ:
        axis[0] = Vec3(0, 0, 1);
        axis[1] = Vec3(-1, 0, 0);
        axis[2] = Vec3(0, -1, 0);
        break;
    case RHI::NegativeZ:
        axis[0] = Vec3(0, 0, -1);
        axis[1] = Vec3(1, 0, 0);
        axis[2] = Vec3(0, -1, 0);
        break;
    }
}

/*
----------------------------------------------------------------------------------------------------
 * 뷰 행렬 설정

  R (Rx, Ry, Rz) : 시점자의 오른쪽 방향 벡터, right
  U (Ux, Uy, Uz) : 시점자의 윗쪽 방향 벡터, up
  B (Bx, By, Bz) : 시점자의 뒷쪽 방향 벡터, back

  시점의 위치 : P (Px, Py, Pz)
  
      | Rx,  Ry,  Rz,  -(R과 P의 내적) |
  M = | Ux,  Uy,  Uz,  -(U와 P의 내적) |
      | Bx,  By,  Bz,  -(B와 P의 내적) |
      | 0,   0,   0,    1             |

----------------------------------------------------------------------------------------------------
*/
void R_SetViewMatrix(const Mat3 &viewAxis, const Vec3 &viewOrigin, float *viewMatrix) {
    viewMatrix[0] = -viewAxis[LeftAxis].x;
    viewMatrix[1] = -viewAxis[LeftAxis].y;
    viewMatrix[2] = -viewAxis[LeftAxis].z;
    viewMatrix[3] = -viewAxis[LeftAxis].Dot(-viewOrigin);

    viewMatrix[4] = viewAxis[UpAxis].x;
    viewMatrix[5] = viewAxis[UpAxis].y;
    viewMatrix[6] = viewAxis[UpAxis].z;
    viewMatrix[7] = viewAxis[UpAxis].Dot(-viewOrigin);

    viewMatrix[8] = -viewAxis[ForwardAxis].x;
    viewMatrix[9] = -viewAxis[ForwardAxis].y;
    viewMatrix[10] = -viewAxis[ForwardAxis].z;
    viewMatrix[11] = -viewAxis[ForwardAxis].Dot(-viewOrigin);

    viewMatrix[12] = 0.0f;
    viewMatrix[13] = 0.0f;
    viewMatrix[14] = 0.0f;
    viewMatrix[15] = 1.0f;
}

/*
----------------------------------------------------------------------------------------------------
 * OpenGL 원근 투영 행렬 설정

  N : near
  F : far
  L : near * tan(xFov) * -1
  R : near * tan(xFov)
  B : near * tan(yFov) * -1
  T : near * tan(yFov)
  S : nudge

  a) perspective projection

      | 2*N / (R-L),  0,            (R+L) / (R-L),   0              |
  M = | 0,            2*N / (T-B),  (T+B) / (T-B),   0              |
      | 0,            0,            -(N+F) / (F-N),  -2*N*F / (F-N) |
      | 0,            0,            -1,              0              |

  b) infinite perspective projection

      | 2*N / (R-L),  0,            (R+L) / (R-L),   0              |
  M = | 0,            2*N / (T-B)   (T+B) / (T-B),   0              |
      | 0,            0,            -1 * S,          -2*N*S         |
      | 0,            0,            -1               0              |

----------------------------------------------------------------------------------------------------
*/
void R_SetPerspectiveProjectionMatrix(float xFov, float yFov, float zNear, float zFar, bool infinite, float *projectionMatrix) {
    const float nudge = 0.999f;

    double xmax = zNear * Math::Tan(xFov * Math::Pi / 360.0);
    double xmin = -xmax;

    double ymax = zNear * Math::Tan(yFov * Math::Pi / 360.0);
    double ymin = -ymax;

    double doubleznear = 2.0 * zNear;
    double one_deltax = 1.0 / (xmax - xmin);
    double one_deltay = 1.0 / (ymax - ymin);
    double one_deltaz = 1.0 / (zFar - zNear);

    if (!infinite) {
        projectionMatrix[0] = (float)(doubleznear * one_deltax);
        projectionMatrix[1] = 0;
        projectionMatrix[2] = 0;//(float)((xmax + xmin) * one_deltax);
        projectionMatrix[3] = 0;

        projectionMatrix[4] = 0;
        projectionMatrix[5] = (float)(doubleznear * one_deltay);
        projectionMatrix[6] = 0;//(float)((ymax + ymin) * one_deltay);
        projectionMatrix[7] = 0;

        projectionMatrix[8] = 0;
        projectionMatrix[9] = 0;
        projectionMatrix[10] = (float)(-(zFar + zNear) * one_deltaz);
        projectionMatrix[11] = (float)(-(zFar * doubleznear) * one_deltaz);

        projectionMatrix[12] = 0;
        projectionMatrix[13] = 0;
        projectionMatrix[14] = -1;
        projectionMatrix[15] = 0;
    } else {
        projectionMatrix[0] = (float)(doubleznear * one_deltax);
        projectionMatrix[1] = 0;
        projectionMatrix[2] = 0;//(float)((xmax + xmin) * one_deltax);
        projectionMatrix[3] = 0;

        projectionMatrix[4] = 0;
        projectionMatrix[5] = (float)(doubleznear * one_deltay);
        projectionMatrix[6] = 0;//(float)((ymax + ymin) * one_deltay);
        projectionMatrix[7] = 0;

        projectionMatrix[8] = 0;
        projectionMatrix[9] = 0;
        projectionMatrix[10] = (float)(-nudge);
        projectionMatrix[11] = (float)(-zNear * nudge);

        projectionMatrix[12] = 0;
        projectionMatrix[13] = 0;
        projectionMatrix[14] = -1;
        projectionMatrix[15] = 0;
    }
}

/*
----------------------------------------------------------------------------------------------------
 * OpenGL 직교 투영 행렬 설정

  N : near
  F : far
  L : xSize
  R : xSize
  B : ySize
  T : ySize
  
      | 2 / (R-L),  0,          0,          -(R+L) / (R-L) |
  M = | 0,          2 / (T-B),  0,          -(T+B) / (T-B) |
      | 0,          0,          -2 / (F-N), -(F+N) / (F-N) |
      | 0,          0,          0,          1              |

----------------------------------------------------------------------------------------------------
*/
void R_SetOrthogonalProjectionMatrix(float xSize, float ySize, float zNear, float zFar, float *projectionMatrix) {
    double xmax = xSize;
    double xmin = -xmax;

    double ymax = ySize;
    double ymin = -ymax;

    double one_deltax = 1.0 / (xmax - xmin);
    double one_deltay = 1.0 / (ymax - ymin);
    double one_deltaz = 1.0 / (zFar - zNear);

    projectionMatrix[0] = (float)(2.f * one_deltax);
    projectionMatrix[1] = 0;
    projectionMatrix[2] = 0;
    projectionMatrix[3] = (float)(-(xmax + xmin) * one_deltax);

    projectionMatrix[4] = 0;
    projectionMatrix[5] = (float)(2.f * one_deltay);
    projectionMatrix[6] = 0;
    projectionMatrix[7] = (float)(-(ymax + ymin) * one_deltay);

    projectionMatrix[8] = 0;
    projectionMatrix[9] = 0;
    projectionMatrix[10] = (float)(-2.f * one_deltaz);
    projectionMatrix[11] = (float)(-(zFar + zNear) * one_deltaz);

    projectionMatrix[12] = 0;
    projectionMatrix[13] = 0;
    projectionMatrix[14] = 0;
    projectionMatrix[15] = 1;
}

/*
----------------------------------------------------------------------------------------------------
 * Orthogonal 2d crop 행렬 설정

      | 2 / (R-L),  0,          0,          -(R+L) / (R-L) |
  M = | 0,          2 / (T-B),  0,          -(T+B) / (T-B) |
      | 0,          0,          1,          0,             |
      | 0,          0,          0,          1              |

----------------------------------------------------------------------------------------------------
*/
void R_Set2DCropMatrix(float xmin, float xmax, float ymin, float ymax, float *cropMatrix) {
    double one_deltax = 1.0 / (xmax - xmin);
    double one_deltay = 1.0 / (ymax - ymin);

    cropMatrix[0] = 2.0f * one_deltax;
    cropMatrix[1] = 0;
    cropMatrix[2] = 0;
    cropMatrix[3] = -(xmax + xmin) * one_deltax;

    cropMatrix[4] = 0;
    cropMatrix[5] = 2.0f * one_deltay;
    cropMatrix[6] = 0;
    cropMatrix[7] = -(ymax + ymin) * one_deltay;

    cropMatrix[8] = 0;
    cropMatrix[9] = 0;
    cropMatrix[10] = 1;
    cropMatrix[11] = 0;

    cropMatrix[12] = 0;
    cropMatrix[13] = 0;
    cropMatrix[14] = 0;
    cropMatrix[15] = 1;
}

void R_TangentsFromTriangle(const Vec3 &v0, const Vec3 &v1, const Vec3 &v2, const Vec2 &st0, const Vec2 &st1, const Vec2 &st2, Vec3 &t0, Vec3 &t1, float *handedness) {
    Vec3 side0 = v1 - v0;
    Vec3 side1 = v2 - v0;

//	assert(side0.LengthSqr() > VECTOR_EPSILON);
//	assert(side1.LengthSqr() > VECTOR_EPSILON);	

    float ds1 = st1[0] - st0[0];
    float dt1 = st1[1] - st0[1];

    float ds2 = st2[0] - st0[0];
    float dt2 = st2[1] - st0[1];

    float div;
    float det = ds1 * dt2 - ds2 * dt1;
    if (Math::Fabs(det) < 0.0001f) {
        div = 1.0f;
    } else {
        div = 1.0f / det;
    }

    Vec3 n = side0.Cross(side1);
    n.Normalize();

    t0.Set(dt2*side0.x - dt1*side1.x, dt2*side0.y - dt1*side1.y, dt2*side0.z - dt1*side1.z);
    t0 *= div;
    t0 -= t0.Dot(n) * n;
    t0.Normalize();

    t1.Set(ds1*side1.x - ds2*side0.x, ds1*side1.y - ds2*side0.y, ds1*side1.z - ds2*side0.z);
    t1 *= div;
    t1 -= t1.Dot(n) * n;
    //t1 -= t1.Dot(n) * n + t1.Dot(t0) * t0;
    t1.Normalize();

    //assert(t0.LengthSqr() > 0.0f);
    //assert(t1.LengthSqr() > 0.0f);

    *handedness = t0.Cross(t1).Dot(n) < 0.0 ? -1.0f : 1.0f;
}

bool R_CullShadowVolumeBackCap(const Mat4 &viewProjMatrix, const OBB &boundingBox, const Vec3 &lightProjectionOrigin) {
    Vec3	silVerts[6];
    Vec4	projectedSilVerts[6];
    byte	bits;

    int numSilVerts = boundingBox.GetProjectionSilhouetteVerts(lightProjectionOrigin, silVerts);
    if (!numSilVerts) {
        return false;
    }

    byte cullBits = 0x0F;

    Vec4 *pv = projectedSilVerts;

    for (int i = 0; i < numSilVerts; i++) {
        silVerts[i] -= lightProjectionOrigin;
        *pv = viewProjMatrix * Vec4(silVerts[i], 0.0f);

        // NOTE: Normalized Device Coordinates 에서 z 값이 -1 로 클리핑(near plane clip)
        // 하기 위해서는 xy 를 -w 로 나눈다.
        if (pv->z < 0.0f) {
            pv->w = -pv->w;
        }

        pv->x /= pv->w;
        pv->y /= pv->w;
    
        bits = (pv->x < -1.0f ? BIT(0) : 0);
        bits |= (pv->x > +1.0f ? BIT(1) : 0);
        bits |= (pv->y < -1.0f ? BIT(2) : 0);
        bits |= (pv->y > +1.0f ? BIT(3) : 0);
        
        cullBits &= bits;

        pv++;
    }

    return cullBits ? true : false;
}

// Practical split scheme:
//
// CLi = n*(f/n)^(i/numsplits)
// CUi = n + (f-n)*(i/numsplits)
// Ci = CLi*(lambda) + CUi*(1-lambda)
//
// lambda scales between logarithmic and uniform
void R_ComputeSplitDistances(float dNear, float dFar, float lamda, int numSplits, float *splitDistances) {
    for (int i = 1; i < numSplits; i++) {
        float fIDM = (float)i / numSplits;
        float fLog = dNear <= 0.0f ? 0.0f : dNear * Math::Pow(dFar / dNear, fIDM);
        float fUniform = dNear + fIDM * (dFar - dNear);
        splitDistances[i] = fLog * lamda + fUniform * (1 - lamda);
    }

    splitDistances[0] = dNear;
    splitDistances[numSplits] = dFar;
}

// Calculate Gaussian weights based on kernel size
void R_ComputeGaussianWeights(int kernelSize, float *weights) {
    float sum = 0.0f;
    float sigma = kernelSize / 6.0f;
    float coeff = 2.0f * sigma * sigma;
    float norm = 1.0f / (sigma * Math::Sqrt(Math::TwoPi));

    for (int i = 0; i < kernelSize; i++) {
        float x = (float)i - kernelSize * 0.5f + 0.5f;
        weights[i] = norm * Math::Exp(-x*x / coeff);
        sum += weights[i];
    }

    float invsum = 1.0f / sum;

    for (int i = 0; i < kernelSize; i++) {
        weights[i] *= invsum;
    }
}

// Generate weights and offsets for filter, taking advantage of linear texture filtering
// GPU Gems 2, Chapter 20. Fast Third Order Texture Filtering 참고
void R_Compute1DLinearSamplingWeightsAndOffsets(int numSamples, const float *weights, float *lweights, Vec2 *hoffsets, Vec2 *voffsets) {
    int numSamplesHalf = (numSamples + 1) / 2;
    float kernelRadius = (numSamples - 1) / 2.0f;

    for (int i = 0; i < numSamplesHalf; i++) {
        float a = weights[i*2];
        float b = (i*2 + 1 < numSamples) ? weights[i*2 + 1] : 0.0f;

        lweights[i] = a + b;

        float offset = b / (a + b);
        assert(offset >= 0.0 && offset <= 1.0);

        hoffsets[i] = Vec2((i*2 - kernelRadius) + offset, 0.0f);
        voffsets[i] = Vec2(0.0f, (i*2 - kernelRadius) + offset);
    }
}

void R_GenerateSphereTriangleStripVerts(const Sphere &sphere, int lats, int longs, Vec3 *verts) {
    Vec3 *vptr = verts;

    for (int i = 0; i < lats; i++) {
        float lat0  = Math::Pi * (0.5f + (float)(i) / lats);
        float z0    = Math::Sin(lat0) * sphere.radius;
        float r0    = Math::Cos(lat0) * sphere.radius;

        float lat1  = Math::Pi * (0.5f + (float)(i+1) / lats);
        float z1    = Math::Sin(lat1) * sphere.radius;
        float r1    = Math::Cos(lat1) * sphere.radius;

        for (int j = 0; j < longs; j++) {
            float lng = Math::TwoPi * (float)j / longs;
            float x = Math::Cos(lng);
            float y = Math::Sin(lng);

            *vptr++ = sphere.origin + Vec3(x * r0, y * r0, z0);
            *vptr++ = sphere.origin + Vec3(x * r1, y * r1, z1);
        }
    }
}

BE_NAMESPACE_END
