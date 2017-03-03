//----------------------------------------------------------------------------------
// File:        jni/nv_glesutil/nv_draw_rect.cpp
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
#include "nv_draw_rect.h"

#include <GLES3/gl3ext.h>

#include <nv_shader/nv_shader.h>

bool NvDrawRect::ms_glesInitialized = false;
NvDrawRect::RectShader* NvDrawRect::ms_shaderTex = NULL;
NvDrawRect::RectShader* NvDrawRect::ms_shaderColor = NULL;
float NvDrawRect::ms_screenWidth = 1.0f;
float NvDrawRect::ms_screenHeight = 1.0f;

int NvDrawRect::ms_instanceCount = 0;

static GLushort indices [4] = {  0, 1, 2, 3 };
static GLfloat vertices[8] = {  1.0f, 0.0f,
							    0.0f, 0.0f,
								1.0f, 1.0f,
							    0.0f, 1.0f };
static GLfloat uvs [8] = {  1.0f,  0.0f,
						    0.0f,  0.0f,
							1.0f,  1.0f,
						    0.0f,  1.0f  }; 

static const char s_vertShader[] = 
"uniform vec4 uDestRect;\n"
"attribute vec2 aPos;\n"
"attribute vec2 aUV;\n"
"varying vec2 vTexCoord;\n"
"void main() {\n"
"   vec2 pos = mix(uDestRect.xy, uDestRect.zw, aPos);\n"
"   gl_Position = vec4(-1.0 + pos.x*2.0, 1.0 - pos.y*2.0, 0.0, 1.0);\n"
"   vTexCoord = aUV;\n"
"}\n";

static const char s_fragShaderTex[] =
"precision lowp float;\n"
"uniform sampler2D uTex;\n"
"varying vec2 vTexCoord;\n"
"uniform vec4 uColor;\n"
"void main() {\n"
"    gl_FragColor = uColor * texture2D(uTex, vTexCoord);\n"
"}\n";

static const char s_fragShaderColor[] =
"precision lowp float;\n"
"varying vec2 vTexCoord;\n"
"uniform vec4 uColor;\n"
"void main() {\n"
"    gl_FragColor = uColor;\n"
"}\n";


NvDrawRect::RectShader::RectShader(const char* vertText, const char* fragText)
{
	m_program = nv_load_program_from_strings(vertText, fragText);
    glUseProgram(m_program);

    m_destRectUniform = glGetUniformLocation(m_program, "uDestRect");
	m_colorUniform = glGetUniformLocation(m_program, "uColor");
	m_posAttrib = glGetAttribLocation(m_program, "aPos");
	m_uvAttrib = glGetAttribLocation(m_program, "aUV");

	GLint texUni = glGetUniformLocation(m_program, "uTex");
	if (texUni != -1)
		glUniform1i(texUni, 0);
}

void NvDrawRect::RectShader::bindShader(float lf, float tf, float rf, float bf, const float color[4])
{
    glUseProgram(m_program);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribPointer(m_posAttrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(m_posAttrib);

	if (m_uvAttrib != -1)
	{
		glVertexAttribPointer(m_uvAttrib, 2, GL_FLOAT, GL_FALSE, 0, uvs);
		glEnableVertexAttribArray(m_uvAttrib);
	}

	glUniform4f(m_destRectUniform, lf, tf, rf, bf);
	glUniform4fv(m_colorUniform, 1, color);
}

NvDrawRect::NvDrawRect()
{
	ms_instanceCount++;
}

NvDrawRect::~NvDrawRect()
{
	ms_instanceCount--; 
	if (!ms_instanceCount)  
		releaseGLES();
}

void NvDrawRect::initGLES()
{
	ms_glesInitialized = true;
	ms_shaderTex = new RectShader(s_vertShader, s_fragShaderTex);
	ms_shaderColor = new RectShader(s_vertShader, s_fragShaderColor);
}

void NvDrawRect::releaseGLES()
{
	ms_glesInitialized = false;
	delete ms_shaderTex;
	delete ms_shaderColor;
	ms_shaderTex = NULL;
	ms_shaderColor = NULL;
}

void NvDrawRect::setScreenResolution(int w, int h)
{
	ms_screenWidth = (float)w;
	ms_screenHeight = (float)h;
}

void NvDrawRect::draw(GLint tex, int l, int t, int r, int b, const float color[4])
{
	RectShader* shader = tex ? ms_shaderTex : ms_shaderColor;

	if (!ms_glesInitialized) 
	{
		initGLES();
		shader = tex ? ms_shaderTex : ms_shaderColor;
		if (!shader) 
			return;
	}

	shader->bindShader(l/ms_screenWidth, t/ms_screenHeight, 
			r/ms_screenWidth, b/ms_screenHeight, color);

	if (tex)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, indices);
}
