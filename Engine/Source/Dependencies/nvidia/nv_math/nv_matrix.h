//----------------------------------------------------------------------------------
// File:        jni/nv_math/nv_matrix.h
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
#ifndef INCLUDED_MATRIX_H
#define INCLUDED_MATRIX_H

#include <GLES3/gl3.h>
#include <math.h>
#include <assert.h>

#define KD_FLT_EPSILON 1.19209290E-07F
#define KD_DEG_TO_RAD_F 0.0174532924F
#define KD_RAD_TO_DEG_F 57.2957802F

void NvBuildMatf(GLfloat r[4][4], const GLfloat* m);
void NvCopyMatf(GLfloat r[4][4], const GLfloat m[4][4]);
void NvExtract3x3Matf(GLfloat r[3][3], const GLfloat m[4][4]);


/* vector utilities */

GLfloat NvVecLengthf(const GLfloat v[3]);

void NvNormalizeVecf(GLfloat r[3], const GLfloat v[3]);

void NvAddVecf(GLfloat r[3], const GLfloat a[3], const GLfloat b[3]);

void NvSubVecf(GLfloat r[3], const GLfloat a[3], const GLfloat b[3]);

void NvCrossProductf(GLfloat r[3], const GLfloat a[3], const GLfloat b[3]);

void NvTransformPointf(GLfloat r[3], const GLfloat m[4][4], const GLfloat a[3]);
void NvTransformHomPointf(GLfloat r[4], const GLfloat m[4][4], const GLfloat a[4]);
void NvTransformVecf(GLfloat r[3], const GLfloat m[4][4], const GLfloat a[3]);


/* matrix utilities */

void NvMultMatf(GLfloat r[4][4], const GLfloat a[4][4], const GLfloat b[4][4]);

void NvInvMatf(GLfloat r[4][4], const GLfloat m[4][4]);


/* matrix building utilities */

void NvBuildIdentityMatf(GLfloat r[4][4]);

void NvBuildTranslateMatf(GLfloat r[4][4], GLfloat x, GLfloat y, GLfloat z);

void NvBuildScaleMatf(GLfloat r[4][4], GLfloat x, GLfloat y, GLfloat z);

void NvBuildRotXDegMatf(GLfloat r[4][4], GLfloat degrees);
void NvBuildRotYDegMatf(GLfloat r[4][4], GLfloat degrees);
void NvBuildRotZDegMatf(GLfloat r[4][4], GLfloat degrees);

void NvBuildRotDegMatf(GLfloat r[4][4], const GLfloat axis[3], GLfloat degrees);

void NvBuildRotXRadMatf(GLfloat r[4][4], GLfloat radians);
void NvBuildRotYRadMatf(GLfloat r[4][4], GLfloat radians);
void NvBuildRotZRadMatf(GLfloat r[4][4], GLfloat radians);

void NvBuildRotRadMatf(GLfloat r[4][4], const GLfloat axis[3], GLfloat radians);


void NvBuildLookatMatf(GLfloat r[4][4],
                       const GLfloat eye[3],
                       const GLfloat obj[3],
                       const GLfloat up[3]);

void NvBuildFrustumMatf(GLfloat r[4][4],
                        GLfloat left,
                        GLfloat right,
                        GLfloat bottom,
                        GLfloat top,
                        GLfloat znear,
                        GLfloat zfar);

void NvBuildOrtho2Matf(GLfloat r[4][4],
                       GLfloat left,
                       GLfloat right,
                       GLfloat bottom,
                       GLfloat top);

void NvBuildOrthoMatf(GLfloat r[4][4],
                      GLfloat left,
                      GLfloat right,
                      GLfloat bottom,
                      GLfloat top,
                      GLfloat znear,
                      GLfloat zfar);

/* matrix concatenation utilities */

void NvMultTranslateMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat x, GLfloat y, GLfloat z);
void NvMultScaleMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat x, GLfloat y, GLfloat z);

void NvMultRotXDegMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat degrees);
void NvMultRotYDegMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat degrees);
void NvMultRotZDegMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat degrees);

void NvMultRotDegMatf(GLfloat r[4][4], const GLfloat m[4][4], const GLfloat axis[3], GLfloat degrees);

void NvMultRotXRadMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat radians);
void NvMultRotYRadMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat radians);
void NvMultRotZRadMatf(GLfloat r[4][4], const GLfloat m[4][4], GLfloat radians);

void NvMultRotRadMatf(GLfloat r[4][4], const GLfloat m[4][4], const GLfloat axis[3], GLfloat radians);

#endif
