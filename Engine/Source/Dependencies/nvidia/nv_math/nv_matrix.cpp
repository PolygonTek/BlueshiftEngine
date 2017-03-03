//----------------------------------------------------------------------------------
// File:        jni/nv_math/nv_matrix.cpp
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
#include "nv_matrix.h"

int NvDifferentMatsf(GLfloat a[4][4], const GLfloat b[4][4])
{
    return ((&a[3][3]<&b[0][0]) || (&b[3][3]<&a[0][0]));
}


void NvBuildMatf(GLfloat r[4][4], const GLfloat* m)
{
    r[0][0] = m[0];
    r[0][1] = m[1];
    r[0][2] = m[2];
    r[0][3] = m[3];

    r[1][0] = m[4];
    r[1][1] = m[5];
    r[1][2] = m[6];
    r[1][3] = m[7];

    r[2][0] = m[8];
    r[2][1] = m[9];
    r[2][2] = m[10];
    r[2][3] = m[11];

    r[3][0] = m[12];
    r[3][1] = m[13];
    r[3][2] = m[14];
    r[3][3] = m[15];
}

void NvCopyMatf(GLfloat r[4][4], const GLfloat m[4][4])
{
    r[0][0] = m[0][0];
    r[0][1] = m[0][1];
    r[0][2] = m[0][2];
    r[0][3] = m[0][3];

    r[1][0] = m[1][0];
    r[1][1] = m[1][1];
    r[1][2] = m[1][2];
    r[1][3] = m[1][3];

    r[2][0] = m[2][0];
    r[2][1] = m[2][1];
    r[2][2] = m[2][2];
    r[2][3] = m[2][3];

    r[3][0] = m[3][0];
    r[3][1] = m[3][1];
    r[3][2] = m[3][2];
    r[3][3] = m[3][3];
}

void NvExtract3x3Matf(GLfloat r[3][3], const GLfloat m[4][4])
{
    r[0][0] = m[0][0];
    r[0][1] = m[0][1];
    r[0][2] = m[0][2];

    r[1][0] = m[1][0];
    r[1][1] = m[1][1];
    r[1][2] = m[1][2];

    r[2][0] = m[2][0];
    r[2][1] = m[2][1];
    r[2][2] = m[2][2];
}


GLfloat NvVecLengthf(const GLfloat v[3])
{
    return sqrtf(v[0] * v[0] +
            v[1] * v[1] +
            v[2] * v[2]);
}


void NvNormalizeVecf(GLfloat r[3], const GLfloat v[3])
{
    GLfloat const l = NvVecLengthf(v);
    r[0] = v[0] / l;
    r[1] = v[1] / l;
    r[2] = v[2] / l;
}


void NvAddVecf(GLfloat r[3], const GLfloat a[3], const GLfloat b[3])
{
    r[0] = a[0] + b[0];
    r[1] = a[1] + b[1];
    r[2] = a[2] + b[2];
}


void NvSubVecf(GLfloat r[3], const GLfloat a[3], const GLfloat b[3])
{
    r[0] = a[0] - b[0];
    r[1] = a[1] - b[1];
    r[2] = a[2] - b[2];
}


void NvCrossProductf(GLfloat r[3], const GLfloat a[3], const GLfloat b[3])
{
    GLfloat t[3];

    t[0] = a[1]*b[2] - a[2]*b[1];
    t[1] = a[2]*b[0] - a[0]*b[2];
    t[2] = a[0]*b[1] - a[1]*b[0];

    r[0] = t[0];
    r[1] = t[1];
    r[2] = t[2];
}

void NvTransformPointf(GLfloat r[3], const GLfloat m[4][4], const GLfloat a[3])
{
    GLfloat inv_w = 1.0f / (m[0][3]*a[0] + m[1][3]*a[1] + m[2][3]*a[2] + m[3][3]);

    r[0] = m[0][0] * a[0] + m[1][0] * a[1] + m[2][0] * a[2] + m[3][0] * inv_w;
    r[1] = m[0][1] * a[0] + m[1][1] * a[1] + m[2][1] * a[2] + m[3][1] * inv_w;
    r[2] = m[0][2] * a[0] + m[1][2] * a[1] + m[2][2] * a[2] + m[3][2] * inv_w;
}

void NvTransformHomPointf(GLfloat r[4], const GLfloat m[4][4], const GLfloat a[4])
{
    r[0] = m[0][0] * a[0] + m[1][0] * a[1] + m[2][0] * a[2] + m[3][0] * a[3];
    r[1] = m[0][1] * a[0] + m[1][1] * a[1] + m[2][1] * a[2] + m[3][1] * a[3];
    r[2] = m[0][2] * a[0] + m[1][2] * a[1] + m[2][2] * a[2] + m[3][2] * a[3];
    r[3] = m[0][3] * a[0] + m[1][3] * a[1] + m[2][3] * a[2] + m[3][3] * a[3];
}

void NvTransformVecf(GLfloat r[3], const GLfloat m[4][4], const GLfloat a[3])
{
    r[0] = m[0][0] * a[0] + m[1][0] * a[1] + m[2][0] * a[2];
    r[1] = m[0][1] * a[0] + m[1][1] * a[1] + m[2][1] * a[2];
    r[2] = m[0][2] * a[0] + m[1][2] * a[1] + m[2][2] * a[2];
}

