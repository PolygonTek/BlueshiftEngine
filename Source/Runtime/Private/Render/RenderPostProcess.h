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

#pragma once

BE_NAMESPACE_BEGIN

/*
-------------------------------------------------------------------------------

    Post-processing functions

-------------------------------------------------------------------------------
*/

class Texture;
class RenderTarget;

void    PP_Init();
void    PP_Free();

void    PP_PassThruPass(const Texture *srcTexture, float s, float t, float s2, float t2, RenderTarget *dstRT);

void    PP_Downscale2x2(const Texture *srcTexture, RenderTarget *dstRT);
void    PP_Downscale4x4(const Texture *srcTexture, RenderTarget *dstRT);
void    PP_Downscale4x4LogLum(const Texture *srcTexture, float s, float t, float s2, float t2, RenderTarget *dstRT);
void    PP_Downscale4x4ExpLum(const Texture *srcTexture, RenderTarget *dstRT);

void    PP_HBlur5x(const Texture *srcTexture, const float *weights, RenderTarget *dstRT);
void    PP_VBlur5x(const Texture *srcTexture, const float *weights, RenderTarget *dstRT);
void    PP_HBlur7x(const Texture *srcTexture, const float *weights, RenderTarget *dstRT);
void    PP_VBlur7x(const Texture *srcTexture, const float *weights, RenderTarget *dstRT);
void    PP_HBlur15x(const Texture *srcTexture, const float *weights, RenderTarget *dstRT);
void    PP_VBlur15x(const Texture *srcTexture, const float *weights, RenderTarget *dstRT);

void    PP_HBlurBilinear3x(const Texture *srcTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT);
void    PP_VBlurBilinear3x(const Texture *srcTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT);	
void    PP_HBlurBilinear4x(const Texture *srcTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT);
void    PP_VBlurBilinear4x(const Texture *srcTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT);
void    PP_HBlurBilinear8x(const Texture *srcTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT);
void    PP_VBlurBilinear8x(const Texture *srcTexture, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT);
void    PP_HBlurAlphaMaskedBilinear8x(const Texture *srcTexture, const Texture *masktex, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT);
void    PP_VBlurAlphaMaskedBilinear8x(const Texture *srcTexture, const Texture *masktex, const Vec2 *sampleOffsets, const float *weights, RenderTarget *dstRT);

void    PP_GaussBlur5x5(const Texture *srcTexture, RenderTarget *tempRT, RenderTarget *dstRT);
void    PP_GaussBlur7x7(const Texture *srcTexture, RenderTarget *tempRT, RenderTarget *dstRT);
void    PP_GaussBlur15x15(const Texture *srcTexture, RenderTarget *tempRT, RenderTarget *dstRT);
void    PP_KawaseBlur(const Texture *srcTexture, int iteration, RenderTarget *dstRT);

void    PP_AoBlur(const Texture *aoTexture, const Texture *depthTexture, RenderTarget *tempRT, const Mat4 &projectionMatrix, RenderTarget *dstRT);

void    PP_LinearizeDepth(const Texture *depthTexture, float zNear, float zFar, RenderTarget *dstRT);

void    PP_CopyCocToAlpha(const Texture *depthTexture, float zFar, RenderTarget *dstRT);
void    PP_CopyColorAndCoc(const Texture *colorTexture, const Texture *depthTexture, float zFar, RenderTarget *dstRT);
void    PP_ApplyDOF(const Texture *tex0, const Texture *tex1, RenderTarget *dstRT);

void    PP_SunShaftsMaskGen(const Texture *colorTexture, const Texture *depthTexture, float s, float t, float s2, float t2, RenderTarget *dstRT);
void    PP_SunShaftsGen(const Texture *srcTexture, const Mat4 &viewProjectionMatrix, const Vec3 &worldSunPos, float shaftScale, RenderTarget *dstRT);
void    PP_SunShaftsDisplay(const Texture *screenTexture, const Texture *shaftTexture, const Vec4 &sunColor, RenderTarget *dstRT);

void    PP_ObjectMotionBlur(const Texture *srcTexture, const Texture *velocityTexture, float s, float t, float s2, float t2, RenderTarget *dstRT);
void    PP_CameraMotionBlur(const Texture *srcTexture, const Texture *depthTexture, const Mat4 viewProjectionMatrix[2], const Vec3 &cameraPos, float blurScale, float frameTime, RenderTarget *dstRT);

void    PP_MeasureLuminance(const Texture *srcTexture, const float *screenTc, RenderTarget *dstRT);
void    PP_LuminanceAdaptation(const Texture *srcTexture0, const Texture *srcTexture1, float frameTime, RenderTarget *dstRT);

void    PP_BrightFilter(const Texture *srcTexture, const Texture *luminanceTexture, RenderTarget *dstRT);

void    PP_ChromaShift(const Texture *srcTexture, RenderTarget *dstRT);

void    PP_SSAO(const Texture *depthTexture, const Texture *downscaledDepthTexture, const VisibleView *visView, RenderTarget *dstRT);

BE_NAMESPACE_END
