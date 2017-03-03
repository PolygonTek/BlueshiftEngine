//----------------------------------------------------------------------------------
// File:        jni/nv_glesutil/nv_draw_rect.h
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
#ifndef _NV_DRAW_RECT_H
#define _NV_DRAW_RECT_H

#include <GLES3/gl3.h>

class NvDrawRect {
public:
	NvDrawRect();
	~NvDrawRect();

	static void setScreenResolution(int w, int h);

	// A GLES context must be bound when calling this, and it must be
	// the same GLES context for all calls.  If you need to delete the
	// GLES context, you must either delete all instances, or call
	// releaseGLES
	void draw(GLint tex, int l, int t, int r, int b, const float color[4]);

	// Most apps can avoid this if they always delete their NvDrawRects
	// when they lose their GLES context
	static void releaseGLES();

protected:

	class RectShader {
	public:
		RectShader(const char* vertText, const char* fragText);
		void bindShader(float lf, float tf, float rf, float bf, const float color[4]);
		GLint m_program;
		GLint m_destRectUniform;
		GLint m_colorUniform;
		GLint m_posAttrib;
		GLint m_uvAttrib;
	};

	static void initGLES();
	
	static bool ms_glesInitialized;
	static RectShader* ms_shaderTex;
	static RectShader* ms_shaderColor;
	static float ms_screenWidth;
	static float ms_screenHeight;
	static int ms_instanceCount;
};

#endif // _NV_DRAW_RECT_H