static void NvMultMat3x3f(GLfloat r[4][4], const GLfloat a[4][4], const GLfloat b[4][4])
{
    assert(NvDifferentMatsf(r, a) && NvDifferentMatsf(r, b));

    r[0][0] = a[0][0]*b[0][0] + a[1][0]*b[0][1] + a[2][0]*b[0][2];
    r[0][1] = a[0][1]*b[0][0] + a[1][1]*b[0][1] + a[2][1]*b[0][2];
    r[0][2] = a[0][2]*b[0][0] + a[1][2]*b[0][1] + a[2][2]*b[0][2];
    r[0][3] = 0.0f;

    r[1][0] = a[0][0]*b[1][0] + a[1][0]*b[1][1] + a[2][0]*b[1][2];
    r[1][1] = a[0][1]*b[1][0] + a[1][1]*b[1][1] + a[2][1]*b[1][2];
    r[1][2] = a[0][2]*b[1][0] + a[1][2]*b[1][1] + a[2][2]*b[1][2];
    r[1][3] = 0.0f;

    r[2][0] = a[0][0]*b[2][0] + a[1][0]*b[2][1] + a[2][0]*b[2][2];
    r[2][1] = a[0][1]*b[2][0] + a[1][1]*b[2][1] + a[2][1]*b[2][2];
    r[2][2] = a[0][2]*b[2][0] + a[1][2]*b[2][1] + a[2][2]*b[2][2];
    r[2][3] = 0.0f;

    r[3][0] = 0.0f;
    r[3][1] = 0.0f;
    r[3][2] = 0.0f;
    r[3][3] = 1.0f;
}


static void NvMultMat4x3f(GLfloat r[4][4], const GLfloat a[4][4], const GLfloat b[4][4])
{
    assert(NvDifferentMatsf(r, a) && NvDifferentMatsf(r, b));

    r[0][0] = a[0][0]*b[0][0] + a[1][0]*b[0][1] + a[2][0]*b[0][2];
    r[0][1] = a[0][1]*b[0][0] + a[1][1]*b[0][1] + a[2][1]*b[0][2];
    r[0][2] = a[0][2]*b[0][0] + a[1][2]*b[0][1] + a[2][2]*b[0][2];
    r[0][3] = 0.0f;

    r[1][0] = a[0][0]*b[1][0] + a[1][0]*b[1][1] + a[2][0]*b[1][2];
    r[1][1] = a[0][1]*b[1][0] + a[1][1]*b[1][1] + a[2][1]*b[1][2];
    r[1][2] = a[0][2]*b[1][0] + a[1][2]*b[1][1] + a[2][2]*b[1][2];
    r[1][3] = 0.0f;

    r[2][0] = a[0][0]*b[2][0] + a[1][0]*b[2][1] + a[2][0]*b[2][2];
    r[2][1] = a[0][1]*b[2][0] + a[1][1]*b[2][1] + a[2][1]*b[2][2];
    r[2][2] = a[0][2]*b[2][0] + a[1][2]*b[2][1] + a[2][2]*b[2][2];
    r[2][3] = 0.0f;

    r[3][0] = a[0][0]*b[3][0] + a[1][0]*b[3][1] + a[2][0]*b[3][2] + a[3][0];
    r[3][1] = a[0][1]*b[3][0] + a[1][1]*b[3][1] + a[2][1]*b[3][2] + a[3][1];
    r[3][2] = a[0][2]*b[3][0] + a[1][2]*b[3][1] + a[2][2]*b[3][2] + a[3][2];
    r[3][3] = 1.0f;
}


static void NvMultMat4x4f(GLfloat r[4][4], const GLfloat a[4][4], const GLfloat b[4][4])
{
    assert(NvDifferentMatsf(r, a) && NvDifferentMatsf(r, b));

    r[0][0] = a[0][0]*b[0][0]+a[1][0]*b[0][1]+a[2][0]*b[0][2]+a[3][0]*b[0][3];
    r[0][1] = a[0][1]*b[0][0]+a[1][1]*b[0][1]+a[2][1]*b[0][2]+a[3][1]*b[0][3];
    r[0][2] = a[0][2]*b[0][0]+a[1][2]*b[0][1]+a[2][2]*b[0][2]+a[3][2]*b[0][3];
    r[0][3] = a[0][3]*b[0][0]+a[1][3]*b[0][1]+a[2][3]*b[0][2]+a[3][3]*b[0][3];

    r[1][0] = a[0][0]*b[1][0]+a[1][0]*b[1][1]+a[2][0]*b[1][2]+a[3][0]*b[1][3];
    r[1][1] = a[0][1]*b[1][0]+a[1][1]*b[1][1]+a[2][1]*b[1][2]+a[3][1]*b[1][3];
    r[1][2] = a[0][2]*b[1][0]+a[1][2]*b[1][1]+a[2][2]*b[1][2]+a[3][2]*b[1][3];
    r[1][3] = a[0][3]*b[1][0]+a[1][3]*b[1][1]+a[2][3]*b[1][2]+a[3][3]*b[1][3];

    r[2][0] = a[0][0]*b[2][0]+a[1][0]*b[2][1]+a[2][0]*b[2][2]+a[3][0]*b[2][3];
    r[2][1] = a[0][1]*b[2][0]+a[1][1]*b[2][1]+a[2][1]*b[2][2]+a[3][1]*b[2][3];
    r[2][2] = a[0][2]*b[2][0]+a[1][2]*b[2][1]+a[2][2]*b[2][2]+a[3][2]*b[2][3];
    r[2][3] = a[0][3]*b[2][0]+a[1][3]*b[2][1]+a[2][3]*b[2][2]+a[3][3]*b[2][3];

    r[3][0] = a[0][0]*b[3][0]+a[1][0]*b[3][1]+a[2][0]*b[3][2]+a[3][0]*b[3][3];
    r[3][1] = a[0][1]*b[3][0]+a[1][1]*b[3][1]+a[2][1]*b[3][2]+a[3][1]*b[3][3];
    r[3][2] = a[0][2]*b[3][0]+a[1][2]*b[3][1]+a[2][2]*b[3][2]+a[3][2]*b[3][3];
    r[3][3] = a[0][3]*b[3][0]+a[1][3]*b[3][1]+a[2][3]*b[3][2]+a[3][3]*b[3][3];
}



