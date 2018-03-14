//----------------------------------------------------------------------------------
// File:        jni/nv_math/nv_quat.h
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
#ifndef INCLUDED_QUAT_H
#define INCLUDED_QUAT_H

#include <GLES3/gl3.h>
#include <math.h>
#include <assert.h>

void NvQuatCopy(GLfloat r[4], const GLfloat q[4]);
void NvQuatConvertTo3x3Mat(GLfloat r[3][3], const GLfloat q[4]);
void NvQuatConvertTo4x4Mat(GLfloat r[4][4], const GLfloat q[4]);
void NvQuatFrom4x4Mat(GLfloat r[4][4], GLfloat qout[4]);
void NvQuatIdentity(GLfloat r[4]);

void NvQuatFromAngleAxis(GLfloat r[4], GLfloat radians, const GLfloat axis[3]);

void NvQuatX(GLfloat r[4], GLfloat radians);

void NvQuatY(GLfloat r[4], GLfloat radians);

void NvQuatZ(GLfloat r[4], GLfloat radians);

void NvQuatFromEuler(GLfloat r[4], GLfloat heading,
                     GLfloat pitch, GLfloat roll);

void NvQuatFromEulerReverse(GLfloat r[4], GLfloat heading,
                     GLfloat pitch, GLfloat roll);

void NvQuatToEuler(GLfloat r[4], GLfloat& heading,
                     GLfloat& pitch, GLfloat& roll);

GLfloat NvQuatDot(const GLfloat q1[4], const GLfloat q2[4]);

void NvQuatMult(GLfloat r[4], const GLfloat q1[4], const GLfloat q2[4]);

void NvQuatNLerp(GLfloat r[4], const GLfloat q1[4], const GLfloat q2[4], GLfloat t);

void NvQuatNormalize(GLfloat r[4], const GLfloat q[4]);

#endif
