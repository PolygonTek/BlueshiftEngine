//----------------------------------------------------------------------------------
// File:        jni/nv_math/nv_quat.cpp
// SDK Version: v10.14 
// Email:       tegradev@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2007-2012, NVIDIA CORPORATION.  All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA OR ITS SUPPLIERS
// BE  LIABLE  FOR  ANY  SPECIAL,  INCIDENTAL,  INDIRECT,  OR  CONSEQUENTIAL DAMAGES
// WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS OF BUSINESS PROFITS,
// BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
// ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
//
//----------------------------------------------------------------------------------
#include "nv_quat.h"
#include <stdlib.h>

void NvQuatCopy(GLfloat r[4], const GLfloat q[4])
{
    memcpy(r, q, 4 * sizeof(GLfloat));
}

void NvQuatConvertTo3x3Mat(GLfloat r[3][3], const GLfloat q[4])
{
    // Assumes that the quaternion is normalized!
    GLfloat x2 = q[0] * 2.0f;
    GLfloat y2 = q[1] * 2.0f;
    GLfloat z2 = q[2] * 2.0f;
    GLfloat xSq2 = x2 * q[0];
    GLfloat ySq2 = y2 * q[1];
    GLfloat zSq2 = z2 * q[2];
    GLfloat xy2 = x2 * q[1];
    GLfloat xz2 = x2 * q[2];
    GLfloat xw2 = x2 * q[3];
    GLfloat yz2 = y2 * q[2];
    GLfloat yw2 = y2 * q[3];
    GLfloat zw2 = z2 * q[3];

    /* Matrix is
     * |  1 - 2y^2 - 2z^2   2xy - 2zw         2xz + 2yw        |
     * |  2xy + 2zw         1 - 2x^2 - 2z^2   2yz - 2xw        |
     * |  2xz - 2yw         2yz + 2xw         1 - 2x^2 - 2y^2  |
     */
    r[0][0] = 1.0f - ySq2 - zSq2;
    r[0][1] = xy2 - zw2;
    r[0][2] = xz2 + yw2;

    r[1][0] = xy2 + zw2;
    r[1][1] = 1.0f - xSq2 - zSq2;
    r[1][2] = yz2 - xw2;

    r[2][0] = xz2 - yw2;
    r[2][1] = yz2 + xw2;
    r[2][2] = 1.0f - xSq2 - ySq2;
}
void NvQuatConvertTo4x4Mat(GLfloat r[4][4], const GLfloat q[4])
{

    /*  Matrix is
     *  | 1 - 2(y^2 + z^2)	2(xy + wz)			2(xz - wy)			0  |
     *  | 2(xy - wz)		1 - 2(x^2 + z^2)	2(yz + wx)			0  |
     *  | 2(xz + wy)		2(yz - wx)			1 - 2(x^2 + y^2)	0  |
     *  | 0					0					0					1  |
	*/
    float x2 = q[0] + q[0]; 
    float y2 = q[1] + q[1]; 
    float z2 = q[2] + q[2];
    float xx = q[0] * x2;
    float xy = q[0] * y2;
    float xz = q[0] * z2;
    float yy = q[1] * y2;
    float yz = q[1] * z2;
    float zz = q[2] * z2;
    float wx = q[3] * x2;
    float wy = q[3] * y2;
    float wz = q[3] * z2;

    r[0][0] = 1.0f - (yy + zz);
    r[0][1] = xy + wz;
    r[0][2] = xz - wy;
    r[0][3] = 0.0f;

    r[1][0] = xy - wz;
    r[1][1] = 1.0f - (xx + zz);
    r[1][2] = yz + wx;
    r[1][3] = 0.0f;

    r[2][0] = xz + wy;
    r[2][1] = yz - wx;
    r[2][2] = 1.0f - (xx + yy);
    r[2][3] = 0.0f;

    r[3][0] = 0.0f;
    r[3][1] = 0.0f;
    r[3][2] = 0.0f;
    r[3][3] = 1.0f;
}

void NvQuatFrom4x4Mat(GLfloat r[4][4], GLfloat qout[4])
{
    float s = 0.0f;
    float q[4] = {0.0f};
    float trace = r[0][0] + r[1][1] + r[2][2];

    if (trace > 0.0f)
    {
        s = sqrtf(trace + 1.0f);
        q[3] = s * 0.5f;
        s = 0.5f / s;
        q[0] = (r[1][2] - r[2][1]) * s;
        q[1] = (r[2][0] - r[0][2]) * s;
        q[2] = (r[0][1] - r[1][0]) * s;
    }
    else
    {
        int nxt[3] = {1, 2, 0};
        int i = 0, j = 0, k = 0;

        if (r[1][1] > r[0][0])
            i = 1;

        if (r[2][2] > r[i][i])
            i = 2;

        j = nxt[i];
        k = nxt[j];
        s = sqrtf((r[i][i] - (r[j][j] + r[k][k])) + 1.0f);

        q[i] = s * 0.5f;
        s = 0.5f / s;
        q[3] = (r[j][k] - r[k][j]) * s;
        q[j] = (r[i][j] + r[j][i]) * s;
        q[k] = (r[i][k] + r[k][i]) * s;
    }

    qout[0] = q[0], qout[1] = q[1], qout[2] = q[2], qout[3] = q[3];
}

void NvQuatIdentity(GLfloat r[4])
{
    r[0] = 0.0f;
    r[1] = 0.0f;
    r[2] = 0.0f;
    r[3] = 1.0f;
}