void NvMultMatf(GLfloat result[4][4], const GLfloat a[4][4], const GLfloat b[4][4])
{
    /*
      Use a temporary matrix for the result and copy the temporary
      into the result when finished. Doing this instead of writing
      the result directly guarantees that the routine will work even
      if the result overlaps one or more of the arguments in
      memory.
    */

    GLfloat r[4][4];

    if ((a[0][3]==0.0f) &&
        (a[1][3]==0.0f) &&
        (a[2][3]==0.0f) &&
        (a[2][3]==1.0f) &&
        (b[0][3]==0.0f) &&
        (b[1][3]==0.0f) &&
        (b[2][3]==0.0f) &&
        (b[2][3]==1.0f))
    {
        if ((a[3][0]==0.0f) &&
            (a[3][1]==0.0f) &&
            (a[3][2]==0.0f) &&
            (b[3][0]==0.0f) &&
            (b[3][1]==0.0f) &&
            (b[3][2]==0.0f))
        {
            NvMultMat3x3f(r, a, b);
        }
        else
        {
            NvMultMat4x3f(r, a, b);
        }
    }
    else
    {
         NvMultMat4x4f(r, a, b);
    }

    NvCopyMatf(result, r);
}


static void NvInvMat3x3f(GLfloat r[4][4], const GLfloat m[4][4])
{
    GLfloat d =
         m[0][0]*m[1][1]*m[2][2] +
         m[0][1]*m[1][2]*m[2][0] +
         m[0][2]*m[1][0]*m[2][1] +
        -m[0][0]*m[1][2]*m[2][1] +
        -m[0][1]*m[1][0]*m[2][2] +
        -m[0][2]*m[1][1]*m[2][0];

    assert(NvDifferentMatsf(r, m));

    r[0][0] = (m[1][1]*m[2][2]-m[1][2]*m[2][1]) / d;
    r[0][1] = (m[0][2]*m[2][1]-m[0][1]*m[2][2]) / d;
    r[0][2] = (m[0][1]*m[1][2]-m[0][2]*m[1][1]) / d;
    r[0][3] = 0.0f;

    r[1][0] = (m[1][2]*m[2][0]-m[1][0]*m[2][2]) / d;
    r[1][1] = (m[0][0]*m[2][2]-m[0][2]*m[2][0]) / d;
    r[1][2] = (m[0][2]*m[1][0]-m[0][0]*m[1][2]) / d;
    r[1][3] = 0.0f;

    r[2][0] = (m[1][0]*m[2][1]-m[1][1]*m[2][0]) / d;
    r[2][1] = (m[0][1]*m[2][0]-m[0][0]*m[2][1]) / d;
    r[2][2] = (m[0][0]*m[1][1]-m[0][1]*m[1][0]) / d;
    r[2][3] = 0.0f;

    r[3][0] = 0.0f;
    r[3][1] = 0.0f;
    r[3][2] = 0.0f;
    r[3][3] = 1.0f;
}


static void NvInvMat4x3f(GLfloat r[4][4], const GLfloat m[4][4])
{
    GLfloat d =
         m[0][0]*m[1][1]*m[2][2] +
         m[0][1]*m[1][2]*m[2][0] +
         m[0][2]*m[1][0]*m[2][1] +
        -m[0][0]*m[1][2]*m[2][1] +
        -m[0][1]*m[1][0]*m[2][2] +
        -m[0][2]*m[1][1]*m[2][0];

    assert(NvDifferentMatsf(r, m));

    r[0][0] = (m[1][1]*m[2][2]-m[1][2]*m[2][1]) / d;
    r[0][1] = (m[0][2]*m[2][1]-m[0][1]*m[2][2]) / d;
    r[0][2] = (m[0][1]*m[1][2]-m[0][2]*m[1][1]) / d;
    r[0][3] = 0.0f;

    r[1][0] = (m[1][2]*m[2][0]-m[1][0]*m[2][2]) / d;
    r[1][1] = (m[0][0]*m[2][2]-m[0][2]*m[2][0]) / d;
    r[1][2] = (m[0][2]*m[1][0]-m[0][0]*m[1][2]) / d;
    r[1][3] = 0.0f;

    r[2][0] = (m[1][0]*m[2][1]-m[1][1]*m[2][0]) / d;
    r[2][1] = (m[0][1]*m[2][0]-m[0][0]*m[2][1]) / d;
    r[2][2] = (m[0][0]*m[1][1]-m[0][1]*m[1][0]) / d;
    r[2][3] = 0.0f;

    /* x */

    r[3][0] = ( m[1][0]*m[2][2]*m[3][1] +
                m[1][1]*m[2][0]*m[3][2] +
                m[1][2]*m[2][1]*m[3][0] +
               -m[1][0]*m[2][1]*m[3][2] +
               -m[1][1]*m[2][2]*m[3][0] +
               -m[1][2]*m[2][0]*m[3][1]) / d;

    /* y */

    r[3][1] = ( m[0][0]*m[2][1]*m[3][2] +
                m[0][1]*m[2][2]*m[3][0] +
                m[0][2]*m[2][0]*m[3][1] +
               -m[0][0]*m[2][2]*m[3][1] +
               -m[0][1]*m[2][0]*m[3][2] +
               -m[0][2]*m[2][1]*m[3][0]) / d;

    /* z */

    r[3][2] = ( m[0][0]*m[1][2]*m[3][1] +
                m[0][1]*m[1][0]*m[3][2] +
                m[0][2]*m[1][1]*m[3][0] +
               -m[0][0]*m[1][1]*m[3][2] +
               -m[0][1]*m[1][2]*m[3][0] +
               -m[0][2]*m[1][0]*m[3][1]) / d;

    r[3][3] = 1.0f;
}


static void NvInvMat4x4f(GLfloat r[4][4], const GLfloat m[4][4])
{
    GLfloat d =
         m[0][0] * m[1][1] * m[2][2] * m[3][3] +
         m[0][0] * m[1][2] * m[2][3] * m[3][1] +
         m[0][0] * m[1][3] * m[2][1] * m[3][2] +
         m[0][1] * m[1][0] * m[2][3] * m[3][2] +
         m[0][1] * m[1][2] * m[2][0] * m[3][3] +
         m[0][1] * m[1][3] * m[2][2] * m[3][0] +
         m[0][2] * m[1][0] * m[2][1] * m[3][3] +
         m[0][2] * m[1][1] * m[2][3] * m[3][0] +
         m[0][2] * m[1][3] * m[2][0] * m[3][1] +
         m[0][3] * m[1][0] * m[2][2] * m[3][1] +
         m[0][3] * m[1][1] * m[2][0] * m[3][2] +
         m[0][3] * m[1][2] * m[2][1] * m[3][0] +
        -m[0][0] * m[1][1] * m[2][3] * m[3][2] +
        -m[0][0] * m[1][2] * m[2][1] * m[3][3] +
        -m[0][0] * m[1][3] * m[2][2] * m[3][1] +
        -m[0][1] * m[1][0] * m[2][2] * m[3][3] +
        -m[0][1] * m[1][2] * m[2][3] * m[3][0] +
        -m[0][1] * m[1][3] * m[2][0] * m[3][2] +
        -m[0][2] * m[1][0] * m[2][3] * m[3][1] +
        -m[0][2] * m[1][1] * m[2][0] * m[3][3] +
        -m[0][2] * m[1][3] * m[2][1] * m[3][0] +
        -m[0][3] * m[1][0] * m[2][1] * m[3][2] +
        -m[0][3] * m[1][1] * m[2][2] * m[3][0] +
        -m[0][3] * m[1][2] * m[2][0] * m[3][1];

    assert(NvDifferentMatsf(r, m));

    r[0][0] = ( m[1][1] * m[2][2] * m[3][3] +
                m[1][2] * m[2][3] * m[3][1] +
                m[1][3] * m[2][1] * m[3][2] +
               -m[1][1] * m[2][3] * m[3][2] +
               -m[1][2] * m[2][1] * m[3][3] +
               -m[1][3] * m[2][2] * m[3][1]) / d;

    r[0][1] = ( m[0][1] * m[2][3] * m[3][2] +
                m[0][2] * m[2][1] * m[3][3] +
                m[0][3] * m[2][2] * m[3][1] +
               -m[0][1] * m[2][2] * m[3][3] +
               -m[0][2] * m[2][3] * m[3][1] +
               -m[0][3] * m[2][1] * m[3][2]) / d;

    r[0][2] = ( m[0][1] * m[1][2] * m[3][3] +
                m[0][2] * m[1][3] * m[3][1] +
                m[0][3] * m[1][1] * m[3][2] +
               -m[0][1] * m[1][3] * m[3][2] +
               -m[0][2] * m[1][1] * m[3][3] +
               -m[0][3] * m[1][2] * m[3][1]) / d;

    r[0][3] = ( m[0][1] * m[1][3] * m[2][2] +
                m[0][2] * m[1][1] * m[2][3] +
                m[0][3] * m[1][2] * m[2][1] +
               -m[0][1] * m[1][2] * m[2][3] +
               -m[0][2] * m[1][3] * m[2][1] +
               -m[0][3] * m[1][1] * m[2][2]) / d;

    r[1][0] = ( m[1][0] * m[2][3] * m[3][2] +
                m[1][2] * m[2][0] * m[3][3] +
                m[1][3] * m[2][2] * m[3][0] +
               -m[1][0] * m[2][2] * m[3][3] +
               -m[1][2] * m[2][3] * m[3][0] +
               -m[1][3] * m[2][0] * m[3][2] ) / d;

    r[1][1] = ( m[0][0] * m[2][2] * m[3][3] +
                m[0][2] * m[2][3] * m[3][0] +
                m[0][3] * m[2][0] * m[3][2] +
               -m[0][0] * m[2][3] * m[3][2] +
               -m[0][2] * m[2][0] * m[3][3] +
               -m[0][3] * m[2][2] * m[3][0]) / d;

    r[1][2] = ( m[0][0] * m[1][3] * m[3][2] +
                m[0][2] * m[1][0] * m[3][3] +
                m[0][3] * m[1][2] * m[3][0] +
               -m[0][0] * m[1][2] * m[3][3] +
               -m[0][2] * m[1][3] * m[3][0] +
               -m[0][3] * m[1][0] * m[3][2]) / d;

    r[1][3] = ( m[0][0] * m[1][2] * m[2][3] +
                m[0][2] * m[1][3] * m[2][0] +
                m[0][3] * m[1][0] * m[2][2] +
               -m[0][0] * m[1][3] * m[2][2] +
               -m[0][2] * m[1][0] * m[2][3] +
               -m[0][3] * m[1][2] * m[2][0]) / d;

    r[2][0] = ( m[1][0] * m[2][1] * m[3][3] +
                m[1][1] * m[2][3] * m[3][0] +
                m[1][3] * m[2][0] * m[3][1] +
               -m[1][0] * m[2][3] * m[3][1] +
               -m[1][1] * m[2][0] * m[3][3] +
               -m[1][3] * m[2][1] * m[3][0]) / d;

    r[2][1] = (-m[0][0] * m[2][1] * m[3][3] +
               -m[0][1] * m[2][3] * m[3][0] +
               -m[0][3] * m[2][0] * m[3][1] +
                m[0][0] * m[2][3] * m[3][1] +
                m[0][1] * m[2][0] * m[3][3] +
                m[0][3] * m[2][1] * m[3][0]) / d;

    r[2][2] = ( m[0][0] * m[1][1] * m[3][3] +
                m[0][1] * m[1][3] * m[3][0] +
                m[0][3] * m[1][0] * m[3][1] +
               -m[0][0] * m[1][3] * m[3][1] +
               -m[0][1] * m[1][0] * m[3][3] +
               -m[0][3] * m[1][1] * m[3][0]) / d;

    r[2][3] = ( m[0][0] * m[1][3] * m[2][1] +
                m[0][1] * m[1][0] * m[2][3] +
                m[0][3] * m[1][1] * m[2][0] +
               -m[0][1] * m[1][3] * m[2][0] +
               -m[0][3] * m[1][0] * m[2][1] +
               -m[0][0] * m[1][1] * m[2][3]) / d;

    r[3][0] = ( m[1][0] * m[2][2] * m[3][1] +
                m[1][1] * m[2][0] * m[3][2] +
                m[1][2] * m[2][1] * m[3][0] +
               -m[1][0] * m[2][1] * m[3][2] +
               -m[1][1] * m[2][2] * m[3][0] +
               -m[1][2] * m[2][0] * m[3][1]) / d;

    r[3][1] = ( m[0][0] * m[2][1] * m[3][2] +
                m[0][1] * m[2][2] * m[3][0] +
                m[0][2] * m[2][0] * m[3][1] +
               -m[0][0] * m[2][2] * m[3][1] +
               -m[0][1] * m[2][0] * m[3][2] +
               -m[0][2] * m[2][1] * m[3][0]) / d;

    r[3][2] = ( m[0][0] * m[1][2] * m[3][1] +
                m[0][1] * m[1][0] * m[3][2] +
                m[0][2] * m[1][1] * m[3][0] +
               -m[0][0] * m[1][1] * m[3][2] +
               -m[0][1] * m[1][2] * m[3][0] +
               -m[0][2] * m[1][0] * m[3][1]) / d;

    r[3][3] = ( m[0][0] * m[1][1] * m[2][2] +
                m[0][1] * m[1][2] * m[2][0] +
                m[0][2] * m[1][0] * m[2][1] +
               -m[0][0] * m[1][2] * m[2][1] +
               -m[0][1] * m[1][0] * m[2][2] +
               -m[0][2] * m[1][1] * m[2][0]) / d;
}