void NvQuatFromAngleAxis(GLfloat r[4], GLfloat radians, const GLfloat axis[3])
{
    GLfloat cosa = cosf(radians * 0.5f);
    GLfloat sina = sinf(radians * 0.5f);

    r[0] = axis[0] * sina;
    r[1] = axis[1] * sina;
    r[2] = axis[2] * sina;
    r[3] = cosa;
}

void NvQuatX(GLfloat r[4], GLfloat radians)
{
    GLfloat cosa = cosf(radians * 0.5f);
    GLfloat sina = sinf(radians * 0.5f);

    r[0] = sina;
    r[1] = 0.0f;
    r[2] = 0.0f;
    r[3] = cosa;
}

void NvQuatY(GLfloat r[4], GLfloat radians)
{
    GLfloat cosa = cosf(radians * 0.5f);
    GLfloat sina = sinf(radians * 0.5f);

    r[0] = 0.0f;
    r[1] = sina;
    r[2] = 0.0f;
    r[3] = cosa;
}

void NvQuatZ(GLfloat r[4], GLfloat radians)
{
    GLfloat cosa = cosf(radians * 0.5f);
    GLfloat sina = sinf(radians * 0.5f);

    r[0] = 0.0f;
    r[1] = 0.0f;
    r[2] = sina;
    r[3] = cosa;
}

void NvQuatFromEuler(GLfloat r[4], GLfloat heading,
                     GLfloat pitch, GLfloat roll)
{
    GLfloat h[4], p[4], ro[4];

    NvQuatY(h, heading);
    NvQuatX(p, pitch);
    NvQuatZ(ro, roll);

    NvQuatMult(r, h, p);
    NvQuatMult(r, ro, r);
}

void NvQuatFromEulerReverse(GLfloat r[4], GLfloat heading,
                     GLfloat pitch, GLfloat roll)
{
    GLfloat h[4], p[4], ro[4];

    NvQuatZ(ro, roll);
    NvQuatX(p, pitch);
    NvQuatY(h, heading);

    NvQuatMult(r, p, h);
    NvQuatMult(r, r, ro);
}

void NvQuatToEuler(GLfloat r[4], GLfloat& heading, GLfloat& pitch, GLfloat& roll) 
{ 
    heading = (float)atan2(2 * r[1] * r[3] - 2 * r[0] * r[2], 1 - 2 * pow(r[1], 2) - 2 * pow(r[2], 2)); 
    pitch = (float)asin(2 * r[0] * r[1] + 2 * r[2] * r[3]); 
    roll = (float)atan2( 2 * r[0] * r[3] - 2 * r[1] * r[2], 1 - 2 * pow(r[0], 2) - 2 * pow(r[0], 2)); 
 
    if ((r[0] * r[1] + r[2] * r[3]) == 0.5) 
    { 
        heading = (float)(2 * atan2(r[0], r[3])); 
        pitch = 0; 
    } 
 
    else if ((r[0] * r[1] + r[2] * r[3]) == -0.5) 
    { 
        heading = (float)(-2 * atan2(r[0], r[3])); 
        pitch = 0; 
    } 
}  

GLfloat NvQuatDot(const GLfloat q1[4], const GLfloat q2[4])
{
    return q1[0] * q2[0] + q1[1] * q2[1] + q1[2] * q2[2] + q1[3] * q2[3];
}

void NvQuatMult(GLfloat r[4], const GLfloat q1[4], const GLfloat q2[4])
{
    const GLfloat q1x = q1[0];
    const GLfloat q1y = q1[1];
    const GLfloat q1z = q1[2];
    const GLfloat q1w = q1[3];
    const GLfloat q2x = q2[0];
    const GLfloat q2y = q2[1];
    const GLfloat q2z = q2[2];
    const GLfloat q2w = q2[3];

    r[0] = q1y * q2z - q2y * q1z + q1w * q2x + q2w * q1x;
    r[1] = q1z * q2x - q2z * q1x + q1w * q2y + q2w * q1y;
    r[2] = q1x * q2y - q2x * q1y + q1w * q2z + q2w * q1z;
    r[3] = q1w * q2w - q1x * q2x - q1y * q2y - q1z * q2z;
}

void NvQuatNLerp(GLfloat r[4], const GLfloat q1[4], const GLfloat q2[4], GLfloat t)
{
    GLfloat omt = 1.0f - t;

    if (NvQuatDot(q1, q2) < 0.0f)
    {
        r[0] = -q1[0] * omt + q2[0] * t;
        r[1] = -q1[1] * omt + q2[1] * t;
        r[2] = -q1[2] * omt + q2[2] * t;
        r[3] = -q1[3] * omt + q2[3] * t;
    }
    else
    {
        r[0] = q1[0] * omt + q2[0] * t;
        r[1] = q1[1] * omt + q2[1] * t;
        r[2] = q1[2] * omt + q2[2] * t;
        r[3] = q1[3] * omt + q2[3] * t;
    }

    NvQuatNormalize(r, r);
}

void NvQuatNormalize(GLfloat r[4], const GLfloat q[4])
{
    GLfloat invLength = 1.0f / sqrtf(NvQuatDot(q, q));

    r[0] = invLength * q[0];
    r[1] = invLength * q[1];
    r[2] = invLength * q[2];
    r[3] = invLength * q[3];
}