void NvInvMatf(GLfloat result[4][4], const GLfloat m[4][4])
{
    /*
      Use a temporary matrix for the result and copy the temporary
      into the result when finished. Doing this instead of writing
      the result directly guarantees that the routine will work even
      if the result overlaps one or more of the arguments in
      memory.
    */

    GLfloat r[4][4];

    if ((m[0][3]==0.0f) &&
        (m[1][3]==0.0f) &&
        (m[2][3]==0.0f) &&
        (m[2][3]==1.0f))
    {
        if ((m[3][0]==0.0f) &&
            (m[3][1]==0.0f) &&
            (m[3][2]==0.0f))
        {
            NvInvMat3x3f(r, m);
        }
        else
        {
            NvInvMat4x3f(r, m);
        }
    }
    else
    {
        NvInvMat4x4f(r, m);
    }

    NvCopyMatf(result, r);
}


void NvBuildIdentityMatf(GLfloat r[4][4])
{
    r[0][0] = 1.0f;
    r[0][1] = 0.0f;
    r[0][2] = 0.0f;
    r[0][3] = 0.0f;

    r[1][0] = 0.0f;
    r[1][1] = 1.0f;
    r[1][2] = 0.0f;
    r[1][3] = 0.0f;

    r[2][0] = 0.0f;
    r[2][1] = 0.0f;
    r[2][2] = 1.0f;
    r[2][3] = 0.0f;

    r[3][0] = 0.0f;
    r[3][1] = 0.0f;
    r[3][2] = 0.0f;
    r[3][3] = 1.0f;
}


void NvBuildTranslateMatf(GLfloat r[4][4], GLfloat x, GLfloat y, GLfloat z)
{
    r[0][0] = 1.0f;
    r[0][1] = 0.0f;
    r[0][2] = 0.0f;
    r[0][3] = 0.0f;

    r[1][0] = 0.0f;
    r[1][1] = 1.0f;
    r[1][2] = 0.0f;
    r[1][3] = 0.0f;

    r[2][0] = 0.0f;
    r[2][1] = 0.0f;
    r[2][2] = 1.0f;
    r[2][3] = 0.0f;

    r[3][0] = x;
    r[3][1] = y;
    r[3][2] = z;
    r[3][3] = 1.0f;
}


void NvMultTranslateMatf(GLfloat result[4][4],
                         const GLfloat m[4][4],
                         GLfloat x,
                         GLfloat y,
                         GLfloat z)
{
    GLfloat r[4][4]; /*temporary storage for result */

    r[0][0] = m[0][0];
    r[0][1] = m[0][1];
    r[0][2] = m[0][2];
    r[0][3] = m[0][3];

    r[1][0] = m[1][0];
    r[1][1] = m[1][1];
    r[1][2] = m[1][2];
    r[1][3] = m[1][3];

    r[2][0] = m[2][0];
    r[2][1] = m[2][1];
    r[2][2] = m[2][2];
    r[2][3] = m[2][3];

    r[3][0] = m[0][0] * x + m[1][0] * y + m[2][0] * z + m[3][0];
    r[3][1] = m[0][1] * x + m[1][1] * y + m[2][1] * z + m[3][1];
    r[3][2] = m[0][2] * x + m[1][2] * y + m[2][2] * z + m[3][2];
    r[3][3] = m[0][3] * x + m[1][3] * y + m[2][3] * z + m[3][3];

    NvCopyMatf(result, r);
}


void NvBuildScaleMatf(GLfloat r[4][4], GLfloat x, GLfloat y, GLfloat z)
{
    r[0][0] = x;
    r[0][1] = 0.0f;
    r[0][2] = 0.0f;
    r[0][3] = 0.0f;

    r[1][0] = 0.0f;
    r[1][1] = y;
    r[1][2] = 0.0f;
    r[1][3] = 0.0f;

    r[2][0] = 0.0f;
    r[2][1] = 0.0f;
    r[2][2] = z;
    r[2][3] = 0.0f;

    r[3][0] = 0.0f;
    r[3][1] = 0.0f;
    r[3][2] = 0.0f;
    r[3][3] = 1.0f;
}


void NvMultScaleMatf(GLfloat result[4][4],
                     const GLfloat m[4][4],
                     GLfloat x,
                     GLfloat y,
                     GLfloat z)
{

    GLfloat r[4][4];


    r[0][0] = m[0][0] * x;
    r[0][1] = m[0][1] * x;
    r[0][2] = m[0][2] * x;
    r[0][3] = m[0][3] * x;

    r[1][0] = m[1][0] * y;
    r[1][1] = m[1][1] * y;
    r[1][2] = m[1][2] * y;
    r[1][3] = m[1][3] * y;

    r[2][0] = m[2][0] * z;
    r[2][1] = m[2][1] * z;
    r[2][2] = m[2][2] * z;
    r[2][3] = m[2][3] * z;

    r[3][0] = m[3][0];
    r[3][1] = m[3][1];
    r[3][2] = m[3][2];
    r[3][3] = m[3][3];

    NvCopyMatf(result, r);
}


void NvBuildRotXRadMatf(GLfloat r[4][4], GLfloat radians)
{
    GLfloat const s = sinf(radians);
    GLfloat const c = cosf(radians);

    r[0][0] = 1.0f;
    r[0][1] = 0.0f;
    r[0][2] = 0.0f;
    r[0][3] = 0.0f;

    r[1][0] = 0.0f;
    r[1][1] = c;
    r[1][2] = s;
    r[1][3] = 0.0f;

    r[2][0] = 0.0f;
    r[2][1] = -s;
    r[2][2] = c;
    r[2][3] = 0.0f;

    r[3][0] = 0.0f;
    r[3][1] = 0.0f;
    r[3][2] = 0.0f;
    r[3][3] = 1.0f;
}


void NvBuildRotYRadMatf(GLfloat r[4][4], GLfloat radians) {

    GLfloat const s = sinf(radians);
    GLfloat const c = cosf(radians);

    r[0][0] = c;
    r[0][1] = 0.0f;
    r[0][2] = -s;
    r[0][3] = 0.0f;

    r[1][0] = 0.0f;
    r[1][1] = 1.0f;
    r[1][2] = 0.0f;
    r[1][3] = 0.0f;

    r[2][0] = s;
    r[2][1] = 0.0f;
    r[2][2] = c;
    r[2][3] = 0.0f;

    r[3][0] = 0.0f;
    r[3][1] = 0.0f;
    r[3][2] = 0.0f;
    r[3][3] = 1.0f;

}


void NvBuildRotZRadMatf(GLfloat r[4][4], GLfloat radians)
{

    GLfloat const s = sinf(radians);
    GLfloat const c = cosf(radians);

    r[0][0] = c;
    r[0][1] = s;
    r[0][2] = 0.0f;
    r[0][3] = 0.0f;

    r[1][0] = -s;
    r[1][1] = c;
    r[1][2] = 0.0f;
    r[1][3] = 0.0f;

    r[2][0] = 0.0f;
    r[2][1] = 0.0f;
    r[2][2] = 1.0f;
    r[2][3] = 0.0f;

    r[3][0] = 0.0f;
    r[3][1] = 0.0f;
    r[3][2] = 0.0f;
    r[3][3] = 1.0f;
}


void NvMultRotXRadMatf(GLfloat result[4][4],
                       const GLfloat m[4][4],
                       GLfloat radians)
{

    GLfloat const s = sinf(radians);
    GLfloat const c = cosf(radians);

    GLfloat r[4][4];

    r[0][0] = m[0][0];
    r[0][1] = m[0][1];
    r[0][2] = m[0][2];
    r[0][3] = m[0][3];

    r[1][0] = m[1][0] *  c + m[2][0] * s;
    r[1][1] = m[1][1] *  c + m[2][1] * s;
    r[1][2] = m[1][2] *  c + m[2][2] * s;
    r[1][3] = m[1][3] *  c + m[2][3] * s;

    r[2][0] = m[1][0] * -s + m[2][0] * c;
    r[2][1] = m[1][1] * -s + m[2][1] * c;
    r[2][2] = m[1][2] * -s + m[2][2] * c;
    r[2][3] = m[1][3] * -s + m[2][3] * c;

    r[3][0] = m[3][0];
    r[3][1] = m[3][1];
    r[3][2] = m[3][2];
    r[3][3] = m[3][3];

    NvCopyMatf(result, r);
}



void NvMultRotYRadMatf(GLfloat result[4][4],
                       const GLfloat m[4][4],
                       GLfloat radians)
{

    GLfloat const s = sinf(radians);
    GLfloat const c = cosf(radians);

    GLfloat r[4][4];

    r[0][0] = m[0][0] * c + m[2][0] * -s;
    r[0][1] = m[0][1] * c + m[2][1] * -s;
    r[0][2] = m[0][2] * c + m[2][2] * -s;
    r[0][3] = m[0][3] * c + m[2][3] * -s;

    r[1][0] = m[1][0];
    r[1][1] = m[1][1];
    r[1][2] = m[1][2];
    r[1][3] = m[1][3];

    r[2][0] = m[0][0] * s + m[2][0] * c;
    r[2][1] = m[0][1] * s + m[2][1] * c;
    r[2][2] = m[0][2] * s + m[2][2] * c;
    r[2][3] = m[0][3] * s + m[2][3] * c;

    r[3][0] = m[3][0];
    r[3][1] = m[3][1];
    r[3][2] = m[3][2];
    r[3][3] = m[3][3];

    NvCopyMatf(result, r);
}

void NvMultRotZRadMatf(GLfloat result[4][4],
                       const GLfloat m[4][4],
                       GLfloat radians)
{

    GLfloat const s = sinf(radians);
    GLfloat const c = cosf(radians);

    GLfloat r[4][4];

    r[0][0] = m[0][0] * c + m[1][0] * s;
    r[0][1] = m[0][1] * c + m[1][1] * s;
    r[0][2] = m[0][2] * c + m[1][2] * s;
    r[0][3] = m[0][3] * c + m[1][3] * s;

    r[1][0] = m[0][0] * -s + m[1][0] * c;
    r[1][1] = m[0][1] * -s + m[1][1] * c;
    r[1][2] = m[0][2] * -s + m[1][2] * c;
    r[1][3] = m[0][3] * -s + m[1][3] * c;

    r[2][0] = m[2][0];
    r[2][1] = m[2][1];
    r[2][2] = m[2][2];
    r[2][3] = m[2][3];

    r[3][0] = m[3][0];
    r[3][1] = m[3][1];
    r[3][2] = m[3][2];
    r[3][3] = m[3][3];

    NvCopyMatf(result, r);
}

void NvBuildRotRadMatf(GLfloat result[4][4],
                       const GLfloat axis[3],
                       GLfloat radians)
{
    /* build a quat first */

    GLfloat i, j, k, r;

    /* should be */
    GLfloat dst_l = sinf(radians/2.0f);

    /* actually is */
    GLfloat const src_l = sqrtf(axis[0] * axis[0] +
                                    axis[1] * axis[1] +
                                    axis[2] * axis[2]);

    if (src_l <= KD_FLT_EPSILON) {
        i = 0.0f;
        j = 0.0f;
        k = 0.0f;
        r = 1.0f;
    } else {
        GLfloat const s = dst_l / src_l;
        i = axis[0] * s;
        j = axis[1] * s;
        k = axis[2] * s;
        r = cosf(radians/2.0f);
    }

    /* build a matrix from the quat */

    result[0][0] = 1.0f - 2.0f * (j * j + k * k);
    result[0][1] = 2.0f * (i * j + r * k);
    result[0][2] = 2.0f * (i * k - r * j);
    result[0][3] = 0.0f;

    result[1][0] = 2.0f * (i * j - r * k);
    result[1][1] = 1.0f - 2.0f * (i * i + k * k);
    result[1][2] = 2.0f * (j * k + r * i);
    result[1][3] = 0.0f;

    result[2][0] = 2.0f * (i * k + r * j);
    result[2][1] = 2.0f * (j * k - r * i);
    result[2][2] = 1.0f - 2.0f * (i * i + j * j);
    result[2][3] = 0.0f;

    result[3][0] = 0.0f;
    result[3][1] = 0.0f;
    result[3][2] = 0.0f;
    result[3][3] = 1.0f;
}


void NvMultRotRadMatf(GLfloat result[4][4],
                      const GLfloat m[4][4],
                      const GLfloat axis[3],
                      GLfloat radians)
{
    GLfloat r[4][4];

    {
        /* build a quat first */

        GLfloat i, j, k, r;

        /* should be */
        GLfloat const dst_l = sinf(radians/2.0f);

        /* actually is */
        GLfloat const src_l = sqrtf(axis[0] * axis[0] +
                                        axis[1] * axis[1] +
                                        axis[2] * axis[2]);

        if (src_l <= KD_FLT_EPSILON) {
            i = 0.0f;
            j = 0.0f;
            k = 0.0f;
            r = 1.0f;
        } else {
            GLfloat const s = dst_l / src_l;
            i = axis[0] * s;
            j = axis[1] * s;
            k = axis[2] * s;
            r = cosf(radians/2.0f);
        }

        {
            /* build a matrix from the quat */

            GLfloat const a00 = 1.0f - 2.0f * (j * j + k * k);
            GLfloat const a01 = 2.0f * (i * j + r * k);
            GLfloat const a02 = 2.0f * (i * k - r * j);

            GLfloat const a10 = 2.0f * (i * j - r * k);
            GLfloat const a11 = 1.0f - 2.0f * (i * i + k * k);
            GLfloat const a12 = 2.0f * (j * k + r * i);

            GLfloat const a20 = 2.0f * (i * k + r * j);
            GLfloat const a21 = 2.0f * (j * k - r * i);
            GLfloat const a22 = 1.0f - 2.0f * (i * i + j * j);

            result[0][0] = m[0][0] * a00 + m[1][0] * a01 + m[2][0] * a02;
            result[0][1] = m[0][1] * a00 + m[1][1] * a01 + m[2][1] * a02;
            result[0][2] = m[0][2] * a00 + m[1][2] * a01 + m[2][2] * a02;
            result[0][3] = m[0][3] * a00 + m[1][3] * a01 + m[2][3] * a02;

            result[1][0] = m[0][0] * a10 + m[1][0] * a11 + m[2][0] * a12;
            result[1][1] = m[0][1] * a10 + m[1][1] * a11 + m[2][1] * a12;
            result[1][2] = m[0][2] * a10 + m[1][2] * a11 + m[2][2] * a12;
            result[1][3] = m[0][3] * a10 + m[1][3] * a11 + m[2][3] * a12;

            result[2][0] = m[0][0] * a20 + m[1][0] * a21 + m[2][0] * a22;
            result[2][1] = m[0][1] * a20 + m[1][1] * a21 + m[2][1] * a22;
            result[2][2] = m[0][2] * a20 + m[1][2] * a21 + m[2][2] * a22;
            result[2][3] = m[0][3] * a20 + m[1][3] * a21 + m[2][3] * a22;

            result[3][0] = m[3][0];
            result[3][1] = m[3][1];
            result[3][2] = m[3][2];
            result[3][3] = m[3][3];
        }
    }

    NvCopyMatf(result, r);
}

void NvBuildRotXDegMatf(GLfloat r[4][4], GLfloat degrees)
{
    NvBuildRotXRadMatf(r, degrees * KD_DEG_TO_RAD_F);
}

void NvBuildRotYDegMatf(GLfloat r[4][4], GLfloat degrees)
{
    NvBuildRotYRadMatf(r, degrees * KD_DEG_TO_RAD_F);
}

void NvBuildRotZDegMatf(GLfloat r[4][4], GLfloat degrees)
{
    NvBuildRotZRadMatf(r, degrees * KD_DEG_TO_RAD_F);
}


void NvMultRotXDegMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat degrees)
{
    NvMultRotXRadMatf(r, m, degrees * KD_DEG_TO_RAD_F);
}

void NvMultRotYDegMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat degrees)
{
    NvMultRotYRadMatf(r, m, degrees * KD_DEG_TO_RAD_F);
}

void NvMultRotZDegMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat degrees)
{
    NvMultRotZRadMatf(r, m, degrees * KD_DEG_TO_RAD_F);
}

void NvBuildRotDegMatf(GLfloat r[4][4], const GLfloat axis[3], GLfloat degrees)
{
    NvBuildRotRadMatf(r, axis, degrees * KD_DEG_TO_RAD_F);
}

void NvMultRotDegMatf(GLfloat r[4][4],
                      const GLfloat m[4][4],
                      const GLfloat axis[3],
                      GLfloat degrees)
{
    NvMultRotRadMatf(r, m, axis, degrees * KD_DEG_TO_RAD_F);
}


void NvBuildLookatMatf(GLfloat r[4][4],
                       const GLfloat eye[3],
                       const GLfloat obj[3],
                       const GLfloat up[3])
{
    GLfloat ev[3];
    GLfloat z[3];
    GLfloat x_tmp[3];
    GLfloat x[3];
    GLfloat y[3];

    NvSubVecf(ev, eye, obj);

    NvNormalizeVecf(z, ev);

    NvCrossProductf(x_tmp, up, z);

    NvNormalizeVecf(x, x_tmp);

    NvCrossProductf(y, z, x);


    r[0][0] = x[0];
    r[0][1] = y[0];
    r[0][2] = z[0];
    r[0][3] = 0.0f;

    r[1][0] = x[1];
    r[1][1] = y[1];
    r[1][2] = z[1];
    r[1][3] = 0.0f;

    r[2][0] = x[2];
    r[2][1] = y[2];
    r[2][2] = z[2];
    r[2][3] = 0.0f;

    r[3][0] = -x[0] * eye[0] + -x[1] * eye[1] + -x[2] * eye[2];
    r[3][1] = -y[0] * eye[0] + -y[1] * eye[1] + -y[2] * eye[2];
    r[3][2] = -z[0] * eye[0] + -z[1] * eye[1] + -z[2] * eye[2];
    r[3][3] = 1.0f;
}


void NvBuildFrustumMatf(GLfloat r[4][4],
                        GLfloat left,
                        GLfloat right,
                        GLfloat bottom,
                        GLfloat top,
                        GLfloat znear,
                        GLfloat zfar)
{
    GLfloat const m00 = znear * 2.0f / (right - left);
    GLfloat const m11 = znear * 2.0f / (top - bottom);
    GLfloat const m22 = -(zfar + znear) / (zfar - znear);

    GLfloat const m20 = (right + left) / (right - left);
    GLfloat const m21 = (top + bottom) / (top - bottom);

    GLfloat const m32 = -(2.0f * zfar * znear) / (zfar - znear);
    GLfloat const m23 = -1.0f;

    r[0][0] = m00;
    r[0][1] = 0.0f;
    r[0][2] = 0.0f;
    r[0][3] = 0.0f;

    r[1][0] = 0.0f;
    r[1][1] = m11;
    r[1][2] = 0.0f;
    r[1][3] = 0.0f;

    r[2][0] = m20;
    r[2][1] = m21;
    r[2][2] = m22;
    r[2][3] = m23;

    r[3][0] = 0.0f;
    r[3][1] = 0.0f;
    r[3][2] = m32;
    r[3][3] = 0.0f;
}


void NvBuildOrtho2Matf(GLfloat r[4][4],
                       GLfloat left,
                       GLfloat right,
                       GLfloat bottom,
                       GLfloat top)
{
    GLfloat const sx = 2.0f / (right - left);
    GLfloat const sy = 2.0f / (top - bottom);

    GLfloat const tx = -(right + left) / (right - left);
    GLfloat const ty = -(top + bottom) / (top - bottom);

    r[0][0] = sx;
    r[0][1] = 0.0f;
    r[0][2] = 0.0f;
    r[0][3] = 0.0f;

    r[1][0] = 0.0f;
    r[1][1] = sy;
    r[1][2] = 0.0f;
    r[1][3] = 0.0f;

    r[2][0] = 0.0f;
    r[2][1] = 0.0f;
    r[2][2] = 1.0f;
    r[2][3] = 0.0f;

    r[3][0] = tx;
    r[3][1] = ty;
    r[3][2] = 0.0f;
    r[3][3] = 1.0f;
}


void NvBuildOrthoMatf(GLfloat r[4][4],
                      GLfloat left,
                      GLfloat right,
                      GLfloat bottom,
                      GLfloat top,
                      GLfloat znear,
                      GLfloat zfar)
{
    GLfloat const sx = 2.0f / (right - left);
    GLfloat const sy = 2.0f / (top - bottom);
    GLfloat const sz = -2.0f / (zfar - znear);

    GLfloat const tx = -(right + left) / (right - left);
    GLfloat const ty = -(top + bottom) / (top - bottom);
    GLfloat const tz = -(zfar + znear) / (zfar - znear);

    r[0][0] = sx;
    r[0][1] = 0.0f;
    r[0][2] = 0.0f;
    r[0][3] = 0.0f;

    r[1][0] = 0.0f;
    r[1][1] = sy;
    r[1][2] = 0.0f;
    r[1][3] = 0.0f;

    r[2][0] = 0.0f;
    r[2][1] = 0.0f;
    r[2][2] = sz;
    r[2][3] = 0.0f;

    r[3][0] = tx;
    r[3][1] = ty;
    r[3][2] = tz;
    r[3][3] = 1.0f;
}

