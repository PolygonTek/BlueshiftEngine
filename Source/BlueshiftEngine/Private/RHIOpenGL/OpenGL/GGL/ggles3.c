/*********************************************************************************************
 *
 * ggles3.c
 * ggl (OpenGL glue code library)
 * Version: 0.5
 *
 * Copyright 2011 Ju Hyung Lee. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *     1. Redistributions of source code must retain the above copyright notice, this list of
 *        conditions and the following disclaimer.
 *
 *     2. Redistributions in binary form must reproduce the above copyright notice, this list
 *        of conditions and the following disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY JU HYUNG LEE ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL JU HYUNG LEE OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Ju Hyung Lee.
 *
 ***********************************************************************************************/

#include "ggles3.h"
#include <string.h>

extern void CheckGLError(const char *msg);

typedef void (APIENTRYP PFNGLACTIVESHADERPROGRAMEXT)(GLuint pipeline, GLuint program);
PFNGLACTIVESHADERPROGRAMEXT gglActiveShaderProgramEXT;
static PFNGLACTIVESHADERPROGRAMEXT _glActiveShaderProgramEXT;
static void APIENTRY d_glActiveShaderProgramEXT(GLuint pipeline, GLuint program) {
	_glActiveShaderProgramEXT(pipeline, program);
	CheckGLError("glActiveShaderProgramEXT");
}
typedef void (APIENTRYP PFNGLACTIVETEXTURE)(GLenum texture);
PFNGLACTIVETEXTURE gglActiveTexture;
static PFNGLACTIVETEXTURE _glActiveTexture;
static void APIENTRY d_glActiveTexture(GLenum texture) {
	_glActiveTexture(texture);
	CheckGLError("glActiveTexture");
}
typedef void (APIENTRYP PFNGLALPHAFUNCQCOM)(GLenum func, GLclampf ref);
PFNGLALPHAFUNCQCOM gglAlphaFuncQCOM;
static PFNGLALPHAFUNCQCOM _glAlphaFuncQCOM;
static void APIENTRY d_glAlphaFuncQCOM(GLenum func, GLclampf ref) {
	_glAlphaFuncQCOM(func, ref);
	CheckGLError("glAlphaFuncQCOM");
}
typedef void (APIENTRYP PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTEL)();
PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTEL gglApplyFramebufferAttachmentCMAAINTEL;
static PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTEL _glApplyFramebufferAttachmentCMAAINTEL;
static void APIENTRY d_glApplyFramebufferAttachmentCMAAINTEL() {
	_glApplyFramebufferAttachmentCMAAINTEL();
	CheckGLError("glApplyFramebufferAttachmentCMAAINTEL");
}
typedef void (APIENTRYP PFNGLATTACHSHADER)(GLuint program, GLuint shader);
PFNGLATTACHSHADER gglAttachShader;
static PFNGLATTACHSHADER _glAttachShader;
static void APIENTRY d_glAttachShader(GLuint program, GLuint shader) {
	_glAttachShader(program, shader);
	CheckGLError("glAttachShader");
}
typedef void (APIENTRYP PFNGLBEGINCONDITIONALRENDERNV)(GLuint id, GLenum mode);
PFNGLBEGINCONDITIONALRENDERNV gglBeginConditionalRenderNV;
static PFNGLBEGINCONDITIONALRENDERNV _glBeginConditionalRenderNV;
static void APIENTRY d_glBeginConditionalRenderNV(GLuint id, GLenum mode) {
	_glBeginConditionalRenderNV(id, mode);
	CheckGLError("glBeginConditionalRenderNV");
}
typedef void (APIENTRYP PFNGLBEGINPERFMONITORAMD)(GLuint monitor);
PFNGLBEGINPERFMONITORAMD gglBeginPerfMonitorAMD;
static PFNGLBEGINPERFMONITORAMD _glBeginPerfMonitorAMD;
static void APIENTRY d_glBeginPerfMonitorAMD(GLuint monitor) {
	_glBeginPerfMonitorAMD(monitor);
	CheckGLError("glBeginPerfMonitorAMD");
}
typedef void (APIENTRYP PFNGLBEGINPERFQUERYINTEL)(GLuint queryHandle);
PFNGLBEGINPERFQUERYINTEL gglBeginPerfQueryINTEL;
static PFNGLBEGINPERFQUERYINTEL _glBeginPerfQueryINTEL;
static void APIENTRY d_glBeginPerfQueryINTEL(GLuint queryHandle) {
	_glBeginPerfQueryINTEL(queryHandle);
	CheckGLError("glBeginPerfQueryINTEL");
}
typedef void (APIENTRYP PFNGLBEGINQUERY)(GLenum target, GLuint id);
PFNGLBEGINQUERY gglBeginQuery;
static PFNGLBEGINQUERY _glBeginQuery;
static void APIENTRY d_glBeginQuery(GLenum target, GLuint id) {
	_glBeginQuery(target, id);
	CheckGLError("glBeginQuery");
}
typedef void (APIENTRYP PFNGLBEGINQUERYEXT)(GLenum target, GLuint id);
PFNGLBEGINQUERYEXT gglBeginQueryEXT;
static PFNGLBEGINQUERYEXT _glBeginQueryEXT;
static void APIENTRY d_glBeginQueryEXT(GLenum target, GLuint id) {
	_glBeginQueryEXT(target, id);
	CheckGLError("glBeginQueryEXT");
}
typedef void (APIENTRYP PFNGLBEGINTRANSFORMFEEDBACK)(GLenum primitiveMode);
PFNGLBEGINTRANSFORMFEEDBACK gglBeginTransformFeedback;
static PFNGLBEGINTRANSFORMFEEDBACK _glBeginTransformFeedback;
static void APIENTRY d_glBeginTransformFeedback(GLenum primitiveMode) {
	_glBeginTransformFeedback(primitiveMode);
	CheckGLError("glBeginTransformFeedback");
}
typedef void (APIENTRYP PFNGLBINDATTRIBLOCATION)(GLuint program, GLuint index, const GLchar *name);
PFNGLBINDATTRIBLOCATION gglBindAttribLocation;
static PFNGLBINDATTRIBLOCATION _glBindAttribLocation;
static void APIENTRY d_glBindAttribLocation(GLuint program, GLuint index, const GLchar *name) {
	_glBindAttribLocation(program, index, name);
	CheckGLError("glBindAttribLocation");
}
typedef void (APIENTRYP PFNGLBINDBUFFER)(GLenum target, GLuint buffer);
PFNGLBINDBUFFER gglBindBuffer;
static PFNGLBINDBUFFER _glBindBuffer;
static void APIENTRY d_glBindBuffer(GLenum target, GLuint buffer) {
	_glBindBuffer(target, buffer);
	CheckGLError("glBindBuffer");
}
typedef void (APIENTRYP PFNGLBINDBUFFERBASE)(GLenum target, GLuint index, GLuint buffer);
PFNGLBINDBUFFERBASE gglBindBufferBase;
static PFNGLBINDBUFFERBASE _glBindBufferBase;
static void APIENTRY d_glBindBufferBase(GLenum target, GLuint index, GLuint buffer) {
	_glBindBufferBase(target, index, buffer);
	CheckGLError("glBindBufferBase");
}
typedef void (APIENTRYP PFNGLBINDBUFFERRANGE)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
PFNGLBINDBUFFERRANGE gglBindBufferRange;
static PFNGLBINDBUFFERRANGE _glBindBufferRange;
static void APIENTRY d_glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) {
	_glBindBufferRange(target, index, buffer, offset, size);
	CheckGLError("glBindBufferRange");
}
typedef void (APIENTRYP PFNGLBINDFRAGDATALOCATIONEXT)(GLuint program, GLuint color, const GLchar *name);
PFNGLBINDFRAGDATALOCATIONEXT gglBindFragDataLocationEXT;
static PFNGLBINDFRAGDATALOCATIONEXT _glBindFragDataLocationEXT;
static void APIENTRY d_glBindFragDataLocationEXT(GLuint program, GLuint color, const GLchar *name) {
	_glBindFragDataLocationEXT(program, color, name);
	CheckGLError("glBindFragDataLocationEXT");
}
typedef void (APIENTRYP PFNGLBINDFRAGDATALOCATIONINDEXEDEXT)(GLuint program, GLuint colorNumber, GLuint index, const GLchar *name);
PFNGLBINDFRAGDATALOCATIONINDEXEDEXT gglBindFragDataLocationIndexedEXT;
static PFNGLBINDFRAGDATALOCATIONINDEXEDEXT _glBindFragDataLocationIndexedEXT;
static void APIENTRY d_glBindFragDataLocationIndexedEXT(GLuint program, GLuint colorNumber, GLuint index, const GLchar *name) {
	_glBindFragDataLocationIndexedEXT(program, colorNumber, index, name);
	CheckGLError("glBindFragDataLocationIndexedEXT");
}
typedef void (APIENTRYP PFNGLBINDFRAMEBUFFER)(GLenum target, GLuint framebuffer);
PFNGLBINDFRAMEBUFFER gglBindFramebuffer;
static PFNGLBINDFRAMEBUFFER _glBindFramebuffer;
static void APIENTRY d_glBindFramebuffer(GLenum target, GLuint framebuffer) {
	_glBindFramebuffer(target, framebuffer);
	CheckGLError("glBindFramebuffer");
}
typedef void (APIENTRYP PFNGLBINDPROGRAMPIPELINEEXT)(GLuint pipeline);
PFNGLBINDPROGRAMPIPELINEEXT gglBindProgramPipelineEXT;
static PFNGLBINDPROGRAMPIPELINEEXT _glBindProgramPipelineEXT;
static void APIENTRY d_glBindProgramPipelineEXT(GLuint pipeline) {
	_glBindProgramPipelineEXT(pipeline);
	CheckGLError("glBindProgramPipelineEXT");
}
typedef void (APIENTRYP PFNGLBINDRENDERBUFFER)(GLenum target, GLuint renderbuffer);
PFNGLBINDRENDERBUFFER gglBindRenderbuffer;
static PFNGLBINDRENDERBUFFER _glBindRenderbuffer;
static void APIENTRY d_glBindRenderbuffer(GLenum target, GLuint renderbuffer) {
	_glBindRenderbuffer(target, renderbuffer);
	CheckGLError("glBindRenderbuffer");
}
typedef void (APIENTRYP PFNGLBINDSAMPLER)(GLuint unit, GLuint sampler);
PFNGLBINDSAMPLER gglBindSampler;
static PFNGLBINDSAMPLER _glBindSampler;
static void APIENTRY d_glBindSampler(GLuint unit, GLuint sampler) {
	_glBindSampler(unit, sampler);
	CheckGLError("glBindSampler");
}
typedef void (APIENTRYP PFNGLBINDTEXTURE)(GLenum target, GLuint texture);
PFNGLBINDTEXTURE gglBindTexture;
static PFNGLBINDTEXTURE _glBindTexture;
static void APIENTRY d_glBindTexture(GLenum target, GLuint texture) {
	_glBindTexture(target, texture);
	CheckGLError("glBindTexture");
}
typedef void (APIENTRYP PFNGLBINDTRANSFORMFEEDBACK)(GLenum target, GLuint id);
PFNGLBINDTRANSFORMFEEDBACK gglBindTransformFeedback;
static PFNGLBINDTRANSFORMFEEDBACK _glBindTransformFeedback;
static void APIENTRY d_glBindTransformFeedback(GLenum target, GLuint id) {
	_glBindTransformFeedback(target, id);
	CheckGLError("glBindTransformFeedback");
}
typedef void (APIENTRYP PFNGLBINDVERTEXARRAY)(GLuint array);
PFNGLBINDVERTEXARRAY gglBindVertexArray;
static PFNGLBINDVERTEXARRAY _glBindVertexArray;
static void APIENTRY d_glBindVertexArray(GLuint array) {
	_glBindVertexArray(array);
	CheckGLError("glBindVertexArray");
}
typedef void (APIENTRYP PFNGLBINDVERTEXARRAYOES)(GLuint array);
PFNGLBINDVERTEXARRAYOES gglBindVertexArrayOES;
static PFNGLBINDVERTEXARRAYOES _glBindVertexArrayOES;
static void APIENTRY d_glBindVertexArrayOES(GLuint array) {
	_glBindVertexArrayOES(array);
	CheckGLError("glBindVertexArrayOES");
}
typedef void (APIENTRYP PFNGLBLENDBARRIERKHR)();
PFNGLBLENDBARRIERKHR gglBlendBarrierKHR;
static PFNGLBLENDBARRIERKHR _glBlendBarrierKHR;
static void APIENTRY d_glBlendBarrierKHR() {
	_glBlendBarrierKHR();
	CheckGLError("glBlendBarrierKHR");
}
typedef void (APIENTRYP PFNGLBLENDBARRIERNV)();
PFNGLBLENDBARRIERNV gglBlendBarrierNV;
static PFNGLBLENDBARRIERNV _glBlendBarrierNV;
static void APIENTRY d_glBlendBarrierNV() {
	_glBlendBarrierNV();
	CheckGLError("glBlendBarrierNV");
}
typedef void (APIENTRYP PFNGLBLENDCOLOR)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
PFNGLBLENDCOLOR gglBlendColor;
static PFNGLBLENDCOLOR _glBlendColor;
static void APIENTRY d_glBlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	_glBlendColor(red, green, blue, alpha);
	CheckGLError("glBlendColor");
}
typedef void (APIENTRYP PFNGLBLENDEQUATION)(GLenum mode);
PFNGLBLENDEQUATION gglBlendEquation;
static PFNGLBLENDEQUATION _glBlendEquation;
static void APIENTRY d_glBlendEquation(GLenum mode) {
	_glBlendEquation(mode);
	CheckGLError("glBlendEquation");
}
typedef void (APIENTRYP PFNGLBLENDEQUATIONSEPARATE)(GLenum modeRGB, GLenum modeAlpha);
PFNGLBLENDEQUATIONSEPARATE gglBlendEquationSeparate;
static PFNGLBLENDEQUATIONSEPARATE _glBlendEquationSeparate;
static void APIENTRY d_glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha) {
	_glBlendEquationSeparate(modeRGB, modeAlpha);
	CheckGLError("glBlendEquationSeparate");
}
typedef void (APIENTRYP PFNGLBLENDEQUATIONSEPARATEIEXT)(GLuint buf, GLenum modeRGB, GLenum modeAlpha);
PFNGLBLENDEQUATIONSEPARATEIEXT gglBlendEquationSeparateiEXT;
static PFNGLBLENDEQUATIONSEPARATEIEXT _glBlendEquationSeparateiEXT;
static void APIENTRY d_glBlendEquationSeparateiEXT(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
	_glBlendEquationSeparateiEXT(buf, modeRGB, modeAlpha);
	CheckGLError("glBlendEquationSeparateiEXT");
}
typedef void (APIENTRYP PFNGLBLENDEQUATIONSEPARATEIOES)(GLuint buf, GLenum modeRGB, GLenum modeAlpha);
PFNGLBLENDEQUATIONSEPARATEIOES gglBlendEquationSeparateiOES;
static PFNGLBLENDEQUATIONSEPARATEIOES _glBlendEquationSeparateiOES;
static void APIENTRY d_glBlendEquationSeparateiOES(GLuint buf, GLenum modeRGB, GLenum modeAlpha) {
	_glBlendEquationSeparateiOES(buf, modeRGB, modeAlpha);
	CheckGLError("glBlendEquationSeparateiOES");
}
typedef void (APIENTRYP PFNGLBLENDEQUATIONIEXT)(GLuint buf, GLenum mode);
PFNGLBLENDEQUATIONIEXT gglBlendEquationiEXT;
static PFNGLBLENDEQUATIONIEXT _glBlendEquationiEXT;
static void APIENTRY d_glBlendEquationiEXT(GLuint buf, GLenum mode) {
	_glBlendEquationiEXT(buf, mode);
	CheckGLError("glBlendEquationiEXT");
}
typedef void (APIENTRYP PFNGLBLENDEQUATIONIOES)(GLuint buf, GLenum mode);
PFNGLBLENDEQUATIONIOES gglBlendEquationiOES;
static PFNGLBLENDEQUATIONIOES _glBlendEquationiOES;
static void APIENTRY d_glBlendEquationiOES(GLuint buf, GLenum mode) {
	_glBlendEquationiOES(buf, mode);
	CheckGLError("glBlendEquationiOES");
}
typedef void (APIENTRYP PFNGLBLENDFUNC)(GLenum sfactor, GLenum dfactor);
PFNGLBLENDFUNC gglBlendFunc;
static PFNGLBLENDFUNC _glBlendFunc;
static void APIENTRY d_glBlendFunc(GLenum sfactor, GLenum dfactor) {
	_glBlendFunc(sfactor, dfactor);
	CheckGLError("glBlendFunc");
}
typedef void (APIENTRYP PFNGLBLENDFUNCSEPARATE)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
PFNGLBLENDFUNCSEPARATE gglBlendFuncSeparate;
static PFNGLBLENDFUNCSEPARATE _glBlendFuncSeparate;
static void APIENTRY d_glBlendFuncSeparate(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha) {
	_glBlendFuncSeparate(sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha);
	CheckGLError("glBlendFuncSeparate");
}
typedef void (APIENTRYP PFNGLBLENDFUNCSEPARATEIEXT)(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
PFNGLBLENDFUNCSEPARATEIEXT gglBlendFuncSeparateiEXT;
static PFNGLBLENDFUNCSEPARATEIEXT _glBlendFuncSeparateiEXT;
static void APIENTRY d_glBlendFuncSeparateiEXT(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
	_glBlendFuncSeparateiEXT(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
	CheckGLError("glBlendFuncSeparateiEXT");
}
typedef void (APIENTRYP PFNGLBLENDFUNCSEPARATEIOES)(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
PFNGLBLENDFUNCSEPARATEIOES gglBlendFuncSeparateiOES;
static PFNGLBLENDFUNCSEPARATEIOES _glBlendFuncSeparateiOES;
static void APIENTRY d_glBlendFuncSeparateiOES(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
	_glBlendFuncSeparateiOES(buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
	CheckGLError("glBlendFuncSeparateiOES");
}
typedef void (APIENTRYP PFNGLBLENDFUNCIEXT)(GLuint buf, GLenum src, GLenum dst);
PFNGLBLENDFUNCIEXT gglBlendFunciEXT;
static PFNGLBLENDFUNCIEXT _glBlendFunciEXT;
static void APIENTRY d_glBlendFunciEXT(GLuint buf, GLenum src, GLenum dst) {
	_glBlendFunciEXT(buf, src, dst);
	CheckGLError("glBlendFunciEXT");
}
typedef void (APIENTRYP PFNGLBLENDFUNCIOES)(GLuint buf, GLenum src, GLenum dst);
PFNGLBLENDFUNCIOES gglBlendFunciOES;
static PFNGLBLENDFUNCIOES _glBlendFunciOES;
static void APIENTRY d_glBlendFunciOES(GLuint buf, GLenum src, GLenum dst) {
	_glBlendFunciOES(buf, src, dst);
	CheckGLError("glBlendFunciOES");
}
typedef void (APIENTRYP PFNGLBLENDPARAMETERINV)(GLenum pname, GLint value);
PFNGLBLENDPARAMETERINV gglBlendParameteriNV;
static PFNGLBLENDPARAMETERINV _glBlendParameteriNV;
static void APIENTRY d_glBlendParameteriNV(GLenum pname, GLint value) {
	_glBlendParameteriNV(pname, value);
	CheckGLError("glBlendParameteriNV");
}
typedef void (APIENTRYP PFNGLBLITFRAMEBUFFER)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
PFNGLBLITFRAMEBUFFER gglBlitFramebuffer;
static PFNGLBLITFRAMEBUFFER _glBlitFramebuffer;
static void APIENTRY d_glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
	_glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	CheckGLError("glBlitFramebuffer");
}
typedef void (APIENTRYP PFNGLBLITFRAMEBUFFERANGLE)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
PFNGLBLITFRAMEBUFFERANGLE gglBlitFramebufferANGLE;
static PFNGLBLITFRAMEBUFFERANGLE _glBlitFramebufferANGLE;
static void APIENTRY d_glBlitFramebufferANGLE(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
	_glBlitFramebufferANGLE(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	CheckGLError("glBlitFramebufferANGLE");
}
typedef void (APIENTRYP PFNGLBLITFRAMEBUFFERNV)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
PFNGLBLITFRAMEBUFFERNV gglBlitFramebufferNV;
static PFNGLBLITFRAMEBUFFERNV _glBlitFramebufferNV;
static void APIENTRY d_glBlitFramebufferNV(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
	_glBlitFramebufferNV(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	CheckGLError("glBlitFramebufferNV");
}
typedef void (APIENTRYP PFNGLBUFFERDATA)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
PFNGLBUFFERDATA gglBufferData;
static PFNGLBUFFERDATA _glBufferData;
static void APIENTRY d_glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
	_glBufferData(target, size, data, usage);
	CheckGLError("glBufferData");
}
typedef void (APIENTRYP PFNGLBUFFERSTORAGEEXT)(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);
PFNGLBUFFERSTORAGEEXT gglBufferStorageEXT;
static PFNGLBUFFERSTORAGEEXT _glBufferStorageEXT;
static void APIENTRY d_glBufferStorageEXT(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags) {
	_glBufferStorageEXT(target, size, data, flags);
	CheckGLError("glBufferStorageEXT");
}
typedef void (APIENTRYP PFNGLBUFFERSUBDATA)(GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
PFNGLBUFFERSUBDATA gglBufferSubData;
static PFNGLBUFFERSUBDATA _glBufferSubData;
static void APIENTRY d_glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data) {
	_glBufferSubData(target, offset, size, data);
	CheckGLError("glBufferSubData");
}
typedef GLenum (APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUS)(GLenum target);
PFNGLCHECKFRAMEBUFFERSTATUS gglCheckFramebufferStatus;
static PFNGLCHECKFRAMEBUFFERSTATUS _glCheckFramebufferStatus;
static GLenum APIENTRY d_glCheckFramebufferStatus(GLenum target) {
	GLenum ret = _glCheckFramebufferStatus(target);
	CheckGLError("glCheckFramebufferStatus");
	return ret;
}
typedef void (APIENTRYP PFNGLCLEAR)(GLbitfield mask);
PFNGLCLEAR gglClear;
static PFNGLCLEAR _glClear;
static void APIENTRY d_glClear(GLbitfield mask) {
	_glClear(mask);
	CheckGLError("glClear");
}
typedef void (APIENTRYP PFNGLCLEARBUFFERFI)(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
PFNGLCLEARBUFFERFI gglClearBufferfi;
static PFNGLCLEARBUFFERFI _glClearBufferfi;
static void APIENTRY d_glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) {
	_glClearBufferfi(buffer, drawbuffer, depth, stencil);
	CheckGLError("glClearBufferfi");
}
typedef void (APIENTRYP PFNGLCLEARBUFFERFV)(GLenum buffer, GLint drawbuffer, const GLfloat *value);
PFNGLCLEARBUFFERFV gglClearBufferfv;
static PFNGLCLEARBUFFERFV _glClearBufferfv;
static void APIENTRY d_glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value) {
	_glClearBufferfv(buffer, drawbuffer, value);
	CheckGLError("glClearBufferfv");
}
typedef void (APIENTRYP PFNGLCLEARBUFFERIV)(GLenum buffer, GLint drawbuffer, const GLint *value);
PFNGLCLEARBUFFERIV gglClearBufferiv;
static PFNGLCLEARBUFFERIV _glClearBufferiv;
static void APIENTRY d_glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value) {
	_glClearBufferiv(buffer, drawbuffer, value);
	CheckGLError("glClearBufferiv");
}
typedef void (APIENTRYP PFNGLCLEARBUFFERUIV)(GLenum buffer, GLint drawbuffer, const GLuint *value);
PFNGLCLEARBUFFERUIV gglClearBufferuiv;
static PFNGLCLEARBUFFERUIV _glClearBufferuiv;
static void APIENTRY d_glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value) {
	_glClearBufferuiv(buffer, drawbuffer, value);
	CheckGLError("glClearBufferuiv");
}
typedef void (APIENTRYP PFNGLCLEARCOLOR)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
PFNGLCLEARCOLOR gglClearColor;
static PFNGLCLEARCOLOR _glClearColor;
static void APIENTRY d_glClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
	_glClearColor(red, green, blue, alpha);
	CheckGLError("glClearColor");
}
typedef void (APIENTRYP PFNGLCLEARDEPTHF)(GLfloat d);
PFNGLCLEARDEPTHF gglClearDepthf;
static PFNGLCLEARDEPTHF _glClearDepthf;
static void APIENTRY d_glClearDepthf(GLfloat d) {
	_glClearDepthf(d);
	CheckGLError("glClearDepthf");
}
typedef void (APIENTRYP PFNGLCLEARPIXELLOCALSTORAGEUIEXT)(GLsizei offset, GLsizei n, const GLuint *values);
PFNGLCLEARPIXELLOCALSTORAGEUIEXT gglClearPixelLocalStorageuiEXT;
static PFNGLCLEARPIXELLOCALSTORAGEUIEXT _glClearPixelLocalStorageuiEXT;
static void APIENTRY d_glClearPixelLocalStorageuiEXT(GLsizei offset, GLsizei n, const GLuint *values) {
	_glClearPixelLocalStorageuiEXT(offset, n, values);
	CheckGLError("glClearPixelLocalStorageuiEXT");
}
typedef void (APIENTRYP PFNGLCLEARSTENCIL)(GLint s);
PFNGLCLEARSTENCIL gglClearStencil;
static PFNGLCLEARSTENCIL _glClearStencil;
static void APIENTRY d_glClearStencil(GLint s) {
	_glClearStencil(s);
	CheckGLError("glClearStencil");
}
typedef void (APIENTRYP PFNGLCLEARTEXIMAGEEXT)(GLuint texture, GLint level, GLenum format, GLenum type, const void *data);
PFNGLCLEARTEXIMAGEEXT gglClearTexImageEXT;
static PFNGLCLEARTEXIMAGEEXT _glClearTexImageEXT;
static void APIENTRY d_glClearTexImageEXT(GLuint texture, GLint level, GLenum format, GLenum type, const void *data) {
	_glClearTexImageEXT(texture, level, format, type, data);
	CheckGLError("glClearTexImageEXT");
}
typedef void (APIENTRYP PFNGLCLEARTEXSUBIMAGEEXT)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data);
PFNGLCLEARTEXSUBIMAGEEXT gglClearTexSubImageEXT;
static PFNGLCLEARTEXSUBIMAGEEXT _glClearTexSubImageEXT;
static void APIENTRY d_glClearTexSubImageEXT(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data) {
	_glClearTexSubImageEXT(texture, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
	CheckGLError("glClearTexSubImageEXT");
}
typedef GLenum (APIENTRYP PFNGLCLIENTWAITSYNC)(GLsync sync, GLbitfield flags, GLuint64 timeout);
PFNGLCLIENTWAITSYNC gglClientWaitSync;
static PFNGLCLIENTWAITSYNC _glClientWaitSync;
static GLenum APIENTRY d_glClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
	GLenum ret = _glClientWaitSync(sync, flags, timeout);
	CheckGLError("glClientWaitSync");
	return ret;
}
typedef GLenum (APIENTRYP PFNGLCLIENTWAITSYNCAPPLE)(GLsync sync, GLbitfield flags, GLuint64 timeout);
PFNGLCLIENTWAITSYNCAPPLE gglClientWaitSyncAPPLE;
static PFNGLCLIENTWAITSYNCAPPLE _glClientWaitSyncAPPLE;
static GLenum APIENTRY d_glClientWaitSyncAPPLE(GLsync sync, GLbitfield flags, GLuint64 timeout) {
	GLenum ret = _glClientWaitSyncAPPLE(sync, flags, timeout);
	CheckGLError("glClientWaitSyncAPPLE");
	return ret;
}
typedef void (APIENTRYP PFNGLCOLORMASK)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
PFNGLCOLORMASK gglColorMask;
static PFNGLCOLORMASK _glColorMask;
static void APIENTRY d_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
	_glColorMask(red, green, blue, alpha);
	CheckGLError("glColorMask");
}
typedef void (APIENTRYP PFNGLCOLORMASKIEXT)(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a);
PFNGLCOLORMASKIEXT gglColorMaskiEXT;
static PFNGLCOLORMASKIEXT _glColorMaskiEXT;
static void APIENTRY d_glColorMaskiEXT(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
	_glColorMaskiEXT(index, r, g, b, a);
	CheckGLError("glColorMaskiEXT");
}
typedef void (APIENTRYP PFNGLCOLORMASKIOES)(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a);
PFNGLCOLORMASKIOES gglColorMaskiOES;
static PFNGLCOLORMASKIOES _glColorMaskiOES;
static void APIENTRY d_glColorMaskiOES(GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a) {
	_glColorMaskiOES(index, r, g, b, a);
	CheckGLError("glColorMaskiOES");
}
typedef void (APIENTRYP PFNGLCOMPILESHADER)(GLuint shader);
PFNGLCOMPILESHADER gglCompileShader;
static PFNGLCOMPILESHADER _glCompileShader;
static void APIENTRY d_glCompileShader(GLuint shader) {
	_glCompileShader(shader);
	CheckGLError("glCompileShader");
}
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXIMAGE2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
PFNGLCOMPRESSEDTEXIMAGE2D gglCompressedTexImage2D;
static PFNGLCOMPRESSEDTEXIMAGE2D _glCompressedTexImage2D;
static void APIENTRY d_glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data) {
	_glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
	CheckGLError("glCompressedTexImage2D");
}
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXIMAGE3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
PFNGLCOMPRESSEDTEXIMAGE3D gglCompressedTexImage3D;
static PFNGLCOMPRESSEDTEXIMAGE3D _glCompressedTexImage3D;
static void APIENTRY d_glCompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data) {
	_glCompressedTexImage3D(target, level, internalformat, width, height, depth, border, imageSize, data);
	CheckGLError("glCompressedTexImage3D");
}
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXIMAGE3DOES)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
PFNGLCOMPRESSEDTEXIMAGE3DOES gglCompressedTexImage3DOES;
static PFNGLCOMPRESSEDTEXIMAGE3DOES _glCompressedTexImage3DOES;
static void APIENTRY d_glCompressedTexImage3DOES(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data) {
	_glCompressedTexImage3DOES(target, level, internalformat, width, height, depth, border, imageSize, data);
	CheckGLError("glCompressedTexImage3DOES");
}
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXSUBIMAGE2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
PFNGLCOMPRESSEDTEXSUBIMAGE2D gglCompressedTexSubImage2D;
static PFNGLCOMPRESSEDTEXSUBIMAGE2D _glCompressedTexSubImage2D;
static void APIENTRY d_glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data) {
	_glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
	CheckGLError("glCompressedTexSubImage2D");
}
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXSUBIMAGE3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
PFNGLCOMPRESSEDTEXSUBIMAGE3D gglCompressedTexSubImage3D;
static PFNGLCOMPRESSEDTEXSUBIMAGE3D _glCompressedTexSubImage3D;
static void APIENTRY d_glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data) {
	_glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
	CheckGLError("glCompressedTexSubImage3D");
}
typedef void (APIENTRYP PFNGLCOMPRESSEDTEXSUBIMAGE3DOES)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
PFNGLCOMPRESSEDTEXSUBIMAGE3DOES gglCompressedTexSubImage3DOES;
static PFNGLCOMPRESSEDTEXSUBIMAGE3DOES _glCompressedTexSubImage3DOES;
static void APIENTRY d_glCompressedTexSubImage3DOES(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data) {
	_glCompressedTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
	CheckGLError("glCompressedTexSubImage3DOES");
}
typedef void (APIENTRYP PFNGLCONSERVATIVERASTERPARAMETERINV)(GLenum pname, GLint param);
PFNGLCONSERVATIVERASTERPARAMETERINV gglConservativeRasterParameteriNV;
static PFNGLCONSERVATIVERASTERPARAMETERINV _glConservativeRasterParameteriNV;
static void APIENTRY d_glConservativeRasterParameteriNV(GLenum pname, GLint param) {
	_glConservativeRasterParameteriNV(pname, param);
	CheckGLError("glConservativeRasterParameteriNV");
}
typedef void (APIENTRYP PFNGLCOPYBUFFERSUBDATA)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
PFNGLCOPYBUFFERSUBDATA gglCopyBufferSubData;
static PFNGLCOPYBUFFERSUBDATA _glCopyBufferSubData;
static void APIENTRY d_glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
	_glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
	CheckGLError("glCopyBufferSubData");
}
typedef void (APIENTRYP PFNGLCOPYBUFFERSUBDATANV)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
PFNGLCOPYBUFFERSUBDATANV gglCopyBufferSubDataNV;
static PFNGLCOPYBUFFERSUBDATANV _glCopyBufferSubDataNV;
static void APIENTRY d_glCopyBufferSubDataNV(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
	_glCopyBufferSubDataNV(readTarget, writeTarget, readOffset, writeOffset, size);
	CheckGLError("glCopyBufferSubDataNV");
}
typedef void (APIENTRYP PFNGLCOPYIMAGESUBDATAEXT)(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);
PFNGLCOPYIMAGESUBDATAEXT gglCopyImageSubDataEXT;
static PFNGLCOPYIMAGESUBDATAEXT _glCopyImageSubDataEXT;
static void APIENTRY d_glCopyImageSubDataEXT(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {
	_glCopyImageSubDataEXT(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth);
	CheckGLError("glCopyImageSubDataEXT");
}
typedef void (APIENTRYP PFNGLCOPYIMAGESUBDATAOES)(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);
PFNGLCOPYIMAGESUBDATAOES gglCopyImageSubDataOES;
static PFNGLCOPYIMAGESUBDATAOES _glCopyImageSubDataOES;
static void APIENTRY d_glCopyImageSubDataOES(GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth) {
	_glCopyImageSubDataOES(srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, srcDepth);
	CheckGLError("glCopyImageSubDataOES");
}
typedef void (APIENTRYP PFNGLCOPYPATHNV)(GLuint resultPath, GLuint srcPath);
PFNGLCOPYPATHNV gglCopyPathNV;
static PFNGLCOPYPATHNV _glCopyPathNV;
static void APIENTRY d_glCopyPathNV(GLuint resultPath, GLuint srcPath) {
	_glCopyPathNV(resultPath, srcPath);
	CheckGLError("glCopyPathNV");
}
typedef void (APIENTRYP PFNGLCOPYTEXIMAGE2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
PFNGLCOPYTEXIMAGE2D gglCopyTexImage2D;
static PFNGLCOPYTEXIMAGE2D _glCopyTexImage2D;
static void APIENTRY d_glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
	_glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
	CheckGLError("glCopyTexImage2D");
}
typedef void (APIENTRYP PFNGLCOPYTEXSUBIMAGE2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
PFNGLCOPYTEXSUBIMAGE2D gglCopyTexSubImage2D;
static PFNGLCOPYTEXSUBIMAGE2D _glCopyTexSubImage2D;
static void APIENTRY d_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
	_glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
	CheckGLError("glCopyTexSubImage2D");
}
typedef void (APIENTRYP PFNGLCOPYTEXSUBIMAGE3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
PFNGLCOPYTEXSUBIMAGE3D gglCopyTexSubImage3D;
static PFNGLCOPYTEXSUBIMAGE3D _glCopyTexSubImage3D;
static void APIENTRY d_glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
	_glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset, x, y, width, height);
	CheckGLError("glCopyTexSubImage3D");
}
typedef void (APIENTRYP PFNGLCOPYTEXSUBIMAGE3DOES)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
PFNGLCOPYTEXSUBIMAGE3DOES gglCopyTexSubImage3DOES;
static PFNGLCOPYTEXSUBIMAGE3DOES _glCopyTexSubImage3DOES;
static void APIENTRY d_glCopyTexSubImage3DOES(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
	_glCopyTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, x, y, width, height);
	CheckGLError("glCopyTexSubImage3DOES");
}
typedef void (APIENTRYP PFNGLCOPYTEXTURELEVELSAPPLE)(GLuint destinationTexture, GLuint sourceTexture, GLint sourceBaseLevel, GLsizei sourceLevelCount);
PFNGLCOPYTEXTURELEVELSAPPLE gglCopyTextureLevelsAPPLE;
static PFNGLCOPYTEXTURELEVELSAPPLE _glCopyTextureLevelsAPPLE;
static void APIENTRY d_glCopyTextureLevelsAPPLE(GLuint destinationTexture, GLuint sourceTexture, GLint sourceBaseLevel, GLsizei sourceLevelCount) {
	_glCopyTextureLevelsAPPLE(destinationTexture, sourceTexture, sourceBaseLevel, sourceLevelCount);
	CheckGLError("glCopyTextureLevelsAPPLE");
}
typedef void (APIENTRYP PFNGLCOVERFILLPATHINSTANCEDNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
PFNGLCOVERFILLPATHINSTANCEDNV gglCoverFillPathInstancedNV;
static PFNGLCOVERFILLPATHINSTANCEDNV _glCoverFillPathInstancedNV;
static void APIENTRY d_glCoverFillPathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues) {
	_glCoverFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues);
	CheckGLError("glCoverFillPathInstancedNV");
}
typedef void (APIENTRYP PFNGLCOVERFILLPATHNV)(GLuint path, GLenum coverMode);
PFNGLCOVERFILLPATHNV gglCoverFillPathNV;
static PFNGLCOVERFILLPATHNV _glCoverFillPathNV;
static void APIENTRY d_glCoverFillPathNV(GLuint path, GLenum coverMode) {
	_glCoverFillPathNV(path, coverMode);
	CheckGLError("glCoverFillPathNV");
}
typedef void (APIENTRYP PFNGLCOVERSTROKEPATHINSTANCEDNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
PFNGLCOVERSTROKEPATHINSTANCEDNV gglCoverStrokePathInstancedNV;
static PFNGLCOVERSTROKEPATHINSTANCEDNV _glCoverStrokePathInstancedNV;
static void APIENTRY d_glCoverStrokePathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues) {
	_glCoverStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, coverMode, transformType, transformValues);
	CheckGLError("glCoverStrokePathInstancedNV");
}
typedef void (APIENTRYP PFNGLCOVERSTROKEPATHNV)(GLuint path, GLenum coverMode);
PFNGLCOVERSTROKEPATHNV gglCoverStrokePathNV;
static PFNGLCOVERSTROKEPATHNV _glCoverStrokePathNV;
static void APIENTRY d_glCoverStrokePathNV(GLuint path, GLenum coverMode) {
	_glCoverStrokePathNV(path, coverMode);
	CheckGLError("glCoverStrokePathNV");
}
typedef void (APIENTRYP PFNGLCOVERAGEMASKNV)(GLboolean mask);
PFNGLCOVERAGEMASKNV gglCoverageMaskNV;
static PFNGLCOVERAGEMASKNV _glCoverageMaskNV;
static void APIENTRY d_glCoverageMaskNV(GLboolean mask) {
	_glCoverageMaskNV(mask);
	CheckGLError("glCoverageMaskNV");
}
typedef void (APIENTRYP PFNGLCOVERAGEMODULATIONNV)(GLenum components);
PFNGLCOVERAGEMODULATIONNV gglCoverageModulationNV;
static PFNGLCOVERAGEMODULATIONNV _glCoverageModulationNV;
static void APIENTRY d_glCoverageModulationNV(GLenum components) {
	_glCoverageModulationNV(components);
	CheckGLError("glCoverageModulationNV");
}
typedef void (APIENTRYP PFNGLCOVERAGEMODULATIONTABLENV)(GLsizei n, const GLfloat *v);
PFNGLCOVERAGEMODULATIONTABLENV gglCoverageModulationTableNV;
static PFNGLCOVERAGEMODULATIONTABLENV _glCoverageModulationTableNV;
static void APIENTRY d_glCoverageModulationTableNV(GLsizei n, const GLfloat *v) {
	_glCoverageModulationTableNV(n, v);
	CheckGLError("glCoverageModulationTableNV");
}
typedef void (APIENTRYP PFNGLCOVERAGEOPERATIONNV)(GLenum operation);
PFNGLCOVERAGEOPERATIONNV gglCoverageOperationNV;
static PFNGLCOVERAGEOPERATIONNV _glCoverageOperationNV;
static void APIENTRY d_glCoverageOperationNV(GLenum operation) {
	_glCoverageOperationNV(operation);
	CheckGLError("glCoverageOperationNV");
}
typedef void (APIENTRYP PFNGLCREATEPERFQUERYINTEL)(GLuint queryId, GLuint *queryHandle);
PFNGLCREATEPERFQUERYINTEL gglCreatePerfQueryINTEL;
static PFNGLCREATEPERFQUERYINTEL _glCreatePerfQueryINTEL;
static void APIENTRY d_glCreatePerfQueryINTEL(GLuint queryId, GLuint *queryHandle) {
	_glCreatePerfQueryINTEL(queryId, queryHandle);
	CheckGLError("glCreatePerfQueryINTEL");
}
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAM)();
PFNGLCREATEPROGRAM gglCreateProgram;
static PFNGLCREATEPROGRAM _glCreateProgram;
static GLuint APIENTRY d_glCreateProgram() {
	GLuint ret = _glCreateProgram();
	CheckGLError("glCreateProgram");
	return ret;
}
typedef GLuint (APIENTRYP PFNGLCREATESHADER)(GLenum type);
PFNGLCREATESHADER gglCreateShader;
static PFNGLCREATESHADER _glCreateShader;
static GLuint APIENTRY d_glCreateShader(GLenum type) {
	GLuint ret = _glCreateShader(type);
	CheckGLError("glCreateShader");
	return ret;
}
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROGRAMVEXT)(GLenum type, GLsizei count, const GLchar **strings);
PFNGLCREATESHADERPROGRAMVEXT gglCreateShaderProgramvEXT;
static PFNGLCREATESHADERPROGRAMVEXT _glCreateShaderProgramvEXT;
static GLuint APIENTRY d_glCreateShaderProgramvEXT(GLenum type, GLsizei count, const GLchar **strings) {
	GLuint ret = _glCreateShaderProgramvEXT(type, count, strings);
	CheckGLError("glCreateShaderProgramvEXT");
	return ret;
}
typedef void (APIENTRYP PFNGLCULLFACE)(GLenum mode);
PFNGLCULLFACE gglCullFace;
static PFNGLCULLFACE _glCullFace;
static void APIENTRY d_glCullFace(GLenum mode) {
	_glCullFace(mode);
	CheckGLError("glCullFace");
}
typedef void (APIENTRYP PFNGLDEBUGMESSAGECALLBACKKHR)(GLDEBUGPROCKHR callback, const void *userParam);
PFNGLDEBUGMESSAGECALLBACKKHR gglDebugMessageCallbackKHR;
static PFNGLDEBUGMESSAGECALLBACKKHR _glDebugMessageCallbackKHR;
static void APIENTRY d_glDebugMessageCallbackKHR(GLDEBUGPROCKHR callback, const void *userParam) {
	_glDebugMessageCallbackKHR(callback, userParam);
	CheckGLError("glDebugMessageCallbackKHR");
}
typedef void (APIENTRYP PFNGLDEBUGMESSAGECONTROLKHR)(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
PFNGLDEBUGMESSAGECONTROLKHR gglDebugMessageControlKHR;
static PFNGLDEBUGMESSAGECONTROLKHR _glDebugMessageControlKHR;
static void APIENTRY d_glDebugMessageControlKHR(GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled) {
	_glDebugMessageControlKHR(source, type, severity, count, ids, enabled);
	CheckGLError("glDebugMessageControlKHR");
}
typedef void (APIENTRYP PFNGLDEBUGMESSAGEINSERTKHR)(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf);
PFNGLDEBUGMESSAGEINSERTKHR gglDebugMessageInsertKHR;
static PFNGLDEBUGMESSAGEINSERTKHR _glDebugMessageInsertKHR;
static void APIENTRY d_glDebugMessageInsertKHR(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf) {
	_glDebugMessageInsertKHR(source, type, id, severity, length, buf);
	CheckGLError("glDebugMessageInsertKHR");
}
typedef void (APIENTRYP PFNGLDELETEBUFFERS)(GLsizei n, const GLuint *buffers);
PFNGLDELETEBUFFERS gglDeleteBuffers;
static PFNGLDELETEBUFFERS _glDeleteBuffers;
static void APIENTRY d_glDeleteBuffers(GLsizei n, const GLuint *buffers) {
	_glDeleteBuffers(n, buffers);
	CheckGLError("glDeleteBuffers");
}
typedef void (APIENTRYP PFNGLDELETEFENCESNV)(GLsizei n, const GLuint *fences);
PFNGLDELETEFENCESNV gglDeleteFencesNV;
static PFNGLDELETEFENCESNV _glDeleteFencesNV;
static void APIENTRY d_glDeleteFencesNV(GLsizei n, const GLuint *fences) {
	_glDeleteFencesNV(n, fences);
	CheckGLError("glDeleteFencesNV");
}
typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERS)(GLsizei n, const GLuint *framebuffers);
PFNGLDELETEFRAMEBUFFERS gglDeleteFramebuffers;
static PFNGLDELETEFRAMEBUFFERS _glDeleteFramebuffers;
static void APIENTRY d_glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers) {
	_glDeleteFramebuffers(n, framebuffers);
	CheckGLError("glDeleteFramebuffers");
}
typedef void (APIENTRYP PFNGLDELETEPATHSNV)(GLuint path, GLsizei range);
PFNGLDELETEPATHSNV gglDeletePathsNV;
static PFNGLDELETEPATHSNV _glDeletePathsNV;
static void APIENTRY d_glDeletePathsNV(GLuint path, GLsizei range) {
	_glDeletePathsNV(path, range);
	CheckGLError("glDeletePathsNV");
}
typedef void (APIENTRYP PFNGLDELETEPERFMONITORSAMD)(GLsizei n, GLuint *monitors);
PFNGLDELETEPERFMONITORSAMD gglDeletePerfMonitorsAMD;
static PFNGLDELETEPERFMONITORSAMD _glDeletePerfMonitorsAMD;
static void APIENTRY d_glDeletePerfMonitorsAMD(GLsizei n, GLuint *monitors) {
	_glDeletePerfMonitorsAMD(n, monitors);
	CheckGLError("glDeletePerfMonitorsAMD");
}
typedef void (APIENTRYP PFNGLDELETEPERFQUERYINTEL)(GLuint queryHandle);
PFNGLDELETEPERFQUERYINTEL gglDeletePerfQueryINTEL;
static PFNGLDELETEPERFQUERYINTEL _glDeletePerfQueryINTEL;
static void APIENTRY d_glDeletePerfQueryINTEL(GLuint queryHandle) {
	_glDeletePerfQueryINTEL(queryHandle);
	CheckGLError("glDeletePerfQueryINTEL");
}
typedef void (APIENTRYP PFNGLDELETEPROGRAM)(GLuint program);
PFNGLDELETEPROGRAM gglDeleteProgram;
static PFNGLDELETEPROGRAM _glDeleteProgram;
static void APIENTRY d_glDeleteProgram(GLuint program) {
	_glDeleteProgram(program);
	CheckGLError("glDeleteProgram");
}
typedef void (APIENTRYP PFNGLDELETEPROGRAMPIPELINESEXT)(GLsizei n, const GLuint *pipelines);
PFNGLDELETEPROGRAMPIPELINESEXT gglDeleteProgramPipelinesEXT;
static PFNGLDELETEPROGRAMPIPELINESEXT _glDeleteProgramPipelinesEXT;
static void APIENTRY d_glDeleteProgramPipelinesEXT(GLsizei n, const GLuint *pipelines) {
	_glDeleteProgramPipelinesEXT(n, pipelines);
	CheckGLError("glDeleteProgramPipelinesEXT");
}
typedef void (APIENTRYP PFNGLDELETEQUERIES)(GLsizei n, const GLuint *ids);
PFNGLDELETEQUERIES gglDeleteQueries;
static PFNGLDELETEQUERIES _glDeleteQueries;
static void APIENTRY d_glDeleteQueries(GLsizei n, const GLuint *ids) {
	_glDeleteQueries(n, ids);
	CheckGLError("glDeleteQueries");
}
typedef void (APIENTRYP PFNGLDELETEQUERIESEXT)(GLsizei n, const GLuint *ids);
PFNGLDELETEQUERIESEXT gglDeleteQueriesEXT;
static PFNGLDELETEQUERIESEXT _glDeleteQueriesEXT;
static void APIENTRY d_glDeleteQueriesEXT(GLsizei n, const GLuint *ids) {
	_glDeleteQueriesEXT(n, ids);
	CheckGLError("glDeleteQueriesEXT");
}
typedef void (APIENTRYP PFNGLDELETERENDERBUFFERS)(GLsizei n, const GLuint *renderbuffers);
PFNGLDELETERENDERBUFFERS gglDeleteRenderbuffers;
static PFNGLDELETERENDERBUFFERS _glDeleteRenderbuffers;
static void APIENTRY d_glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers) {
	_glDeleteRenderbuffers(n, renderbuffers);
	CheckGLError("glDeleteRenderbuffers");
}
typedef void (APIENTRYP PFNGLDELETESAMPLERS)(GLsizei count, const GLuint *samplers);
PFNGLDELETESAMPLERS gglDeleteSamplers;
static PFNGLDELETESAMPLERS _glDeleteSamplers;
static void APIENTRY d_glDeleteSamplers(GLsizei count, const GLuint *samplers) {
	_glDeleteSamplers(count, samplers);
	CheckGLError("glDeleteSamplers");
}
typedef void (APIENTRYP PFNGLDELETESHADER)(GLuint shader);
PFNGLDELETESHADER gglDeleteShader;
static PFNGLDELETESHADER _glDeleteShader;
static void APIENTRY d_glDeleteShader(GLuint shader) {
	_glDeleteShader(shader);
	CheckGLError("glDeleteShader");
}
typedef void (APIENTRYP PFNGLDELETESYNC)(GLsync sync);
PFNGLDELETESYNC gglDeleteSync;
static PFNGLDELETESYNC _glDeleteSync;
static void APIENTRY d_glDeleteSync(GLsync sync) {
	_glDeleteSync(sync);
	CheckGLError("glDeleteSync");
}
typedef void (APIENTRYP PFNGLDELETESYNCAPPLE)(GLsync sync);
PFNGLDELETESYNCAPPLE gglDeleteSyncAPPLE;
static PFNGLDELETESYNCAPPLE _glDeleteSyncAPPLE;
static void APIENTRY d_glDeleteSyncAPPLE(GLsync sync) {
	_glDeleteSyncAPPLE(sync);
	CheckGLError("glDeleteSyncAPPLE");
}
typedef void (APIENTRYP PFNGLDELETETEXTURES)(GLsizei n, const GLuint *textures);
PFNGLDELETETEXTURES gglDeleteTextures;
static PFNGLDELETETEXTURES _glDeleteTextures;
static void APIENTRY d_glDeleteTextures(GLsizei n, const GLuint *textures) {
	_glDeleteTextures(n, textures);
	CheckGLError("glDeleteTextures");
}
typedef void (APIENTRYP PFNGLDELETETRANSFORMFEEDBACKS)(GLsizei n, const GLuint *ids);
PFNGLDELETETRANSFORMFEEDBACKS gglDeleteTransformFeedbacks;
static PFNGLDELETETRANSFORMFEEDBACKS _glDeleteTransformFeedbacks;
static void APIENTRY d_glDeleteTransformFeedbacks(GLsizei n, const GLuint *ids) {
	_glDeleteTransformFeedbacks(n, ids);
	CheckGLError("glDeleteTransformFeedbacks");
}
typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYS)(GLsizei n, const GLuint *arrays);
PFNGLDELETEVERTEXARRAYS gglDeleteVertexArrays;
static PFNGLDELETEVERTEXARRAYS _glDeleteVertexArrays;
static void APIENTRY d_glDeleteVertexArrays(GLsizei n, const GLuint *arrays) {
	_glDeleteVertexArrays(n, arrays);
	CheckGLError("glDeleteVertexArrays");
}
typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSOES)(GLsizei n, const GLuint *arrays);
PFNGLDELETEVERTEXARRAYSOES gglDeleteVertexArraysOES;
static PFNGLDELETEVERTEXARRAYSOES _glDeleteVertexArraysOES;
static void APIENTRY d_glDeleteVertexArraysOES(GLsizei n, const GLuint *arrays) {
	_glDeleteVertexArraysOES(n, arrays);
	CheckGLError("glDeleteVertexArraysOES");
}
typedef void (APIENTRYP PFNGLDEPTHFUNC)(GLenum func);
PFNGLDEPTHFUNC gglDepthFunc;
static PFNGLDEPTHFUNC _glDepthFunc;
static void APIENTRY d_glDepthFunc(GLenum func) {
	_glDepthFunc(func);
	CheckGLError("glDepthFunc");
}
typedef void (APIENTRYP PFNGLDEPTHMASK)(GLboolean flag);
PFNGLDEPTHMASK gglDepthMask;
static PFNGLDEPTHMASK _glDepthMask;
static void APIENTRY d_glDepthMask(GLboolean flag) {
	_glDepthMask(flag);
	CheckGLError("glDepthMask");
}
typedef void (APIENTRYP PFNGLDEPTHRANGEARRAYFVNV)(GLuint first, GLsizei count, const GLfloat *v);
PFNGLDEPTHRANGEARRAYFVNV gglDepthRangeArrayfvNV;
static PFNGLDEPTHRANGEARRAYFVNV _glDepthRangeArrayfvNV;
static void APIENTRY d_glDepthRangeArrayfvNV(GLuint first, GLsizei count, const GLfloat *v) {
	_glDepthRangeArrayfvNV(first, count, v);
	CheckGLError("glDepthRangeArrayfvNV");
}
typedef void (APIENTRYP PFNGLDEPTHRANGEARRAYFVOES)(GLuint first, GLsizei count, const GLfloat *v);
PFNGLDEPTHRANGEARRAYFVOES gglDepthRangeArrayfvOES;
static PFNGLDEPTHRANGEARRAYFVOES _glDepthRangeArrayfvOES;
static void APIENTRY d_glDepthRangeArrayfvOES(GLuint first, GLsizei count, const GLfloat *v) {
	_glDepthRangeArrayfvOES(first, count, v);
	CheckGLError("glDepthRangeArrayfvOES");
}
typedef void (APIENTRYP PFNGLDEPTHRANGEINDEXEDFNV)(GLuint index, GLfloat n, GLfloat f);
PFNGLDEPTHRANGEINDEXEDFNV gglDepthRangeIndexedfNV;
static PFNGLDEPTHRANGEINDEXEDFNV _glDepthRangeIndexedfNV;
static void APIENTRY d_glDepthRangeIndexedfNV(GLuint index, GLfloat n, GLfloat f) {
	_glDepthRangeIndexedfNV(index, n, f);
	CheckGLError("glDepthRangeIndexedfNV");
}
typedef void (APIENTRYP PFNGLDEPTHRANGEINDEXEDFOES)(GLuint index, GLfloat n, GLfloat f);
PFNGLDEPTHRANGEINDEXEDFOES gglDepthRangeIndexedfOES;
static PFNGLDEPTHRANGEINDEXEDFOES _glDepthRangeIndexedfOES;
static void APIENTRY d_glDepthRangeIndexedfOES(GLuint index, GLfloat n, GLfloat f) {
	_glDepthRangeIndexedfOES(index, n, f);
	CheckGLError("glDepthRangeIndexedfOES");
}
typedef void (APIENTRYP PFNGLDEPTHRANGEF)(GLfloat n, GLfloat f);
PFNGLDEPTHRANGEF gglDepthRangef;
static PFNGLDEPTHRANGEF _glDepthRangef;
static void APIENTRY d_glDepthRangef(GLfloat n, GLfloat f) {
	_glDepthRangef(n, f);
	CheckGLError("glDepthRangef");
}
typedef void (APIENTRYP PFNGLDETACHSHADER)(GLuint program, GLuint shader);
PFNGLDETACHSHADER gglDetachShader;
static PFNGLDETACHSHADER _glDetachShader;
static void APIENTRY d_glDetachShader(GLuint program, GLuint shader) {
	_glDetachShader(program, shader);
	CheckGLError("glDetachShader");
}
typedef void (APIENTRYP PFNGLDISABLE)(GLenum cap);
PFNGLDISABLE gglDisable;
static PFNGLDISABLE _glDisable;
static void APIENTRY d_glDisable(GLenum cap) {
	_glDisable(cap);
	CheckGLError("glDisable");
}
typedef void (APIENTRYP PFNGLDISABLEDRIVERCONTROLQCOM)(GLuint driverControl);
PFNGLDISABLEDRIVERCONTROLQCOM gglDisableDriverControlQCOM;
static PFNGLDISABLEDRIVERCONTROLQCOM _glDisableDriverControlQCOM;
static void APIENTRY d_glDisableDriverControlQCOM(GLuint driverControl) {
	_glDisableDriverControlQCOM(driverControl);
	CheckGLError("glDisableDriverControlQCOM");
}
typedef void (APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAY)(GLuint index);
PFNGLDISABLEVERTEXATTRIBARRAY gglDisableVertexAttribArray;
static PFNGLDISABLEVERTEXATTRIBARRAY _glDisableVertexAttribArray;
static void APIENTRY d_glDisableVertexAttribArray(GLuint index) {
	_glDisableVertexAttribArray(index);
	CheckGLError("glDisableVertexAttribArray");
}
typedef void (APIENTRYP PFNGLDISABLEIEXT)(GLenum target, GLuint index);
PFNGLDISABLEIEXT gglDisableiEXT;
static PFNGLDISABLEIEXT _glDisableiEXT;
static void APIENTRY d_glDisableiEXT(GLenum target, GLuint index) {
	_glDisableiEXT(target, index);
	CheckGLError("glDisableiEXT");
}
typedef void (APIENTRYP PFNGLDISABLEINV)(GLenum target, GLuint index);
PFNGLDISABLEINV gglDisableiNV;
static PFNGLDISABLEINV _glDisableiNV;
static void APIENTRY d_glDisableiNV(GLenum target, GLuint index) {
	_glDisableiNV(target, index);
	CheckGLError("glDisableiNV");
}
typedef void (APIENTRYP PFNGLDISABLEIOES)(GLenum target, GLuint index);
PFNGLDISABLEIOES gglDisableiOES;
static PFNGLDISABLEIOES _glDisableiOES;
static void APIENTRY d_glDisableiOES(GLenum target, GLuint index) {
	_glDisableiOES(target, index);
	CheckGLError("glDisableiOES");
}
typedef void (APIENTRYP PFNGLDISCARDFRAMEBUFFEREXT)(GLenum target, GLsizei numAttachments, const GLenum *attachments);
PFNGLDISCARDFRAMEBUFFEREXT gglDiscardFramebufferEXT;
static PFNGLDISCARDFRAMEBUFFEREXT _glDiscardFramebufferEXT;
static void APIENTRY d_glDiscardFramebufferEXT(GLenum target, GLsizei numAttachments, const GLenum *attachments) {
	_glDiscardFramebufferEXT(target, numAttachments, attachments);
	CheckGLError("glDiscardFramebufferEXT");
}
typedef void (APIENTRYP PFNGLDRAWARRAYS)(GLenum mode, GLint first, GLsizei count);
PFNGLDRAWARRAYS gglDrawArrays;
static PFNGLDRAWARRAYS _glDrawArrays;
static void APIENTRY d_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
	_glDrawArrays(mode, first, count);
	CheckGLError("glDrawArrays");
}
typedef void (APIENTRYP PFNGLDRAWARRAYSINSTANCED)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
PFNGLDRAWARRAYSINSTANCED gglDrawArraysInstanced;
static PFNGLDRAWARRAYSINSTANCED _glDrawArraysInstanced;
static void APIENTRY d_glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount) {
	_glDrawArraysInstanced(mode, first, count, instancecount);
	CheckGLError("glDrawArraysInstanced");
}
typedef void (APIENTRYP PFNGLDRAWARRAYSINSTANCEDANGLE)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
PFNGLDRAWARRAYSINSTANCEDANGLE gglDrawArraysInstancedANGLE;
static PFNGLDRAWARRAYSINSTANCEDANGLE _glDrawArraysInstancedANGLE;
static void APIENTRY d_glDrawArraysInstancedANGLE(GLenum mode, GLint first, GLsizei count, GLsizei primcount) {
	_glDrawArraysInstancedANGLE(mode, first, count, primcount);
	CheckGLError("glDrawArraysInstancedANGLE");
}
typedef void (APIENTRYP PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEEXT)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEEXT gglDrawArraysInstancedBaseInstanceEXT;
static PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEEXT _glDrawArraysInstancedBaseInstanceEXT;
static void APIENTRY d_glDrawArraysInstancedBaseInstanceEXT(GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance) {
	_glDrawArraysInstancedBaseInstanceEXT(mode, first, count, instancecount, baseinstance);
	CheckGLError("glDrawArraysInstancedBaseInstanceEXT");
}
typedef void (APIENTRYP PFNGLDRAWARRAYSINSTANCEDEXT)(GLenum mode, GLint start, GLsizei count, GLsizei primcount);
PFNGLDRAWARRAYSINSTANCEDEXT gglDrawArraysInstancedEXT;
static PFNGLDRAWARRAYSINSTANCEDEXT _glDrawArraysInstancedEXT;
static void APIENTRY d_glDrawArraysInstancedEXT(GLenum mode, GLint start, GLsizei count, GLsizei primcount) {
	_glDrawArraysInstancedEXT(mode, start, count, primcount);
	CheckGLError("glDrawArraysInstancedEXT");
}
typedef void (APIENTRYP PFNGLDRAWARRAYSINSTANCEDNV)(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
PFNGLDRAWARRAYSINSTANCEDNV gglDrawArraysInstancedNV;
static PFNGLDRAWARRAYSINSTANCEDNV _glDrawArraysInstancedNV;
static void APIENTRY d_glDrawArraysInstancedNV(GLenum mode, GLint first, GLsizei count, GLsizei primcount) {
	_glDrawArraysInstancedNV(mode, first, count, primcount);
	CheckGLError("glDrawArraysInstancedNV");
}
typedef void (APIENTRYP PFNGLDRAWBUFFERS)(GLsizei n, const GLenum *bufs);
PFNGLDRAWBUFFERS gglDrawBuffers;
static PFNGLDRAWBUFFERS _glDrawBuffers;
static void APIENTRY d_glDrawBuffers(GLsizei n, const GLenum *bufs) {
	_glDrawBuffers(n, bufs);
	CheckGLError("glDrawBuffers");
}
typedef void (APIENTRYP PFNGLDRAWBUFFERSEXT)(GLsizei n, const GLenum *bufs);
PFNGLDRAWBUFFERSEXT gglDrawBuffersEXT;
static PFNGLDRAWBUFFERSEXT _glDrawBuffersEXT;
static void APIENTRY d_glDrawBuffersEXT(GLsizei n, const GLenum *bufs) {
	_glDrawBuffersEXT(n, bufs);
	CheckGLError("glDrawBuffersEXT");
}
typedef void (APIENTRYP PFNGLDRAWBUFFERSINDEXEDEXT)(GLint n, const GLenum *location, const GLint *indices);
PFNGLDRAWBUFFERSINDEXEDEXT gglDrawBuffersIndexedEXT;
static PFNGLDRAWBUFFERSINDEXEDEXT _glDrawBuffersIndexedEXT;
static void APIENTRY d_glDrawBuffersIndexedEXT(GLint n, const GLenum *location, const GLint *indices) {
	_glDrawBuffersIndexedEXT(n, location, indices);
	CheckGLError("glDrawBuffersIndexedEXT");
}
typedef void (APIENTRYP PFNGLDRAWBUFFERSNV)(GLsizei n, const GLenum *bufs);
PFNGLDRAWBUFFERSNV gglDrawBuffersNV;
static PFNGLDRAWBUFFERSNV _glDrawBuffersNV;
static void APIENTRY d_glDrawBuffersNV(GLsizei n, const GLenum *bufs) {
	_glDrawBuffersNV(n, bufs);
	CheckGLError("glDrawBuffersNV");
}
typedef void (APIENTRYP PFNGLDRAWELEMENTS)(GLenum mode, GLsizei count, GLenum type, const void *indices);
PFNGLDRAWELEMENTS gglDrawElements;
static PFNGLDRAWELEMENTS _glDrawElements;
static void APIENTRY d_glDrawElements(GLenum mode, GLsizei count, GLenum type, const void *indices) {
	_glDrawElements(mode, count, type, indices);
	CheckGLError("glDrawElements");
}
typedef void (APIENTRYP PFNGLDRAWELEMENTSBASEVERTEXEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
PFNGLDRAWELEMENTSBASEVERTEXEXT gglDrawElementsBaseVertexEXT;
static PFNGLDRAWELEMENTSBASEVERTEXEXT _glDrawElementsBaseVertexEXT;
static void APIENTRY d_glDrawElementsBaseVertexEXT(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
	_glDrawElementsBaseVertexEXT(mode, count, type, indices, basevertex);
	CheckGLError("glDrawElementsBaseVertexEXT");
}
typedef void (APIENTRYP PFNGLDRAWELEMENTSBASEVERTEXOES)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
PFNGLDRAWELEMENTSBASEVERTEXOES gglDrawElementsBaseVertexOES;
static PFNGLDRAWELEMENTSBASEVERTEXOES _glDrawElementsBaseVertexOES;
static void APIENTRY d_glDrawElementsBaseVertexOES(GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
	_glDrawElementsBaseVertexOES(mode, count, type, indices, basevertex);
	CheckGLError("glDrawElementsBaseVertexOES");
}
typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCED)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
PFNGLDRAWELEMENTSINSTANCED gglDrawElementsInstanced;
static PFNGLDRAWELEMENTSINSTANCED _glDrawElementsInstanced;
static void APIENTRY d_glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount) {
	_glDrawElementsInstanced(mode, count, type, indices, instancecount);
	CheckGLError("glDrawElementsInstanced");
}
typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCEDANGLE)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
PFNGLDRAWELEMENTSINSTANCEDANGLE gglDrawElementsInstancedANGLE;
static PFNGLDRAWELEMENTSINSTANCEDANGLE _glDrawElementsInstancedANGLE;
static void APIENTRY d_glDrawElementsInstancedANGLE(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount) {
	_glDrawElementsInstancedANGLE(mode, count, type, indices, primcount);
	CheckGLError("glDrawElementsInstancedANGLE");
}
typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance);
PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEEXT gglDrawElementsInstancedBaseInstanceEXT;
static PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEEXT _glDrawElementsInstancedBaseInstanceEXT;
static void APIENTRY d_glDrawElementsInstancedBaseInstanceEXT(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance) {
	_glDrawElementsInstancedBaseInstanceEXT(mode, count, type, indices, instancecount, baseinstance);
	CheckGLError("glDrawElementsInstancedBaseInstanceEXT");
}
typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance);
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEEXT gglDrawElementsInstancedBaseVertexBaseInstanceEXT;
static PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEEXT _glDrawElementsInstancedBaseVertexBaseInstanceEXT;
static void APIENTRY d_glDrawElementsInstancedBaseVertexBaseInstanceEXT(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance) {
	_glDrawElementsInstancedBaseVertexBaseInstanceEXT(mode, count, type, indices, instancecount, basevertex, baseinstance);
	CheckGLError("glDrawElementsInstancedBaseVertexBaseInstanceEXT");
}
typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex);
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXEXT gglDrawElementsInstancedBaseVertexEXT;
static PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXEXT _glDrawElementsInstancedBaseVertexEXT;
static void APIENTRY d_glDrawElementsInstancedBaseVertexEXT(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex) {
	_glDrawElementsInstancedBaseVertexEXT(mode, count, type, indices, instancecount, basevertex);
	CheckGLError("glDrawElementsInstancedBaseVertexEXT");
}
typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXOES)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex);
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXOES gglDrawElementsInstancedBaseVertexOES;
static PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXOES _glDrawElementsInstancedBaseVertexOES;
static void APIENTRY d_glDrawElementsInstancedBaseVertexOES(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex) {
	_glDrawElementsInstancedBaseVertexOES(mode, count, type, indices, instancecount, basevertex);
	CheckGLError("glDrawElementsInstancedBaseVertexOES");
}
typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCEDEXT)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
PFNGLDRAWELEMENTSINSTANCEDEXT gglDrawElementsInstancedEXT;
static PFNGLDRAWELEMENTSINSTANCEDEXT _glDrawElementsInstancedEXT;
static void APIENTRY d_glDrawElementsInstancedEXT(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount) {
	_glDrawElementsInstancedEXT(mode, count, type, indices, primcount);
	CheckGLError("glDrawElementsInstancedEXT");
}
typedef void (APIENTRYP PFNGLDRAWELEMENTSINSTANCEDNV)(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
PFNGLDRAWELEMENTSINSTANCEDNV gglDrawElementsInstancedNV;
static PFNGLDRAWELEMENTSINSTANCEDNV _glDrawElementsInstancedNV;
static void APIENTRY d_glDrawElementsInstancedNV(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount) {
	_glDrawElementsInstancedNV(mode, count, type, indices, primcount);
	CheckGLError("glDrawElementsInstancedNV");
}
typedef void (APIENTRYP PFNGLDRAWRANGEELEMENTS)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
PFNGLDRAWRANGEELEMENTS gglDrawRangeElements;
static PFNGLDRAWRANGEELEMENTS _glDrawRangeElements;
static void APIENTRY d_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices) {
	_glDrawRangeElements(mode, start, end, count, type, indices);
	CheckGLError("glDrawRangeElements");
}
typedef void (APIENTRYP PFNGLDRAWRANGEELEMENTSBASEVERTEXEXT)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex);
PFNGLDRAWRANGEELEMENTSBASEVERTEXEXT gglDrawRangeElementsBaseVertexEXT;
static PFNGLDRAWRANGEELEMENTSBASEVERTEXEXT _glDrawRangeElementsBaseVertexEXT;
static void APIENTRY d_glDrawRangeElementsBaseVertexEXT(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
	_glDrawRangeElementsBaseVertexEXT(mode, start, end, count, type, indices, basevertex);
	CheckGLError("glDrawRangeElementsBaseVertexEXT");
}
typedef void (APIENTRYP PFNGLDRAWRANGEELEMENTSBASEVERTEXOES)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex);
PFNGLDRAWRANGEELEMENTSBASEVERTEXOES gglDrawRangeElementsBaseVertexOES;
static PFNGLDRAWRANGEELEMENTSBASEVERTEXOES _glDrawRangeElementsBaseVertexOES;
static void APIENTRY d_glDrawRangeElementsBaseVertexOES(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex) {
	_glDrawRangeElementsBaseVertexOES(mode, start, end, count, type, indices, basevertex);
	CheckGLError("glDrawRangeElementsBaseVertexOES");
}
typedef void (APIENTRYP PFNGLDRAWTRANSFORMFEEDBACKEXT)(GLenum mode, GLuint id);
PFNGLDRAWTRANSFORMFEEDBACKEXT gglDrawTransformFeedbackEXT;
static PFNGLDRAWTRANSFORMFEEDBACKEXT _glDrawTransformFeedbackEXT;
static void APIENTRY d_glDrawTransformFeedbackEXT(GLenum mode, GLuint id) {
	_glDrawTransformFeedbackEXT(mode, id);
	CheckGLError("glDrawTransformFeedbackEXT");
}
typedef void (APIENTRYP PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDEXT)(GLenum mode, GLuint id, GLsizei instancecount);
PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDEXT gglDrawTransformFeedbackInstancedEXT;
static PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDEXT _glDrawTransformFeedbackInstancedEXT;
static void APIENTRY d_glDrawTransformFeedbackInstancedEXT(GLenum mode, GLuint id, GLsizei instancecount) {
	_glDrawTransformFeedbackInstancedEXT(mode, id, instancecount);
	CheckGLError("glDrawTransformFeedbackInstancedEXT");
}
typedef void (APIENTRYP PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOES)(GLenum target, GLeglImageOES image);
PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOES gglEGLImageTargetRenderbufferStorageOES;
static PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOES _glEGLImageTargetRenderbufferStorageOES;
static void APIENTRY d_glEGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image) {
	_glEGLImageTargetRenderbufferStorageOES(target, image);
	CheckGLError("glEGLImageTargetRenderbufferStorageOES");
}
typedef void (APIENTRYP PFNGLEGLIMAGETARGETTEXTURE2DOES)(GLenum target, GLeglImageOES image);
PFNGLEGLIMAGETARGETTEXTURE2DOES gglEGLImageTargetTexture2DOES;
static PFNGLEGLIMAGETARGETTEXTURE2DOES _glEGLImageTargetTexture2DOES;
static void APIENTRY d_glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image) {
	_glEGLImageTargetTexture2DOES(target, image);
	CheckGLError("glEGLImageTargetTexture2DOES");
}
typedef void (APIENTRYP PFNGLENABLE)(GLenum cap);
PFNGLENABLE gglEnable;
static PFNGLENABLE _glEnable;
static void APIENTRY d_glEnable(GLenum cap) {
	_glEnable(cap);
	CheckGLError("glEnable");
}
typedef void (APIENTRYP PFNGLENABLEDRIVERCONTROLQCOM)(GLuint driverControl);
PFNGLENABLEDRIVERCONTROLQCOM gglEnableDriverControlQCOM;
static PFNGLENABLEDRIVERCONTROLQCOM _glEnableDriverControlQCOM;
static void APIENTRY d_glEnableDriverControlQCOM(GLuint driverControl) {
	_glEnableDriverControlQCOM(driverControl);
	CheckGLError("glEnableDriverControlQCOM");
}
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAY)(GLuint index);
PFNGLENABLEVERTEXATTRIBARRAY gglEnableVertexAttribArray;
static PFNGLENABLEVERTEXATTRIBARRAY _glEnableVertexAttribArray;
static void APIENTRY d_glEnableVertexAttribArray(GLuint index) {
	_glEnableVertexAttribArray(index);
	CheckGLError("glEnableVertexAttribArray");
}
typedef void (APIENTRYP PFNGLENABLEIEXT)(GLenum target, GLuint index);
PFNGLENABLEIEXT gglEnableiEXT;
static PFNGLENABLEIEXT _glEnableiEXT;
static void APIENTRY d_glEnableiEXT(GLenum target, GLuint index) {
	_glEnableiEXT(target, index);
	CheckGLError("glEnableiEXT");
}
typedef void (APIENTRYP PFNGLENABLEINV)(GLenum target, GLuint index);
PFNGLENABLEINV gglEnableiNV;
static PFNGLENABLEINV _glEnableiNV;
static void APIENTRY d_glEnableiNV(GLenum target, GLuint index) {
	_glEnableiNV(target, index);
	CheckGLError("glEnableiNV");
}
typedef void (APIENTRYP PFNGLENABLEIOES)(GLenum target, GLuint index);
PFNGLENABLEIOES gglEnableiOES;
static PFNGLENABLEIOES _glEnableiOES;
static void APIENTRY d_glEnableiOES(GLenum target, GLuint index) {
	_glEnableiOES(target, index);
	CheckGLError("glEnableiOES");
}
typedef void (APIENTRYP PFNGLENDCONDITIONALRENDERNV)();
PFNGLENDCONDITIONALRENDERNV gglEndConditionalRenderNV;
static PFNGLENDCONDITIONALRENDERNV _glEndConditionalRenderNV;
static void APIENTRY d_glEndConditionalRenderNV() {
	_glEndConditionalRenderNV();
	CheckGLError("glEndConditionalRenderNV");
}
typedef void (APIENTRYP PFNGLENDPERFMONITORAMD)(GLuint monitor);
PFNGLENDPERFMONITORAMD gglEndPerfMonitorAMD;
static PFNGLENDPERFMONITORAMD _glEndPerfMonitorAMD;
static void APIENTRY d_glEndPerfMonitorAMD(GLuint monitor) {
	_glEndPerfMonitorAMD(monitor);
	CheckGLError("glEndPerfMonitorAMD");
}
typedef void (APIENTRYP PFNGLENDPERFQUERYINTEL)(GLuint queryHandle);
PFNGLENDPERFQUERYINTEL gglEndPerfQueryINTEL;
static PFNGLENDPERFQUERYINTEL _glEndPerfQueryINTEL;
static void APIENTRY d_glEndPerfQueryINTEL(GLuint queryHandle) {
	_glEndPerfQueryINTEL(queryHandle);
	CheckGLError("glEndPerfQueryINTEL");
}
typedef void (APIENTRYP PFNGLENDQUERY)(GLenum target);
PFNGLENDQUERY gglEndQuery;
static PFNGLENDQUERY _glEndQuery;
static void APIENTRY d_glEndQuery(GLenum target) {
	_glEndQuery(target);
	CheckGLError("glEndQuery");
}
typedef void (APIENTRYP PFNGLENDQUERYEXT)(GLenum target);
PFNGLENDQUERYEXT gglEndQueryEXT;
static PFNGLENDQUERYEXT _glEndQueryEXT;
static void APIENTRY d_glEndQueryEXT(GLenum target) {
	_glEndQueryEXT(target);
	CheckGLError("glEndQueryEXT");
}
typedef void (APIENTRYP PFNGLENDTILINGQCOM)(GLbitfield preserveMask);
PFNGLENDTILINGQCOM gglEndTilingQCOM;
static PFNGLENDTILINGQCOM _glEndTilingQCOM;
static void APIENTRY d_glEndTilingQCOM(GLbitfield preserveMask) {
	_glEndTilingQCOM(preserveMask);
	CheckGLError("glEndTilingQCOM");
}
typedef void (APIENTRYP PFNGLENDTRANSFORMFEEDBACK)();
PFNGLENDTRANSFORMFEEDBACK gglEndTransformFeedback;
static PFNGLENDTRANSFORMFEEDBACK _glEndTransformFeedback;
static void APIENTRY d_glEndTransformFeedback() {
	_glEndTransformFeedback();
	CheckGLError("glEndTransformFeedback");
}
typedef void (APIENTRYP PFNGLEXTGETBUFFERPOINTERVQCOM)(GLenum target, void **params);
PFNGLEXTGETBUFFERPOINTERVQCOM gglExtGetBufferPointervQCOM;
static PFNGLEXTGETBUFFERPOINTERVQCOM _glExtGetBufferPointervQCOM;
static void APIENTRY d_glExtGetBufferPointervQCOM(GLenum target, void **params) {
	_glExtGetBufferPointervQCOM(target, params);
	CheckGLError("glExtGetBufferPointervQCOM");
}
typedef void (APIENTRYP PFNGLEXTGETBUFFERSQCOM)(GLuint *buffers, GLint maxBuffers, GLint *numBuffers);
PFNGLEXTGETBUFFERSQCOM gglExtGetBuffersQCOM;
static PFNGLEXTGETBUFFERSQCOM _glExtGetBuffersQCOM;
static void APIENTRY d_glExtGetBuffersQCOM(GLuint *buffers, GLint maxBuffers, GLint *numBuffers) {
	_glExtGetBuffersQCOM(buffers, maxBuffers, numBuffers);
	CheckGLError("glExtGetBuffersQCOM");
}
typedef void (APIENTRYP PFNGLEXTGETFRAMEBUFFERSQCOM)(GLuint *framebuffers, GLint maxFramebuffers, GLint *numFramebuffers);
PFNGLEXTGETFRAMEBUFFERSQCOM gglExtGetFramebuffersQCOM;
static PFNGLEXTGETFRAMEBUFFERSQCOM _glExtGetFramebuffersQCOM;
static void APIENTRY d_glExtGetFramebuffersQCOM(GLuint *framebuffers, GLint maxFramebuffers, GLint *numFramebuffers) {
	_glExtGetFramebuffersQCOM(framebuffers, maxFramebuffers, numFramebuffers);
	CheckGLError("glExtGetFramebuffersQCOM");
}
typedef void (APIENTRYP PFNGLEXTGETPROGRAMBINARYSOURCEQCOM)(GLuint program, GLenum shadertype, GLchar *source, GLint *length);
PFNGLEXTGETPROGRAMBINARYSOURCEQCOM gglExtGetProgramBinarySourceQCOM;
static PFNGLEXTGETPROGRAMBINARYSOURCEQCOM _glExtGetProgramBinarySourceQCOM;
static void APIENTRY d_glExtGetProgramBinarySourceQCOM(GLuint program, GLenum shadertype, GLchar *source, GLint *length) {
	_glExtGetProgramBinarySourceQCOM(program, shadertype, source, length);
	CheckGLError("glExtGetProgramBinarySourceQCOM");
}
typedef void (APIENTRYP PFNGLEXTGETPROGRAMSQCOM)(GLuint *programs, GLint maxPrograms, GLint *numPrograms);
PFNGLEXTGETPROGRAMSQCOM gglExtGetProgramsQCOM;
static PFNGLEXTGETPROGRAMSQCOM _glExtGetProgramsQCOM;
static void APIENTRY d_glExtGetProgramsQCOM(GLuint *programs, GLint maxPrograms, GLint *numPrograms) {
	_glExtGetProgramsQCOM(programs, maxPrograms, numPrograms);
	CheckGLError("glExtGetProgramsQCOM");
}
typedef void (APIENTRYP PFNGLEXTGETRENDERBUFFERSQCOM)(GLuint *renderbuffers, GLint maxRenderbuffers, GLint *numRenderbuffers);
PFNGLEXTGETRENDERBUFFERSQCOM gglExtGetRenderbuffersQCOM;
static PFNGLEXTGETRENDERBUFFERSQCOM _glExtGetRenderbuffersQCOM;
static void APIENTRY d_glExtGetRenderbuffersQCOM(GLuint *renderbuffers, GLint maxRenderbuffers, GLint *numRenderbuffers) {
	_glExtGetRenderbuffersQCOM(renderbuffers, maxRenderbuffers, numRenderbuffers);
	CheckGLError("glExtGetRenderbuffersQCOM");
}
typedef void (APIENTRYP PFNGLEXTGETSHADERSQCOM)(GLuint *shaders, GLint maxShaders, GLint *numShaders);
PFNGLEXTGETSHADERSQCOM gglExtGetShadersQCOM;
static PFNGLEXTGETSHADERSQCOM _glExtGetShadersQCOM;
static void APIENTRY d_glExtGetShadersQCOM(GLuint *shaders, GLint maxShaders, GLint *numShaders) {
	_glExtGetShadersQCOM(shaders, maxShaders, numShaders);
	CheckGLError("glExtGetShadersQCOM");
}
typedef void (APIENTRYP PFNGLEXTGETTEXLEVELPARAMETERIVQCOM)(GLuint texture, GLenum face, GLint level, GLenum pname, GLint *params);
PFNGLEXTGETTEXLEVELPARAMETERIVQCOM gglExtGetTexLevelParameterivQCOM;
static PFNGLEXTGETTEXLEVELPARAMETERIVQCOM _glExtGetTexLevelParameterivQCOM;
static void APIENTRY d_glExtGetTexLevelParameterivQCOM(GLuint texture, GLenum face, GLint level, GLenum pname, GLint *params) {
	_glExtGetTexLevelParameterivQCOM(texture, face, level, pname, params);
	CheckGLError("glExtGetTexLevelParameterivQCOM");
}
typedef void (APIENTRYP PFNGLEXTGETTEXSUBIMAGEQCOM)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, void *texels);
PFNGLEXTGETTEXSUBIMAGEQCOM gglExtGetTexSubImageQCOM;
static PFNGLEXTGETTEXSUBIMAGEQCOM _glExtGetTexSubImageQCOM;
static void APIENTRY d_glExtGetTexSubImageQCOM(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, void *texels) {
	_glExtGetTexSubImageQCOM(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, texels);
	CheckGLError("glExtGetTexSubImageQCOM");
}
typedef void (APIENTRYP PFNGLEXTGETTEXTURESQCOM)(GLuint *textures, GLint maxTextures, GLint *numTextures);
PFNGLEXTGETTEXTURESQCOM gglExtGetTexturesQCOM;
static PFNGLEXTGETTEXTURESQCOM _glExtGetTexturesQCOM;
static void APIENTRY d_glExtGetTexturesQCOM(GLuint *textures, GLint maxTextures, GLint *numTextures) {
	_glExtGetTexturesQCOM(textures, maxTextures, numTextures);
	CheckGLError("glExtGetTexturesQCOM");
}
typedef GLboolean (APIENTRYP PFNGLEXTISPROGRAMBINARYQCOM)(GLuint program);
PFNGLEXTISPROGRAMBINARYQCOM gglExtIsProgramBinaryQCOM;
static PFNGLEXTISPROGRAMBINARYQCOM _glExtIsProgramBinaryQCOM;
static GLboolean APIENTRY d_glExtIsProgramBinaryQCOM(GLuint program) {
	GLboolean ret = _glExtIsProgramBinaryQCOM(program);
	CheckGLError("glExtIsProgramBinaryQCOM");
	return ret;
}
typedef void (APIENTRYP PFNGLEXTTEXOBJECTSTATEOVERRIDEIQCOM)(GLenum target, GLenum pname, GLint param);
PFNGLEXTTEXOBJECTSTATEOVERRIDEIQCOM gglExtTexObjectStateOverrideiQCOM;
static PFNGLEXTTEXOBJECTSTATEOVERRIDEIQCOM _glExtTexObjectStateOverrideiQCOM;
static void APIENTRY d_glExtTexObjectStateOverrideiQCOM(GLenum target, GLenum pname, GLint param) {
	_glExtTexObjectStateOverrideiQCOM(target, pname, param);
	CheckGLError("glExtTexObjectStateOverrideiQCOM");
}
typedef GLsync (APIENTRYP PFNGLFENCESYNC)(GLenum condition, GLbitfield flags);
PFNGLFENCESYNC gglFenceSync;
static PFNGLFENCESYNC _glFenceSync;
static GLsync APIENTRY d_glFenceSync(GLenum condition, GLbitfield flags) {
	GLsync ret = _glFenceSync(condition, flags);
	CheckGLError("glFenceSync");
	return ret;
}
typedef GLsync (APIENTRYP PFNGLFENCESYNCAPPLE)(GLenum condition, GLbitfield flags);
PFNGLFENCESYNCAPPLE gglFenceSyncAPPLE;
static PFNGLFENCESYNCAPPLE _glFenceSyncAPPLE;
static GLsync APIENTRY d_glFenceSyncAPPLE(GLenum condition, GLbitfield flags) {
	GLsync ret = _glFenceSyncAPPLE(condition, flags);
	CheckGLError("glFenceSyncAPPLE");
	return ret;
}
typedef void (APIENTRYP PFNGLFINISH)();
PFNGLFINISH gglFinish;
static PFNGLFINISH _glFinish;
static void APIENTRY d_glFinish() {
	_glFinish();
	CheckGLError("glFinish");
}
typedef void (APIENTRYP PFNGLFINISHFENCENV)(GLuint fence);
PFNGLFINISHFENCENV gglFinishFenceNV;
static PFNGLFINISHFENCENV _glFinishFenceNV;
static void APIENTRY d_glFinishFenceNV(GLuint fence) {
	_glFinishFenceNV(fence);
	CheckGLError("glFinishFenceNV");
}
typedef void (APIENTRYP PFNGLFLUSH)();
PFNGLFLUSH gglFlush;
static PFNGLFLUSH _glFlush;
static void APIENTRY d_glFlush() {
	_glFlush();
	CheckGLError("glFlush");
}
typedef void (APIENTRYP PFNGLFLUSHMAPPEDBUFFERRANGE)(GLenum target, GLintptr offset, GLsizeiptr length);
PFNGLFLUSHMAPPEDBUFFERRANGE gglFlushMappedBufferRange;
static PFNGLFLUSHMAPPEDBUFFERRANGE _glFlushMappedBufferRange;
static void APIENTRY d_glFlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length) {
	_glFlushMappedBufferRange(target, offset, length);
	CheckGLError("glFlushMappedBufferRange");
}
typedef void (APIENTRYP PFNGLFLUSHMAPPEDBUFFERRANGEEXT)(GLenum target, GLintptr offset, GLsizeiptr length);
PFNGLFLUSHMAPPEDBUFFERRANGEEXT gglFlushMappedBufferRangeEXT;
static PFNGLFLUSHMAPPEDBUFFERRANGEEXT _glFlushMappedBufferRangeEXT;
static void APIENTRY d_glFlushMappedBufferRangeEXT(GLenum target, GLintptr offset, GLsizeiptr length) {
	_glFlushMappedBufferRangeEXT(target, offset, length);
	CheckGLError("glFlushMappedBufferRangeEXT");
}
typedef void (APIENTRYP PFNGLFRAGMENTCOVERAGECOLORNV)(GLuint color);
PFNGLFRAGMENTCOVERAGECOLORNV gglFragmentCoverageColorNV;
static PFNGLFRAGMENTCOVERAGECOLORNV _glFragmentCoverageColorNV;
static void APIENTRY d_glFragmentCoverageColorNV(GLuint color) {
	_glFragmentCoverageColorNV(color);
	CheckGLError("glFragmentCoverageColorNV");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERPIXELLOCALSTORAGESIZEEXT)(GLuint target, GLsizei size);
PFNGLFRAMEBUFFERPIXELLOCALSTORAGESIZEEXT gglFramebufferPixelLocalStorageSizeEXT;
static PFNGLFRAMEBUFFERPIXELLOCALSTORAGESIZEEXT _glFramebufferPixelLocalStorageSizeEXT;
static void APIENTRY d_glFramebufferPixelLocalStorageSizeEXT(GLuint target, GLsizei size) {
	_glFramebufferPixelLocalStorageSizeEXT(target, size);
	CheckGLError("glFramebufferPixelLocalStorageSizeEXT");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFER)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
PFNGLFRAMEBUFFERRENDERBUFFER gglFramebufferRenderbuffer;
static PFNGLFRAMEBUFFERRENDERBUFFER _glFramebufferRenderbuffer;
static void APIENTRY d_glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
	_glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
	CheckGLError("glFramebufferRenderbuffer");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNV)(GLenum target, GLuint start, GLsizei count, const GLfloat *v);
PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNV gglFramebufferSampleLocationsfvNV;
static PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNV _glFramebufferSampleLocationsfvNV;
static void APIENTRY d_glFramebufferSampleLocationsfvNV(GLenum target, GLuint start, GLsizei count, const GLfloat *v) {
	_glFramebufferSampleLocationsfvNV(target, start, count, v);
	CheckGLError("glFramebufferSampleLocationsfvNV");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
PFNGLFRAMEBUFFERTEXTURE2D gglFramebufferTexture2D;
static PFNGLFRAMEBUFFERTEXTURE2D _glFramebufferTexture2D;
static void APIENTRY d_glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
	_glFramebufferTexture2D(target, attachment, textarget, texture, level);
	CheckGLError("glFramebufferTexture2D");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DDOWNSAMPLEIMG)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint xscale, GLint yscale);
PFNGLFRAMEBUFFERTEXTURE2DDOWNSAMPLEIMG gglFramebufferTexture2DDownsampleIMG;
static PFNGLFRAMEBUFFERTEXTURE2DDOWNSAMPLEIMG _glFramebufferTexture2DDownsampleIMG;
static void APIENTRY d_glFramebufferTexture2DDownsampleIMG(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint xscale, GLint yscale) {
	_glFramebufferTexture2DDownsampleIMG(target, attachment, textarget, texture, level, xscale, yscale);
	CheckGLError("glFramebufferTexture2DDownsampleIMG");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXT gglFramebufferTexture2DMultisampleEXT;
static PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXT _glFramebufferTexture2DMultisampleEXT;
static void APIENTRY d_glFramebufferTexture2DMultisampleEXT(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples) {
	_glFramebufferTexture2DMultisampleEXT(target, attachment, textarget, texture, level, samples);
	CheckGLError("glFramebufferTexture2DMultisampleEXT");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);
PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG gglFramebufferTexture2DMultisampleIMG;
static PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG _glFramebufferTexture2DMultisampleIMG;
static void APIENTRY d_glFramebufferTexture2DMultisampleIMG(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples) {
	_glFramebufferTexture2DMultisampleIMG(target, attachment, textarget, texture, level, samples);
	CheckGLError("glFramebufferTexture2DMultisampleIMG");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE3DOES)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
PFNGLFRAMEBUFFERTEXTURE3DOES gglFramebufferTexture3DOES;
static PFNGLFRAMEBUFFERTEXTURE3DOES _glFramebufferTexture3DOES;
static void APIENTRY d_glFramebufferTexture3DOES(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) {
	_glFramebufferTexture3DOES(target, attachment, textarget, texture, level, zoffset);
	CheckGLError("glFramebufferTexture3DOES");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTUREEXT)(GLenum target, GLenum attachment, GLuint texture, GLint level);
PFNGLFRAMEBUFFERTEXTUREEXT gglFramebufferTextureEXT;
static PFNGLFRAMEBUFFERTEXTUREEXT _glFramebufferTextureEXT;
static void APIENTRY d_glFramebufferTextureEXT(GLenum target, GLenum attachment, GLuint texture, GLint level) {
	_glFramebufferTextureEXT(target, attachment, texture, level);
	CheckGLError("glFramebufferTextureEXT");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURELAYER)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
PFNGLFRAMEBUFFERTEXTURELAYER gglFramebufferTextureLayer;
static PFNGLFRAMEBUFFERTEXTURELAYER _glFramebufferTextureLayer;
static void APIENTRY d_glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
	_glFramebufferTextureLayer(target, attachment, texture, level, layer);
	CheckGLError("glFramebufferTextureLayer");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURELAYERDOWNSAMPLEIMG)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer, GLint xscale, GLint yscale);
PFNGLFRAMEBUFFERTEXTURELAYERDOWNSAMPLEIMG gglFramebufferTextureLayerDownsampleIMG;
static PFNGLFRAMEBUFFERTEXTURELAYERDOWNSAMPLEIMG _glFramebufferTextureLayerDownsampleIMG;
static void APIENTRY d_glFramebufferTextureLayerDownsampleIMG(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer, GLint xscale, GLint yscale) {
	_glFramebufferTextureLayerDownsampleIMG(target, attachment, texture, level, layer, xscale, yscale);
	CheckGLError("glFramebufferTextureLayerDownsampleIMG");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVR)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLsizei samples, GLint baseViewIndex, GLsizei numViews);
PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVR gglFramebufferTextureMultisampleMultiviewOVR;
static PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVR _glFramebufferTextureMultisampleMultiviewOVR;
static void APIENTRY d_glFramebufferTextureMultisampleMultiviewOVR(GLenum target, GLenum attachment, GLuint texture, GLint level, GLsizei samples, GLint baseViewIndex, GLsizei numViews) {
	_glFramebufferTextureMultisampleMultiviewOVR(target, attachment, texture, level, samples, baseViewIndex, numViews);
	CheckGLError("glFramebufferTextureMultisampleMultiviewOVR");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVR)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews);
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVR gglFramebufferTextureMultiviewOVR;
static PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVR _glFramebufferTextureMultiviewOVR;
static void APIENTRY d_glFramebufferTextureMultiviewOVR(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews) {
	_glFramebufferTextureMultiviewOVR(target, attachment, texture, level, baseViewIndex, numViews);
	CheckGLError("glFramebufferTextureMultiviewOVR");
}
typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTUREOES)(GLenum target, GLenum attachment, GLuint texture, GLint level);
PFNGLFRAMEBUFFERTEXTUREOES gglFramebufferTextureOES;
static PFNGLFRAMEBUFFERTEXTUREOES _glFramebufferTextureOES;
static void APIENTRY d_glFramebufferTextureOES(GLenum target, GLenum attachment, GLuint texture, GLint level) {
	_glFramebufferTextureOES(target, attachment, texture, level);
	CheckGLError("glFramebufferTextureOES");
}
typedef void (APIENTRYP PFNGLFRONTFACE)(GLenum mode);
PFNGLFRONTFACE gglFrontFace;
static PFNGLFRONTFACE _glFrontFace;
static void APIENTRY d_glFrontFace(GLenum mode) {
	_glFrontFace(mode);
	CheckGLError("glFrontFace");
}
typedef void (APIENTRYP PFNGLGENBUFFERS)(GLsizei n, GLuint *buffers);
PFNGLGENBUFFERS gglGenBuffers;
static PFNGLGENBUFFERS _glGenBuffers;
static void APIENTRY d_glGenBuffers(GLsizei n, GLuint *buffers) {
	_glGenBuffers(n, buffers);
	CheckGLError("glGenBuffers");
}
typedef void (APIENTRYP PFNGLGENFENCESNV)(GLsizei n, GLuint *fences);
PFNGLGENFENCESNV gglGenFencesNV;
static PFNGLGENFENCESNV _glGenFencesNV;
static void APIENTRY d_glGenFencesNV(GLsizei n, GLuint *fences) {
	_glGenFencesNV(n, fences);
	CheckGLError("glGenFencesNV");
}
typedef void (APIENTRYP PFNGLGENFRAMEBUFFERS)(GLsizei n, GLuint *framebuffers);
PFNGLGENFRAMEBUFFERS gglGenFramebuffers;
static PFNGLGENFRAMEBUFFERS _glGenFramebuffers;
static void APIENTRY d_glGenFramebuffers(GLsizei n, GLuint *framebuffers) {
	_glGenFramebuffers(n, framebuffers);
	CheckGLError("glGenFramebuffers");
}
typedef GLuint (APIENTRYP PFNGLGENPATHSNV)(GLsizei range);
PFNGLGENPATHSNV gglGenPathsNV;
static PFNGLGENPATHSNV _glGenPathsNV;
static GLuint APIENTRY d_glGenPathsNV(GLsizei range) {
	GLuint ret = _glGenPathsNV(range);
	CheckGLError("glGenPathsNV");
	return ret;
}
typedef void (APIENTRYP PFNGLGENPERFMONITORSAMD)(GLsizei n, GLuint *monitors);
PFNGLGENPERFMONITORSAMD gglGenPerfMonitorsAMD;
static PFNGLGENPERFMONITORSAMD _glGenPerfMonitorsAMD;
static void APIENTRY d_glGenPerfMonitorsAMD(GLsizei n, GLuint *monitors) {
	_glGenPerfMonitorsAMD(n, monitors);
	CheckGLError("glGenPerfMonitorsAMD");
}
typedef void (APIENTRYP PFNGLGENPROGRAMPIPELINESEXT)(GLsizei n, GLuint *pipelines);
PFNGLGENPROGRAMPIPELINESEXT gglGenProgramPipelinesEXT;
static PFNGLGENPROGRAMPIPELINESEXT _glGenProgramPipelinesEXT;
static void APIENTRY d_glGenProgramPipelinesEXT(GLsizei n, GLuint *pipelines) {
	_glGenProgramPipelinesEXT(n, pipelines);
	CheckGLError("glGenProgramPipelinesEXT");
}
typedef void (APIENTRYP PFNGLGENQUERIES)(GLsizei n, GLuint *ids);
PFNGLGENQUERIES gglGenQueries;
static PFNGLGENQUERIES _glGenQueries;
static void APIENTRY d_glGenQueries(GLsizei n, GLuint *ids) {
	_glGenQueries(n, ids);
	CheckGLError("glGenQueries");
}
typedef void (APIENTRYP PFNGLGENQUERIESEXT)(GLsizei n, GLuint *ids);
PFNGLGENQUERIESEXT gglGenQueriesEXT;
static PFNGLGENQUERIESEXT _glGenQueriesEXT;
static void APIENTRY d_glGenQueriesEXT(GLsizei n, GLuint *ids) {
	_glGenQueriesEXT(n, ids);
	CheckGLError("glGenQueriesEXT");
}
typedef void (APIENTRYP PFNGLGENRENDERBUFFERS)(GLsizei n, GLuint *renderbuffers);
PFNGLGENRENDERBUFFERS gglGenRenderbuffers;
static PFNGLGENRENDERBUFFERS _glGenRenderbuffers;
static void APIENTRY d_glGenRenderbuffers(GLsizei n, GLuint *renderbuffers) {
	_glGenRenderbuffers(n, renderbuffers);
	CheckGLError("glGenRenderbuffers");
}
typedef void (APIENTRYP PFNGLGENSAMPLERS)(GLsizei count, GLuint *samplers);
PFNGLGENSAMPLERS gglGenSamplers;
static PFNGLGENSAMPLERS _glGenSamplers;
static void APIENTRY d_glGenSamplers(GLsizei count, GLuint *samplers) {
	_glGenSamplers(count, samplers);
	CheckGLError("glGenSamplers");
}
typedef void (APIENTRYP PFNGLGENTEXTURES)(GLsizei n, GLuint *textures);
PFNGLGENTEXTURES gglGenTextures;
static PFNGLGENTEXTURES _glGenTextures;
static void APIENTRY d_glGenTextures(GLsizei n, GLuint *textures) {
	_glGenTextures(n, textures);
	CheckGLError("glGenTextures");
}
typedef void (APIENTRYP PFNGLGENTRANSFORMFEEDBACKS)(GLsizei n, GLuint *ids);
PFNGLGENTRANSFORMFEEDBACKS gglGenTransformFeedbacks;
static PFNGLGENTRANSFORMFEEDBACKS _glGenTransformFeedbacks;
static void APIENTRY d_glGenTransformFeedbacks(GLsizei n, GLuint *ids) {
	_glGenTransformFeedbacks(n, ids);
	CheckGLError("glGenTransformFeedbacks");
}
typedef void (APIENTRYP PFNGLGENVERTEXARRAYS)(GLsizei n, GLuint *arrays);
PFNGLGENVERTEXARRAYS gglGenVertexArrays;
static PFNGLGENVERTEXARRAYS _glGenVertexArrays;
static void APIENTRY d_glGenVertexArrays(GLsizei n, GLuint *arrays) {
	_glGenVertexArrays(n, arrays);
	CheckGLError("glGenVertexArrays");
}
typedef void (APIENTRYP PFNGLGENVERTEXARRAYSOES)(GLsizei n, GLuint *arrays);
PFNGLGENVERTEXARRAYSOES gglGenVertexArraysOES;
static PFNGLGENVERTEXARRAYSOES _glGenVertexArraysOES;
static void APIENTRY d_glGenVertexArraysOES(GLsizei n, GLuint *arrays) {
	_glGenVertexArraysOES(n, arrays);
	CheckGLError("glGenVertexArraysOES");
}
typedef void (APIENTRYP PFNGLGENERATEMIPMAP)(GLenum target);
PFNGLGENERATEMIPMAP gglGenerateMipmap;
static PFNGLGENERATEMIPMAP _glGenerateMipmap;
static void APIENTRY d_glGenerateMipmap(GLenum target) {
	_glGenerateMipmap(target);
	CheckGLError("glGenerateMipmap");
}
typedef void (APIENTRYP PFNGLGETACTIVEATTRIB)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
PFNGLGETACTIVEATTRIB gglGetActiveAttrib;
static PFNGLGETACTIVEATTRIB _glGetActiveAttrib;
static void APIENTRY d_glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
	_glGetActiveAttrib(program, index, bufSize, length, size, type, name);
	CheckGLError("glGetActiveAttrib");
}
typedef void (APIENTRYP PFNGLGETACTIVEUNIFORM)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
PFNGLGETACTIVEUNIFORM gglGetActiveUniform;
static PFNGLGETACTIVEUNIFORM _glGetActiveUniform;
static void APIENTRY d_glGetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) {
	_glGetActiveUniform(program, index, bufSize, length, size, type, name);
	CheckGLError("glGetActiveUniform");
}
typedef void (APIENTRYP PFNGLGETACTIVEUNIFORMBLOCKNAME)(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
PFNGLGETACTIVEUNIFORMBLOCKNAME gglGetActiveUniformBlockName;
static PFNGLGETACTIVEUNIFORMBLOCKNAME _glGetActiveUniformBlockName;
static void APIENTRY d_glGetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName) {
	_glGetActiveUniformBlockName(program, uniformBlockIndex, bufSize, length, uniformBlockName);
	CheckGLError("glGetActiveUniformBlockName");
}
typedef void (APIENTRYP PFNGLGETACTIVEUNIFORMBLOCKIV)(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
PFNGLGETACTIVEUNIFORMBLOCKIV gglGetActiveUniformBlockiv;
static PFNGLGETACTIVEUNIFORMBLOCKIV _glGetActiveUniformBlockiv;
static void APIENTRY d_glGetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params) {
	_glGetActiveUniformBlockiv(program, uniformBlockIndex, pname, params);
	CheckGLError("glGetActiveUniformBlockiv");
}
typedef void (APIENTRYP PFNGLGETACTIVEUNIFORMSIV)(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
PFNGLGETACTIVEUNIFORMSIV gglGetActiveUniformsiv;
static PFNGLGETACTIVEUNIFORMSIV _glGetActiveUniformsiv;
static void APIENTRY d_glGetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params) {
	_glGetActiveUniformsiv(program, uniformCount, uniformIndices, pname, params);
	CheckGLError("glGetActiveUniformsiv");
}
typedef void (APIENTRYP PFNGLGETATTACHEDSHADERS)(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders);
PFNGLGETATTACHEDSHADERS gglGetAttachedShaders;
static PFNGLGETATTACHEDSHADERS _glGetAttachedShaders;
static void APIENTRY d_glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders) {
	_glGetAttachedShaders(program, maxCount, count, shaders);
	CheckGLError("glGetAttachedShaders");
}
typedef GLint (APIENTRYP PFNGLGETATTRIBLOCATION)(GLuint program, const GLchar *name);
PFNGLGETATTRIBLOCATION gglGetAttribLocation;
static PFNGLGETATTRIBLOCATION _glGetAttribLocation;
static GLint APIENTRY d_glGetAttribLocation(GLuint program, const GLchar *name) {
	GLint ret = _glGetAttribLocation(program, name);
	CheckGLError("glGetAttribLocation");
	return ret;
}
typedef void (APIENTRYP PFNGLGETBOOLEANV)(GLenum pname, GLboolean *data);
PFNGLGETBOOLEANV gglGetBooleanv;
static PFNGLGETBOOLEANV _glGetBooleanv;
static void APIENTRY d_glGetBooleanv(GLenum pname, GLboolean *data) {
	_glGetBooleanv(pname, data);
	CheckGLError("glGetBooleanv");
}
typedef void (APIENTRYP PFNGLGETBUFFERPARAMETERI64V)(GLenum target, GLenum pname, GLint64 *params);
PFNGLGETBUFFERPARAMETERI64V gglGetBufferParameteri64v;
static PFNGLGETBUFFERPARAMETERI64V _glGetBufferParameteri64v;
static void APIENTRY d_glGetBufferParameteri64v(GLenum target, GLenum pname, GLint64 *params) {
	_glGetBufferParameteri64v(target, pname, params);
	CheckGLError("glGetBufferParameteri64v");
}
typedef void (APIENTRYP PFNGLGETBUFFERPARAMETERIV)(GLenum target, GLenum pname, GLint *params);
PFNGLGETBUFFERPARAMETERIV gglGetBufferParameteriv;
static PFNGLGETBUFFERPARAMETERIV _glGetBufferParameteriv;
static void APIENTRY d_glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params) {
	_glGetBufferParameteriv(target, pname, params);
	CheckGLError("glGetBufferParameteriv");
}
typedef void (APIENTRYP PFNGLGETBUFFERPOINTERV)(GLenum target, GLenum pname, void **params);
PFNGLGETBUFFERPOINTERV gglGetBufferPointerv;
static PFNGLGETBUFFERPOINTERV _glGetBufferPointerv;
static void APIENTRY d_glGetBufferPointerv(GLenum target, GLenum pname, void **params) {
	_glGetBufferPointerv(target, pname, params);
	CheckGLError("glGetBufferPointerv");
}
typedef void (APIENTRYP PFNGLGETBUFFERPOINTERVOES)(GLenum target, GLenum pname, void **params);
PFNGLGETBUFFERPOINTERVOES gglGetBufferPointervOES;
static PFNGLGETBUFFERPOINTERVOES _glGetBufferPointervOES;
static void APIENTRY d_glGetBufferPointervOES(GLenum target, GLenum pname, void **params) {
	_glGetBufferPointervOES(target, pname, params);
	CheckGLError("glGetBufferPointervOES");
}
typedef void (APIENTRYP PFNGLGETCOVERAGEMODULATIONTABLENV)(GLsizei bufsize, GLfloat *v);
PFNGLGETCOVERAGEMODULATIONTABLENV gglGetCoverageModulationTableNV;
static PFNGLGETCOVERAGEMODULATIONTABLENV _glGetCoverageModulationTableNV;
static void APIENTRY d_glGetCoverageModulationTableNV(GLsizei bufsize, GLfloat *v) {
	_glGetCoverageModulationTableNV(bufsize, v);
	CheckGLError("glGetCoverageModulationTableNV");
}
typedef GLuint (APIENTRYP PFNGLGETDEBUGMESSAGELOGKHR)(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog);
PFNGLGETDEBUGMESSAGELOGKHR gglGetDebugMessageLogKHR;
static PFNGLGETDEBUGMESSAGELOGKHR _glGetDebugMessageLogKHR;
static GLuint APIENTRY d_glGetDebugMessageLogKHR(GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog) {
	GLuint ret = _glGetDebugMessageLogKHR(count, bufSize, sources, types, ids, severities, lengths, messageLog);
	CheckGLError("glGetDebugMessageLogKHR");
	return ret;
}
typedef void (APIENTRYP PFNGLGETDRIVERCONTROLSTRINGQCOM)(GLuint driverControl, GLsizei bufSize, GLsizei *length, GLchar *driverControlString);
PFNGLGETDRIVERCONTROLSTRINGQCOM gglGetDriverControlStringQCOM;
static PFNGLGETDRIVERCONTROLSTRINGQCOM _glGetDriverControlStringQCOM;
static void APIENTRY d_glGetDriverControlStringQCOM(GLuint driverControl, GLsizei bufSize, GLsizei *length, GLchar *driverControlString) {
	_glGetDriverControlStringQCOM(driverControl, bufSize, length, driverControlString);
	CheckGLError("glGetDriverControlStringQCOM");
}
typedef void (APIENTRYP PFNGLGETDRIVERCONTROLSQCOM)(GLint *num, GLsizei size, GLuint *driverControls);
PFNGLGETDRIVERCONTROLSQCOM gglGetDriverControlsQCOM;
static PFNGLGETDRIVERCONTROLSQCOM _glGetDriverControlsQCOM;
static void APIENTRY d_glGetDriverControlsQCOM(GLint *num, GLsizei size, GLuint *driverControls) {
	_glGetDriverControlsQCOM(num, size, driverControls);
	CheckGLError("glGetDriverControlsQCOM");
}
typedef GLenum (APIENTRYP PFNGLGETERROR)();
PFNGLGETERROR gglGetError;
static PFNGLGETERROR _glGetError;
static GLenum APIENTRY d_glGetError() {
	GLenum ret = _glGetError();
	CheckGLError("glGetError");
	return ret;
}
typedef void (APIENTRYP PFNGLGETFENCEIVNV)(GLuint fence, GLenum pname, GLint *params);
PFNGLGETFENCEIVNV gglGetFenceivNV;
static PFNGLGETFENCEIVNV _glGetFenceivNV;
static void APIENTRY d_glGetFenceivNV(GLuint fence, GLenum pname, GLint *params) {
	_glGetFenceivNV(fence, pname, params);
	CheckGLError("glGetFenceivNV");
}
typedef void (APIENTRYP PFNGLGETFIRSTPERFQUERYIDINTEL)(GLuint *queryId);
PFNGLGETFIRSTPERFQUERYIDINTEL gglGetFirstPerfQueryIdINTEL;
static PFNGLGETFIRSTPERFQUERYIDINTEL _glGetFirstPerfQueryIdINTEL;
static void APIENTRY d_glGetFirstPerfQueryIdINTEL(GLuint *queryId) {
	_glGetFirstPerfQueryIdINTEL(queryId);
	CheckGLError("glGetFirstPerfQueryIdINTEL");
}
typedef void (APIENTRYP PFNGLGETFLOATI_VNV)(GLenum target, GLuint index, GLfloat *data);
PFNGLGETFLOATI_VNV gglGetFloati_vNV;
static PFNGLGETFLOATI_VNV _glGetFloati_vNV;
static void APIENTRY d_glGetFloati_vNV(GLenum target, GLuint index, GLfloat *data) {
	_glGetFloati_vNV(target, index, data);
	CheckGLError("glGetFloati_vNV");
}
typedef void (APIENTRYP PFNGLGETFLOATI_VOES)(GLenum target, GLuint index, GLfloat *data);
PFNGLGETFLOATI_VOES gglGetFloati_vOES;
static PFNGLGETFLOATI_VOES _glGetFloati_vOES;
static void APIENTRY d_glGetFloati_vOES(GLenum target, GLuint index, GLfloat *data) {
	_glGetFloati_vOES(target, index, data);
	CheckGLError("glGetFloati_vOES");
}
typedef void (APIENTRYP PFNGLGETFLOATV)(GLenum pname, GLfloat *data);
PFNGLGETFLOATV gglGetFloatv;
static PFNGLGETFLOATV _glGetFloatv;
static void APIENTRY d_glGetFloatv(GLenum pname, GLfloat *data) {
	_glGetFloatv(pname, data);
	CheckGLError("glGetFloatv");
}
typedef GLint (APIENTRYP PFNGLGETFRAGDATAINDEXEXT)(GLuint program, const GLchar *name);
PFNGLGETFRAGDATAINDEXEXT gglGetFragDataIndexEXT;
static PFNGLGETFRAGDATAINDEXEXT _glGetFragDataIndexEXT;
static GLint APIENTRY d_glGetFragDataIndexEXT(GLuint program, const GLchar *name) {
	GLint ret = _glGetFragDataIndexEXT(program, name);
	CheckGLError("glGetFragDataIndexEXT");
	return ret;
}
typedef GLint (APIENTRYP PFNGLGETFRAGDATALOCATION)(GLuint program, const GLchar *name);
PFNGLGETFRAGDATALOCATION gglGetFragDataLocation;
static PFNGLGETFRAGDATALOCATION _glGetFragDataLocation;
static GLint APIENTRY d_glGetFragDataLocation(GLuint program, const GLchar *name) {
	GLint ret = _glGetFragDataLocation(program, name);
	CheckGLError("glGetFragDataLocation");
	return ret;
}
typedef void (APIENTRYP PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIV)(GLenum target, GLenum attachment, GLenum pname, GLint *params);
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIV gglGetFramebufferAttachmentParameteriv;
static PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIV _glGetFramebufferAttachmentParameteriv;
static void APIENTRY d_glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
	_glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
	CheckGLError("glGetFramebufferAttachmentParameteriv");
}
typedef GLsizei (APIENTRYP PFNGLGETFRAMEBUFFERPIXELLOCALSTORAGESIZEEXT)(GLuint target);
PFNGLGETFRAMEBUFFERPIXELLOCALSTORAGESIZEEXT gglGetFramebufferPixelLocalStorageSizeEXT;
static PFNGLGETFRAMEBUFFERPIXELLOCALSTORAGESIZEEXT _glGetFramebufferPixelLocalStorageSizeEXT;
static GLsizei APIENTRY d_glGetFramebufferPixelLocalStorageSizeEXT(GLuint target) {
	GLsizei ret = _glGetFramebufferPixelLocalStorageSizeEXT(target);
	CheckGLError("glGetFramebufferPixelLocalStorageSizeEXT");
	return ret;
}
typedef GLenum (APIENTRYP PFNGLGETGRAPHICSRESETSTATUSEXT)();
PFNGLGETGRAPHICSRESETSTATUSEXT gglGetGraphicsResetStatusEXT;
static PFNGLGETGRAPHICSRESETSTATUSEXT _glGetGraphicsResetStatusEXT;
static GLenum APIENTRY d_glGetGraphicsResetStatusEXT() {
	GLenum ret = _glGetGraphicsResetStatusEXT();
	CheckGLError("glGetGraphicsResetStatusEXT");
	return ret;
}
typedef GLenum (APIENTRYP PFNGLGETGRAPHICSRESETSTATUSKHR)();
PFNGLGETGRAPHICSRESETSTATUSKHR gglGetGraphicsResetStatusKHR;
static PFNGLGETGRAPHICSRESETSTATUSKHR _glGetGraphicsResetStatusKHR;
static GLenum APIENTRY d_glGetGraphicsResetStatusKHR() {
	GLenum ret = _glGetGraphicsResetStatusKHR();
	CheckGLError("glGetGraphicsResetStatusKHR");
	return ret;
}
typedef GLuint64 (APIENTRYP PFNGLGETIMAGEHANDLENV)(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format);
PFNGLGETIMAGEHANDLENV gglGetImageHandleNV;
static PFNGLGETIMAGEHANDLENV _glGetImageHandleNV;
static GLuint64 APIENTRY d_glGetImageHandleNV(GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format) {
	GLuint64 ret = _glGetImageHandleNV(texture, level, layered, layer, format);
	CheckGLError("glGetImageHandleNV");
	return ret;
}
typedef void (APIENTRYP PFNGLGETINTEGER64I_V)(GLenum target, GLuint index, GLint64 *data);
PFNGLGETINTEGER64I_V gglGetInteger64i_v;
static PFNGLGETINTEGER64I_V _glGetInteger64i_v;
static void APIENTRY d_glGetInteger64i_v(GLenum target, GLuint index, GLint64 *data) {
	_glGetInteger64i_v(target, index, data);
	CheckGLError("glGetInteger64i_v");
}
typedef void (APIENTRYP PFNGLGETINTEGER64V)(GLenum pname, GLint64 *data);
PFNGLGETINTEGER64V gglGetInteger64v;
static PFNGLGETINTEGER64V _glGetInteger64v;
static void APIENTRY d_glGetInteger64v(GLenum pname, GLint64 *data) {
	_glGetInteger64v(pname, data);
	CheckGLError("glGetInteger64v");
}
typedef void (APIENTRYP PFNGLGETINTEGER64VAPPLE)(GLenum pname, GLint64 *params);
PFNGLGETINTEGER64VAPPLE gglGetInteger64vAPPLE;
static PFNGLGETINTEGER64VAPPLE _glGetInteger64vAPPLE;
static void APIENTRY d_glGetInteger64vAPPLE(GLenum pname, GLint64 *params) {
	_glGetInteger64vAPPLE(pname, params);
	CheckGLError("glGetInteger64vAPPLE");
}
typedef void (APIENTRYP PFNGLGETINTEGERI_V)(GLenum target, GLuint index, GLint *data);
PFNGLGETINTEGERI_V gglGetIntegeri_v;
static PFNGLGETINTEGERI_V _glGetIntegeri_v;
static void APIENTRY d_glGetIntegeri_v(GLenum target, GLuint index, GLint *data) {
	_glGetIntegeri_v(target, index, data);
	CheckGLError("glGetIntegeri_v");
}
typedef void (APIENTRYP PFNGLGETINTEGERI_VEXT)(GLenum target, GLuint index, GLint *data);
PFNGLGETINTEGERI_VEXT gglGetIntegeri_vEXT;
static PFNGLGETINTEGERI_VEXT _glGetIntegeri_vEXT;
static void APIENTRY d_glGetIntegeri_vEXT(GLenum target, GLuint index, GLint *data) {
	_glGetIntegeri_vEXT(target, index, data);
	CheckGLError("glGetIntegeri_vEXT");
}
typedef void (APIENTRYP PFNGLGETINTEGERV)(GLenum pname, GLint *data);
PFNGLGETINTEGERV gglGetIntegerv;
static PFNGLGETINTEGERV _glGetIntegerv;
static void APIENTRY d_glGetIntegerv(GLenum pname, GLint *data) {
	_glGetIntegerv(pname, data);
	CheckGLError("glGetIntegerv");
}
typedef void (APIENTRYP PFNGLGETINTERNALFORMATSAMPLEIVNV)(GLenum target, GLenum internalformat, GLsizei samples, GLenum pname, GLsizei bufSize, GLint *params);
PFNGLGETINTERNALFORMATSAMPLEIVNV gglGetInternalformatSampleivNV;
static PFNGLGETINTERNALFORMATSAMPLEIVNV _glGetInternalformatSampleivNV;
static void APIENTRY d_glGetInternalformatSampleivNV(GLenum target, GLenum internalformat, GLsizei samples, GLenum pname, GLsizei bufSize, GLint *params) {
	_glGetInternalformatSampleivNV(target, internalformat, samples, pname, bufSize, params);
	CheckGLError("glGetInternalformatSampleivNV");
}
typedef void (APIENTRYP PFNGLGETINTERNALFORMATIV)(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params);
PFNGLGETINTERNALFORMATIV gglGetInternalformativ;
static PFNGLGETINTERNALFORMATIV _glGetInternalformativ;
static void APIENTRY d_glGetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params) {
	_glGetInternalformativ(target, internalformat, pname, bufSize, params);
	CheckGLError("glGetInternalformativ");
}
typedef void (APIENTRYP PFNGLGETNEXTPERFQUERYIDINTEL)(GLuint queryId, GLuint *nextQueryId);
PFNGLGETNEXTPERFQUERYIDINTEL gglGetNextPerfQueryIdINTEL;
static PFNGLGETNEXTPERFQUERYIDINTEL _glGetNextPerfQueryIdINTEL;
static void APIENTRY d_glGetNextPerfQueryIdINTEL(GLuint queryId, GLuint *nextQueryId) {
	_glGetNextPerfQueryIdINTEL(queryId, nextQueryId);
	CheckGLError("glGetNextPerfQueryIdINTEL");
}
typedef void (APIENTRYP PFNGLGETOBJECTLABELEXT)(GLenum type, GLuint object, GLsizei bufSize, GLsizei *length, GLchar *label);
PFNGLGETOBJECTLABELEXT gglGetObjectLabelEXT;
static PFNGLGETOBJECTLABELEXT _glGetObjectLabelEXT;
static void APIENTRY d_glGetObjectLabelEXT(GLenum type, GLuint object, GLsizei bufSize, GLsizei *length, GLchar *label) {
	_glGetObjectLabelEXT(type, object, bufSize, length, label);
	CheckGLError("glGetObjectLabelEXT");
}
typedef void (APIENTRYP PFNGLGETOBJECTLABELKHR)(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label);
PFNGLGETOBJECTLABELKHR gglGetObjectLabelKHR;
static PFNGLGETOBJECTLABELKHR _glGetObjectLabelKHR;
static void APIENTRY d_glGetObjectLabelKHR(GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label) {
	_glGetObjectLabelKHR(identifier, name, bufSize, length, label);
	CheckGLError("glGetObjectLabelKHR");
}
typedef void (APIENTRYP PFNGLGETOBJECTPTRLABELKHR)(const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label);
PFNGLGETOBJECTPTRLABELKHR gglGetObjectPtrLabelKHR;
static PFNGLGETOBJECTPTRLABELKHR _glGetObjectPtrLabelKHR;
static void APIENTRY d_glGetObjectPtrLabelKHR(const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label) {
	_glGetObjectPtrLabelKHR(ptr, bufSize, length, label);
	CheckGLError("glGetObjectPtrLabelKHR");
}
typedef void (APIENTRYP PFNGLGETPATHCOMMANDSNV)(GLuint path, GLubyte *commands);
PFNGLGETPATHCOMMANDSNV gglGetPathCommandsNV;
static PFNGLGETPATHCOMMANDSNV _glGetPathCommandsNV;
static void APIENTRY d_glGetPathCommandsNV(GLuint path, GLubyte *commands) {
	_glGetPathCommandsNV(path, commands);
	CheckGLError("glGetPathCommandsNV");
}
typedef void (APIENTRYP PFNGLGETPATHCOORDSNV)(GLuint path, GLfloat *coords);
PFNGLGETPATHCOORDSNV gglGetPathCoordsNV;
static PFNGLGETPATHCOORDSNV _glGetPathCoordsNV;
static void APIENTRY d_glGetPathCoordsNV(GLuint path, GLfloat *coords) {
	_glGetPathCoordsNV(path, coords);
	CheckGLError("glGetPathCoordsNV");
}
typedef void (APIENTRYP PFNGLGETPATHDASHARRAYNV)(GLuint path, GLfloat *dashArray);
PFNGLGETPATHDASHARRAYNV gglGetPathDashArrayNV;
static PFNGLGETPATHDASHARRAYNV _glGetPathDashArrayNV;
static void APIENTRY d_glGetPathDashArrayNV(GLuint path, GLfloat *dashArray) {
	_glGetPathDashArrayNV(path, dashArray);
	CheckGLError("glGetPathDashArrayNV");
}
typedef GLfloat (APIENTRYP PFNGLGETPATHLENGTHNV)(GLuint path, GLsizei startSegment, GLsizei numSegments);
PFNGLGETPATHLENGTHNV gglGetPathLengthNV;
static PFNGLGETPATHLENGTHNV _glGetPathLengthNV;
static GLfloat APIENTRY d_glGetPathLengthNV(GLuint path, GLsizei startSegment, GLsizei numSegments) {
	GLfloat ret = _glGetPathLengthNV(path, startSegment, numSegments);
	CheckGLError("glGetPathLengthNV");
	return ret;
}
typedef void (APIENTRYP PFNGLGETPATHMETRICRANGENV)(GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics);
PFNGLGETPATHMETRICRANGENV gglGetPathMetricRangeNV;
static PFNGLGETPATHMETRICRANGENV _glGetPathMetricRangeNV;
static void APIENTRY d_glGetPathMetricRangeNV(GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics) {
	_glGetPathMetricRangeNV(metricQueryMask, firstPathName, numPaths, stride, metrics);
	CheckGLError("glGetPathMetricRangeNV");
}
typedef void (APIENTRYP PFNGLGETPATHMETRICSNV)(GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics);
PFNGLGETPATHMETRICSNV gglGetPathMetricsNV;
static PFNGLGETPATHMETRICSNV _glGetPathMetricsNV;
static void APIENTRY d_glGetPathMetricsNV(GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics) {
	_glGetPathMetricsNV(metricQueryMask, numPaths, pathNameType, paths, pathBase, stride, metrics);
	CheckGLError("glGetPathMetricsNV");
}
typedef void (APIENTRYP PFNGLGETPATHPARAMETERFVNV)(GLuint path, GLenum pname, GLfloat *value);
PFNGLGETPATHPARAMETERFVNV gglGetPathParameterfvNV;
static PFNGLGETPATHPARAMETERFVNV _glGetPathParameterfvNV;
static void APIENTRY d_glGetPathParameterfvNV(GLuint path, GLenum pname, GLfloat *value) {
	_glGetPathParameterfvNV(path, pname, value);
	CheckGLError("glGetPathParameterfvNV");
}
typedef void (APIENTRYP PFNGLGETPATHPARAMETERIVNV)(GLuint path, GLenum pname, GLint *value);
PFNGLGETPATHPARAMETERIVNV gglGetPathParameterivNV;
static PFNGLGETPATHPARAMETERIVNV _glGetPathParameterivNV;
static void APIENTRY d_glGetPathParameterivNV(GLuint path, GLenum pname, GLint *value) {
	_glGetPathParameterivNV(path, pname, value);
	CheckGLError("glGetPathParameterivNV");
}
typedef void (APIENTRYP PFNGLGETPATHSPACINGNV)(GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat *returnedSpacing);
PFNGLGETPATHSPACINGNV gglGetPathSpacingNV;
static PFNGLGETPATHSPACINGNV _glGetPathSpacingNV;
static void APIENTRY d_glGetPathSpacingNV(GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat *returnedSpacing) {
	_glGetPathSpacingNV(pathListMode, numPaths, pathNameType, paths, pathBase, advanceScale, kerningScale, transformType, returnedSpacing);
	CheckGLError("glGetPathSpacingNV");
}
typedef void (APIENTRYP PFNGLGETPERFCOUNTERINFOINTEL)(GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar *counterName, GLuint counterDescLength, GLchar *counterDesc, GLuint *counterOffset, GLuint *counterDataSize, GLuint *counterTypeEnum, GLuint *counterDataTypeEnum, GLuint64 *rawCounterMaxValue);
PFNGLGETPERFCOUNTERINFOINTEL gglGetPerfCounterInfoINTEL;
static PFNGLGETPERFCOUNTERINFOINTEL _glGetPerfCounterInfoINTEL;
static void APIENTRY d_glGetPerfCounterInfoINTEL(GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar *counterName, GLuint counterDescLength, GLchar *counterDesc, GLuint *counterOffset, GLuint *counterDataSize, GLuint *counterTypeEnum, GLuint *counterDataTypeEnum, GLuint64 *rawCounterMaxValue) {
	_glGetPerfCounterInfoINTEL(queryId, counterId, counterNameLength, counterName, counterDescLength, counterDesc, counterOffset, counterDataSize, counterTypeEnum, counterDataTypeEnum, rawCounterMaxValue);
	CheckGLError("glGetPerfCounterInfoINTEL");
}
typedef void (APIENTRYP PFNGLGETPERFMONITORCOUNTERDATAAMD)(GLuint monitor, GLenum pname, GLsizei dataSize, GLuint *data, GLint *bytesWritten);
PFNGLGETPERFMONITORCOUNTERDATAAMD gglGetPerfMonitorCounterDataAMD;
static PFNGLGETPERFMONITORCOUNTERDATAAMD _glGetPerfMonitorCounterDataAMD;
static void APIENTRY d_glGetPerfMonitorCounterDataAMD(GLuint monitor, GLenum pname, GLsizei dataSize, GLuint *data, GLint *bytesWritten) {
	_glGetPerfMonitorCounterDataAMD(monitor, pname, dataSize, data, bytesWritten);
	CheckGLError("glGetPerfMonitorCounterDataAMD");
}
typedef void (APIENTRYP PFNGLGETPERFMONITORCOUNTERINFOAMD)(GLuint group, GLuint counter, GLenum pname, void *data);
PFNGLGETPERFMONITORCOUNTERINFOAMD gglGetPerfMonitorCounterInfoAMD;
static PFNGLGETPERFMONITORCOUNTERINFOAMD _glGetPerfMonitorCounterInfoAMD;
static void APIENTRY d_glGetPerfMonitorCounterInfoAMD(GLuint group, GLuint counter, GLenum pname, void *data) {
	_glGetPerfMonitorCounterInfoAMD(group, counter, pname, data);
	CheckGLError("glGetPerfMonitorCounterInfoAMD");
}
typedef void (APIENTRYP PFNGLGETPERFMONITORCOUNTERSTRINGAMD)(GLuint group, GLuint counter, GLsizei bufSize, GLsizei *length, GLchar *counterString);
PFNGLGETPERFMONITORCOUNTERSTRINGAMD gglGetPerfMonitorCounterStringAMD;
static PFNGLGETPERFMONITORCOUNTERSTRINGAMD _glGetPerfMonitorCounterStringAMD;
static void APIENTRY d_glGetPerfMonitorCounterStringAMD(GLuint group, GLuint counter, GLsizei bufSize, GLsizei *length, GLchar *counterString) {
	_glGetPerfMonitorCounterStringAMD(group, counter, bufSize, length, counterString);
	CheckGLError("glGetPerfMonitorCounterStringAMD");
}
typedef void (APIENTRYP PFNGLGETPERFMONITORCOUNTERSAMD)(GLuint group, GLint *numCounters, GLint *maxActiveCounters, GLsizei counterSize, GLuint *counters);
PFNGLGETPERFMONITORCOUNTERSAMD gglGetPerfMonitorCountersAMD;
static PFNGLGETPERFMONITORCOUNTERSAMD _glGetPerfMonitorCountersAMD;
static void APIENTRY d_glGetPerfMonitorCountersAMD(GLuint group, GLint *numCounters, GLint *maxActiveCounters, GLsizei counterSize, GLuint *counters) {
	_glGetPerfMonitorCountersAMD(group, numCounters, maxActiveCounters, counterSize, counters);
	CheckGLError("glGetPerfMonitorCountersAMD");
}
typedef void (APIENTRYP PFNGLGETPERFMONITORGROUPSTRINGAMD)(GLuint group, GLsizei bufSize, GLsizei *length, GLchar *groupString);
PFNGLGETPERFMONITORGROUPSTRINGAMD gglGetPerfMonitorGroupStringAMD;
static PFNGLGETPERFMONITORGROUPSTRINGAMD _glGetPerfMonitorGroupStringAMD;
static void APIENTRY d_glGetPerfMonitorGroupStringAMD(GLuint group, GLsizei bufSize, GLsizei *length, GLchar *groupString) {
	_glGetPerfMonitorGroupStringAMD(group, bufSize, length, groupString);
	CheckGLError("glGetPerfMonitorGroupStringAMD");
}
typedef void (APIENTRYP PFNGLGETPERFMONITORGROUPSAMD)(GLint *numGroups, GLsizei groupsSize, GLuint *groups);
PFNGLGETPERFMONITORGROUPSAMD gglGetPerfMonitorGroupsAMD;
static PFNGLGETPERFMONITORGROUPSAMD _glGetPerfMonitorGroupsAMD;
static void APIENTRY d_glGetPerfMonitorGroupsAMD(GLint *numGroups, GLsizei groupsSize, GLuint *groups) {
	_glGetPerfMonitorGroupsAMD(numGroups, groupsSize, groups);
	CheckGLError("glGetPerfMonitorGroupsAMD");
}
typedef void (APIENTRYP PFNGLGETPERFQUERYDATAINTEL)(GLuint queryHandle, GLuint flags, GLsizei dataSize, GLvoid *data, GLuint *bytesWritten);
PFNGLGETPERFQUERYDATAINTEL gglGetPerfQueryDataINTEL;
static PFNGLGETPERFQUERYDATAINTEL _glGetPerfQueryDataINTEL;
static void APIENTRY d_glGetPerfQueryDataINTEL(GLuint queryHandle, GLuint flags, GLsizei dataSize, GLvoid *data, GLuint *bytesWritten) {
	_glGetPerfQueryDataINTEL(queryHandle, flags, dataSize, data, bytesWritten);
	CheckGLError("glGetPerfQueryDataINTEL");
}
typedef void (APIENTRYP PFNGLGETPERFQUERYIDBYNAMEINTEL)(GLchar *queryName, GLuint *queryId);
PFNGLGETPERFQUERYIDBYNAMEINTEL gglGetPerfQueryIdByNameINTEL;
static PFNGLGETPERFQUERYIDBYNAMEINTEL _glGetPerfQueryIdByNameINTEL;
static void APIENTRY d_glGetPerfQueryIdByNameINTEL(GLchar *queryName, GLuint *queryId) {
	_glGetPerfQueryIdByNameINTEL(queryName, queryId);
	CheckGLError("glGetPerfQueryIdByNameINTEL");
}
typedef void (APIENTRYP PFNGLGETPERFQUERYINFOINTEL)(GLuint queryId, GLuint queryNameLength, GLchar *queryName, GLuint *dataSize, GLuint *noCounters, GLuint *noInstances, GLuint *capsMask);
PFNGLGETPERFQUERYINFOINTEL gglGetPerfQueryInfoINTEL;
static PFNGLGETPERFQUERYINFOINTEL _glGetPerfQueryInfoINTEL;
static void APIENTRY d_glGetPerfQueryInfoINTEL(GLuint queryId, GLuint queryNameLength, GLchar *queryName, GLuint *dataSize, GLuint *noCounters, GLuint *noInstances, GLuint *capsMask) {
	_glGetPerfQueryInfoINTEL(queryId, queryNameLength, queryName, dataSize, noCounters, noInstances, capsMask);
	CheckGLError("glGetPerfQueryInfoINTEL");
}
typedef void (APIENTRYP PFNGLGETPOINTERVKHR)(GLenum pname, void **params);
PFNGLGETPOINTERVKHR gglGetPointervKHR;
static PFNGLGETPOINTERVKHR _glGetPointervKHR;
static void APIENTRY d_glGetPointervKHR(GLenum pname, void **params) {
	_glGetPointervKHR(pname, params);
	CheckGLError("glGetPointervKHR");
}
typedef void (APIENTRYP PFNGLGETPROGRAMBINARY)(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
PFNGLGETPROGRAMBINARY gglGetProgramBinary;
static PFNGLGETPROGRAMBINARY _glGetProgramBinary;
static void APIENTRY d_glGetProgramBinary(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary) {
	_glGetProgramBinary(program, bufSize, length, binaryFormat, binary);
	CheckGLError("glGetProgramBinary");
}
typedef void (APIENTRYP PFNGLGETPROGRAMBINARYOES)(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
PFNGLGETPROGRAMBINARYOES gglGetProgramBinaryOES;
static PFNGLGETPROGRAMBINARYOES _glGetProgramBinaryOES;
static void APIENTRY d_glGetProgramBinaryOES(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary) {
	_glGetProgramBinaryOES(program, bufSize, length, binaryFormat, binary);
	CheckGLError("glGetProgramBinaryOES");
}
typedef void (APIENTRYP PFNGLGETPROGRAMINFOLOG)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
PFNGLGETPROGRAMINFOLOG gglGetProgramInfoLog;
static PFNGLGETPROGRAMINFOLOG _glGetProgramInfoLog;
static void APIENTRY d_glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
	_glGetProgramInfoLog(program, bufSize, length, infoLog);
	CheckGLError("glGetProgramInfoLog");
}
typedef void (APIENTRYP PFNGLGETPROGRAMPIPELINEINFOLOGEXT)(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
PFNGLGETPROGRAMPIPELINEINFOLOGEXT gglGetProgramPipelineInfoLogEXT;
static PFNGLGETPROGRAMPIPELINEINFOLOGEXT _glGetProgramPipelineInfoLogEXT;
static void APIENTRY d_glGetProgramPipelineInfoLogEXT(GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
	_glGetProgramPipelineInfoLogEXT(pipeline, bufSize, length, infoLog);
	CheckGLError("glGetProgramPipelineInfoLogEXT");
}
typedef void (APIENTRYP PFNGLGETPROGRAMPIPELINEIVEXT)(GLuint pipeline, GLenum pname, GLint *params);
PFNGLGETPROGRAMPIPELINEIVEXT gglGetProgramPipelineivEXT;
static PFNGLGETPROGRAMPIPELINEIVEXT _glGetProgramPipelineivEXT;
static void APIENTRY d_glGetProgramPipelineivEXT(GLuint pipeline, GLenum pname, GLint *params) {
	_glGetProgramPipelineivEXT(pipeline, pname, params);
	CheckGLError("glGetProgramPipelineivEXT");
}
typedef GLint (APIENTRYP PFNGLGETPROGRAMRESOURCELOCATIONINDEXEXT)(GLuint program, GLenum programInterface, const GLchar *name);
PFNGLGETPROGRAMRESOURCELOCATIONINDEXEXT gglGetProgramResourceLocationIndexEXT;
static PFNGLGETPROGRAMRESOURCELOCATIONINDEXEXT _glGetProgramResourceLocationIndexEXT;
static GLint APIENTRY d_glGetProgramResourceLocationIndexEXT(GLuint program, GLenum programInterface, const GLchar *name) {
	GLint ret = _glGetProgramResourceLocationIndexEXT(program, programInterface, name);
	CheckGLError("glGetProgramResourceLocationIndexEXT");
	return ret;
}
typedef void (APIENTRYP PFNGLGETPROGRAMRESOURCEFVNV)(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLfloat *params);
PFNGLGETPROGRAMRESOURCEFVNV gglGetProgramResourcefvNV;
static PFNGLGETPROGRAMRESOURCEFVNV _glGetProgramResourcefvNV;
static void APIENTRY d_glGetProgramResourcefvNV(GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLfloat *params) {
	_glGetProgramResourcefvNV(program, programInterface, index, propCount, props, bufSize, length, params);
	CheckGLError("glGetProgramResourcefvNV");
}
typedef void (APIENTRYP PFNGLGETPROGRAMIV)(GLuint program, GLenum pname, GLint *params);
PFNGLGETPROGRAMIV gglGetProgramiv;
static PFNGLGETPROGRAMIV _glGetProgramiv;
static void APIENTRY d_glGetProgramiv(GLuint program, GLenum pname, GLint *params) {
	_glGetProgramiv(program, pname, params);
	CheckGLError("glGetProgramiv");
}
typedef void (APIENTRYP PFNGLGETQUERYOBJECTI64VEXT)(GLuint id, GLenum pname, GLint64 *params);
PFNGLGETQUERYOBJECTI64VEXT gglGetQueryObjecti64vEXT;
static PFNGLGETQUERYOBJECTI64VEXT _glGetQueryObjecti64vEXT;
static void APIENTRY d_glGetQueryObjecti64vEXT(GLuint id, GLenum pname, GLint64 *params) {
	_glGetQueryObjecti64vEXT(id, pname, params);
	CheckGLError("glGetQueryObjecti64vEXT");
}
typedef void (APIENTRYP PFNGLGETQUERYOBJECTIVEXT)(GLuint id, GLenum pname, GLint *params);
PFNGLGETQUERYOBJECTIVEXT gglGetQueryObjectivEXT;
static PFNGLGETQUERYOBJECTIVEXT _glGetQueryObjectivEXT;
static void APIENTRY d_glGetQueryObjectivEXT(GLuint id, GLenum pname, GLint *params) {
	_glGetQueryObjectivEXT(id, pname, params);
	CheckGLError("glGetQueryObjectivEXT");
}
typedef void (APIENTRYP PFNGLGETQUERYOBJECTUI64VEXT)(GLuint id, GLenum pname, GLuint64 *params);
PFNGLGETQUERYOBJECTUI64VEXT gglGetQueryObjectui64vEXT;
static PFNGLGETQUERYOBJECTUI64VEXT _glGetQueryObjectui64vEXT;
static void APIENTRY d_glGetQueryObjectui64vEXT(GLuint id, GLenum pname, GLuint64 *params) {
	_glGetQueryObjectui64vEXT(id, pname, params);
	CheckGLError("glGetQueryObjectui64vEXT");
}
typedef void (APIENTRYP PFNGLGETQUERYOBJECTUIV)(GLuint id, GLenum pname, GLuint *params);
PFNGLGETQUERYOBJECTUIV gglGetQueryObjectuiv;
static PFNGLGETQUERYOBJECTUIV _glGetQueryObjectuiv;
static void APIENTRY d_glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params) {
	_glGetQueryObjectuiv(id, pname, params);
	CheckGLError("glGetQueryObjectuiv");
}
typedef void (APIENTRYP PFNGLGETQUERYOBJECTUIVEXT)(GLuint id, GLenum pname, GLuint *params);
PFNGLGETQUERYOBJECTUIVEXT gglGetQueryObjectuivEXT;
static PFNGLGETQUERYOBJECTUIVEXT _glGetQueryObjectuivEXT;
static void APIENTRY d_glGetQueryObjectuivEXT(GLuint id, GLenum pname, GLuint *params) {
	_glGetQueryObjectuivEXT(id, pname, params);
	CheckGLError("glGetQueryObjectuivEXT");
}
typedef void (APIENTRYP PFNGLGETQUERYIV)(GLenum target, GLenum pname, GLint *params);
PFNGLGETQUERYIV gglGetQueryiv;
static PFNGLGETQUERYIV _glGetQueryiv;
static void APIENTRY d_glGetQueryiv(GLenum target, GLenum pname, GLint *params) {
	_glGetQueryiv(target, pname, params);
	CheckGLError("glGetQueryiv");
}
typedef void (APIENTRYP PFNGLGETQUERYIVEXT)(GLenum target, GLenum pname, GLint *params);
PFNGLGETQUERYIVEXT gglGetQueryivEXT;
static PFNGLGETQUERYIVEXT _glGetQueryivEXT;
static void APIENTRY d_glGetQueryivEXT(GLenum target, GLenum pname, GLint *params) {
	_glGetQueryivEXT(target, pname, params);
	CheckGLError("glGetQueryivEXT");
}
typedef void (APIENTRYP PFNGLGETRENDERBUFFERPARAMETERIV)(GLenum target, GLenum pname, GLint *params);
PFNGLGETRENDERBUFFERPARAMETERIV gglGetRenderbufferParameteriv;
static PFNGLGETRENDERBUFFERPARAMETERIV _glGetRenderbufferParameteriv;
static void APIENTRY d_glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint *params) {
	_glGetRenderbufferParameteriv(target, pname, params);
	CheckGLError("glGetRenderbufferParameteriv");
}
typedef void (APIENTRYP PFNGLGETSAMPLERPARAMETERIIVEXT)(GLuint sampler, GLenum pname, GLint *params);
PFNGLGETSAMPLERPARAMETERIIVEXT gglGetSamplerParameterIivEXT;
static PFNGLGETSAMPLERPARAMETERIIVEXT _glGetSamplerParameterIivEXT;
static void APIENTRY d_glGetSamplerParameterIivEXT(GLuint sampler, GLenum pname, GLint *params) {
	_glGetSamplerParameterIivEXT(sampler, pname, params);
	CheckGLError("glGetSamplerParameterIivEXT");
}
typedef void (APIENTRYP PFNGLGETSAMPLERPARAMETERIIVOES)(GLuint sampler, GLenum pname, GLint *params);
PFNGLGETSAMPLERPARAMETERIIVOES gglGetSamplerParameterIivOES;
static PFNGLGETSAMPLERPARAMETERIIVOES _glGetSamplerParameterIivOES;
static void APIENTRY d_glGetSamplerParameterIivOES(GLuint sampler, GLenum pname, GLint *params) {
	_glGetSamplerParameterIivOES(sampler, pname, params);
	CheckGLError("glGetSamplerParameterIivOES");
}
typedef void (APIENTRYP PFNGLGETSAMPLERPARAMETERIUIVEXT)(GLuint sampler, GLenum pname, GLuint *params);
PFNGLGETSAMPLERPARAMETERIUIVEXT gglGetSamplerParameterIuivEXT;
static PFNGLGETSAMPLERPARAMETERIUIVEXT _glGetSamplerParameterIuivEXT;
static void APIENTRY d_glGetSamplerParameterIuivEXT(GLuint sampler, GLenum pname, GLuint *params) {
	_glGetSamplerParameterIuivEXT(sampler, pname, params);
	CheckGLError("glGetSamplerParameterIuivEXT");
}
typedef void (APIENTRYP PFNGLGETSAMPLERPARAMETERIUIVOES)(GLuint sampler, GLenum pname, GLuint *params);
PFNGLGETSAMPLERPARAMETERIUIVOES gglGetSamplerParameterIuivOES;
static PFNGLGETSAMPLERPARAMETERIUIVOES _glGetSamplerParameterIuivOES;
static void APIENTRY d_glGetSamplerParameterIuivOES(GLuint sampler, GLenum pname, GLuint *params) {
	_glGetSamplerParameterIuivOES(sampler, pname, params);
	CheckGLError("glGetSamplerParameterIuivOES");
}
typedef void (APIENTRYP PFNGLGETSAMPLERPARAMETERFV)(GLuint sampler, GLenum pname, GLfloat *params);
PFNGLGETSAMPLERPARAMETERFV gglGetSamplerParameterfv;
static PFNGLGETSAMPLERPARAMETERFV _glGetSamplerParameterfv;
static void APIENTRY d_glGetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat *params) {
	_glGetSamplerParameterfv(sampler, pname, params);
	CheckGLError("glGetSamplerParameterfv");
}
typedef void (APIENTRYP PFNGLGETSAMPLERPARAMETERIV)(GLuint sampler, GLenum pname, GLint *params);
PFNGLGETSAMPLERPARAMETERIV gglGetSamplerParameteriv;
static PFNGLGETSAMPLERPARAMETERIV _glGetSamplerParameteriv;
static void APIENTRY d_glGetSamplerParameteriv(GLuint sampler, GLenum pname, GLint *params) {
	_glGetSamplerParameteriv(sampler, pname, params);
	CheckGLError("glGetSamplerParameteriv");
}
typedef void (APIENTRYP PFNGLGETSHADERINFOLOG)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
PFNGLGETSHADERINFOLOG gglGetShaderInfoLog;
static PFNGLGETSHADERINFOLOG _glGetShaderInfoLog;
static void APIENTRY d_glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) {
	_glGetShaderInfoLog(shader, bufSize, length, infoLog);
	CheckGLError("glGetShaderInfoLog");
}
typedef void (APIENTRYP PFNGLGETSHADERPRECISIONFORMAT)(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision);
PFNGLGETSHADERPRECISIONFORMAT gglGetShaderPrecisionFormat;
static PFNGLGETSHADERPRECISIONFORMAT _glGetShaderPrecisionFormat;
static void APIENTRY d_glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision) {
	_glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
	CheckGLError("glGetShaderPrecisionFormat");
}
typedef void (APIENTRYP PFNGLGETSHADERSOURCE)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);
PFNGLGETSHADERSOURCE gglGetShaderSource;
static PFNGLGETSHADERSOURCE _glGetShaderSource;
static void APIENTRY d_glGetShaderSource(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) {
	_glGetShaderSource(shader, bufSize, length, source);
	CheckGLError("glGetShaderSource");
}
typedef void (APIENTRYP PFNGLGETSHADERIV)(GLuint shader, GLenum pname, GLint *params);
PFNGLGETSHADERIV gglGetShaderiv;
static PFNGLGETSHADERIV _glGetShaderiv;
static void APIENTRY d_glGetShaderiv(GLuint shader, GLenum pname, GLint *params) {
	_glGetShaderiv(shader, pname, params);
	CheckGLError("glGetShaderiv");
}
typedef const GLubyte * (APIENTRYP PFNGLGETSTRING)(GLenum name);
PFNGLGETSTRING gglGetString;
static PFNGLGETSTRING _glGetString;
static const GLubyte * APIENTRY d_glGetString(GLenum name) {
	const GLubyte * ret = _glGetString(name);
	CheckGLError("glGetString");
	return ret;
}
typedef const GLubyte * (APIENTRYP PFNGLGETSTRINGI)(GLenum name, GLuint index);
PFNGLGETSTRINGI gglGetStringi;
static PFNGLGETSTRINGI _glGetStringi;
static const GLubyte * APIENTRY d_glGetStringi(GLenum name, GLuint index) {
	const GLubyte * ret = _glGetStringi(name, index);
	CheckGLError("glGetStringi");
	return ret;
}
typedef void (APIENTRYP PFNGLGETSYNCIV)(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
PFNGLGETSYNCIV gglGetSynciv;
static PFNGLGETSYNCIV _glGetSynciv;
static void APIENTRY d_glGetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values) {
	_glGetSynciv(sync, pname, bufSize, length, values);
	CheckGLError("glGetSynciv");
}
typedef void (APIENTRYP PFNGLGETSYNCIVAPPLE)(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
PFNGLGETSYNCIVAPPLE gglGetSyncivAPPLE;
static PFNGLGETSYNCIVAPPLE _glGetSyncivAPPLE;
static void APIENTRY d_glGetSyncivAPPLE(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values) {
	_glGetSyncivAPPLE(sync, pname, bufSize, length, values);
	CheckGLError("glGetSyncivAPPLE");
}
typedef void (APIENTRYP PFNGLGETTEXPARAMETERIIVEXT)(GLenum target, GLenum pname, GLint *params);
PFNGLGETTEXPARAMETERIIVEXT gglGetTexParameterIivEXT;
static PFNGLGETTEXPARAMETERIIVEXT _glGetTexParameterIivEXT;
static void APIENTRY d_glGetTexParameterIivEXT(GLenum target, GLenum pname, GLint *params) {
	_glGetTexParameterIivEXT(target, pname, params);
	CheckGLError("glGetTexParameterIivEXT");
}
typedef void (APIENTRYP PFNGLGETTEXPARAMETERIIVOES)(GLenum target, GLenum pname, GLint *params);
PFNGLGETTEXPARAMETERIIVOES gglGetTexParameterIivOES;
static PFNGLGETTEXPARAMETERIIVOES _glGetTexParameterIivOES;
static void APIENTRY d_glGetTexParameterIivOES(GLenum target, GLenum pname, GLint *params) {
	_glGetTexParameterIivOES(target, pname, params);
	CheckGLError("glGetTexParameterIivOES");
}
typedef void (APIENTRYP PFNGLGETTEXPARAMETERIUIVEXT)(GLenum target, GLenum pname, GLuint *params);
PFNGLGETTEXPARAMETERIUIVEXT gglGetTexParameterIuivEXT;
static PFNGLGETTEXPARAMETERIUIVEXT _glGetTexParameterIuivEXT;
static void APIENTRY d_glGetTexParameterIuivEXT(GLenum target, GLenum pname, GLuint *params) {
	_glGetTexParameterIuivEXT(target, pname, params);
	CheckGLError("glGetTexParameterIuivEXT");
}
typedef void (APIENTRYP PFNGLGETTEXPARAMETERIUIVOES)(GLenum target, GLenum pname, GLuint *params);
PFNGLGETTEXPARAMETERIUIVOES gglGetTexParameterIuivOES;
static PFNGLGETTEXPARAMETERIUIVOES _glGetTexParameterIuivOES;
static void APIENTRY d_glGetTexParameterIuivOES(GLenum target, GLenum pname, GLuint *params) {
	_glGetTexParameterIuivOES(target, pname, params);
	CheckGLError("glGetTexParameterIuivOES");
}
typedef void (APIENTRYP PFNGLGETTEXPARAMETERFV)(GLenum target, GLenum pname, GLfloat *params);
PFNGLGETTEXPARAMETERFV gglGetTexParameterfv;
static PFNGLGETTEXPARAMETERFV _glGetTexParameterfv;
static void APIENTRY d_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) {
	_glGetTexParameterfv(target, pname, params);
	CheckGLError("glGetTexParameterfv");
}
typedef void (APIENTRYP PFNGLGETTEXPARAMETERIV)(GLenum target, GLenum pname, GLint *params);
PFNGLGETTEXPARAMETERIV gglGetTexParameteriv;
static PFNGLGETTEXPARAMETERIV _glGetTexParameteriv;
static void APIENTRY d_glGetTexParameteriv(GLenum target, GLenum pname, GLint *params) {
	_glGetTexParameteriv(target, pname, params);
	CheckGLError("glGetTexParameteriv");
}
typedef GLuint64 (APIENTRYP PFNGLGETTEXTUREHANDLEIMG)(GLuint texture);
PFNGLGETTEXTUREHANDLEIMG gglGetTextureHandleIMG;
static PFNGLGETTEXTUREHANDLEIMG _glGetTextureHandleIMG;
static GLuint64 APIENTRY d_glGetTextureHandleIMG(GLuint texture) {
	GLuint64 ret = _glGetTextureHandleIMG(texture);
	CheckGLError("glGetTextureHandleIMG");
	return ret;
}
typedef GLuint64 (APIENTRYP PFNGLGETTEXTUREHANDLENV)(GLuint texture);
PFNGLGETTEXTUREHANDLENV gglGetTextureHandleNV;
static PFNGLGETTEXTUREHANDLENV _glGetTextureHandleNV;
static GLuint64 APIENTRY d_glGetTextureHandleNV(GLuint texture) {
	GLuint64 ret = _glGetTextureHandleNV(texture);
	CheckGLError("glGetTextureHandleNV");
	return ret;
}
typedef GLuint64 (APIENTRYP PFNGLGETTEXTURESAMPLERHANDLEIMG)(GLuint texture, GLuint sampler);
PFNGLGETTEXTURESAMPLERHANDLEIMG gglGetTextureSamplerHandleIMG;
static PFNGLGETTEXTURESAMPLERHANDLEIMG _glGetTextureSamplerHandleIMG;
static GLuint64 APIENTRY d_glGetTextureSamplerHandleIMG(GLuint texture, GLuint sampler) {
	GLuint64 ret = _glGetTextureSamplerHandleIMG(texture, sampler);
	CheckGLError("glGetTextureSamplerHandleIMG");
	return ret;
}
typedef GLuint64 (APIENTRYP PFNGLGETTEXTURESAMPLERHANDLENV)(GLuint texture, GLuint sampler);
PFNGLGETTEXTURESAMPLERHANDLENV gglGetTextureSamplerHandleNV;
static PFNGLGETTEXTURESAMPLERHANDLENV _glGetTextureSamplerHandleNV;
static GLuint64 APIENTRY d_glGetTextureSamplerHandleNV(GLuint texture, GLuint sampler) {
	GLuint64 ret = _glGetTextureSamplerHandleNV(texture, sampler);
	CheckGLError("glGetTextureSamplerHandleNV");
	return ret;
}
typedef void (APIENTRYP PFNGLGETTRANSFORMFEEDBACKVARYING)(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
PFNGLGETTRANSFORMFEEDBACKVARYING gglGetTransformFeedbackVarying;
static PFNGLGETTRANSFORMFEEDBACKVARYING _glGetTransformFeedbackVarying;
static void APIENTRY d_glGetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name) {
	_glGetTransformFeedbackVarying(program, index, bufSize, length, size, type, name);
	CheckGLError("glGetTransformFeedbackVarying");
}
typedef void (APIENTRYP PFNGLGETTRANSLATEDSHADERSOURCEANGLE)(GLuint shader, GLsizei bufsize, GLsizei *length, GLchar *source);
PFNGLGETTRANSLATEDSHADERSOURCEANGLE gglGetTranslatedShaderSourceANGLE;
static PFNGLGETTRANSLATEDSHADERSOURCEANGLE _glGetTranslatedShaderSourceANGLE;
static void APIENTRY d_glGetTranslatedShaderSourceANGLE(GLuint shader, GLsizei bufsize, GLsizei *length, GLchar *source) {
	_glGetTranslatedShaderSourceANGLE(shader, bufsize, length, source);
	CheckGLError("glGetTranslatedShaderSourceANGLE");
}
typedef GLuint (APIENTRYP PFNGLGETUNIFORMBLOCKINDEX)(GLuint program, const GLchar *uniformBlockName);
PFNGLGETUNIFORMBLOCKINDEX gglGetUniformBlockIndex;
static PFNGLGETUNIFORMBLOCKINDEX _glGetUniformBlockIndex;
static GLuint APIENTRY d_glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName) {
	GLuint ret = _glGetUniformBlockIndex(program, uniformBlockName);
	CheckGLError("glGetUniformBlockIndex");
	return ret;
}
typedef void (APIENTRYP PFNGLGETUNIFORMINDICES)(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices);
PFNGLGETUNIFORMINDICES gglGetUniformIndices;
static PFNGLGETUNIFORMINDICES _glGetUniformIndices;
static void APIENTRY d_glGetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices) {
	_glGetUniformIndices(program, uniformCount, uniformNames, uniformIndices);
	CheckGLError("glGetUniformIndices");
}
typedef GLint (APIENTRYP PFNGLGETUNIFORMLOCATION)(GLuint program, const GLchar *name);
PFNGLGETUNIFORMLOCATION gglGetUniformLocation;
static PFNGLGETUNIFORMLOCATION _glGetUniformLocation;
static GLint APIENTRY d_glGetUniformLocation(GLuint program, const GLchar *name) {
	GLint ret = _glGetUniformLocation(program, name);
	CheckGLError("glGetUniformLocation");
	return ret;
}
typedef void (APIENTRYP PFNGLGETUNIFORMFV)(GLuint program, GLint location, GLfloat *params);
PFNGLGETUNIFORMFV gglGetUniformfv;
static PFNGLGETUNIFORMFV _glGetUniformfv;
static void APIENTRY d_glGetUniformfv(GLuint program, GLint location, GLfloat *params) {
	_glGetUniformfv(program, location, params);
	CheckGLError("glGetUniformfv");
}
typedef void (APIENTRYP PFNGLGETUNIFORMI64VNV)(GLuint program, GLint location, GLint64EXT *params);
PFNGLGETUNIFORMI64VNV gglGetUniformi64vNV;
static PFNGLGETUNIFORMI64VNV _glGetUniformi64vNV;
static void APIENTRY d_glGetUniformi64vNV(GLuint program, GLint location, GLint64EXT *params) {
	_glGetUniformi64vNV(program, location, params);
	CheckGLError("glGetUniformi64vNV");
}
typedef void (APIENTRYP PFNGLGETUNIFORMIV)(GLuint program, GLint location, GLint *params);
PFNGLGETUNIFORMIV gglGetUniformiv;
static PFNGLGETUNIFORMIV _glGetUniformiv;
static void APIENTRY d_glGetUniformiv(GLuint program, GLint location, GLint *params) {
	_glGetUniformiv(program, location, params);
	CheckGLError("glGetUniformiv");
}
typedef void (APIENTRYP PFNGLGETUNIFORMUIV)(GLuint program, GLint location, GLuint *params);
PFNGLGETUNIFORMUIV gglGetUniformuiv;
static PFNGLGETUNIFORMUIV _glGetUniformuiv;
static void APIENTRY d_glGetUniformuiv(GLuint program, GLint location, GLuint *params) {
	_glGetUniformuiv(program, location, params);
	CheckGLError("glGetUniformuiv");
}
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBIIV)(GLuint index, GLenum pname, GLint *params);
PFNGLGETVERTEXATTRIBIIV gglGetVertexAttribIiv;
static PFNGLGETVERTEXATTRIBIIV _glGetVertexAttribIiv;
static void APIENTRY d_glGetVertexAttribIiv(GLuint index, GLenum pname, GLint *params) {
	_glGetVertexAttribIiv(index, pname, params);
	CheckGLError("glGetVertexAttribIiv");
}
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBIUIV)(GLuint index, GLenum pname, GLuint *params);
PFNGLGETVERTEXATTRIBIUIV gglGetVertexAttribIuiv;
static PFNGLGETVERTEXATTRIBIUIV _glGetVertexAttribIuiv;
static void APIENTRY d_glGetVertexAttribIuiv(GLuint index, GLenum pname, GLuint *params) {
	_glGetVertexAttribIuiv(index, pname, params);
	CheckGLError("glGetVertexAttribIuiv");
}
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBPOINTERV)(GLuint index, GLenum pname, void **pointer);
PFNGLGETVERTEXATTRIBPOINTERV gglGetVertexAttribPointerv;
static PFNGLGETVERTEXATTRIBPOINTERV _glGetVertexAttribPointerv;
static void APIENTRY d_glGetVertexAttribPointerv(GLuint index, GLenum pname, void **pointer) {
	_glGetVertexAttribPointerv(index, pname, pointer);
	CheckGLError("glGetVertexAttribPointerv");
}
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBFV)(GLuint index, GLenum pname, GLfloat *params);
PFNGLGETVERTEXATTRIBFV gglGetVertexAttribfv;
static PFNGLGETVERTEXATTRIBFV _glGetVertexAttribfv;
static void APIENTRY d_glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat *params) {
	_glGetVertexAttribfv(index, pname, params);
	CheckGLError("glGetVertexAttribfv");
}
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBIV)(GLuint index, GLenum pname, GLint *params);
PFNGLGETVERTEXATTRIBIV gglGetVertexAttribiv;
static PFNGLGETVERTEXATTRIBIV _glGetVertexAttribiv;
static void APIENTRY d_glGetVertexAttribiv(GLuint index, GLenum pname, GLint *params) {
	_glGetVertexAttribiv(index, pname, params);
	CheckGLError("glGetVertexAttribiv");
}
typedef void (APIENTRYP PFNGLGETNUNIFORMFVEXT)(GLuint program, GLint location, GLsizei bufSize, GLfloat *params);
PFNGLGETNUNIFORMFVEXT gglGetnUniformfvEXT;
static PFNGLGETNUNIFORMFVEXT _glGetnUniformfvEXT;
static void APIENTRY d_glGetnUniformfvEXT(GLuint program, GLint location, GLsizei bufSize, GLfloat *params) {
	_glGetnUniformfvEXT(program, location, bufSize, params);
	CheckGLError("glGetnUniformfvEXT");
}
typedef void (APIENTRYP PFNGLGETNUNIFORMFVKHR)(GLuint program, GLint location, GLsizei bufSize, GLfloat *params);
PFNGLGETNUNIFORMFVKHR gglGetnUniformfvKHR;
static PFNGLGETNUNIFORMFVKHR _glGetnUniformfvKHR;
static void APIENTRY d_glGetnUniformfvKHR(GLuint program, GLint location, GLsizei bufSize, GLfloat *params) {
	_glGetnUniformfvKHR(program, location, bufSize, params);
	CheckGLError("glGetnUniformfvKHR");
}
typedef void (APIENTRYP PFNGLGETNUNIFORMIVEXT)(GLuint program, GLint location, GLsizei bufSize, GLint *params);
PFNGLGETNUNIFORMIVEXT gglGetnUniformivEXT;
static PFNGLGETNUNIFORMIVEXT _glGetnUniformivEXT;
static void APIENTRY d_glGetnUniformivEXT(GLuint program, GLint location, GLsizei bufSize, GLint *params) {
	_glGetnUniformivEXT(program, location, bufSize, params);
	CheckGLError("glGetnUniformivEXT");
}
typedef void (APIENTRYP PFNGLGETNUNIFORMIVKHR)(GLuint program, GLint location, GLsizei bufSize, GLint *params);
PFNGLGETNUNIFORMIVKHR gglGetnUniformivKHR;
static PFNGLGETNUNIFORMIVKHR _glGetnUniformivKHR;
static void APIENTRY d_glGetnUniformivKHR(GLuint program, GLint location, GLsizei bufSize, GLint *params) {
	_glGetnUniformivKHR(program, location, bufSize, params);
	CheckGLError("glGetnUniformivKHR");
}
typedef void (APIENTRYP PFNGLGETNUNIFORMUIVKHR)(GLuint program, GLint location, GLsizei bufSize, GLuint *params);
PFNGLGETNUNIFORMUIVKHR gglGetnUniformuivKHR;
static PFNGLGETNUNIFORMUIVKHR _glGetnUniformuivKHR;
static void APIENTRY d_glGetnUniformuivKHR(GLuint program, GLint location, GLsizei bufSize, GLuint *params) {
	_glGetnUniformuivKHR(program, location, bufSize, params);
	CheckGLError("glGetnUniformuivKHR");
}
typedef void (APIENTRYP PFNGLHINT)(GLenum target, GLenum mode);
PFNGLHINT gglHint;
static PFNGLHINT _glHint;
static void APIENTRY d_glHint(GLenum target, GLenum mode) {
	_glHint(target, mode);
	CheckGLError("glHint");
}
typedef void (APIENTRYP PFNGLINSERTEVENTMARKEREXT)(GLsizei length, const GLchar *marker);
PFNGLINSERTEVENTMARKEREXT gglInsertEventMarkerEXT;
static PFNGLINSERTEVENTMARKEREXT _glInsertEventMarkerEXT;
static void APIENTRY d_glInsertEventMarkerEXT(GLsizei length, const GLchar *marker) {
	_glInsertEventMarkerEXT(length, marker);
	CheckGLError("glInsertEventMarkerEXT");
}
typedef void (APIENTRYP PFNGLINTERPOLATEPATHSNV)(GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight);
PFNGLINTERPOLATEPATHSNV gglInterpolatePathsNV;
static PFNGLINTERPOLATEPATHSNV _glInterpolatePathsNV;
static void APIENTRY d_glInterpolatePathsNV(GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight) {
	_glInterpolatePathsNV(resultPath, pathA, pathB, weight);
	CheckGLError("glInterpolatePathsNV");
}
typedef void (APIENTRYP PFNGLINVALIDATEFRAMEBUFFER)(GLenum target, GLsizei numAttachments, const GLenum *attachments);
PFNGLINVALIDATEFRAMEBUFFER gglInvalidateFramebuffer;
static PFNGLINVALIDATEFRAMEBUFFER _glInvalidateFramebuffer;
static void APIENTRY d_glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments) {
	_glInvalidateFramebuffer(target, numAttachments, attachments);
	CheckGLError("glInvalidateFramebuffer");
}
typedef void (APIENTRYP PFNGLINVALIDATESUBFRAMEBUFFER)(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height);
PFNGLINVALIDATESUBFRAMEBUFFER gglInvalidateSubFramebuffer;
static PFNGLINVALIDATESUBFRAMEBUFFER _glInvalidateSubFramebuffer;
static void APIENTRY d_glInvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height) {
	_glInvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
	CheckGLError("glInvalidateSubFramebuffer");
}
typedef GLboolean (APIENTRYP PFNGLISBUFFER)(GLuint buffer);
PFNGLISBUFFER gglIsBuffer;
static PFNGLISBUFFER _glIsBuffer;
static GLboolean APIENTRY d_glIsBuffer(GLuint buffer) {
	GLboolean ret = _glIsBuffer(buffer);
	CheckGLError("glIsBuffer");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISENABLED)(GLenum cap);
PFNGLISENABLED gglIsEnabled;
static PFNGLISENABLED _glIsEnabled;
static GLboolean APIENTRY d_glIsEnabled(GLenum cap) {
	GLboolean ret = _glIsEnabled(cap);
	CheckGLError("glIsEnabled");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISENABLEDIEXT)(GLenum target, GLuint index);
PFNGLISENABLEDIEXT gglIsEnablediEXT;
static PFNGLISENABLEDIEXT _glIsEnablediEXT;
static GLboolean APIENTRY d_glIsEnablediEXT(GLenum target, GLuint index) {
	GLboolean ret = _glIsEnablediEXT(target, index);
	CheckGLError("glIsEnablediEXT");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISENABLEDINV)(GLenum target, GLuint index);
PFNGLISENABLEDINV gglIsEnablediNV;
static PFNGLISENABLEDINV _glIsEnablediNV;
static GLboolean APIENTRY d_glIsEnablediNV(GLenum target, GLuint index) {
	GLboolean ret = _glIsEnablediNV(target, index);
	CheckGLError("glIsEnablediNV");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISENABLEDIOES)(GLenum target, GLuint index);
PFNGLISENABLEDIOES gglIsEnablediOES;
static PFNGLISENABLEDIOES _glIsEnablediOES;
static GLboolean APIENTRY d_glIsEnablediOES(GLenum target, GLuint index) {
	GLboolean ret = _glIsEnablediOES(target, index);
	CheckGLError("glIsEnablediOES");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISFENCENV)(GLuint fence);
PFNGLISFENCENV gglIsFenceNV;
static PFNGLISFENCENV _glIsFenceNV;
static GLboolean APIENTRY d_glIsFenceNV(GLuint fence) {
	GLboolean ret = _glIsFenceNV(fence);
	CheckGLError("glIsFenceNV");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISFRAMEBUFFER)(GLuint framebuffer);
PFNGLISFRAMEBUFFER gglIsFramebuffer;
static PFNGLISFRAMEBUFFER _glIsFramebuffer;
static GLboolean APIENTRY d_glIsFramebuffer(GLuint framebuffer) {
	GLboolean ret = _glIsFramebuffer(framebuffer);
	CheckGLError("glIsFramebuffer");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISIMAGEHANDLERESIDENTNV)(GLuint64 handle);
PFNGLISIMAGEHANDLERESIDENTNV gglIsImageHandleResidentNV;
static PFNGLISIMAGEHANDLERESIDENTNV _glIsImageHandleResidentNV;
static GLboolean APIENTRY d_glIsImageHandleResidentNV(GLuint64 handle) {
	GLboolean ret = _glIsImageHandleResidentNV(handle);
	CheckGLError("glIsImageHandleResidentNV");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISPATHNV)(GLuint path);
PFNGLISPATHNV gglIsPathNV;
static PFNGLISPATHNV _glIsPathNV;
static GLboolean APIENTRY d_glIsPathNV(GLuint path) {
	GLboolean ret = _glIsPathNV(path);
	CheckGLError("glIsPathNV");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISPOINTINFILLPATHNV)(GLuint path, GLuint mask, GLfloat x, GLfloat y);
PFNGLISPOINTINFILLPATHNV gglIsPointInFillPathNV;
static PFNGLISPOINTINFILLPATHNV _glIsPointInFillPathNV;
static GLboolean APIENTRY d_glIsPointInFillPathNV(GLuint path, GLuint mask, GLfloat x, GLfloat y) {
	GLboolean ret = _glIsPointInFillPathNV(path, mask, x, y);
	CheckGLError("glIsPointInFillPathNV");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISPOINTINSTROKEPATHNV)(GLuint path, GLfloat x, GLfloat y);
PFNGLISPOINTINSTROKEPATHNV gglIsPointInStrokePathNV;
static PFNGLISPOINTINSTROKEPATHNV _glIsPointInStrokePathNV;
static GLboolean APIENTRY d_glIsPointInStrokePathNV(GLuint path, GLfloat x, GLfloat y) {
	GLboolean ret = _glIsPointInStrokePathNV(path, x, y);
	CheckGLError("glIsPointInStrokePathNV");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISPROGRAM)(GLuint program);
PFNGLISPROGRAM gglIsProgram;
static PFNGLISPROGRAM _glIsProgram;
static GLboolean APIENTRY d_glIsProgram(GLuint program) {
	GLboolean ret = _glIsProgram(program);
	CheckGLError("glIsProgram");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISPROGRAMPIPELINEEXT)(GLuint pipeline);
PFNGLISPROGRAMPIPELINEEXT gglIsProgramPipelineEXT;
static PFNGLISPROGRAMPIPELINEEXT _glIsProgramPipelineEXT;
static GLboolean APIENTRY d_glIsProgramPipelineEXT(GLuint pipeline) {
	GLboolean ret = _glIsProgramPipelineEXT(pipeline);
	CheckGLError("glIsProgramPipelineEXT");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISQUERY)(GLuint id);
PFNGLISQUERY gglIsQuery;
static PFNGLISQUERY _glIsQuery;
static GLboolean APIENTRY d_glIsQuery(GLuint id) {
	GLboolean ret = _glIsQuery(id);
	CheckGLError("glIsQuery");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISQUERYEXT)(GLuint id);
PFNGLISQUERYEXT gglIsQueryEXT;
static PFNGLISQUERYEXT _glIsQueryEXT;
static GLboolean APIENTRY d_glIsQueryEXT(GLuint id) {
	GLboolean ret = _glIsQueryEXT(id);
	CheckGLError("glIsQueryEXT");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISRENDERBUFFER)(GLuint renderbuffer);
PFNGLISRENDERBUFFER gglIsRenderbuffer;
static PFNGLISRENDERBUFFER _glIsRenderbuffer;
static GLboolean APIENTRY d_glIsRenderbuffer(GLuint renderbuffer) {
	GLboolean ret = _glIsRenderbuffer(renderbuffer);
	CheckGLError("glIsRenderbuffer");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISSAMPLER)(GLuint sampler);
PFNGLISSAMPLER gglIsSampler;
static PFNGLISSAMPLER _glIsSampler;
static GLboolean APIENTRY d_glIsSampler(GLuint sampler) {
	GLboolean ret = _glIsSampler(sampler);
	CheckGLError("glIsSampler");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISSHADER)(GLuint shader);
PFNGLISSHADER gglIsShader;
static PFNGLISSHADER _glIsShader;
static GLboolean APIENTRY d_glIsShader(GLuint shader) {
	GLboolean ret = _glIsShader(shader);
	CheckGLError("glIsShader");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISSYNC)(GLsync sync);
PFNGLISSYNC gglIsSync;
static PFNGLISSYNC _glIsSync;
static GLboolean APIENTRY d_glIsSync(GLsync sync) {
	GLboolean ret = _glIsSync(sync);
	CheckGLError("glIsSync");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISSYNCAPPLE)(GLsync sync);
PFNGLISSYNCAPPLE gglIsSyncAPPLE;
static PFNGLISSYNCAPPLE _glIsSyncAPPLE;
static GLboolean APIENTRY d_glIsSyncAPPLE(GLsync sync) {
	GLboolean ret = _glIsSyncAPPLE(sync);
	CheckGLError("glIsSyncAPPLE");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISTEXTURE)(GLuint texture);
PFNGLISTEXTURE gglIsTexture;
static PFNGLISTEXTURE _glIsTexture;
static GLboolean APIENTRY d_glIsTexture(GLuint texture) {
	GLboolean ret = _glIsTexture(texture);
	CheckGLError("glIsTexture");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISTEXTUREHANDLERESIDENTNV)(GLuint64 handle);
PFNGLISTEXTUREHANDLERESIDENTNV gglIsTextureHandleResidentNV;
static PFNGLISTEXTUREHANDLERESIDENTNV _glIsTextureHandleResidentNV;
static GLboolean APIENTRY d_glIsTextureHandleResidentNV(GLuint64 handle) {
	GLboolean ret = _glIsTextureHandleResidentNV(handle);
	CheckGLError("glIsTextureHandleResidentNV");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISTRANSFORMFEEDBACK)(GLuint id);
PFNGLISTRANSFORMFEEDBACK gglIsTransformFeedback;
static PFNGLISTRANSFORMFEEDBACK _glIsTransformFeedback;
static GLboolean APIENTRY d_glIsTransformFeedback(GLuint id) {
	GLboolean ret = _glIsTransformFeedback(id);
	CheckGLError("glIsTransformFeedback");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISVERTEXARRAY)(GLuint array);
PFNGLISVERTEXARRAY gglIsVertexArray;
static PFNGLISVERTEXARRAY _glIsVertexArray;
static GLboolean APIENTRY d_glIsVertexArray(GLuint array) {
	GLboolean ret = _glIsVertexArray(array);
	CheckGLError("glIsVertexArray");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLISVERTEXARRAYOES)(GLuint array);
PFNGLISVERTEXARRAYOES gglIsVertexArrayOES;
static PFNGLISVERTEXARRAYOES _glIsVertexArrayOES;
static GLboolean APIENTRY d_glIsVertexArrayOES(GLuint array) {
	GLboolean ret = _glIsVertexArrayOES(array);
	CheckGLError("glIsVertexArrayOES");
	return ret;
}
typedef void (APIENTRYP PFNGLLABELOBJECTEXT)(GLenum type, GLuint object, GLsizei length, const GLchar *label);
PFNGLLABELOBJECTEXT gglLabelObjectEXT;
static PFNGLLABELOBJECTEXT _glLabelObjectEXT;
static void APIENTRY d_glLabelObjectEXT(GLenum type, GLuint object, GLsizei length, const GLchar *label) {
	_glLabelObjectEXT(type, object, length, label);
	CheckGLError("glLabelObjectEXT");
}
typedef void (APIENTRYP PFNGLLINEWIDTH)(GLfloat width);
PFNGLLINEWIDTH gglLineWidth;
static PFNGLLINEWIDTH _glLineWidth;
static void APIENTRY d_glLineWidth(GLfloat width) {
	_glLineWidth(width);
	CheckGLError("glLineWidth");
}
typedef void (APIENTRYP PFNGLLINKPROGRAM)(GLuint program);
PFNGLLINKPROGRAM gglLinkProgram;
static PFNGLLINKPROGRAM _glLinkProgram;
static void APIENTRY d_glLinkProgram(GLuint program) {
	_glLinkProgram(program);
	CheckGLError("glLinkProgram");
}
typedef void (APIENTRYP PFNGLMAKEIMAGEHANDLENONRESIDENTNV)(GLuint64 handle);
PFNGLMAKEIMAGEHANDLENONRESIDENTNV gglMakeImageHandleNonResidentNV;
static PFNGLMAKEIMAGEHANDLENONRESIDENTNV _glMakeImageHandleNonResidentNV;
static void APIENTRY d_glMakeImageHandleNonResidentNV(GLuint64 handle) {
	_glMakeImageHandleNonResidentNV(handle);
	CheckGLError("glMakeImageHandleNonResidentNV");
}
typedef void (APIENTRYP PFNGLMAKEIMAGEHANDLERESIDENTNV)(GLuint64 handle, GLenum access);
PFNGLMAKEIMAGEHANDLERESIDENTNV gglMakeImageHandleResidentNV;
static PFNGLMAKEIMAGEHANDLERESIDENTNV _glMakeImageHandleResidentNV;
static void APIENTRY d_glMakeImageHandleResidentNV(GLuint64 handle, GLenum access) {
	_glMakeImageHandleResidentNV(handle, access);
	CheckGLError("glMakeImageHandleResidentNV");
}
typedef void (APIENTRYP PFNGLMAKETEXTUREHANDLENONRESIDENTNV)(GLuint64 handle);
PFNGLMAKETEXTUREHANDLENONRESIDENTNV gglMakeTextureHandleNonResidentNV;
static PFNGLMAKETEXTUREHANDLENONRESIDENTNV _glMakeTextureHandleNonResidentNV;
static void APIENTRY d_glMakeTextureHandleNonResidentNV(GLuint64 handle) {
	_glMakeTextureHandleNonResidentNV(handle);
	CheckGLError("glMakeTextureHandleNonResidentNV");
}
typedef void (APIENTRYP PFNGLMAKETEXTUREHANDLERESIDENTNV)(GLuint64 handle);
PFNGLMAKETEXTUREHANDLERESIDENTNV gglMakeTextureHandleResidentNV;
static PFNGLMAKETEXTUREHANDLERESIDENTNV _glMakeTextureHandleResidentNV;
static void APIENTRY d_glMakeTextureHandleResidentNV(GLuint64 handle) {
	_glMakeTextureHandleResidentNV(handle);
	CheckGLError("glMakeTextureHandleResidentNV");
}
typedef void * (APIENTRYP PFNGLMAPBUFFEROES)(GLenum target, GLenum access);
PFNGLMAPBUFFEROES gglMapBufferOES;
static PFNGLMAPBUFFEROES _glMapBufferOES;
static void * APIENTRY d_glMapBufferOES(GLenum target, GLenum access) {
	void * ret = _glMapBufferOES(target, access);
	CheckGLError("glMapBufferOES");
	return ret;
}
typedef void * (APIENTRYP PFNGLMAPBUFFERRANGE)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
PFNGLMAPBUFFERRANGE gglMapBufferRange;
static PFNGLMAPBUFFERRANGE _glMapBufferRange;
static void * APIENTRY d_glMapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
	void * ret = _glMapBufferRange(target, offset, length, access);
	CheckGLError("glMapBufferRange");
	return ret;
}
typedef void * (APIENTRYP PFNGLMAPBUFFERRANGEEXT)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
PFNGLMAPBUFFERRANGEEXT gglMapBufferRangeEXT;
static PFNGLMAPBUFFERRANGEEXT _glMapBufferRangeEXT;
static void * APIENTRY d_glMapBufferRangeEXT(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) {
	void * ret = _glMapBufferRangeEXT(target, offset, length, access);
	CheckGLError("glMapBufferRangeEXT");
	return ret;
}
typedef void (APIENTRYP PFNGLMATRIXLOAD3X2FNV)(GLenum matrixMode, const GLfloat *m);
PFNGLMATRIXLOAD3X2FNV gglMatrixLoad3x2fNV;
static PFNGLMATRIXLOAD3X2FNV _glMatrixLoad3x2fNV;
static void APIENTRY d_glMatrixLoad3x2fNV(GLenum matrixMode, const GLfloat *m) {
	_glMatrixLoad3x2fNV(matrixMode, m);
	CheckGLError("glMatrixLoad3x2fNV");
}
typedef void (APIENTRYP PFNGLMATRIXLOAD3X3FNV)(GLenum matrixMode, const GLfloat *m);
PFNGLMATRIXLOAD3X3FNV gglMatrixLoad3x3fNV;
static PFNGLMATRIXLOAD3X3FNV _glMatrixLoad3x3fNV;
static void APIENTRY d_glMatrixLoad3x3fNV(GLenum matrixMode, const GLfloat *m) {
	_glMatrixLoad3x3fNV(matrixMode, m);
	CheckGLError("glMatrixLoad3x3fNV");
}
typedef void (APIENTRYP PFNGLMATRIXLOADTRANSPOSE3X3FNV)(GLenum matrixMode, const GLfloat *m);
PFNGLMATRIXLOADTRANSPOSE3X3FNV gglMatrixLoadTranspose3x3fNV;
static PFNGLMATRIXLOADTRANSPOSE3X3FNV _glMatrixLoadTranspose3x3fNV;
static void APIENTRY d_glMatrixLoadTranspose3x3fNV(GLenum matrixMode, const GLfloat *m) {
	_glMatrixLoadTranspose3x3fNV(matrixMode, m);
	CheckGLError("glMatrixLoadTranspose3x3fNV");
}
typedef void (APIENTRYP PFNGLMATRIXMULT3X2FNV)(GLenum matrixMode, const GLfloat *m);
PFNGLMATRIXMULT3X2FNV gglMatrixMult3x2fNV;
static PFNGLMATRIXMULT3X2FNV _glMatrixMult3x2fNV;
static void APIENTRY d_glMatrixMult3x2fNV(GLenum matrixMode, const GLfloat *m) {
	_glMatrixMult3x2fNV(matrixMode, m);
	CheckGLError("glMatrixMult3x2fNV");
}
typedef void (APIENTRYP PFNGLMATRIXMULT3X3FNV)(GLenum matrixMode, const GLfloat *m);
PFNGLMATRIXMULT3X3FNV gglMatrixMult3x3fNV;
static PFNGLMATRIXMULT3X3FNV _glMatrixMult3x3fNV;
static void APIENTRY d_glMatrixMult3x3fNV(GLenum matrixMode, const GLfloat *m) {
	_glMatrixMult3x3fNV(matrixMode, m);
	CheckGLError("glMatrixMult3x3fNV");
}
typedef void (APIENTRYP PFNGLMATRIXMULTTRANSPOSE3X3FNV)(GLenum matrixMode, const GLfloat *m);
PFNGLMATRIXMULTTRANSPOSE3X3FNV gglMatrixMultTranspose3x3fNV;
static PFNGLMATRIXMULTTRANSPOSE3X3FNV _glMatrixMultTranspose3x3fNV;
static void APIENTRY d_glMatrixMultTranspose3x3fNV(GLenum matrixMode, const GLfloat *m) {
	_glMatrixMultTranspose3x3fNV(matrixMode, m);
	CheckGLError("glMatrixMultTranspose3x3fNV");
}
typedef void (APIENTRYP PFNGLMINSAMPLESHADINGOES)(GLfloat value);
PFNGLMINSAMPLESHADINGOES gglMinSampleShadingOES;
static PFNGLMINSAMPLESHADINGOES _glMinSampleShadingOES;
static void APIENTRY d_glMinSampleShadingOES(GLfloat value) {
	_glMinSampleShadingOES(value);
	CheckGLError("glMinSampleShadingOES");
}
typedef void (APIENTRYP PFNGLMULTIDRAWARRAYSEXT)(GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount);
PFNGLMULTIDRAWARRAYSEXT gglMultiDrawArraysEXT;
static PFNGLMULTIDRAWARRAYSEXT _glMultiDrawArraysEXT;
static void APIENTRY d_glMultiDrawArraysEXT(GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount) {
	_glMultiDrawArraysEXT(mode, first, count, primcount);
	CheckGLError("glMultiDrawArraysEXT");
}
typedef void (APIENTRYP PFNGLMULTIDRAWARRAYSINDIRECTEXT)(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride);
PFNGLMULTIDRAWARRAYSINDIRECTEXT gglMultiDrawArraysIndirectEXT;
static PFNGLMULTIDRAWARRAYSINDIRECTEXT _glMultiDrawArraysIndirectEXT;
static void APIENTRY d_glMultiDrawArraysIndirectEXT(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride) {
	_glMultiDrawArraysIndirectEXT(mode, indirect, drawcount, stride);
	CheckGLError("glMultiDrawArraysIndirectEXT");
}
typedef void (APIENTRYP PFNGLMULTIDRAWELEMENTSBASEVERTEXEXT)(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount, const GLint *basevertex);
PFNGLMULTIDRAWELEMENTSBASEVERTEXEXT gglMultiDrawElementsBaseVertexEXT;
static PFNGLMULTIDRAWELEMENTSBASEVERTEXEXT _glMultiDrawElementsBaseVertexEXT;
static void APIENTRY d_glMultiDrawElementsBaseVertexEXT(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount, const GLint *basevertex) {
	_glMultiDrawElementsBaseVertexEXT(mode, count, type, indices, primcount, basevertex);
	CheckGLError("glMultiDrawElementsBaseVertexEXT");
}
typedef void (APIENTRYP PFNGLMULTIDRAWELEMENTSBASEVERTEXOES)(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount, const GLint *basevertex);
PFNGLMULTIDRAWELEMENTSBASEVERTEXOES gglMultiDrawElementsBaseVertexOES;
static PFNGLMULTIDRAWELEMENTSBASEVERTEXOES _glMultiDrawElementsBaseVertexOES;
static void APIENTRY d_glMultiDrawElementsBaseVertexOES(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount, const GLint *basevertex) {
	_glMultiDrawElementsBaseVertexOES(mode, count, type, indices, primcount, basevertex);
	CheckGLError("glMultiDrawElementsBaseVertexOES");
}
typedef void (APIENTRYP PFNGLMULTIDRAWELEMENTSEXT)(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount);
PFNGLMULTIDRAWELEMENTSEXT gglMultiDrawElementsEXT;
static PFNGLMULTIDRAWELEMENTSEXT _glMultiDrawElementsEXT;
static void APIENTRY d_glMultiDrawElementsEXT(GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount) {
	_glMultiDrawElementsEXT(mode, count, type, indices, primcount);
	CheckGLError("glMultiDrawElementsEXT");
}
typedef void (APIENTRYP PFNGLMULTIDRAWELEMENTSINDIRECTEXT)(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride);
PFNGLMULTIDRAWELEMENTSINDIRECTEXT gglMultiDrawElementsIndirectEXT;
static PFNGLMULTIDRAWELEMENTSINDIRECTEXT _glMultiDrawElementsIndirectEXT;
static void APIENTRY d_glMultiDrawElementsIndirectEXT(GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride) {
	_glMultiDrawElementsIndirectEXT(mode, type, indirect, drawcount, stride);
	CheckGLError("glMultiDrawElementsIndirectEXT");
}
typedef void (APIENTRYP PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNV)(GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v);
PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNV gglNamedFramebufferSampleLocationsfvNV;
static PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNV _glNamedFramebufferSampleLocationsfvNV;
static void APIENTRY d_glNamedFramebufferSampleLocationsfvNV(GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v) {
	_glNamedFramebufferSampleLocationsfvNV(framebuffer, start, count, v);
	CheckGLError("glNamedFramebufferSampleLocationsfvNV");
}
typedef void (APIENTRYP PFNGLOBJECTLABELKHR)(GLenum identifier, GLuint name, GLsizei length, const GLchar *label);
PFNGLOBJECTLABELKHR gglObjectLabelKHR;
static PFNGLOBJECTLABELKHR _glObjectLabelKHR;
static void APIENTRY d_glObjectLabelKHR(GLenum identifier, GLuint name, GLsizei length, const GLchar *label) {
	_glObjectLabelKHR(identifier, name, length, label);
	CheckGLError("glObjectLabelKHR");
}
typedef void (APIENTRYP PFNGLOBJECTPTRLABELKHR)(const void *ptr, GLsizei length, const GLchar *label);
PFNGLOBJECTPTRLABELKHR gglObjectPtrLabelKHR;
static PFNGLOBJECTPTRLABELKHR _glObjectPtrLabelKHR;
static void APIENTRY d_glObjectPtrLabelKHR(const void *ptr, GLsizei length, const GLchar *label) {
	_glObjectPtrLabelKHR(ptr, length, label);
	CheckGLError("glObjectPtrLabelKHR");
}
typedef void (APIENTRYP PFNGLPATCHPARAMETERIEXT)(GLenum pname, GLint value);
PFNGLPATCHPARAMETERIEXT gglPatchParameteriEXT;
static PFNGLPATCHPARAMETERIEXT _glPatchParameteriEXT;
static void APIENTRY d_glPatchParameteriEXT(GLenum pname, GLint value) {
	_glPatchParameteriEXT(pname, value);
	CheckGLError("glPatchParameteriEXT");
}
typedef void (APIENTRYP PFNGLPATCHPARAMETERIOES)(GLenum pname, GLint value);
PFNGLPATCHPARAMETERIOES gglPatchParameteriOES;
static PFNGLPATCHPARAMETERIOES _glPatchParameteriOES;
static void APIENTRY d_glPatchParameteriOES(GLenum pname, GLint value) {
	_glPatchParameteriOES(pname, value);
	CheckGLError("glPatchParameteriOES");
}
typedef void (APIENTRYP PFNGLPATHCOMMANDSNV)(GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords);
PFNGLPATHCOMMANDSNV gglPathCommandsNV;
static PFNGLPATHCOMMANDSNV _glPathCommandsNV;
static void APIENTRY d_glPathCommandsNV(GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords) {
	_glPathCommandsNV(path, numCommands, commands, numCoords, coordType, coords);
	CheckGLError("glPathCommandsNV");
}
typedef void (APIENTRYP PFNGLPATHCOORDSNV)(GLuint path, GLsizei numCoords, GLenum coordType, const void *coords);
PFNGLPATHCOORDSNV gglPathCoordsNV;
static PFNGLPATHCOORDSNV _glPathCoordsNV;
static void APIENTRY d_glPathCoordsNV(GLuint path, GLsizei numCoords, GLenum coordType, const void *coords) {
	_glPathCoordsNV(path, numCoords, coordType, coords);
	CheckGLError("glPathCoordsNV");
}
typedef void (APIENTRYP PFNGLPATHCOVERDEPTHFUNCNV)(GLenum func);
PFNGLPATHCOVERDEPTHFUNCNV gglPathCoverDepthFuncNV;
static PFNGLPATHCOVERDEPTHFUNCNV _glPathCoverDepthFuncNV;
static void APIENTRY d_glPathCoverDepthFuncNV(GLenum func) {
	_glPathCoverDepthFuncNV(func);
	CheckGLError("glPathCoverDepthFuncNV");
}
typedef void (APIENTRYP PFNGLPATHDASHARRAYNV)(GLuint path, GLsizei dashCount, const GLfloat *dashArray);
PFNGLPATHDASHARRAYNV gglPathDashArrayNV;
static PFNGLPATHDASHARRAYNV _glPathDashArrayNV;
static void APIENTRY d_glPathDashArrayNV(GLuint path, GLsizei dashCount, const GLfloat *dashArray) {
	_glPathDashArrayNV(path, dashCount, dashArray);
	CheckGLError("glPathDashArrayNV");
}
typedef GLenum (APIENTRYP PFNGLPATHGLYPHINDEXARRAYNV)(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
PFNGLPATHGLYPHINDEXARRAYNV gglPathGlyphIndexArrayNV;
static PFNGLPATHGLYPHINDEXARRAYNV _glPathGlyphIndexArrayNV;
static GLenum APIENTRY d_glPathGlyphIndexArrayNV(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale) {
	GLenum ret = _glPathGlyphIndexArrayNV(firstPathName, fontTarget, fontName, fontStyle, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale);
	CheckGLError("glPathGlyphIndexArrayNV");
	return ret;
}
typedef GLenum (APIENTRYP PFNGLPATHGLYPHINDEXRANGENV)(GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint baseAndCount[2]);
PFNGLPATHGLYPHINDEXRANGENV gglPathGlyphIndexRangeNV;
static PFNGLPATHGLYPHINDEXRANGENV _glPathGlyphIndexRangeNV;
static GLenum APIENTRY d_glPathGlyphIndexRangeNV(GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint baseAndCount[2]) {
	GLenum ret = _glPathGlyphIndexRangeNV(fontTarget, fontName, fontStyle, pathParameterTemplate, emScale, baseAndCount);
	CheckGLError("glPathGlyphIndexRangeNV");
	return ret;
}
typedef void (APIENTRYP PFNGLPATHGLYPHRANGENV)(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
PFNGLPATHGLYPHRANGENV gglPathGlyphRangeNV;
static PFNGLPATHGLYPHRANGENV _glPathGlyphRangeNV;
static void APIENTRY d_glPathGlyphRangeNV(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale) {
	_glPathGlyphRangeNV(firstPathName, fontTarget, fontName, fontStyle, firstGlyph, numGlyphs, handleMissingGlyphs, pathParameterTemplate, emScale);
	CheckGLError("glPathGlyphRangeNV");
}
typedef void (APIENTRYP PFNGLPATHGLYPHSNV)(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void *charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
PFNGLPATHGLYPHSNV gglPathGlyphsNV;
static PFNGLPATHGLYPHSNV _glPathGlyphsNV;
static void APIENTRY d_glPathGlyphsNV(GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void *charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale) {
	_glPathGlyphsNV(firstPathName, fontTarget, fontName, fontStyle, numGlyphs, type, charcodes, handleMissingGlyphs, pathParameterTemplate, emScale);
	CheckGLError("glPathGlyphsNV");
}
typedef GLenum (APIENTRYP PFNGLPATHMEMORYGLYPHINDEXARRAYNV)(GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void *fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale);
PFNGLPATHMEMORYGLYPHINDEXARRAYNV gglPathMemoryGlyphIndexArrayNV;
static PFNGLPATHMEMORYGLYPHINDEXARRAYNV _glPathMemoryGlyphIndexArrayNV;
static GLenum APIENTRY d_glPathMemoryGlyphIndexArrayNV(GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void *fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale) {
	GLenum ret = _glPathMemoryGlyphIndexArrayNV(firstPathName, fontTarget, fontSize, fontData, faceIndex, firstGlyphIndex, numGlyphs, pathParameterTemplate, emScale);
	CheckGLError("glPathMemoryGlyphIndexArrayNV");
	return ret;
}
typedef void (APIENTRYP PFNGLPATHPARAMETERFNV)(GLuint path, GLenum pname, GLfloat value);
PFNGLPATHPARAMETERFNV gglPathParameterfNV;
static PFNGLPATHPARAMETERFNV _glPathParameterfNV;
static void APIENTRY d_glPathParameterfNV(GLuint path, GLenum pname, GLfloat value) {
	_glPathParameterfNV(path, pname, value);
	CheckGLError("glPathParameterfNV");
}
typedef void (APIENTRYP PFNGLPATHPARAMETERFVNV)(GLuint path, GLenum pname, const GLfloat *value);
PFNGLPATHPARAMETERFVNV gglPathParameterfvNV;
static PFNGLPATHPARAMETERFVNV _glPathParameterfvNV;
static void APIENTRY d_glPathParameterfvNV(GLuint path, GLenum pname, const GLfloat *value) {
	_glPathParameterfvNV(path, pname, value);
	CheckGLError("glPathParameterfvNV");
}
typedef void (APIENTRYP PFNGLPATHPARAMETERINV)(GLuint path, GLenum pname, GLint value);
PFNGLPATHPARAMETERINV gglPathParameteriNV;
static PFNGLPATHPARAMETERINV _glPathParameteriNV;
static void APIENTRY d_glPathParameteriNV(GLuint path, GLenum pname, GLint value) {
	_glPathParameteriNV(path, pname, value);
	CheckGLError("glPathParameteriNV");
}
typedef void (APIENTRYP PFNGLPATHPARAMETERIVNV)(GLuint path, GLenum pname, const GLint *value);
PFNGLPATHPARAMETERIVNV gglPathParameterivNV;
static PFNGLPATHPARAMETERIVNV _glPathParameterivNV;
static void APIENTRY d_glPathParameterivNV(GLuint path, GLenum pname, const GLint *value) {
	_glPathParameterivNV(path, pname, value);
	CheckGLError("glPathParameterivNV");
}
typedef void (APIENTRYP PFNGLPATHSTENCILDEPTHOFFSETNV)(GLfloat factor, GLfloat units);
PFNGLPATHSTENCILDEPTHOFFSETNV gglPathStencilDepthOffsetNV;
static PFNGLPATHSTENCILDEPTHOFFSETNV _glPathStencilDepthOffsetNV;
static void APIENTRY d_glPathStencilDepthOffsetNV(GLfloat factor, GLfloat units) {
	_glPathStencilDepthOffsetNV(factor, units);
	CheckGLError("glPathStencilDepthOffsetNV");
}
typedef void (APIENTRYP PFNGLPATHSTENCILFUNCNV)(GLenum func, GLint ref, GLuint mask);
PFNGLPATHSTENCILFUNCNV gglPathStencilFuncNV;
static PFNGLPATHSTENCILFUNCNV _glPathStencilFuncNV;
static void APIENTRY d_glPathStencilFuncNV(GLenum func, GLint ref, GLuint mask) {
	_glPathStencilFuncNV(func, ref, mask);
	CheckGLError("glPathStencilFuncNV");
}
typedef void (APIENTRYP PFNGLPATHSTRINGNV)(GLuint path, GLenum format, GLsizei length, const void *pathString);
PFNGLPATHSTRINGNV gglPathStringNV;
static PFNGLPATHSTRINGNV _glPathStringNV;
static void APIENTRY d_glPathStringNV(GLuint path, GLenum format, GLsizei length, const void *pathString) {
	_glPathStringNV(path, format, length, pathString);
	CheckGLError("glPathStringNV");
}
typedef void (APIENTRYP PFNGLPATHSUBCOMMANDSNV)(GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords);
PFNGLPATHSUBCOMMANDSNV gglPathSubCommandsNV;
static PFNGLPATHSUBCOMMANDSNV _glPathSubCommandsNV;
static void APIENTRY d_glPathSubCommandsNV(GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords) {
	_glPathSubCommandsNV(path, commandStart, commandsToDelete, numCommands, commands, numCoords, coordType, coords);
	CheckGLError("glPathSubCommandsNV");
}
typedef void (APIENTRYP PFNGLPATHSUBCOORDSNV)(GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void *coords);
PFNGLPATHSUBCOORDSNV gglPathSubCoordsNV;
static PFNGLPATHSUBCOORDSNV _glPathSubCoordsNV;
static void APIENTRY d_glPathSubCoordsNV(GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void *coords) {
	_glPathSubCoordsNV(path, coordStart, numCoords, coordType, coords);
	CheckGLError("glPathSubCoordsNV");
}
typedef void (APIENTRYP PFNGLPAUSETRANSFORMFEEDBACK)();
PFNGLPAUSETRANSFORMFEEDBACK gglPauseTransformFeedback;
static PFNGLPAUSETRANSFORMFEEDBACK _glPauseTransformFeedback;
static void APIENTRY d_glPauseTransformFeedback() {
	_glPauseTransformFeedback();
	CheckGLError("glPauseTransformFeedback");
}
typedef void (APIENTRYP PFNGLPIXELSTOREI)(GLenum pname, GLint param);
PFNGLPIXELSTOREI gglPixelStorei;
static PFNGLPIXELSTOREI _glPixelStorei;
static void APIENTRY d_glPixelStorei(GLenum pname, GLint param) {
	_glPixelStorei(pname, param);
	CheckGLError("glPixelStorei");
}
typedef GLboolean (APIENTRYP PFNGLPOINTALONGPATHNV)(GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat *x, GLfloat *y, GLfloat *tangentX, GLfloat *tangentY);
PFNGLPOINTALONGPATHNV gglPointAlongPathNV;
static PFNGLPOINTALONGPATHNV _glPointAlongPathNV;
static GLboolean APIENTRY d_glPointAlongPathNV(GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat *x, GLfloat *y, GLfloat *tangentX, GLfloat *tangentY) {
	GLboolean ret = _glPointAlongPathNV(path, startSegment, numSegments, distance, x, y, tangentX, tangentY);
	CheckGLError("glPointAlongPathNV");
	return ret;
}
typedef void (APIENTRYP PFNGLPOLYGONMODENV)(GLenum face, GLenum mode);
PFNGLPOLYGONMODENV gglPolygonModeNV;
static PFNGLPOLYGONMODENV _glPolygonModeNV;
static void APIENTRY d_glPolygonModeNV(GLenum face, GLenum mode) {
	_glPolygonModeNV(face, mode);
	CheckGLError("glPolygonModeNV");
}
typedef void (APIENTRYP PFNGLPOLYGONOFFSET)(GLfloat factor, GLfloat units);
PFNGLPOLYGONOFFSET gglPolygonOffset;
static PFNGLPOLYGONOFFSET _glPolygonOffset;
static void APIENTRY d_glPolygonOffset(GLfloat factor, GLfloat units) {
	_glPolygonOffset(factor, units);
	CheckGLError("glPolygonOffset");
}
typedef void (APIENTRYP PFNGLPOLYGONOFFSETCLAMPEXT)(GLfloat factor, GLfloat units, GLfloat clamp);
PFNGLPOLYGONOFFSETCLAMPEXT gglPolygonOffsetClampEXT;
static PFNGLPOLYGONOFFSETCLAMPEXT _glPolygonOffsetClampEXT;
static void APIENTRY d_glPolygonOffsetClampEXT(GLfloat factor, GLfloat units, GLfloat clamp) {
	_glPolygonOffsetClampEXT(factor, units, clamp);
	CheckGLError("glPolygonOffsetClampEXT");
}
typedef void (APIENTRYP PFNGLPOPDEBUGGROUPKHR)();
PFNGLPOPDEBUGGROUPKHR gglPopDebugGroupKHR;
static PFNGLPOPDEBUGGROUPKHR _glPopDebugGroupKHR;
static void APIENTRY d_glPopDebugGroupKHR() {
	_glPopDebugGroupKHR();
	CheckGLError("glPopDebugGroupKHR");
}
typedef void (APIENTRYP PFNGLPOPGROUPMARKEREXT)();
PFNGLPOPGROUPMARKEREXT gglPopGroupMarkerEXT;
static PFNGLPOPGROUPMARKEREXT _glPopGroupMarkerEXT;
static void APIENTRY d_glPopGroupMarkerEXT() {
	_glPopGroupMarkerEXT();
	CheckGLError("glPopGroupMarkerEXT");
}
typedef void (APIENTRYP PFNGLPRIMITIVEBOUNDINGBOXEXT)(GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW);
PFNGLPRIMITIVEBOUNDINGBOXEXT gglPrimitiveBoundingBoxEXT;
static PFNGLPRIMITIVEBOUNDINGBOXEXT _glPrimitiveBoundingBoxEXT;
static void APIENTRY d_glPrimitiveBoundingBoxEXT(GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW) {
	_glPrimitiveBoundingBoxEXT(minX, minY, minZ, minW, maxX, maxY, maxZ, maxW);
	CheckGLError("glPrimitiveBoundingBoxEXT");
}
typedef void (APIENTRYP PFNGLPRIMITIVEBOUNDINGBOXOES)(GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW);
PFNGLPRIMITIVEBOUNDINGBOXOES gglPrimitiveBoundingBoxOES;
static PFNGLPRIMITIVEBOUNDINGBOXOES _glPrimitiveBoundingBoxOES;
static void APIENTRY d_glPrimitiveBoundingBoxOES(GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW) {
	_glPrimitiveBoundingBoxOES(minX, minY, minZ, minW, maxX, maxY, maxZ, maxW);
	CheckGLError("glPrimitiveBoundingBoxOES");
}
typedef void (APIENTRYP PFNGLPROGRAMBINARY)(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
PFNGLPROGRAMBINARY gglProgramBinary;
static PFNGLPROGRAMBINARY _glProgramBinary;
static void APIENTRY d_glProgramBinary(GLuint program, GLenum binaryFormat, const void *binary, GLsizei length) {
	_glProgramBinary(program, binaryFormat, binary, length);
	CheckGLError("glProgramBinary");
}
typedef void (APIENTRYP PFNGLPROGRAMBINARYOES)(GLuint program, GLenum binaryFormat, const void *binary, GLint length);
PFNGLPROGRAMBINARYOES gglProgramBinaryOES;
static PFNGLPROGRAMBINARYOES _glProgramBinaryOES;
static void APIENTRY d_glProgramBinaryOES(GLuint program, GLenum binaryFormat, const void *binary, GLint length) {
	_glProgramBinaryOES(program, binaryFormat, binary, length);
	CheckGLError("glProgramBinaryOES");
}
typedef void (APIENTRYP PFNGLPROGRAMPARAMETERI)(GLuint program, GLenum pname, GLint value);
PFNGLPROGRAMPARAMETERI gglProgramParameteri;
static PFNGLPROGRAMPARAMETERI _glProgramParameteri;
static void APIENTRY d_glProgramParameteri(GLuint program, GLenum pname, GLint value) {
	_glProgramParameteri(program, pname, value);
	CheckGLError("glProgramParameteri");
}
typedef void (APIENTRYP PFNGLPROGRAMPARAMETERIEXT)(GLuint program, GLenum pname, GLint value);
PFNGLPROGRAMPARAMETERIEXT gglProgramParameteriEXT;
static PFNGLPROGRAMPARAMETERIEXT _glProgramParameteriEXT;
static void APIENTRY d_glProgramParameteriEXT(GLuint program, GLenum pname, GLint value) {
	_glProgramParameteriEXT(program, pname, value);
	CheckGLError("glProgramParameteriEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMPATHFRAGMENTINPUTGENNV)(GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat *coeffs);
PFNGLPROGRAMPATHFRAGMENTINPUTGENNV gglProgramPathFragmentInputGenNV;
static PFNGLPROGRAMPATHFRAGMENTINPUTGENNV _glProgramPathFragmentInputGenNV;
static void APIENTRY d_glProgramPathFragmentInputGenNV(GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat *coeffs) {
	_glProgramPathFragmentInputGenNV(program, location, genMode, components, coeffs);
	CheckGLError("glProgramPathFragmentInputGenNV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM1FEXT)(GLuint program, GLint location, GLfloat v0);
PFNGLPROGRAMUNIFORM1FEXT gglProgramUniform1fEXT;
static PFNGLPROGRAMUNIFORM1FEXT _glProgramUniform1fEXT;
static void APIENTRY d_glProgramUniform1fEXT(GLuint program, GLint location, GLfloat v0) {
	_glProgramUniform1fEXT(program, location, v0);
	CheckGLError("glProgramUniform1fEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM1FVEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
PFNGLPROGRAMUNIFORM1FVEXT gglProgramUniform1fvEXT;
static PFNGLPROGRAMUNIFORM1FVEXT _glProgramUniform1fvEXT;
static void APIENTRY d_glProgramUniform1fvEXT(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
	_glProgramUniform1fvEXT(program, location, count, value);
	CheckGLError("glProgramUniform1fvEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM1I64NV)(GLuint program, GLint location, GLint64EXT x);
PFNGLPROGRAMUNIFORM1I64NV gglProgramUniform1i64NV;
static PFNGLPROGRAMUNIFORM1I64NV _glProgramUniform1i64NV;
static void APIENTRY d_glProgramUniform1i64NV(GLuint program, GLint location, GLint64EXT x) {
	_glProgramUniform1i64NV(program, location, x);
	CheckGLError("glProgramUniform1i64NV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM1I64VNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value);
PFNGLPROGRAMUNIFORM1I64VNV gglProgramUniform1i64vNV;
static PFNGLPROGRAMUNIFORM1I64VNV _glProgramUniform1i64vNV;
static void APIENTRY d_glProgramUniform1i64vNV(GLuint program, GLint location, GLsizei count, const GLint64EXT *value) {
	_glProgramUniform1i64vNV(program, location, count, value);
	CheckGLError("glProgramUniform1i64vNV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM1IEXT)(GLuint program, GLint location, GLint v0);
PFNGLPROGRAMUNIFORM1IEXT gglProgramUniform1iEXT;
static PFNGLPROGRAMUNIFORM1IEXT _glProgramUniform1iEXT;
static void APIENTRY d_glProgramUniform1iEXT(GLuint program, GLint location, GLint v0) {
	_glProgramUniform1iEXT(program, location, v0);
	CheckGLError("glProgramUniform1iEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM1IVEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
PFNGLPROGRAMUNIFORM1IVEXT gglProgramUniform1ivEXT;
static PFNGLPROGRAMUNIFORM1IVEXT _glProgramUniform1ivEXT;
static void APIENTRY d_glProgramUniform1ivEXT(GLuint program, GLint location, GLsizei count, const GLint *value) {
	_glProgramUniform1ivEXT(program, location, count, value);
	CheckGLError("glProgramUniform1ivEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM1UI64NV)(GLuint program, GLint location, GLuint64EXT x);
PFNGLPROGRAMUNIFORM1UI64NV gglProgramUniform1ui64NV;
static PFNGLPROGRAMUNIFORM1UI64NV _glProgramUniform1ui64NV;
static void APIENTRY d_glProgramUniform1ui64NV(GLuint program, GLint location, GLuint64EXT x) {
	_glProgramUniform1ui64NV(program, location, x);
	CheckGLError("glProgramUniform1ui64NV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM1UI64VNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);
PFNGLPROGRAMUNIFORM1UI64VNV gglProgramUniform1ui64vNV;
static PFNGLPROGRAMUNIFORM1UI64VNV _glProgramUniform1ui64vNV;
static void APIENTRY d_glProgramUniform1ui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) {
	_glProgramUniform1ui64vNV(program, location, count, value);
	CheckGLError("glProgramUniform1ui64vNV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM1UIEXT)(GLuint program, GLint location, GLuint v0);
PFNGLPROGRAMUNIFORM1UIEXT gglProgramUniform1uiEXT;
static PFNGLPROGRAMUNIFORM1UIEXT _glProgramUniform1uiEXT;
static void APIENTRY d_glProgramUniform1uiEXT(GLuint program, GLint location, GLuint v0) {
	_glProgramUniform1uiEXT(program, location, v0);
	CheckGLError("glProgramUniform1uiEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM1UIVEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
PFNGLPROGRAMUNIFORM1UIVEXT gglProgramUniform1uivEXT;
static PFNGLPROGRAMUNIFORM1UIVEXT _glProgramUniform1uivEXT;
static void APIENTRY d_glProgramUniform1uivEXT(GLuint program, GLint location, GLsizei count, const GLuint *value) {
	_glProgramUniform1uivEXT(program, location, count, value);
	CheckGLError("glProgramUniform1uivEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM2FEXT)(GLuint program, GLint location, GLfloat v0, GLfloat v1);
PFNGLPROGRAMUNIFORM2FEXT gglProgramUniform2fEXT;
static PFNGLPROGRAMUNIFORM2FEXT _glProgramUniform2fEXT;
static void APIENTRY d_glProgramUniform2fEXT(GLuint program, GLint location, GLfloat v0, GLfloat v1) {
	_glProgramUniform2fEXT(program, location, v0, v1);
	CheckGLError("glProgramUniform2fEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM2FVEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
PFNGLPROGRAMUNIFORM2FVEXT gglProgramUniform2fvEXT;
static PFNGLPROGRAMUNIFORM2FVEXT _glProgramUniform2fvEXT;
static void APIENTRY d_glProgramUniform2fvEXT(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
	_glProgramUniform2fvEXT(program, location, count, value);
	CheckGLError("glProgramUniform2fvEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM2I64NV)(GLuint program, GLint location, GLint64EXT x, GLint64EXT y);
PFNGLPROGRAMUNIFORM2I64NV gglProgramUniform2i64NV;
static PFNGLPROGRAMUNIFORM2I64NV _glProgramUniform2i64NV;
static void APIENTRY d_glProgramUniform2i64NV(GLuint program, GLint location, GLint64EXT x, GLint64EXT y) {
	_glProgramUniform2i64NV(program, location, x, y);
	CheckGLError("glProgramUniform2i64NV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM2I64VNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value);
PFNGLPROGRAMUNIFORM2I64VNV gglProgramUniform2i64vNV;
static PFNGLPROGRAMUNIFORM2I64VNV _glProgramUniform2i64vNV;
static void APIENTRY d_glProgramUniform2i64vNV(GLuint program, GLint location, GLsizei count, const GLint64EXT *value) {
	_glProgramUniform2i64vNV(program, location, count, value);
	CheckGLError("glProgramUniform2i64vNV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM2IEXT)(GLuint program, GLint location, GLint v0, GLint v1);
PFNGLPROGRAMUNIFORM2IEXT gglProgramUniform2iEXT;
static PFNGLPROGRAMUNIFORM2IEXT _glProgramUniform2iEXT;
static void APIENTRY d_glProgramUniform2iEXT(GLuint program, GLint location, GLint v0, GLint v1) {
	_glProgramUniform2iEXT(program, location, v0, v1);
	CheckGLError("glProgramUniform2iEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM2IVEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
PFNGLPROGRAMUNIFORM2IVEXT gglProgramUniform2ivEXT;
static PFNGLPROGRAMUNIFORM2IVEXT _glProgramUniform2ivEXT;
static void APIENTRY d_glProgramUniform2ivEXT(GLuint program, GLint location, GLsizei count, const GLint *value) {
	_glProgramUniform2ivEXT(program, location, count, value);
	CheckGLError("glProgramUniform2ivEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM2UI64NV)(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y);
PFNGLPROGRAMUNIFORM2UI64NV gglProgramUniform2ui64NV;
static PFNGLPROGRAMUNIFORM2UI64NV _glProgramUniform2ui64NV;
static void APIENTRY d_glProgramUniform2ui64NV(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y) {
	_glProgramUniform2ui64NV(program, location, x, y);
	CheckGLError("glProgramUniform2ui64NV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM2UI64VNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);
PFNGLPROGRAMUNIFORM2UI64VNV gglProgramUniform2ui64vNV;
static PFNGLPROGRAMUNIFORM2UI64VNV _glProgramUniform2ui64vNV;
static void APIENTRY d_glProgramUniform2ui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) {
	_glProgramUniform2ui64vNV(program, location, count, value);
	CheckGLError("glProgramUniform2ui64vNV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM2UIEXT)(GLuint program, GLint location, GLuint v0, GLuint v1);
PFNGLPROGRAMUNIFORM2UIEXT gglProgramUniform2uiEXT;
static PFNGLPROGRAMUNIFORM2UIEXT _glProgramUniform2uiEXT;
static void APIENTRY d_glProgramUniform2uiEXT(GLuint program, GLint location, GLuint v0, GLuint v1) {
	_glProgramUniform2uiEXT(program, location, v0, v1);
	CheckGLError("glProgramUniform2uiEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM2UIVEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
PFNGLPROGRAMUNIFORM2UIVEXT gglProgramUniform2uivEXT;
static PFNGLPROGRAMUNIFORM2UIVEXT _glProgramUniform2uivEXT;
static void APIENTRY d_glProgramUniform2uivEXT(GLuint program, GLint location, GLsizei count, const GLuint *value) {
	_glProgramUniform2uivEXT(program, location, count, value);
	CheckGLError("glProgramUniform2uivEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM3FEXT)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
PFNGLPROGRAMUNIFORM3FEXT gglProgramUniform3fEXT;
static PFNGLPROGRAMUNIFORM3FEXT _glProgramUniform3fEXT;
static void APIENTRY d_glProgramUniform3fEXT(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
	_glProgramUniform3fEXT(program, location, v0, v1, v2);
	CheckGLError("glProgramUniform3fEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM3FVEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
PFNGLPROGRAMUNIFORM3FVEXT gglProgramUniform3fvEXT;
static PFNGLPROGRAMUNIFORM3FVEXT _glProgramUniform3fvEXT;
static void APIENTRY d_glProgramUniform3fvEXT(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
	_glProgramUniform3fvEXT(program, location, count, value);
	CheckGLError("glProgramUniform3fvEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM3I64NV)(GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z);
PFNGLPROGRAMUNIFORM3I64NV gglProgramUniform3i64NV;
static PFNGLPROGRAMUNIFORM3I64NV _glProgramUniform3i64NV;
static void APIENTRY d_glProgramUniform3i64NV(GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z) {
	_glProgramUniform3i64NV(program, location, x, y, z);
	CheckGLError("glProgramUniform3i64NV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM3I64VNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value);
PFNGLPROGRAMUNIFORM3I64VNV gglProgramUniform3i64vNV;
static PFNGLPROGRAMUNIFORM3I64VNV _glProgramUniform3i64vNV;
static void APIENTRY d_glProgramUniform3i64vNV(GLuint program, GLint location, GLsizei count, const GLint64EXT *value) {
	_glProgramUniform3i64vNV(program, location, count, value);
	CheckGLError("glProgramUniform3i64vNV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM3IEXT)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
PFNGLPROGRAMUNIFORM3IEXT gglProgramUniform3iEXT;
static PFNGLPROGRAMUNIFORM3IEXT _glProgramUniform3iEXT;
static void APIENTRY d_glProgramUniform3iEXT(GLuint program, GLint location, GLint v0, GLint v1, GLint v2) {
	_glProgramUniform3iEXT(program, location, v0, v1, v2);
	CheckGLError("glProgramUniform3iEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM3IVEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
PFNGLPROGRAMUNIFORM3IVEXT gglProgramUniform3ivEXT;
static PFNGLPROGRAMUNIFORM3IVEXT _glProgramUniform3ivEXT;
static void APIENTRY d_glProgramUniform3ivEXT(GLuint program, GLint location, GLsizei count, const GLint *value) {
	_glProgramUniform3ivEXT(program, location, count, value);
	CheckGLError("glProgramUniform3ivEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM3UI64NV)(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z);
PFNGLPROGRAMUNIFORM3UI64NV gglProgramUniform3ui64NV;
static PFNGLPROGRAMUNIFORM3UI64NV _glProgramUniform3ui64NV;
static void APIENTRY d_glProgramUniform3ui64NV(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z) {
	_glProgramUniform3ui64NV(program, location, x, y, z);
	CheckGLError("glProgramUniform3ui64NV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM3UI64VNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);
PFNGLPROGRAMUNIFORM3UI64VNV gglProgramUniform3ui64vNV;
static PFNGLPROGRAMUNIFORM3UI64VNV _glProgramUniform3ui64vNV;
static void APIENTRY d_glProgramUniform3ui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) {
	_glProgramUniform3ui64vNV(program, location, count, value);
	CheckGLError("glProgramUniform3ui64vNV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM3UIEXT)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2);
PFNGLPROGRAMUNIFORM3UIEXT gglProgramUniform3uiEXT;
static PFNGLPROGRAMUNIFORM3UIEXT _glProgramUniform3uiEXT;
static void APIENTRY d_glProgramUniform3uiEXT(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) {
	_glProgramUniform3uiEXT(program, location, v0, v1, v2);
	CheckGLError("glProgramUniform3uiEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM3UIVEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
PFNGLPROGRAMUNIFORM3UIVEXT gglProgramUniform3uivEXT;
static PFNGLPROGRAMUNIFORM3UIVEXT _glProgramUniform3uivEXT;
static void APIENTRY d_glProgramUniform3uivEXT(GLuint program, GLint location, GLsizei count, const GLuint *value) {
	_glProgramUniform3uivEXT(program, location, count, value);
	CheckGLError("glProgramUniform3uivEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM4FEXT)(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
PFNGLPROGRAMUNIFORM4FEXT gglProgramUniform4fEXT;
static PFNGLPROGRAMUNIFORM4FEXT _glProgramUniform4fEXT;
static void APIENTRY d_glProgramUniform4fEXT(GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
	_glProgramUniform4fEXT(program, location, v0, v1, v2, v3);
	CheckGLError("glProgramUniform4fEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM4FVEXT)(GLuint program, GLint location, GLsizei count, const GLfloat *value);
PFNGLPROGRAMUNIFORM4FVEXT gglProgramUniform4fvEXT;
static PFNGLPROGRAMUNIFORM4FVEXT _glProgramUniform4fvEXT;
static void APIENTRY d_glProgramUniform4fvEXT(GLuint program, GLint location, GLsizei count, const GLfloat *value) {
	_glProgramUniform4fvEXT(program, location, count, value);
	CheckGLError("glProgramUniform4fvEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM4I64NV)(GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w);
PFNGLPROGRAMUNIFORM4I64NV gglProgramUniform4i64NV;
static PFNGLPROGRAMUNIFORM4I64NV _glProgramUniform4i64NV;
static void APIENTRY d_glProgramUniform4i64NV(GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w) {
	_glProgramUniform4i64NV(program, location, x, y, z, w);
	CheckGLError("glProgramUniform4i64NV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM4I64VNV)(GLuint program, GLint location, GLsizei count, const GLint64EXT *value);
PFNGLPROGRAMUNIFORM4I64VNV gglProgramUniform4i64vNV;
static PFNGLPROGRAMUNIFORM4I64VNV _glProgramUniform4i64vNV;
static void APIENTRY d_glProgramUniform4i64vNV(GLuint program, GLint location, GLsizei count, const GLint64EXT *value) {
	_glProgramUniform4i64vNV(program, location, count, value);
	CheckGLError("glProgramUniform4i64vNV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM4IEXT)(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
PFNGLPROGRAMUNIFORM4IEXT gglProgramUniform4iEXT;
static PFNGLPROGRAMUNIFORM4IEXT _glProgramUniform4iEXT;
static void APIENTRY d_glProgramUniform4iEXT(GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
	_glProgramUniform4iEXT(program, location, v0, v1, v2, v3);
	CheckGLError("glProgramUniform4iEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM4IVEXT)(GLuint program, GLint location, GLsizei count, const GLint *value);
PFNGLPROGRAMUNIFORM4IVEXT gglProgramUniform4ivEXT;
static PFNGLPROGRAMUNIFORM4IVEXT _glProgramUniform4ivEXT;
static void APIENTRY d_glProgramUniform4ivEXT(GLuint program, GLint location, GLsizei count, const GLint *value) {
	_glProgramUniform4ivEXT(program, location, count, value);
	CheckGLError("glProgramUniform4ivEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM4UI64NV)(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w);
PFNGLPROGRAMUNIFORM4UI64NV gglProgramUniform4ui64NV;
static PFNGLPROGRAMUNIFORM4UI64NV _glProgramUniform4ui64NV;
static void APIENTRY d_glProgramUniform4ui64NV(GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w) {
	_glProgramUniform4ui64NV(program, location, x, y, z, w);
	CheckGLError("glProgramUniform4ui64NV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM4UI64VNV)(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value);
PFNGLPROGRAMUNIFORM4UI64VNV gglProgramUniform4ui64vNV;
static PFNGLPROGRAMUNIFORM4UI64VNV _glProgramUniform4ui64vNV;
static void APIENTRY d_glProgramUniform4ui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64EXT *value) {
	_glProgramUniform4ui64vNV(program, location, count, value);
	CheckGLError("glProgramUniform4ui64vNV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM4UIEXT)(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
PFNGLPROGRAMUNIFORM4UIEXT gglProgramUniform4uiEXT;
static PFNGLPROGRAMUNIFORM4UIEXT _glProgramUniform4uiEXT;
static void APIENTRY d_glProgramUniform4uiEXT(GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
	_glProgramUniform4uiEXT(program, location, v0, v1, v2, v3);
	CheckGLError("glProgramUniform4uiEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORM4UIVEXT)(GLuint program, GLint location, GLsizei count, const GLuint *value);
PFNGLPROGRAMUNIFORM4UIVEXT gglProgramUniform4uivEXT;
static PFNGLPROGRAMUNIFORM4UIVEXT _glProgramUniform4uivEXT;
static void APIENTRY d_glProgramUniform4uivEXT(GLuint program, GLint location, GLsizei count, const GLuint *value) {
	_glProgramUniform4uivEXT(program, location, count, value);
	CheckGLError("glProgramUniform4uivEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMHANDLEUI64IMG)(GLuint program, GLint location, GLuint64 value);
PFNGLPROGRAMUNIFORMHANDLEUI64IMG gglProgramUniformHandleui64IMG;
static PFNGLPROGRAMUNIFORMHANDLEUI64IMG _glProgramUniformHandleui64IMG;
static void APIENTRY d_glProgramUniformHandleui64IMG(GLuint program, GLint location, GLuint64 value) {
	_glProgramUniformHandleui64IMG(program, location, value);
	CheckGLError("glProgramUniformHandleui64IMG");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMHANDLEUI64NV)(GLuint program, GLint location, GLuint64 value);
PFNGLPROGRAMUNIFORMHANDLEUI64NV gglProgramUniformHandleui64NV;
static PFNGLPROGRAMUNIFORMHANDLEUI64NV _glProgramUniformHandleui64NV;
static void APIENTRY d_glProgramUniformHandleui64NV(GLuint program, GLint location, GLuint64 value) {
	_glProgramUniformHandleui64NV(program, location, value);
	CheckGLError("glProgramUniformHandleui64NV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMHANDLEUI64VIMG)(GLuint program, GLint location, GLsizei count, const GLuint64 *values);
PFNGLPROGRAMUNIFORMHANDLEUI64VIMG gglProgramUniformHandleui64vIMG;
static PFNGLPROGRAMUNIFORMHANDLEUI64VIMG _glProgramUniformHandleui64vIMG;
static void APIENTRY d_glProgramUniformHandleui64vIMG(GLuint program, GLint location, GLsizei count, const GLuint64 *values) {
	_glProgramUniformHandleui64vIMG(program, location, count, values);
	CheckGLError("glProgramUniformHandleui64vIMG");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMHANDLEUI64VNV)(GLuint program, GLint location, GLsizei count, const GLuint64 *values);
PFNGLPROGRAMUNIFORMHANDLEUI64VNV gglProgramUniformHandleui64vNV;
static PFNGLPROGRAMUNIFORMHANDLEUI64VNV _glProgramUniformHandleui64vNV;
static void APIENTRY d_glProgramUniformHandleui64vNV(GLuint program, GLint location, GLsizei count, const GLuint64 *values) {
	_glProgramUniformHandleui64vNV(program, location, count, values);
	CheckGLError("glProgramUniformHandleui64vNV");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMMATRIX2FVEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLPROGRAMUNIFORMMATRIX2FVEXT gglProgramUniformMatrix2fvEXT;
static PFNGLPROGRAMUNIFORMMATRIX2FVEXT _glProgramUniformMatrix2fvEXT;
static void APIENTRY d_glProgramUniformMatrix2fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glProgramUniformMatrix2fvEXT(program, location, count, transpose, value);
	CheckGLError("glProgramUniformMatrix2fvEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMMATRIX2X3FVEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLPROGRAMUNIFORMMATRIX2X3FVEXT gglProgramUniformMatrix2x3fvEXT;
static PFNGLPROGRAMUNIFORMMATRIX2X3FVEXT _glProgramUniformMatrix2x3fvEXT;
static void APIENTRY d_glProgramUniformMatrix2x3fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glProgramUniformMatrix2x3fvEXT(program, location, count, transpose, value);
	CheckGLError("glProgramUniformMatrix2x3fvEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMMATRIX2X4FVEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLPROGRAMUNIFORMMATRIX2X4FVEXT gglProgramUniformMatrix2x4fvEXT;
static PFNGLPROGRAMUNIFORMMATRIX2X4FVEXT _glProgramUniformMatrix2x4fvEXT;
static void APIENTRY d_glProgramUniformMatrix2x4fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glProgramUniformMatrix2x4fvEXT(program, location, count, transpose, value);
	CheckGLError("glProgramUniformMatrix2x4fvEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMMATRIX3FVEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLPROGRAMUNIFORMMATRIX3FVEXT gglProgramUniformMatrix3fvEXT;
static PFNGLPROGRAMUNIFORMMATRIX3FVEXT _glProgramUniformMatrix3fvEXT;
static void APIENTRY d_glProgramUniformMatrix3fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glProgramUniformMatrix3fvEXT(program, location, count, transpose, value);
	CheckGLError("glProgramUniformMatrix3fvEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMMATRIX3X2FVEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLPROGRAMUNIFORMMATRIX3X2FVEXT gglProgramUniformMatrix3x2fvEXT;
static PFNGLPROGRAMUNIFORMMATRIX3X2FVEXT _glProgramUniformMatrix3x2fvEXT;
static void APIENTRY d_glProgramUniformMatrix3x2fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glProgramUniformMatrix3x2fvEXT(program, location, count, transpose, value);
	CheckGLError("glProgramUniformMatrix3x2fvEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMMATRIX3X4FVEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLPROGRAMUNIFORMMATRIX3X4FVEXT gglProgramUniformMatrix3x4fvEXT;
static PFNGLPROGRAMUNIFORMMATRIX3X4FVEXT _glProgramUniformMatrix3x4fvEXT;
static void APIENTRY d_glProgramUniformMatrix3x4fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glProgramUniformMatrix3x4fvEXT(program, location, count, transpose, value);
	CheckGLError("glProgramUniformMatrix3x4fvEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMMATRIX4FVEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLPROGRAMUNIFORMMATRIX4FVEXT gglProgramUniformMatrix4fvEXT;
static PFNGLPROGRAMUNIFORMMATRIX4FVEXT _glProgramUniformMatrix4fvEXT;
static void APIENTRY d_glProgramUniformMatrix4fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glProgramUniformMatrix4fvEXT(program, location, count, transpose, value);
	CheckGLError("glProgramUniformMatrix4fvEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMMATRIX4X2FVEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLPROGRAMUNIFORMMATRIX4X2FVEXT gglProgramUniformMatrix4x2fvEXT;
static PFNGLPROGRAMUNIFORMMATRIX4X2FVEXT _glProgramUniformMatrix4x2fvEXT;
static void APIENTRY d_glProgramUniformMatrix4x2fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glProgramUniformMatrix4x2fvEXT(program, location, count, transpose, value);
	CheckGLError("glProgramUniformMatrix4x2fvEXT");
}
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMMATRIX4X3FVEXT)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLPROGRAMUNIFORMMATRIX4X3FVEXT gglProgramUniformMatrix4x3fvEXT;
static PFNGLPROGRAMUNIFORMMATRIX4X3FVEXT _glProgramUniformMatrix4x3fvEXT;
static void APIENTRY d_glProgramUniformMatrix4x3fvEXT(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glProgramUniformMatrix4x3fvEXT(program, location, count, transpose, value);
	CheckGLError("glProgramUniformMatrix4x3fvEXT");
}
typedef void (APIENTRYP PFNGLPUSHDEBUGGROUPKHR)(GLenum source, GLuint id, GLsizei length, const GLchar *message);
PFNGLPUSHDEBUGGROUPKHR gglPushDebugGroupKHR;
static PFNGLPUSHDEBUGGROUPKHR _glPushDebugGroupKHR;
static void APIENTRY d_glPushDebugGroupKHR(GLenum source, GLuint id, GLsizei length, const GLchar *message) {
	_glPushDebugGroupKHR(source, id, length, message);
	CheckGLError("glPushDebugGroupKHR");
}
typedef void (APIENTRYP PFNGLPUSHGROUPMARKEREXT)(GLsizei length, const GLchar *marker);
PFNGLPUSHGROUPMARKEREXT gglPushGroupMarkerEXT;
static PFNGLPUSHGROUPMARKEREXT _glPushGroupMarkerEXT;
static void APIENTRY d_glPushGroupMarkerEXT(GLsizei length, const GLchar *marker) {
	_glPushGroupMarkerEXT(length, marker);
	CheckGLError("glPushGroupMarkerEXT");
}
typedef void (APIENTRYP PFNGLQUERYCOUNTEREXT)(GLuint id, GLenum target);
PFNGLQUERYCOUNTEREXT gglQueryCounterEXT;
static PFNGLQUERYCOUNTEREXT _glQueryCounterEXT;
static void APIENTRY d_glQueryCounterEXT(GLuint id, GLenum target) {
	_glQueryCounterEXT(id, target);
	CheckGLError("glQueryCounterEXT");
}
typedef void (APIENTRYP PFNGLRASTERSAMPLESEXT)(GLuint samples, GLboolean fixedsamplelocations);
PFNGLRASTERSAMPLESEXT gglRasterSamplesEXT;
static PFNGLRASTERSAMPLESEXT _glRasterSamplesEXT;
static void APIENTRY d_glRasterSamplesEXT(GLuint samples, GLboolean fixedsamplelocations) {
	_glRasterSamplesEXT(samples, fixedsamplelocations);
	CheckGLError("glRasterSamplesEXT");
}
typedef void (APIENTRYP PFNGLREADBUFFER)(GLenum src);
PFNGLREADBUFFER gglReadBuffer;
static PFNGLREADBUFFER _glReadBuffer;
static void APIENTRY d_glReadBuffer(GLenum src) {
	_glReadBuffer(src);
	CheckGLError("glReadBuffer");
}
typedef void (APIENTRYP PFNGLREADBUFFERINDEXEDEXT)(GLenum src, GLint index);
PFNGLREADBUFFERINDEXEDEXT gglReadBufferIndexedEXT;
static PFNGLREADBUFFERINDEXEDEXT _glReadBufferIndexedEXT;
static void APIENTRY d_glReadBufferIndexedEXT(GLenum src, GLint index) {
	_glReadBufferIndexedEXT(src, index);
	CheckGLError("glReadBufferIndexedEXT");
}
typedef void (APIENTRYP PFNGLREADBUFFERNV)(GLenum mode);
PFNGLREADBUFFERNV gglReadBufferNV;
static PFNGLREADBUFFERNV _glReadBufferNV;
static void APIENTRY d_glReadBufferNV(GLenum mode) {
	_glReadBufferNV(mode);
	CheckGLError("glReadBufferNV");
}
typedef void (APIENTRYP PFNGLREADPIXELS)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels);
PFNGLREADPIXELS gglReadPixels;
static PFNGLREADPIXELS _glReadPixels;
static void APIENTRY d_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels) {
	_glReadPixels(x, y, width, height, format, type, pixels);
	CheckGLError("glReadPixels");
}
typedef void (APIENTRYP PFNGLREADNPIXELSEXT)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data);
PFNGLREADNPIXELSEXT gglReadnPixelsEXT;
static PFNGLREADNPIXELSEXT _glReadnPixelsEXT;
static void APIENTRY d_glReadnPixelsEXT(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data) {
	_glReadnPixelsEXT(x, y, width, height, format, type, bufSize, data);
	CheckGLError("glReadnPixelsEXT");
}
typedef void (APIENTRYP PFNGLREADNPIXELSKHR)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data);
PFNGLREADNPIXELSKHR gglReadnPixelsKHR;
static PFNGLREADNPIXELSKHR _glReadnPixelsKHR;
static void APIENTRY d_glReadnPixelsKHR(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data) {
	_glReadnPixelsKHR(x, y, width, height, format, type, bufSize, data);
	CheckGLError("glReadnPixelsKHR");
}
typedef void (APIENTRYP PFNGLRELEASESHADERCOMPILER)();
PFNGLRELEASESHADERCOMPILER gglReleaseShaderCompiler;
static PFNGLRELEASESHADERCOMPILER _glReleaseShaderCompiler;
static void APIENTRY d_glReleaseShaderCompiler() {
	_glReleaseShaderCompiler();
	CheckGLError("glReleaseShaderCompiler");
}
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGE)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
PFNGLRENDERBUFFERSTORAGE gglRenderbufferStorage;
static PFNGLRENDERBUFFERSTORAGE _glRenderbufferStorage;
static void APIENTRY d_glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
	_glRenderbufferStorage(target, internalformat, width, height);
	CheckGLError("glRenderbufferStorage");
}
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEMULTISAMPLE)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
PFNGLRENDERBUFFERSTORAGEMULTISAMPLE gglRenderbufferStorageMultisample;
static PFNGLRENDERBUFFERSTORAGEMULTISAMPLE _glRenderbufferStorageMultisample;
static void APIENTRY d_glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
	_glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
	CheckGLError("glRenderbufferStorageMultisample");
}
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEMULTISAMPLEANGLE)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEANGLE gglRenderbufferStorageMultisampleANGLE;
static PFNGLRENDERBUFFERSTORAGEMULTISAMPLEANGLE _glRenderbufferStorageMultisampleANGLE;
static void APIENTRY d_glRenderbufferStorageMultisampleANGLE(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
	_glRenderbufferStorageMultisampleANGLE(target, samples, internalformat, width, height);
	CheckGLError("glRenderbufferStorageMultisampleANGLE");
}
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEMULTISAMPLEAPPLE)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEAPPLE gglRenderbufferStorageMultisampleAPPLE;
static PFNGLRENDERBUFFERSTORAGEMULTISAMPLEAPPLE _glRenderbufferStorageMultisampleAPPLE;
static void APIENTRY d_glRenderbufferStorageMultisampleAPPLE(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
	_glRenderbufferStorageMultisampleAPPLE(target, samples, internalformat, width, height);
	CheckGLError("glRenderbufferStorageMultisampleAPPLE");
}
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXT)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXT gglRenderbufferStorageMultisampleEXT;
static PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXT _glRenderbufferStorageMultisampleEXT;
static void APIENTRY d_glRenderbufferStorageMultisampleEXT(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
	_glRenderbufferStorageMultisampleEXT(target, samples, internalformat, width, height);
	CheckGLError("glRenderbufferStorageMultisampleEXT");
}
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG gglRenderbufferStorageMultisampleIMG;
static PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG _glRenderbufferStorageMultisampleIMG;
static void APIENTRY d_glRenderbufferStorageMultisampleIMG(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
	_glRenderbufferStorageMultisampleIMG(target, samples, internalformat, width, height);
	CheckGLError("glRenderbufferStorageMultisampleIMG");
}
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEMULTISAMPLENV)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
PFNGLRENDERBUFFERSTORAGEMULTISAMPLENV gglRenderbufferStorageMultisampleNV;
static PFNGLRENDERBUFFERSTORAGEMULTISAMPLENV _glRenderbufferStorageMultisampleNV;
static void APIENTRY d_glRenderbufferStorageMultisampleNV(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
	_glRenderbufferStorageMultisampleNV(target, samples, internalformat, width, height);
	CheckGLError("glRenderbufferStorageMultisampleNV");
}
typedef void (APIENTRYP PFNGLRESOLVEDEPTHVALUESNV)();
PFNGLRESOLVEDEPTHVALUESNV gglResolveDepthValuesNV;
static PFNGLRESOLVEDEPTHVALUESNV _glResolveDepthValuesNV;
static void APIENTRY d_glResolveDepthValuesNV() {
	_glResolveDepthValuesNV();
	CheckGLError("glResolveDepthValuesNV");
}
typedef void (APIENTRYP PFNGLRESOLVEMULTISAMPLEFRAMEBUFFERAPPLE)();
PFNGLRESOLVEMULTISAMPLEFRAMEBUFFERAPPLE gglResolveMultisampleFramebufferAPPLE;
static PFNGLRESOLVEMULTISAMPLEFRAMEBUFFERAPPLE _glResolveMultisampleFramebufferAPPLE;
static void APIENTRY d_glResolveMultisampleFramebufferAPPLE() {
	_glResolveMultisampleFramebufferAPPLE();
	CheckGLError("glResolveMultisampleFramebufferAPPLE");
}
typedef void (APIENTRYP PFNGLRESUMETRANSFORMFEEDBACK)();
PFNGLRESUMETRANSFORMFEEDBACK gglResumeTransformFeedback;
static PFNGLRESUMETRANSFORMFEEDBACK _glResumeTransformFeedback;
static void APIENTRY d_glResumeTransformFeedback() {
	_glResumeTransformFeedback();
	CheckGLError("glResumeTransformFeedback");
}
typedef void (APIENTRYP PFNGLSAMPLECOVERAGE)(GLfloat value, GLboolean invert);
PFNGLSAMPLECOVERAGE gglSampleCoverage;
static PFNGLSAMPLECOVERAGE _glSampleCoverage;
static void APIENTRY d_glSampleCoverage(GLfloat value, GLboolean invert) {
	_glSampleCoverage(value, invert);
	CheckGLError("glSampleCoverage");
}
typedef void (APIENTRYP PFNGLSAMPLERPARAMETERIIVEXT)(GLuint sampler, GLenum pname, const GLint *param);
PFNGLSAMPLERPARAMETERIIVEXT gglSamplerParameterIivEXT;
static PFNGLSAMPLERPARAMETERIIVEXT _glSamplerParameterIivEXT;
static void APIENTRY d_glSamplerParameterIivEXT(GLuint sampler, GLenum pname, const GLint *param) {
	_glSamplerParameterIivEXT(sampler, pname, param);
	CheckGLError("glSamplerParameterIivEXT");
}
typedef void (APIENTRYP PFNGLSAMPLERPARAMETERIIVOES)(GLuint sampler, GLenum pname, const GLint *param);
PFNGLSAMPLERPARAMETERIIVOES gglSamplerParameterIivOES;
static PFNGLSAMPLERPARAMETERIIVOES _glSamplerParameterIivOES;
static void APIENTRY d_glSamplerParameterIivOES(GLuint sampler, GLenum pname, const GLint *param) {
	_glSamplerParameterIivOES(sampler, pname, param);
	CheckGLError("glSamplerParameterIivOES");
}
typedef void (APIENTRYP PFNGLSAMPLERPARAMETERIUIVEXT)(GLuint sampler, GLenum pname, const GLuint *param);
PFNGLSAMPLERPARAMETERIUIVEXT gglSamplerParameterIuivEXT;
static PFNGLSAMPLERPARAMETERIUIVEXT _glSamplerParameterIuivEXT;
static void APIENTRY d_glSamplerParameterIuivEXT(GLuint sampler, GLenum pname, const GLuint *param) {
	_glSamplerParameterIuivEXT(sampler, pname, param);
	CheckGLError("glSamplerParameterIuivEXT");
}
typedef void (APIENTRYP PFNGLSAMPLERPARAMETERIUIVOES)(GLuint sampler, GLenum pname, const GLuint *param);
PFNGLSAMPLERPARAMETERIUIVOES gglSamplerParameterIuivOES;
static PFNGLSAMPLERPARAMETERIUIVOES _glSamplerParameterIuivOES;
static void APIENTRY d_glSamplerParameterIuivOES(GLuint sampler, GLenum pname, const GLuint *param) {
	_glSamplerParameterIuivOES(sampler, pname, param);
	CheckGLError("glSamplerParameterIuivOES");
}
typedef void (APIENTRYP PFNGLSAMPLERPARAMETERF)(GLuint sampler, GLenum pname, GLfloat param);
PFNGLSAMPLERPARAMETERF gglSamplerParameterf;
static PFNGLSAMPLERPARAMETERF _glSamplerParameterf;
static void APIENTRY d_glSamplerParameterf(GLuint sampler, GLenum pname, GLfloat param) {
	_glSamplerParameterf(sampler, pname, param);
	CheckGLError("glSamplerParameterf");
}
typedef void (APIENTRYP PFNGLSAMPLERPARAMETERFV)(GLuint sampler, GLenum pname, const GLfloat *param);
PFNGLSAMPLERPARAMETERFV gglSamplerParameterfv;
static PFNGLSAMPLERPARAMETERFV _glSamplerParameterfv;
static void APIENTRY d_glSamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat *param) {
	_glSamplerParameterfv(sampler, pname, param);
	CheckGLError("glSamplerParameterfv");
}
typedef void (APIENTRYP PFNGLSAMPLERPARAMETERI)(GLuint sampler, GLenum pname, GLint param);
PFNGLSAMPLERPARAMETERI gglSamplerParameteri;
static PFNGLSAMPLERPARAMETERI _glSamplerParameteri;
static void APIENTRY d_glSamplerParameteri(GLuint sampler, GLenum pname, GLint param) {
	_glSamplerParameteri(sampler, pname, param);
	CheckGLError("glSamplerParameteri");
}
typedef void (APIENTRYP PFNGLSAMPLERPARAMETERIV)(GLuint sampler, GLenum pname, const GLint *param);
PFNGLSAMPLERPARAMETERIV gglSamplerParameteriv;
static PFNGLSAMPLERPARAMETERIV _glSamplerParameteriv;
static void APIENTRY d_glSamplerParameteriv(GLuint sampler, GLenum pname, const GLint *param) {
	_glSamplerParameteriv(sampler, pname, param);
	CheckGLError("glSamplerParameteriv");
}
typedef void (APIENTRYP PFNGLSCISSOR)(GLint x, GLint y, GLsizei width, GLsizei height);
PFNGLSCISSOR gglScissor;
static PFNGLSCISSOR _glScissor;
static void APIENTRY d_glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
	_glScissor(x, y, width, height);
	CheckGLError("glScissor");
}
typedef void (APIENTRYP PFNGLSCISSORARRAYVNV)(GLuint first, GLsizei count, const GLint *v);
PFNGLSCISSORARRAYVNV gglScissorArrayvNV;
static PFNGLSCISSORARRAYVNV _glScissorArrayvNV;
static void APIENTRY d_glScissorArrayvNV(GLuint first, GLsizei count, const GLint *v) {
	_glScissorArrayvNV(first, count, v);
	CheckGLError("glScissorArrayvNV");
}
typedef void (APIENTRYP PFNGLSCISSORARRAYVOES)(GLuint first, GLsizei count, const GLint *v);
PFNGLSCISSORARRAYVOES gglScissorArrayvOES;
static PFNGLSCISSORARRAYVOES _glScissorArrayvOES;
static void APIENTRY d_glScissorArrayvOES(GLuint first, GLsizei count, const GLint *v) {
	_glScissorArrayvOES(first, count, v);
	CheckGLError("glScissorArrayvOES");
}
typedef void (APIENTRYP PFNGLSCISSORINDEXEDNV)(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height);
PFNGLSCISSORINDEXEDNV gglScissorIndexedNV;
static PFNGLSCISSORINDEXEDNV _glScissorIndexedNV;
static void APIENTRY d_glScissorIndexedNV(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height) {
	_glScissorIndexedNV(index, left, bottom, width, height);
	CheckGLError("glScissorIndexedNV");
}
typedef void (APIENTRYP PFNGLSCISSORINDEXEDOES)(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height);
PFNGLSCISSORINDEXEDOES gglScissorIndexedOES;
static PFNGLSCISSORINDEXEDOES _glScissorIndexedOES;
static void APIENTRY d_glScissorIndexedOES(GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height) {
	_glScissorIndexedOES(index, left, bottom, width, height);
	CheckGLError("glScissorIndexedOES");
}
typedef void (APIENTRYP PFNGLSCISSORINDEXEDVNV)(GLuint index, const GLint *v);
PFNGLSCISSORINDEXEDVNV gglScissorIndexedvNV;
static PFNGLSCISSORINDEXEDVNV _glScissorIndexedvNV;
static void APIENTRY d_glScissorIndexedvNV(GLuint index, const GLint *v) {
	_glScissorIndexedvNV(index, v);
	CheckGLError("glScissorIndexedvNV");
}
typedef void (APIENTRYP PFNGLSCISSORINDEXEDVOES)(GLuint index, const GLint *v);
PFNGLSCISSORINDEXEDVOES gglScissorIndexedvOES;
static PFNGLSCISSORINDEXEDVOES _glScissorIndexedvOES;
static void APIENTRY d_glScissorIndexedvOES(GLuint index, const GLint *v) {
	_glScissorIndexedvOES(index, v);
	CheckGLError("glScissorIndexedvOES");
}
typedef void (APIENTRYP PFNGLSELECTPERFMONITORCOUNTERSAMD)(GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint *counterList);
PFNGLSELECTPERFMONITORCOUNTERSAMD gglSelectPerfMonitorCountersAMD;
static PFNGLSELECTPERFMONITORCOUNTERSAMD _glSelectPerfMonitorCountersAMD;
static void APIENTRY d_glSelectPerfMonitorCountersAMD(GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint *counterList) {
	_glSelectPerfMonitorCountersAMD(monitor, enable, group, numCounters, counterList);
	CheckGLError("glSelectPerfMonitorCountersAMD");
}
typedef void (APIENTRYP PFNGLSETFENCENV)(GLuint fence, GLenum condition);
PFNGLSETFENCENV gglSetFenceNV;
static PFNGLSETFENCENV _glSetFenceNV;
static void APIENTRY d_glSetFenceNV(GLuint fence, GLenum condition) {
	_glSetFenceNV(fence, condition);
	CheckGLError("glSetFenceNV");
}
typedef void (APIENTRYP PFNGLSHADERBINARY)(GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length);
PFNGLSHADERBINARY gglShaderBinary;
static PFNGLSHADERBINARY _glShaderBinary;
static void APIENTRY d_glShaderBinary(GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length) {
	_glShaderBinary(count, shaders, binaryformat, binary, length);
	CheckGLError("glShaderBinary");
}
typedef void (APIENTRYP PFNGLSHADERSOURCE)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
PFNGLSHADERSOURCE gglShaderSource;
static PFNGLSHADERSOURCE _glShaderSource;
static void APIENTRY d_glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length) {
	_glShaderSource(shader, count, string, length);
	CheckGLError("glShaderSource");
}
typedef void (APIENTRYP PFNGLSTARTTILINGQCOM)(GLuint x, GLuint y, GLuint width, GLuint height, GLbitfield preserveMask);
PFNGLSTARTTILINGQCOM gglStartTilingQCOM;
static PFNGLSTARTTILINGQCOM _glStartTilingQCOM;
static void APIENTRY d_glStartTilingQCOM(GLuint x, GLuint y, GLuint width, GLuint height, GLbitfield preserveMask) {
	_glStartTilingQCOM(x, y, width, height, preserveMask);
	CheckGLError("glStartTilingQCOM");
}
typedef void (APIENTRYP PFNGLSTENCILFILLPATHINSTANCEDNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues);
PFNGLSTENCILFILLPATHINSTANCEDNV gglStencilFillPathInstancedNV;
static PFNGLSTENCILFILLPATHINSTANCEDNV _glStencilFillPathInstancedNV;
static void APIENTRY d_glStencilFillPathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues) {
	_glStencilFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, transformType, transformValues);
	CheckGLError("glStencilFillPathInstancedNV");
}
typedef void (APIENTRYP PFNGLSTENCILFILLPATHNV)(GLuint path, GLenum fillMode, GLuint mask);
PFNGLSTENCILFILLPATHNV gglStencilFillPathNV;
static PFNGLSTENCILFILLPATHNV _glStencilFillPathNV;
static void APIENTRY d_glStencilFillPathNV(GLuint path, GLenum fillMode, GLuint mask) {
	_glStencilFillPathNV(path, fillMode, mask);
	CheckGLError("glStencilFillPathNV");
}
typedef void (APIENTRYP PFNGLSTENCILFUNC)(GLenum func, GLint ref, GLuint mask);
PFNGLSTENCILFUNC gglStencilFunc;
static PFNGLSTENCILFUNC _glStencilFunc;
static void APIENTRY d_glStencilFunc(GLenum func, GLint ref, GLuint mask) {
	_glStencilFunc(func, ref, mask);
	CheckGLError("glStencilFunc");
}
typedef void (APIENTRYP PFNGLSTENCILFUNCSEPARATE)(GLenum face, GLenum func, GLint ref, GLuint mask);
PFNGLSTENCILFUNCSEPARATE gglStencilFuncSeparate;
static PFNGLSTENCILFUNCSEPARATE _glStencilFuncSeparate;
static void APIENTRY d_glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask) {
	_glStencilFuncSeparate(face, func, ref, mask);
	CheckGLError("glStencilFuncSeparate");
}
typedef void (APIENTRYP PFNGLSTENCILMASK)(GLuint mask);
PFNGLSTENCILMASK gglStencilMask;
static PFNGLSTENCILMASK _glStencilMask;
static void APIENTRY d_glStencilMask(GLuint mask) {
	_glStencilMask(mask);
	CheckGLError("glStencilMask");
}
typedef void (APIENTRYP PFNGLSTENCILMASKSEPARATE)(GLenum face, GLuint mask);
PFNGLSTENCILMASKSEPARATE gglStencilMaskSeparate;
static PFNGLSTENCILMASKSEPARATE _glStencilMaskSeparate;
static void APIENTRY d_glStencilMaskSeparate(GLenum face, GLuint mask) {
	_glStencilMaskSeparate(face, mask);
	CheckGLError("glStencilMaskSeparate");
}
typedef void (APIENTRYP PFNGLSTENCILOP)(GLenum fail, GLenum zfail, GLenum zpass);
PFNGLSTENCILOP gglStencilOp;
static PFNGLSTENCILOP _glStencilOp;
static void APIENTRY d_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
	_glStencilOp(fail, zfail, zpass);
	CheckGLError("glStencilOp");
}
typedef void (APIENTRYP PFNGLSTENCILOPSEPARATE)(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
PFNGLSTENCILOPSEPARATE gglStencilOpSeparate;
static PFNGLSTENCILOPSEPARATE _glStencilOpSeparate;
static void APIENTRY d_glStencilOpSeparate(GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass) {
	_glStencilOpSeparate(face, sfail, dpfail, dppass);
	CheckGLError("glStencilOpSeparate");
}
typedef void (APIENTRYP PFNGLSTENCILSTROKEPATHINSTANCEDNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat *transformValues);
PFNGLSTENCILSTROKEPATHINSTANCEDNV gglStencilStrokePathInstancedNV;
static PFNGLSTENCILSTROKEPATHINSTANCEDNV _glStencilStrokePathInstancedNV;
static void APIENTRY d_glStencilStrokePathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat *transformValues) {
	_glStencilStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, reference, mask, transformType, transformValues);
	CheckGLError("glStencilStrokePathInstancedNV");
}
typedef void (APIENTRYP PFNGLSTENCILSTROKEPATHNV)(GLuint path, GLint reference, GLuint mask);
PFNGLSTENCILSTROKEPATHNV gglStencilStrokePathNV;
static PFNGLSTENCILSTROKEPATHNV _glStencilStrokePathNV;
static void APIENTRY d_glStencilStrokePathNV(GLuint path, GLint reference, GLuint mask) {
	_glStencilStrokePathNV(path, reference, mask);
	CheckGLError("glStencilStrokePathNV");
}
typedef void (APIENTRYP PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNV gglStencilThenCoverFillPathInstancedNV;
static PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNV _glStencilThenCoverFillPathInstancedNV;
static void APIENTRY d_glStencilThenCoverFillPathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues) {
	_glStencilThenCoverFillPathInstancedNV(numPaths, pathNameType, paths, pathBase, fillMode, mask, coverMode, transformType, transformValues);
	CheckGLError("glStencilThenCoverFillPathInstancedNV");
}
typedef void (APIENTRYP PFNGLSTENCILTHENCOVERFILLPATHNV)(GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode);
PFNGLSTENCILTHENCOVERFILLPATHNV gglStencilThenCoverFillPathNV;
static PFNGLSTENCILTHENCOVERFILLPATHNV _glStencilThenCoverFillPathNV;
static void APIENTRY d_glStencilThenCoverFillPathNV(GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode) {
	_glStencilThenCoverFillPathNV(path, fillMode, mask, coverMode);
	CheckGLError("glStencilThenCoverFillPathNV");
}
typedef void (APIENTRYP PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNV)(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues);
PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNV gglStencilThenCoverStrokePathInstancedNV;
static PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNV _glStencilThenCoverStrokePathInstancedNV;
static void APIENTRY d_glStencilThenCoverStrokePathInstancedNV(GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues) {
	_glStencilThenCoverStrokePathInstancedNV(numPaths, pathNameType, paths, pathBase, reference, mask, coverMode, transformType, transformValues);
	CheckGLError("glStencilThenCoverStrokePathInstancedNV");
}
typedef void (APIENTRYP PFNGLSTENCILTHENCOVERSTROKEPATHNV)(GLuint path, GLint reference, GLuint mask, GLenum coverMode);
PFNGLSTENCILTHENCOVERSTROKEPATHNV gglStencilThenCoverStrokePathNV;
static PFNGLSTENCILTHENCOVERSTROKEPATHNV _glStencilThenCoverStrokePathNV;
static void APIENTRY d_glStencilThenCoverStrokePathNV(GLuint path, GLint reference, GLuint mask, GLenum coverMode) {
	_glStencilThenCoverStrokePathNV(path, reference, mask, coverMode);
	CheckGLError("glStencilThenCoverStrokePathNV");
}
typedef void (APIENTRYP PFNGLSUBPIXELPRECISIONBIASNV)(GLuint xbits, GLuint ybits);
PFNGLSUBPIXELPRECISIONBIASNV gglSubpixelPrecisionBiasNV;
static PFNGLSUBPIXELPRECISIONBIASNV _glSubpixelPrecisionBiasNV;
static void APIENTRY d_glSubpixelPrecisionBiasNV(GLuint xbits, GLuint ybits) {
	_glSubpixelPrecisionBiasNV(xbits, ybits);
	CheckGLError("glSubpixelPrecisionBiasNV");
}
typedef GLboolean (APIENTRYP PFNGLTESTFENCENV)(GLuint fence);
PFNGLTESTFENCENV gglTestFenceNV;
static PFNGLTESTFENCENV _glTestFenceNV;
static GLboolean APIENTRY d_glTestFenceNV(GLuint fence) {
	GLboolean ret = _glTestFenceNV(fence);
	CheckGLError("glTestFenceNV");
	return ret;
}
typedef void (APIENTRYP PFNGLTEXBUFFEREXT)(GLenum target, GLenum internalformat, GLuint buffer);
PFNGLTEXBUFFEREXT gglTexBufferEXT;
static PFNGLTEXBUFFEREXT _glTexBufferEXT;
static void APIENTRY d_glTexBufferEXT(GLenum target, GLenum internalformat, GLuint buffer) {
	_glTexBufferEXT(target, internalformat, buffer);
	CheckGLError("glTexBufferEXT");
}
typedef void (APIENTRYP PFNGLTEXBUFFEROES)(GLenum target, GLenum internalformat, GLuint buffer);
PFNGLTEXBUFFEROES gglTexBufferOES;
static PFNGLTEXBUFFEROES _glTexBufferOES;
static void APIENTRY d_glTexBufferOES(GLenum target, GLenum internalformat, GLuint buffer) {
	_glTexBufferOES(target, internalformat, buffer);
	CheckGLError("glTexBufferOES");
}
typedef void (APIENTRYP PFNGLTEXBUFFERRANGEEXT)(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
PFNGLTEXBUFFERRANGEEXT gglTexBufferRangeEXT;
static PFNGLTEXBUFFERRANGEEXT _glTexBufferRangeEXT;
static void APIENTRY d_glTexBufferRangeEXT(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
	_glTexBufferRangeEXT(target, internalformat, buffer, offset, size);
	CheckGLError("glTexBufferRangeEXT");
}
typedef void (APIENTRYP PFNGLTEXBUFFERRANGEOES)(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
PFNGLTEXBUFFERRANGEOES gglTexBufferRangeOES;
static PFNGLTEXBUFFERRANGEOES _glTexBufferRangeOES;
static void APIENTRY d_glTexBufferRangeOES(GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size) {
	_glTexBufferRangeOES(target, internalformat, buffer, offset, size);
	CheckGLError("glTexBufferRangeOES");
}
typedef void (APIENTRYP PFNGLTEXIMAGE2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels);
PFNGLTEXIMAGE2D gglTexImage2D;
static PFNGLTEXIMAGE2D _glTexImage2D;
static void APIENTRY d_glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels) {
	_glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
	CheckGLError("glTexImage2D");
}
typedef void (APIENTRYP PFNGLTEXIMAGE3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
PFNGLTEXIMAGE3D gglTexImage3D;
static PFNGLTEXIMAGE3D _glTexImage3D;
static void APIENTRY d_glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) {
	_glTexImage3D(target, level, internalformat, width, height, depth, border, format, type, pixels);
	CheckGLError("glTexImage3D");
}
typedef void (APIENTRYP PFNGLTEXIMAGE3DOES)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
PFNGLTEXIMAGE3DOES gglTexImage3DOES;
static PFNGLTEXIMAGE3DOES _glTexImage3DOES;
static void APIENTRY d_glTexImage3DOES(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels) {
	_glTexImage3DOES(target, level, internalformat, width, height, depth, border, format, type, pixels);
	CheckGLError("glTexImage3DOES");
}
typedef void (APIENTRYP PFNGLTEXPAGECOMMITMENTEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit);
PFNGLTEXPAGECOMMITMENTEXT gglTexPageCommitmentEXT;
static PFNGLTEXPAGECOMMITMENTEXT _glTexPageCommitmentEXT;
static void APIENTRY d_glTexPageCommitmentEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit) {
	_glTexPageCommitmentEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, commit);
	CheckGLError("glTexPageCommitmentEXT");
}
typedef void (APIENTRYP PFNGLTEXPARAMETERIIVEXT)(GLenum target, GLenum pname, const GLint *params);
PFNGLTEXPARAMETERIIVEXT gglTexParameterIivEXT;
static PFNGLTEXPARAMETERIIVEXT _glTexParameterIivEXT;
static void APIENTRY d_glTexParameterIivEXT(GLenum target, GLenum pname, const GLint *params) {
	_glTexParameterIivEXT(target, pname, params);
	CheckGLError("glTexParameterIivEXT");
}
typedef void (APIENTRYP PFNGLTEXPARAMETERIIVOES)(GLenum target, GLenum pname, const GLint *params);
PFNGLTEXPARAMETERIIVOES gglTexParameterIivOES;
static PFNGLTEXPARAMETERIIVOES _glTexParameterIivOES;
static void APIENTRY d_glTexParameterIivOES(GLenum target, GLenum pname, const GLint *params) {
	_glTexParameterIivOES(target, pname, params);
	CheckGLError("glTexParameterIivOES");
}
typedef void (APIENTRYP PFNGLTEXPARAMETERIUIVEXT)(GLenum target, GLenum pname, const GLuint *params);
PFNGLTEXPARAMETERIUIVEXT gglTexParameterIuivEXT;
static PFNGLTEXPARAMETERIUIVEXT _glTexParameterIuivEXT;
static void APIENTRY d_glTexParameterIuivEXT(GLenum target, GLenum pname, const GLuint *params) {
	_glTexParameterIuivEXT(target, pname, params);
	CheckGLError("glTexParameterIuivEXT");
}
typedef void (APIENTRYP PFNGLTEXPARAMETERIUIVOES)(GLenum target, GLenum pname, const GLuint *params);
PFNGLTEXPARAMETERIUIVOES gglTexParameterIuivOES;
static PFNGLTEXPARAMETERIUIVOES _glTexParameterIuivOES;
static void APIENTRY d_glTexParameterIuivOES(GLenum target, GLenum pname, const GLuint *params) {
	_glTexParameterIuivOES(target, pname, params);
	CheckGLError("glTexParameterIuivOES");
}
typedef void (APIENTRYP PFNGLTEXPARAMETERF)(GLenum target, GLenum pname, GLfloat param);
PFNGLTEXPARAMETERF gglTexParameterf;
static PFNGLTEXPARAMETERF _glTexParameterf;
static void APIENTRY d_glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
	_glTexParameterf(target, pname, param);
	CheckGLError("glTexParameterf");
}
typedef void (APIENTRYP PFNGLTEXPARAMETERFV)(GLenum target, GLenum pname, const GLfloat *params);
PFNGLTEXPARAMETERFV gglTexParameterfv;
static PFNGLTEXPARAMETERFV _glTexParameterfv;
static void APIENTRY d_glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
	_glTexParameterfv(target, pname, params);
	CheckGLError("glTexParameterfv");
}
typedef void (APIENTRYP PFNGLTEXPARAMETERI)(GLenum target, GLenum pname, GLint param);
PFNGLTEXPARAMETERI gglTexParameteri;
static PFNGLTEXPARAMETERI _glTexParameteri;
static void APIENTRY d_glTexParameteri(GLenum target, GLenum pname, GLint param) {
	_glTexParameteri(target, pname, param);
	CheckGLError("glTexParameteri");
}
typedef void (APIENTRYP PFNGLTEXPARAMETERIV)(GLenum target, GLenum pname, const GLint *params);
PFNGLTEXPARAMETERIV gglTexParameteriv;
static PFNGLTEXPARAMETERIV _glTexParameteriv;
static void APIENTRY d_glTexParameteriv(GLenum target, GLenum pname, const GLint *params) {
	_glTexParameteriv(target, pname, params);
	CheckGLError("glTexParameteriv");
}
typedef void (APIENTRYP PFNGLTEXSTORAGE1DEXT)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width);
PFNGLTEXSTORAGE1DEXT gglTexStorage1DEXT;
static PFNGLTEXSTORAGE1DEXT _glTexStorage1DEXT;
static void APIENTRY d_glTexStorage1DEXT(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) {
	_glTexStorage1DEXT(target, levels, internalformat, width);
	CheckGLError("glTexStorage1DEXT");
}
typedef void (APIENTRYP PFNGLTEXSTORAGE2D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
PFNGLTEXSTORAGE2D gglTexStorage2D;
static PFNGLTEXSTORAGE2D _glTexStorage2D;
static void APIENTRY d_glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
	_glTexStorage2D(target, levels, internalformat, width, height);
	CheckGLError("glTexStorage2D");
}
typedef void (APIENTRYP PFNGLTEXSTORAGE2DEXT)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
PFNGLTEXSTORAGE2DEXT gglTexStorage2DEXT;
static PFNGLTEXSTORAGE2DEXT _glTexStorage2DEXT;
static void APIENTRY d_glTexStorage2DEXT(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
	_glTexStorage2DEXT(target, levels, internalformat, width, height);
	CheckGLError("glTexStorage2DEXT");
}
typedef void (APIENTRYP PFNGLTEXSTORAGE3D)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
PFNGLTEXSTORAGE3D gglTexStorage3D;
static PFNGLTEXSTORAGE3D _glTexStorage3D;
static void APIENTRY d_glTexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
	_glTexStorage3D(target, levels, internalformat, width, height, depth);
	CheckGLError("glTexStorage3D");
}
typedef void (APIENTRYP PFNGLTEXSTORAGE3DEXT)(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
PFNGLTEXSTORAGE3DEXT gglTexStorage3DEXT;
static PFNGLTEXSTORAGE3DEXT _glTexStorage3DEXT;
static void APIENTRY d_glTexStorage3DEXT(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
	_glTexStorage3DEXT(target, levels, internalformat, width, height, depth);
	CheckGLError("glTexStorage3DEXT");
}
typedef void (APIENTRYP PFNGLTEXSTORAGE3DMULTISAMPLEOES)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
PFNGLTEXSTORAGE3DMULTISAMPLEOES gglTexStorage3DMultisampleOES;
static PFNGLTEXSTORAGE3DMULTISAMPLEOES _glTexStorage3DMultisampleOES;
static void APIENTRY d_glTexStorage3DMultisampleOES(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations) {
	_glTexStorage3DMultisampleOES(target, samples, internalformat, width, height, depth, fixedsamplelocations);
	CheckGLError("glTexStorage3DMultisampleOES");
}
typedef void (APIENTRYP PFNGLTEXSUBIMAGE2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
PFNGLTEXSUBIMAGE2D gglTexSubImage2D;
static PFNGLTEXSUBIMAGE2D _glTexSubImage2D;
static void APIENTRY d_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels) {
	_glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
	CheckGLError("glTexSubImage2D");
}
typedef void (APIENTRYP PFNGLTEXSUBIMAGE3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
PFNGLTEXSUBIMAGE3D gglTexSubImage3D;
static PFNGLTEXSUBIMAGE3D _glTexSubImage3D;
static void APIENTRY d_glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) {
	_glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
	CheckGLError("glTexSubImage3D");
}
typedef void (APIENTRYP PFNGLTEXSUBIMAGE3DOES)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
PFNGLTEXSUBIMAGE3DOES gglTexSubImage3DOES;
static PFNGLTEXSUBIMAGE3DOES _glTexSubImage3DOES;
static void APIENTRY d_glTexSubImage3DOES(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels) {
	_glTexSubImage3DOES(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
	CheckGLError("glTexSubImage3DOES");
}
typedef void (APIENTRYP PFNGLTEXTURESTORAGE1DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width);
PFNGLTEXTURESTORAGE1DEXT gglTextureStorage1DEXT;
static PFNGLTEXTURESTORAGE1DEXT _glTextureStorage1DEXT;
static void APIENTRY d_glTextureStorage1DEXT(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width) {
	_glTextureStorage1DEXT(texture, target, levels, internalformat, width);
	CheckGLError("glTextureStorage1DEXT");
}
typedef void (APIENTRYP PFNGLTEXTURESTORAGE2DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
PFNGLTEXTURESTORAGE2DEXT gglTextureStorage2DEXT;
static PFNGLTEXTURESTORAGE2DEXT _glTextureStorage2DEXT;
static void APIENTRY d_glTextureStorage2DEXT(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) {
	_glTextureStorage2DEXT(texture, target, levels, internalformat, width, height);
	CheckGLError("glTextureStorage2DEXT");
}
typedef void (APIENTRYP PFNGLTEXTURESTORAGE3DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
PFNGLTEXTURESTORAGE3DEXT gglTextureStorage3DEXT;
static PFNGLTEXTURESTORAGE3DEXT _glTextureStorage3DEXT;
static void APIENTRY d_glTextureStorage3DEXT(GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) {
	_glTextureStorage3DEXT(texture, target, levels, internalformat, width, height, depth);
	CheckGLError("glTextureStorage3DEXT");
}
typedef void (APIENTRYP PFNGLTEXTUREVIEWEXT)(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers);
PFNGLTEXTUREVIEWEXT gglTextureViewEXT;
static PFNGLTEXTUREVIEWEXT _glTextureViewEXT;
static void APIENTRY d_glTextureViewEXT(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) {
	_glTextureViewEXT(texture, target, origtexture, internalformat, minlevel, numlevels, minlayer, numlayers);
	CheckGLError("glTextureViewEXT");
}
typedef void (APIENTRYP PFNGLTEXTUREVIEWOES)(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers);
PFNGLTEXTUREVIEWOES gglTextureViewOES;
static PFNGLTEXTUREVIEWOES _glTextureViewOES;
static void APIENTRY d_glTextureViewOES(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers) {
	_glTextureViewOES(texture, target, origtexture, internalformat, minlevel, numlevels, minlayer, numlayers);
	CheckGLError("glTextureViewOES");
}
typedef void (APIENTRYP PFNGLTRANSFORMFEEDBACKVARYINGS)(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode);
PFNGLTRANSFORMFEEDBACKVARYINGS gglTransformFeedbackVaryings;
static PFNGLTRANSFORMFEEDBACKVARYINGS _glTransformFeedbackVaryings;
static void APIENTRY d_glTransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode) {
	_glTransformFeedbackVaryings(program, count, varyings, bufferMode);
	CheckGLError("glTransformFeedbackVaryings");
}
typedef void (APIENTRYP PFNGLTRANSFORMPATHNV)(GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat *transformValues);
PFNGLTRANSFORMPATHNV gglTransformPathNV;
static PFNGLTRANSFORMPATHNV _glTransformPathNV;
static void APIENTRY d_glTransformPathNV(GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat *transformValues) {
	_glTransformPathNV(resultPath, srcPath, transformType, transformValues);
	CheckGLError("glTransformPathNV");
}
typedef void (APIENTRYP PFNGLUNIFORM1F)(GLint location, GLfloat v0);
PFNGLUNIFORM1F gglUniform1f;
static PFNGLUNIFORM1F _glUniform1f;
static void APIENTRY d_glUniform1f(GLint location, GLfloat v0) {
	_glUniform1f(location, v0);
	CheckGLError("glUniform1f");
}
typedef void (APIENTRYP PFNGLUNIFORM1FV)(GLint location, GLsizei count, const GLfloat *value);
PFNGLUNIFORM1FV gglUniform1fv;
static PFNGLUNIFORM1FV _glUniform1fv;
static void APIENTRY d_glUniform1fv(GLint location, GLsizei count, const GLfloat *value) {
	_glUniform1fv(location, count, value);
	CheckGLError("glUniform1fv");
}
typedef void (APIENTRYP PFNGLUNIFORM1I)(GLint location, GLint v0);
PFNGLUNIFORM1I gglUniform1i;
static PFNGLUNIFORM1I _glUniform1i;
static void APIENTRY d_glUniform1i(GLint location, GLint v0) {
	_glUniform1i(location, v0);
	CheckGLError("glUniform1i");
}
typedef void (APIENTRYP PFNGLUNIFORM1I64NV)(GLint location, GLint64EXT x);
PFNGLUNIFORM1I64NV gglUniform1i64NV;
static PFNGLUNIFORM1I64NV _glUniform1i64NV;
static void APIENTRY d_glUniform1i64NV(GLint location, GLint64EXT x) {
	_glUniform1i64NV(location, x);
	CheckGLError("glUniform1i64NV");
}
typedef void (APIENTRYP PFNGLUNIFORM1I64VNV)(GLint location, GLsizei count, const GLint64EXT *value);
PFNGLUNIFORM1I64VNV gglUniform1i64vNV;
static PFNGLUNIFORM1I64VNV _glUniform1i64vNV;
static void APIENTRY d_glUniform1i64vNV(GLint location, GLsizei count, const GLint64EXT *value) {
	_glUniform1i64vNV(location, count, value);
	CheckGLError("glUniform1i64vNV");
}
typedef void (APIENTRYP PFNGLUNIFORM1IV)(GLint location, GLsizei count, const GLint *value);
PFNGLUNIFORM1IV gglUniform1iv;
static PFNGLUNIFORM1IV _glUniform1iv;
static void APIENTRY d_glUniform1iv(GLint location, GLsizei count, const GLint *value) {
	_glUniform1iv(location, count, value);
	CheckGLError("glUniform1iv");
}
typedef void (APIENTRYP PFNGLUNIFORM1UI)(GLint location, GLuint v0);
PFNGLUNIFORM1UI gglUniform1ui;
static PFNGLUNIFORM1UI _glUniform1ui;
static void APIENTRY d_glUniform1ui(GLint location, GLuint v0) {
	_glUniform1ui(location, v0);
	CheckGLError("glUniform1ui");
}
typedef void (APIENTRYP PFNGLUNIFORM1UI64NV)(GLint location, GLuint64EXT x);
PFNGLUNIFORM1UI64NV gglUniform1ui64NV;
static PFNGLUNIFORM1UI64NV _glUniform1ui64NV;
static void APIENTRY d_glUniform1ui64NV(GLint location, GLuint64EXT x) {
	_glUniform1ui64NV(location, x);
	CheckGLError("glUniform1ui64NV");
}
typedef void (APIENTRYP PFNGLUNIFORM1UI64VNV)(GLint location, GLsizei count, const GLuint64EXT *value);
PFNGLUNIFORM1UI64VNV gglUniform1ui64vNV;
static PFNGLUNIFORM1UI64VNV _glUniform1ui64vNV;
static void APIENTRY d_glUniform1ui64vNV(GLint location, GLsizei count, const GLuint64EXT *value) {
	_glUniform1ui64vNV(location, count, value);
	CheckGLError("glUniform1ui64vNV");
}
typedef void (APIENTRYP PFNGLUNIFORM1UIV)(GLint location, GLsizei count, const GLuint *value);
PFNGLUNIFORM1UIV gglUniform1uiv;
static PFNGLUNIFORM1UIV _glUniform1uiv;
static void APIENTRY d_glUniform1uiv(GLint location, GLsizei count, const GLuint *value) {
	_glUniform1uiv(location, count, value);
	CheckGLError("glUniform1uiv");
}
typedef void (APIENTRYP PFNGLUNIFORM2F)(GLint location, GLfloat v0, GLfloat v1);
PFNGLUNIFORM2F gglUniform2f;
static PFNGLUNIFORM2F _glUniform2f;
static void APIENTRY d_glUniform2f(GLint location, GLfloat v0, GLfloat v1) {
	_glUniform2f(location, v0, v1);
	CheckGLError("glUniform2f");
}
typedef void (APIENTRYP PFNGLUNIFORM2FV)(GLint location, GLsizei count, const GLfloat *value);
PFNGLUNIFORM2FV gglUniform2fv;
static PFNGLUNIFORM2FV _glUniform2fv;
static void APIENTRY d_glUniform2fv(GLint location, GLsizei count, const GLfloat *value) {
	_glUniform2fv(location, count, value);
	CheckGLError("glUniform2fv");
}
typedef void (APIENTRYP PFNGLUNIFORM2I)(GLint location, GLint v0, GLint v1);
PFNGLUNIFORM2I gglUniform2i;
static PFNGLUNIFORM2I _glUniform2i;
static void APIENTRY d_glUniform2i(GLint location, GLint v0, GLint v1) {
	_glUniform2i(location, v0, v1);
	CheckGLError("glUniform2i");
}
typedef void (APIENTRYP PFNGLUNIFORM2I64NV)(GLint location, GLint64EXT x, GLint64EXT y);
PFNGLUNIFORM2I64NV gglUniform2i64NV;
static PFNGLUNIFORM2I64NV _glUniform2i64NV;
static void APIENTRY d_glUniform2i64NV(GLint location, GLint64EXT x, GLint64EXT y) {
	_glUniform2i64NV(location, x, y);
	CheckGLError("glUniform2i64NV");
}
typedef void (APIENTRYP PFNGLUNIFORM2I64VNV)(GLint location, GLsizei count, const GLint64EXT *value);
PFNGLUNIFORM2I64VNV gglUniform2i64vNV;
static PFNGLUNIFORM2I64VNV _glUniform2i64vNV;
static void APIENTRY d_glUniform2i64vNV(GLint location, GLsizei count, const GLint64EXT *value) {
	_glUniform2i64vNV(location, count, value);
	CheckGLError("glUniform2i64vNV");
}
typedef void (APIENTRYP PFNGLUNIFORM2IV)(GLint location, GLsizei count, const GLint *value);
PFNGLUNIFORM2IV gglUniform2iv;
static PFNGLUNIFORM2IV _glUniform2iv;
static void APIENTRY d_glUniform2iv(GLint location, GLsizei count, const GLint *value) {
	_glUniform2iv(location, count, value);
	CheckGLError("glUniform2iv");
}
typedef void (APIENTRYP PFNGLUNIFORM2UI)(GLint location, GLuint v0, GLuint v1);
PFNGLUNIFORM2UI gglUniform2ui;
static PFNGLUNIFORM2UI _glUniform2ui;
static void APIENTRY d_glUniform2ui(GLint location, GLuint v0, GLuint v1) {
	_glUniform2ui(location, v0, v1);
	CheckGLError("glUniform2ui");
}
typedef void (APIENTRYP PFNGLUNIFORM2UI64NV)(GLint location, GLuint64EXT x, GLuint64EXT y);
PFNGLUNIFORM2UI64NV gglUniform2ui64NV;
static PFNGLUNIFORM2UI64NV _glUniform2ui64NV;
static void APIENTRY d_glUniform2ui64NV(GLint location, GLuint64EXT x, GLuint64EXT y) {
	_glUniform2ui64NV(location, x, y);
	CheckGLError("glUniform2ui64NV");
}
typedef void (APIENTRYP PFNGLUNIFORM2UI64VNV)(GLint location, GLsizei count, const GLuint64EXT *value);
PFNGLUNIFORM2UI64VNV gglUniform2ui64vNV;
static PFNGLUNIFORM2UI64VNV _glUniform2ui64vNV;
static void APIENTRY d_glUniform2ui64vNV(GLint location, GLsizei count, const GLuint64EXT *value) {
	_glUniform2ui64vNV(location, count, value);
	CheckGLError("glUniform2ui64vNV");
}
typedef void (APIENTRYP PFNGLUNIFORM2UIV)(GLint location, GLsizei count, const GLuint *value);
PFNGLUNIFORM2UIV gglUniform2uiv;
static PFNGLUNIFORM2UIV _glUniform2uiv;
static void APIENTRY d_glUniform2uiv(GLint location, GLsizei count, const GLuint *value) {
	_glUniform2uiv(location, count, value);
	CheckGLError("glUniform2uiv");
}
typedef void (APIENTRYP PFNGLUNIFORM3F)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
PFNGLUNIFORM3F gglUniform3f;
static PFNGLUNIFORM3F _glUniform3f;
static void APIENTRY d_glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
	_glUniform3f(location, v0, v1, v2);
	CheckGLError("glUniform3f");
}
typedef void (APIENTRYP PFNGLUNIFORM3FV)(GLint location, GLsizei count, const GLfloat *value);
PFNGLUNIFORM3FV gglUniform3fv;
static PFNGLUNIFORM3FV _glUniform3fv;
static void APIENTRY d_glUniform3fv(GLint location, GLsizei count, const GLfloat *value) {
	_glUniform3fv(location, count, value);
	CheckGLError("glUniform3fv");
}
typedef void (APIENTRYP PFNGLUNIFORM3I)(GLint location, GLint v0, GLint v1, GLint v2);
PFNGLUNIFORM3I gglUniform3i;
static PFNGLUNIFORM3I _glUniform3i;
static void APIENTRY d_glUniform3i(GLint location, GLint v0, GLint v1, GLint v2) {
	_glUniform3i(location, v0, v1, v2);
	CheckGLError("glUniform3i");
}
typedef void (APIENTRYP PFNGLUNIFORM3I64NV)(GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z);
PFNGLUNIFORM3I64NV gglUniform3i64NV;
static PFNGLUNIFORM3I64NV _glUniform3i64NV;
static void APIENTRY d_glUniform3i64NV(GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z) {
	_glUniform3i64NV(location, x, y, z);
	CheckGLError("glUniform3i64NV");
}
typedef void (APIENTRYP PFNGLUNIFORM3I64VNV)(GLint location, GLsizei count, const GLint64EXT *value);
PFNGLUNIFORM3I64VNV gglUniform3i64vNV;
static PFNGLUNIFORM3I64VNV _glUniform3i64vNV;
static void APIENTRY d_glUniform3i64vNV(GLint location, GLsizei count, const GLint64EXT *value) {
	_glUniform3i64vNV(location, count, value);
	CheckGLError("glUniform3i64vNV");
}
typedef void (APIENTRYP PFNGLUNIFORM3IV)(GLint location, GLsizei count, const GLint *value);
PFNGLUNIFORM3IV gglUniform3iv;
static PFNGLUNIFORM3IV _glUniform3iv;
static void APIENTRY d_glUniform3iv(GLint location, GLsizei count, const GLint *value) {
	_glUniform3iv(location, count, value);
	CheckGLError("glUniform3iv");
}
typedef void (APIENTRYP PFNGLUNIFORM3UI)(GLint location, GLuint v0, GLuint v1, GLuint v2);
PFNGLUNIFORM3UI gglUniform3ui;
static PFNGLUNIFORM3UI _glUniform3ui;
static void APIENTRY d_glUniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2) {
	_glUniform3ui(location, v0, v1, v2);
	CheckGLError("glUniform3ui");
}
typedef void (APIENTRYP PFNGLUNIFORM3UI64NV)(GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z);
PFNGLUNIFORM3UI64NV gglUniform3ui64NV;
static PFNGLUNIFORM3UI64NV _glUniform3ui64NV;
static void APIENTRY d_glUniform3ui64NV(GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z) {
	_glUniform3ui64NV(location, x, y, z);
	CheckGLError("glUniform3ui64NV");
}
typedef void (APIENTRYP PFNGLUNIFORM3UI64VNV)(GLint location, GLsizei count, const GLuint64EXT *value);
PFNGLUNIFORM3UI64VNV gglUniform3ui64vNV;
static PFNGLUNIFORM3UI64VNV _glUniform3ui64vNV;
static void APIENTRY d_glUniform3ui64vNV(GLint location, GLsizei count, const GLuint64EXT *value) {
	_glUniform3ui64vNV(location, count, value);
	CheckGLError("glUniform3ui64vNV");
}
typedef void (APIENTRYP PFNGLUNIFORM3UIV)(GLint location, GLsizei count, const GLuint *value);
PFNGLUNIFORM3UIV gglUniform3uiv;
static PFNGLUNIFORM3UIV _glUniform3uiv;
static void APIENTRY d_glUniform3uiv(GLint location, GLsizei count, const GLuint *value) {
	_glUniform3uiv(location, count, value);
	CheckGLError("glUniform3uiv");
}
typedef void (APIENTRYP PFNGLUNIFORM4F)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
PFNGLUNIFORM4F gglUniform4f;
static PFNGLUNIFORM4F _glUniform4f;
static void APIENTRY d_glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
	_glUniform4f(location, v0, v1, v2, v3);
	CheckGLError("glUniform4f");
}
typedef void (APIENTRYP PFNGLUNIFORM4FV)(GLint location, GLsizei count, const GLfloat *value);
PFNGLUNIFORM4FV gglUniform4fv;
static PFNGLUNIFORM4FV _glUniform4fv;
static void APIENTRY d_glUniform4fv(GLint location, GLsizei count, const GLfloat *value) {
	_glUniform4fv(location, count, value);
	CheckGLError("glUniform4fv");
}
typedef void (APIENTRYP PFNGLUNIFORM4I)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
PFNGLUNIFORM4I gglUniform4i;
static PFNGLUNIFORM4I _glUniform4i;
static void APIENTRY d_glUniform4i(GLint location, GLint v0, GLint v1, GLint v2, GLint v3) {
	_glUniform4i(location, v0, v1, v2, v3);
	CheckGLError("glUniform4i");
}
typedef void (APIENTRYP PFNGLUNIFORM4I64NV)(GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w);
PFNGLUNIFORM4I64NV gglUniform4i64NV;
static PFNGLUNIFORM4I64NV _glUniform4i64NV;
static void APIENTRY d_glUniform4i64NV(GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w) {
	_glUniform4i64NV(location, x, y, z, w);
	CheckGLError("glUniform4i64NV");
}
typedef void (APIENTRYP PFNGLUNIFORM4I64VNV)(GLint location, GLsizei count, const GLint64EXT *value);
PFNGLUNIFORM4I64VNV gglUniform4i64vNV;
static PFNGLUNIFORM4I64VNV _glUniform4i64vNV;
static void APIENTRY d_glUniform4i64vNV(GLint location, GLsizei count, const GLint64EXT *value) {
	_glUniform4i64vNV(location, count, value);
	CheckGLError("glUniform4i64vNV");
}
typedef void (APIENTRYP PFNGLUNIFORM4IV)(GLint location, GLsizei count, const GLint *value);
PFNGLUNIFORM4IV gglUniform4iv;
static PFNGLUNIFORM4IV _glUniform4iv;
static void APIENTRY d_glUniform4iv(GLint location, GLsizei count, const GLint *value) {
	_glUniform4iv(location, count, value);
	CheckGLError("glUniform4iv");
}
typedef void (APIENTRYP PFNGLUNIFORM4UI)(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
PFNGLUNIFORM4UI gglUniform4ui;
static PFNGLUNIFORM4UI _glUniform4ui;
static void APIENTRY d_glUniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
	_glUniform4ui(location, v0, v1, v2, v3);
	CheckGLError("glUniform4ui");
}
typedef void (APIENTRYP PFNGLUNIFORM4UI64NV)(GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w);
PFNGLUNIFORM4UI64NV gglUniform4ui64NV;
static PFNGLUNIFORM4UI64NV _glUniform4ui64NV;
static void APIENTRY d_glUniform4ui64NV(GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w) {
	_glUniform4ui64NV(location, x, y, z, w);
	CheckGLError("glUniform4ui64NV");
}
typedef void (APIENTRYP PFNGLUNIFORM4UI64VNV)(GLint location, GLsizei count, const GLuint64EXT *value);
PFNGLUNIFORM4UI64VNV gglUniform4ui64vNV;
static PFNGLUNIFORM4UI64VNV _glUniform4ui64vNV;
static void APIENTRY d_glUniform4ui64vNV(GLint location, GLsizei count, const GLuint64EXT *value) {
	_glUniform4ui64vNV(location, count, value);
	CheckGLError("glUniform4ui64vNV");
}
typedef void (APIENTRYP PFNGLUNIFORM4UIV)(GLint location, GLsizei count, const GLuint *value);
PFNGLUNIFORM4UIV gglUniform4uiv;
static PFNGLUNIFORM4UIV _glUniform4uiv;
static void APIENTRY d_glUniform4uiv(GLint location, GLsizei count, const GLuint *value) {
	_glUniform4uiv(location, count, value);
	CheckGLError("glUniform4uiv");
}
typedef void (APIENTRYP PFNGLUNIFORMBLOCKBINDING)(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
PFNGLUNIFORMBLOCKBINDING gglUniformBlockBinding;
static PFNGLUNIFORMBLOCKBINDING _glUniformBlockBinding;
static void APIENTRY d_glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding) {
	_glUniformBlockBinding(program, uniformBlockIndex, uniformBlockBinding);
	CheckGLError("glUniformBlockBinding");
}
typedef void (APIENTRYP PFNGLUNIFORMHANDLEUI64IMG)(GLint location, GLuint64 value);
PFNGLUNIFORMHANDLEUI64IMG gglUniformHandleui64IMG;
static PFNGLUNIFORMHANDLEUI64IMG _glUniformHandleui64IMG;
static void APIENTRY d_glUniformHandleui64IMG(GLint location, GLuint64 value) {
	_glUniformHandleui64IMG(location, value);
	CheckGLError("glUniformHandleui64IMG");
}
typedef void (APIENTRYP PFNGLUNIFORMHANDLEUI64NV)(GLint location, GLuint64 value);
PFNGLUNIFORMHANDLEUI64NV gglUniformHandleui64NV;
static PFNGLUNIFORMHANDLEUI64NV _glUniformHandleui64NV;
static void APIENTRY d_glUniformHandleui64NV(GLint location, GLuint64 value) {
	_glUniformHandleui64NV(location, value);
	CheckGLError("glUniformHandleui64NV");
}
typedef void (APIENTRYP PFNGLUNIFORMHANDLEUI64VIMG)(GLint location, GLsizei count, const GLuint64 *value);
PFNGLUNIFORMHANDLEUI64VIMG gglUniformHandleui64vIMG;
static PFNGLUNIFORMHANDLEUI64VIMG _glUniformHandleui64vIMG;
static void APIENTRY d_glUniformHandleui64vIMG(GLint location, GLsizei count, const GLuint64 *value) {
	_glUniformHandleui64vIMG(location, count, value);
	CheckGLError("glUniformHandleui64vIMG");
}
typedef void (APIENTRYP PFNGLUNIFORMHANDLEUI64VNV)(GLint location, GLsizei count, const GLuint64 *value);
PFNGLUNIFORMHANDLEUI64VNV gglUniformHandleui64vNV;
static PFNGLUNIFORMHANDLEUI64VNV _glUniformHandleui64vNV;
static void APIENTRY d_glUniformHandleui64vNV(GLint location, GLsizei count, const GLuint64 *value) {
	_glUniformHandleui64vNV(location, count, value);
	CheckGLError("glUniformHandleui64vNV");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX2FV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX2FV gglUniformMatrix2fv;
static PFNGLUNIFORMMATRIX2FV _glUniformMatrix2fv;
static void APIENTRY d_glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix2fv(location, count, transpose, value);
	CheckGLError("glUniformMatrix2fv");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX2X3FV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX2X3FV gglUniformMatrix2x3fv;
static PFNGLUNIFORMMATRIX2X3FV _glUniformMatrix2x3fv;
static void APIENTRY d_glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix2x3fv(location, count, transpose, value);
	CheckGLError("glUniformMatrix2x3fv");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX2X3FVNV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX2X3FVNV gglUniformMatrix2x3fvNV;
static PFNGLUNIFORMMATRIX2X3FVNV _glUniformMatrix2x3fvNV;
static void APIENTRY d_glUniformMatrix2x3fvNV(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix2x3fvNV(location, count, transpose, value);
	CheckGLError("glUniformMatrix2x3fvNV");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX2X4FV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX2X4FV gglUniformMatrix2x4fv;
static PFNGLUNIFORMMATRIX2X4FV _glUniformMatrix2x4fv;
static void APIENTRY d_glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix2x4fv(location, count, transpose, value);
	CheckGLError("glUniformMatrix2x4fv");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX2X4FVNV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX2X4FVNV gglUniformMatrix2x4fvNV;
static PFNGLUNIFORMMATRIX2X4FVNV _glUniformMatrix2x4fvNV;
static void APIENTRY d_glUniformMatrix2x4fvNV(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix2x4fvNV(location, count, transpose, value);
	CheckGLError("glUniformMatrix2x4fvNV");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX3FV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX3FV gglUniformMatrix3fv;
static PFNGLUNIFORMMATRIX3FV _glUniformMatrix3fv;
static void APIENTRY d_glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix3fv(location, count, transpose, value);
	CheckGLError("glUniformMatrix3fv");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX3X2FV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX3X2FV gglUniformMatrix3x2fv;
static PFNGLUNIFORMMATRIX3X2FV _glUniformMatrix3x2fv;
static void APIENTRY d_glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix3x2fv(location, count, transpose, value);
	CheckGLError("glUniformMatrix3x2fv");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX3X2FVNV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX3X2FVNV gglUniformMatrix3x2fvNV;
static PFNGLUNIFORMMATRIX3X2FVNV _glUniformMatrix3x2fvNV;
static void APIENTRY d_glUniformMatrix3x2fvNV(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix3x2fvNV(location, count, transpose, value);
	CheckGLError("glUniformMatrix3x2fvNV");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX3X4FV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX3X4FV gglUniformMatrix3x4fv;
static PFNGLUNIFORMMATRIX3X4FV _glUniformMatrix3x4fv;
static void APIENTRY d_glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix3x4fv(location, count, transpose, value);
	CheckGLError("glUniformMatrix3x4fv");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX3X4FVNV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX3X4FVNV gglUniformMatrix3x4fvNV;
static PFNGLUNIFORMMATRIX3X4FVNV _glUniformMatrix3x4fvNV;
static void APIENTRY d_glUniformMatrix3x4fvNV(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix3x4fvNV(location, count, transpose, value);
	CheckGLError("glUniformMatrix3x4fvNV");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4FV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX4FV gglUniformMatrix4fv;
static PFNGLUNIFORMMATRIX4FV _glUniformMatrix4fv;
static void APIENTRY d_glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix4fv(location, count, transpose, value);
	CheckGLError("glUniformMatrix4fv");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4X2FV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX4X2FV gglUniformMatrix4x2fv;
static PFNGLUNIFORMMATRIX4X2FV _glUniformMatrix4x2fv;
static void APIENTRY d_glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix4x2fv(location, count, transpose, value);
	CheckGLError("glUniformMatrix4x2fv");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4X2FVNV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX4X2FVNV gglUniformMatrix4x2fvNV;
static PFNGLUNIFORMMATRIX4X2FVNV _glUniformMatrix4x2fvNV;
static void APIENTRY d_glUniformMatrix4x2fvNV(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix4x2fvNV(location, count, transpose, value);
	CheckGLError("glUniformMatrix4x2fvNV");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4X3FV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX4X3FV gglUniformMatrix4x3fv;
static PFNGLUNIFORMMATRIX4X3FV _glUniformMatrix4x3fv;
static void APIENTRY d_glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix4x3fv(location, count, transpose, value);
	CheckGLError("glUniformMatrix4x3fv");
}
typedef void (APIENTRYP PFNGLUNIFORMMATRIX4X3FVNV)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
PFNGLUNIFORMMATRIX4X3FVNV gglUniformMatrix4x3fvNV;
static PFNGLUNIFORMMATRIX4X3FVNV _glUniformMatrix4x3fvNV;
static void APIENTRY d_glUniformMatrix4x3fvNV(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) {
	_glUniformMatrix4x3fvNV(location, count, transpose, value);
	CheckGLError("glUniformMatrix4x3fvNV");
}
typedef GLboolean (APIENTRYP PFNGLUNMAPBUFFER)(GLenum target);
PFNGLUNMAPBUFFER gglUnmapBuffer;
static PFNGLUNMAPBUFFER _glUnmapBuffer;
static GLboolean APIENTRY d_glUnmapBuffer(GLenum target) {
	GLboolean ret = _glUnmapBuffer(target);
	CheckGLError("glUnmapBuffer");
	return ret;
}
typedef GLboolean (APIENTRYP PFNGLUNMAPBUFFEROES)(GLenum target);
PFNGLUNMAPBUFFEROES gglUnmapBufferOES;
static PFNGLUNMAPBUFFEROES _glUnmapBufferOES;
static GLboolean APIENTRY d_glUnmapBufferOES(GLenum target) {
	GLboolean ret = _glUnmapBufferOES(target);
	CheckGLError("glUnmapBufferOES");
	return ret;
}
typedef void (APIENTRYP PFNGLUSEPROGRAM)(GLuint program);
PFNGLUSEPROGRAM gglUseProgram;
static PFNGLUSEPROGRAM _glUseProgram;
static void APIENTRY d_glUseProgram(GLuint program) {
	_glUseProgram(program);
	CheckGLError("glUseProgram");
}
typedef void (APIENTRYP PFNGLUSEPROGRAMSTAGESEXT)(GLuint pipeline, GLbitfield stages, GLuint program);
PFNGLUSEPROGRAMSTAGESEXT gglUseProgramStagesEXT;
static PFNGLUSEPROGRAMSTAGESEXT _glUseProgramStagesEXT;
static void APIENTRY d_glUseProgramStagesEXT(GLuint pipeline, GLbitfield stages, GLuint program) {
	_glUseProgramStagesEXT(pipeline, stages, program);
	CheckGLError("glUseProgramStagesEXT");
}
typedef void (APIENTRYP PFNGLVALIDATEPROGRAM)(GLuint program);
PFNGLVALIDATEPROGRAM gglValidateProgram;
static PFNGLVALIDATEPROGRAM _glValidateProgram;
static void APIENTRY d_glValidateProgram(GLuint program) {
	_glValidateProgram(program);
	CheckGLError("glValidateProgram");
}
typedef void (APIENTRYP PFNGLVALIDATEPROGRAMPIPELINEEXT)(GLuint pipeline);
PFNGLVALIDATEPROGRAMPIPELINEEXT gglValidateProgramPipelineEXT;
static PFNGLVALIDATEPROGRAMPIPELINEEXT _glValidateProgramPipelineEXT;
static void APIENTRY d_glValidateProgramPipelineEXT(GLuint pipeline) {
	_glValidateProgramPipelineEXT(pipeline);
	CheckGLError("glValidateProgramPipelineEXT");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIB1F)(GLuint index, GLfloat x);
PFNGLVERTEXATTRIB1F gglVertexAttrib1f;
static PFNGLVERTEXATTRIB1F _glVertexAttrib1f;
static void APIENTRY d_glVertexAttrib1f(GLuint index, GLfloat x) {
	_glVertexAttrib1f(index, x);
	CheckGLError("glVertexAttrib1f");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIB1FV)(GLuint index, const GLfloat *v);
PFNGLVERTEXATTRIB1FV gglVertexAttrib1fv;
static PFNGLVERTEXATTRIB1FV _glVertexAttrib1fv;
static void APIENTRY d_glVertexAttrib1fv(GLuint index, const GLfloat *v) {
	_glVertexAttrib1fv(index, v);
	CheckGLError("glVertexAttrib1fv");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIB2F)(GLuint index, GLfloat x, GLfloat y);
PFNGLVERTEXATTRIB2F gglVertexAttrib2f;
static PFNGLVERTEXATTRIB2F _glVertexAttrib2f;
static void APIENTRY d_glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y) {
	_glVertexAttrib2f(index, x, y);
	CheckGLError("glVertexAttrib2f");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIB2FV)(GLuint index, const GLfloat *v);
PFNGLVERTEXATTRIB2FV gglVertexAttrib2fv;
static PFNGLVERTEXATTRIB2FV _glVertexAttrib2fv;
static void APIENTRY d_glVertexAttrib2fv(GLuint index, const GLfloat *v) {
	_glVertexAttrib2fv(index, v);
	CheckGLError("glVertexAttrib2fv");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIB3F)(GLuint index, GLfloat x, GLfloat y, GLfloat z);
PFNGLVERTEXATTRIB3F gglVertexAttrib3f;
static PFNGLVERTEXATTRIB3F _glVertexAttrib3f;
static void APIENTRY d_glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z) {
	_glVertexAttrib3f(index, x, y, z);
	CheckGLError("glVertexAttrib3f");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIB3FV)(GLuint index, const GLfloat *v);
PFNGLVERTEXATTRIB3FV gglVertexAttrib3fv;
static PFNGLVERTEXATTRIB3FV _glVertexAttrib3fv;
static void APIENTRY d_glVertexAttrib3fv(GLuint index, const GLfloat *v) {
	_glVertexAttrib3fv(index, v);
	CheckGLError("glVertexAttrib3fv");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIB4F)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
PFNGLVERTEXATTRIB4F gglVertexAttrib4f;
static PFNGLVERTEXATTRIB4F _glVertexAttrib4f;
static void APIENTRY d_glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
	_glVertexAttrib4f(index, x, y, z, w);
	CheckGLError("glVertexAttrib4f");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIB4FV)(GLuint index, const GLfloat *v);
PFNGLVERTEXATTRIB4FV gglVertexAttrib4fv;
static PFNGLVERTEXATTRIB4FV _glVertexAttrib4fv;
static void APIENTRY d_glVertexAttrib4fv(GLuint index, const GLfloat *v) {
	_glVertexAttrib4fv(index, v);
	CheckGLError("glVertexAttrib4fv");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIBDIVISOR)(GLuint index, GLuint divisor);
PFNGLVERTEXATTRIBDIVISOR gglVertexAttribDivisor;
static PFNGLVERTEXATTRIBDIVISOR _glVertexAttribDivisor;
static void APIENTRY d_glVertexAttribDivisor(GLuint index, GLuint divisor) {
	_glVertexAttribDivisor(index, divisor);
	CheckGLError("glVertexAttribDivisor");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIBDIVISORANGLE)(GLuint index, GLuint divisor);
PFNGLVERTEXATTRIBDIVISORANGLE gglVertexAttribDivisorANGLE;
static PFNGLVERTEXATTRIBDIVISORANGLE _glVertexAttribDivisorANGLE;
static void APIENTRY d_glVertexAttribDivisorANGLE(GLuint index, GLuint divisor) {
	_glVertexAttribDivisorANGLE(index, divisor);
	CheckGLError("glVertexAttribDivisorANGLE");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIBDIVISOREXT)(GLuint index, GLuint divisor);
PFNGLVERTEXATTRIBDIVISOREXT gglVertexAttribDivisorEXT;
static PFNGLVERTEXATTRIBDIVISOREXT _glVertexAttribDivisorEXT;
static void APIENTRY d_glVertexAttribDivisorEXT(GLuint index, GLuint divisor) {
	_glVertexAttribDivisorEXT(index, divisor);
	CheckGLError("glVertexAttribDivisorEXT");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIBDIVISORNV)(GLuint index, GLuint divisor);
PFNGLVERTEXATTRIBDIVISORNV gglVertexAttribDivisorNV;
static PFNGLVERTEXATTRIBDIVISORNV _glVertexAttribDivisorNV;
static void APIENTRY d_glVertexAttribDivisorNV(GLuint index, GLuint divisor) {
	_glVertexAttribDivisorNV(index, divisor);
	CheckGLError("glVertexAttribDivisorNV");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIBI4I)(GLuint index, GLint x, GLint y, GLint z, GLint w);
PFNGLVERTEXATTRIBI4I gglVertexAttribI4i;
static PFNGLVERTEXATTRIBI4I _glVertexAttribI4i;
static void APIENTRY d_glVertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w) {
	_glVertexAttribI4i(index, x, y, z, w);
	CheckGLError("glVertexAttribI4i");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIBI4IV)(GLuint index, const GLint *v);
PFNGLVERTEXATTRIBI4IV gglVertexAttribI4iv;
static PFNGLVERTEXATTRIBI4IV _glVertexAttribI4iv;
static void APIENTRY d_glVertexAttribI4iv(GLuint index, const GLint *v) {
	_glVertexAttribI4iv(index, v);
	CheckGLError("glVertexAttribI4iv");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIBI4UI)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
PFNGLVERTEXATTRIBI4UI gglVertexAttribI4ui;
static PFNGLVERTEXATTRIBI4UI _glVertexAttribI4ui;
static void APIENTRY d_glVertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) {
	_glVertexAttribI4ui(index, x, y, z, w);
	CheckGLError("glVertexAttribI4ui");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIBI4UIV)(GLuint index, const GLuint *v);
PFNGLVERTEXATTRIBI4UIV gglVertexAttribI4uiv;
static PFNGLVERTEXATTRIBI4UIV _glVertexAttribI4uiv;
static void APIENTRY d_glVertexAttribI4uiv(GLuint index, const GLuint *v) {
	_glVertexAttribI4uiv(index, v);
	CheckGLError("glVertexAttribI4uiv");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIBIPOINTER)(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
PFNGLVERTEXATTRIBIPOINTER gglVertexAttribIPointer;
static PFNGLVERTEXATTRIBIPOINTER _glVertexAttribIPointer;
static void APIENTRY d_glVertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer) {
	_glVertexAttribIPointer(index, size, type, stride, pointer);
	CheckGLError("glVertexAttribIPointer");
}
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTER)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
PFNGLVERTEXATTRIBPOINTER gglVertexAttribPointer;
static PFNGLVERTEXATTRIBPOINTER _glVertexAttribPointer;
static void APIENTRY d_glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer) {
	_glVertexAttribPointer(index, size, type, normalized, stride, pointer);
	CheckGLError("glVertexAttribPointer");
}
typedef void (APIENTRYP PFNGLVIEWPORT)(GLint x, GLint y, GLsizei width, GLsizei height);
PFNGLVIEWPORT gglViewport;
static PFNGLVIEWPORT _glViewport;
static void APIENTRY d_glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	_glViewport(x, y, width, height);
	CheckGLError("glViewport");
}
typedef void (APIENTRYP PFNGLVIEWPORTARRAYVNV)(GLuint first, GLsizei count, const GLfloat *v);
PFNGLVIEWPORTARRAYVNV gglViewportArrayvNV;
static PFNGLVIEWPORTARRAYVNV _glViewportArrayvNV;
static void APIENTRY d_glViewportArrayvNV(GLuint first, GLsizei count, const GLfloat *v) {
	_glViewportArrayvNV(first, count, v);
	CheckGLError("glViewportArrayvNV");
}
typedef void (APIENTRYP PFNGLVIEWPORTARRAYVOES)(GLuint first, GLsizei count, const GLfloat *v);
PFNGLVIEWPORTARRAYVOES gglViewportArrayvOES;
static PFNGLVIEWPORTARRAYVOES _glViewportArrayvOES;
static void APIENTRY d_glViewportArrayvOES(GLuint first, GLsizei count, const GLfloat *v) {
	_glViewportArrayvOES(first, count, v);
	CheckGLError("glViewportArrayvOES");
}
typedef void (APIENTRYP PFNGLVIEWPORTINDEXEDFOES)(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h);
PFNGLVIEWPORTINDEXEDFOES gglViewportIndexedfOES;
static PFNGLVIEWPORTINDEXEDFOES _glViewportIndexedfOES;
static void APIENTRY d_glViewportIndexedfOES(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h) {
	_glViewportIndexedfOES(index, x, y, w, h);
	CheckGLError("glViewportIndexedfOES");
}
typedef void (APIENTRYP PFNGLVIEWPORTINDEXEDFNV)(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h);
PFNGLVIEWPORTINDEXEDFNV gglViewportIndexedfNV;
static PFNGLVIEWPORTINDEXEDFNV _glViewportIndexedfNV;
static void APIENTRY d_glViewportIndexedfNV(GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h) {
	_glViewportIndexedfNV(index, x, y, w, h);
	CheckGLError("glViewportIndexedfNV");
}
typedef void (APIENTRYP PFNGLVIEWPORTINDEXEDFVOES)(GLuint index, const GLfloat *v);
PFNGLVIEWPORTINDEXEDFVOES gglViewportIndexedfvOES;
static PFNGLVIEWPORTINDEXEDFVOES _glViewportIndexedfvOES;
static void APIENTRY d_glViewportIndexedfvOES(GLuint index, const GLfloat *v) {
	_glViewportIndexedfvOES(index, v);
	CheckGLError("glViewportIndexedfvOES");
}
typedef void (APIENTRYP PFNGLVIEWPORTINDEXEDFVNV)(GLuint index, const GLfloat *v);
PFNGLVIEWPORTINDEXEDFVNV gglViewportIndexedfvNV;
static PFNGLVIEWPORTINDEXEDFVNV _glViewportIndexedfvNV;
static void APIENTRY d_glViewportIndexedfvNV(GLuint index, const GLfloat *v) {
	_glViewportIndexedfvNV(index, v);
	CheckGLError("glViewportIndexedfvNV");
}
typedef void (APIENTRYP PFNGLVIEWPORTSWIZZLENV)(GLuint index, GLenum swizzlex, GLenum swizzley, GLenum swizzlez, GLenum swizzlew);
PFNGLVIEWPORTSWIZZLENV gglViewportSwizzleNV;
static PFNGLVIEWPORTSWIZZLENV _glViewportSwizzleNV;
static void APIENTRY d_glViewportSwizzleNV(GLuint index, GLenum swizzlex, GLenum swizzley, GLenum swizzlez, GLenum swizzlew) {
	_glViewportSwizzleNV(index, swizzlex, swizzley, swizzlez, swizzlew);
	CheckGLError("glViewportSwizzleNV");
}
typedef void (APIENTRYP PFNGLWAITSYNC)(GLsync sync, GLbitfield flags, GLuint64 timeout);
PFNGLWAITSYNC gglWaitSync;
static PFNGLWAITSYNC _glWaitSync;
static void APIENTRY d_glWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout) {
	_glWaitSync(sync, flags, timeout);
	CheckGLError("glWaitSync");
}
typedef void (APIENTRYP PFNGLWAITSYNCAPPLE)(GLsync sync, GLbitfield flags, GLuint64 timeout);
PFNGLWAITSYNCAPPLE gglWaitSyncAPPLE;
static PFNGLWAITSYNCAPPLE _glWaitSyncAPPLE;
static void APIENTRY d_glWaitSyncAPPLE(GLsync sync, GLbitfield flags, GLuint64 timeout) {
	_glWaitSyncAPPLE(sync, flags, timeout);
	CheckGLError("glWaitSyncAPPLE");
}
typedef void (APIENTRYP PFNGLWEIGHTPATHSNV)(GLuint resultPath, GLsizei numPaths, const GLuint *paths, const GLfloat *weights);
PFNGLWEIGHTPATHSNV gglWeightPathsNV;
static PFNGLWEIGHTPATHSNV _glWeightPathsNV;
static void APIENTRY d_glWeightPathsNV(GLuint resultPath, GLsizei numPaths, const GLuint *paths, const GLfloat *weights) {
	_glWeightPathsNV(resultPath, numPaths, paths, weights);
	CheckGLError("glWeightPathsNV");
}
typedef void (APIENTRYP PFNGLWINDOWRECTANGLESEXT)(GLenum mode, GLsizei count, const GLint *box);
PFNGLWINDOWRECTANGLESEXT gglWindowRectanglesEXT;
static PFNGLWINDOWRECTANGLESEXT _glWindowRectanglesEXT;
static void APIENTRY d_glWindowRectanglesEXT(GLenum mode, GLsizei count, const GLint *box) {
	_glWindowRectanglesEXT(mode, count, box);
	CheckGLError("glWindowRectanglesEXT");
}

#if defined(ANDROID) || defined(__linux__)
#include <EGL/egl.h>
#define GPA(a) eglGetProcAddress(#a)
#elif defined(__APPLE__)
#define GPA(a) a
#endif

gglext_t gglext;
static GLint gglext_count = 0;
static const char *gglext_strings[512];

static int ggl_check_extension(const char *ext) {
	GLint i = 0;
	for (; i < gglext_count; i++) {
		if (!strcmp(gglext_strings[i], ext)) {
			return 1;
		}
	}
	return 0;
}

void ggl_init(int enableDebug) {
	/* GL_ES_VERSION_2_0 */
	_glActiveTexture = glActiveTexture;
	_glAttachShader = glAttachShader;
	_glBindAttribLocation = glBindAttribLocation;
	_glBindBuffer = glBindBuffer;
	_glBindFramebuffer = glBindFramebuffer;
	_glBindRenderbuffer = glBindRenderbuffer;
	_glBindTexture = glBindTexture;
	_glBlendColor = glBlendColor;
	_glBlendEquation = glBlendEquation;
	_glBlendEquationSeparate = glBlendEquationSeparate;
	_glBlendFunc = glBlendFunc;
	_glBlendFuncSeparate = glBlendFuncSeparate;
	_glBufferData = glBufferData;
	_glBufferSubData = glBufferSubData;
	_glCheckFramebufferStatus = glCheckFramebufferStatus;
	_glClear = glClear;
	_glClearColor = glClearColor;
	_glClearDepthf = glClearDepthf;
	_glClearStencil = glClearStencil;
	_glColorMask = glColorMask;
	_glCompileShader = glCompileShader;
	_glCompressedTexImage2D = glCompressedTexImage2D;
	_glCompressedTexSubImage2D = glCompressedTexSubImage2D;
	_glCopyTexImage2D = glCopyTexImage2D;
	_glCopyTexSubImage2D = glCopyTexSubImage2D;
	_glCreateProgram = glCreateProgram;
	_glCreateShader = glCreateShader;
	_glCullFace = glCullFace;
	_glDeleteBuffers = glDeleteBuffers;
	_glDeleteFramebuffers = glDeleteFramebuffers;
	_glDeleteProgram = glDeleteProgram;
	_glDeleteRenderbuffers = glDeleteRenderbuffers;
	_glDeleteShader = glDeleteShader;
	_glDeleteTextures = glDeleteTextures;
	_glDepthFunc = glDepthFunc;
	_glDepthMask = glDepthMask;
	_glDepthRangef = glDepthRangef;
	_glDetachShader = glDetachShader;
	_glDisable = glDisable;
	_glDisableVertexAttribArray = glDisableVertexAttribArray;
	_glDrawArrays = glDrawArrays;
	_glDrawElements = glDrawElements;
	_glEnable = glEnable;
	_glEnableVertexAttribArray = glEnableVertexAttribArray;
	_glFinish = glFinish;
	_glFlush = glFlush;
	_glFramebufferRenderbuffer = glFramebufferRenderbuffer;
	_glFramebufferTexture2D = glFramebufferTexture2D;
	_glFrontFace = glFrontFace;
	_glGenBuffers = glGenBuffers;
	_glGenerateMipmap = glGenerateMipmap;
	_glGenFramebuffers = glGenFramebuffers;
	_glGenRenderbuffers = glGenRenderbuffers;
	_glGenTextures = glGenTextures;
	_glGetActiveAttrib = glGetActiveAttrib;
	_glGetActiveUniform = glGetActiveUniform;
	_glGetAttachedShaders = glGetAttachedShaders;
	_glGetAttribLocation = glGetAttribLocation;
	_glGetBooleanv = glGetBooleanv;
	_glGetBufferParameteriv = glGetBufferParameteriv;
	_glGetError = glGetError;
	_glGetFloatv = glGetFloatv;
	_glGetFramebufferAttachmentParameteriv = glGetFramebufferAttachmentParameteriv;
	_glGetIntegerv = glGetIntegerv;
	_glGetProgramiv = glGetProgramiv;
	_glGetProgramInfoLog = glGetProgramInfoLog;
	_glGetRenderbufferParameteriv = glGetRenderbufferParameteriv;
	_glGetShaderiv = glGetShaderiv;
	_glGetShaderInfoLog = glGetShaderInfoLog;
	_glGetShaderPrecisionFormat = glGetShaderPrecisionFormat;
	_glGetShaderSource = glGetShaderSource;
	_glGetString = glGetString;
	_glGetTexParameterfv = glGetTexParameterfv;
	_glGetTexParameteriv = glGetTexParameteriv;
	_glGetUniformfv = glGetUniformfv;
	_glGetUniformiv = glGetUniformiv;
	_glGetUniformLocation = glGetUniformLocation;
	_glGetVertexAttribfv = glGetVertexAttribfv;
	_glGetVertexAttribiv = glGetVertexAttribiv;
	_glGetVertexAttribPointerv = glGetVertexAttribPointerv;
	_glHint = glHint;
	_glIsBuffer = glIsBuffer;
	_glIsEnabled = glIsEnabled;
	_glIsFramebuffer = glIsFramebuffer;
	_glIsProgram = glIsProgram;
	_glIsRenderbuffer = glIsRenderbuffer;
	_glIsShader = glIsShader;
	_glIsTexture = glIsTexture;
	_glLineWidth = glLineWidth;
	_glLinkProgram = glLinkProgram;
	_glPixelStorei = glPixelStorei;
	_glPolygonOffset = glPolygonOffset;
	_glReadPixels = glReadPixels;
	_glReleaseShaderCompiler = glReleaseShaderCompiler;
	_glRenderbufferStorage = glRenderbufferStorage;
	_glSampleCoverage = glSampleCoverage;
	_glScissor = glScissor;
	_glShaderBinary = glShaderBinary;
	_glShaderSource = glShaderSource;
	_glStencilFunc = glStencilFunc;
	_glStencilFuncSeparate = glStencilFuncSeparate;
	_glStencilMask = glStencilMask;
	_glStencilMaskSeparate = glStencilMaskSeparate;
	_glStencilOp = glStencilOp;
	_glStencilOpSeparate = glStencilOpSeparate;
	_glTexImage2D = glTexImage2D;
	_glTexParameterf = glTexParameterf;
	_glTexParameterfv = glTexParameterfv;
	_glTexParameteri = glTexParameteri;
	_glTexParameteriv = glTexParameteriv;
	_glTexSubImage2D = glTexSubImage2D;
	_glUniform1f = glUniform1f;
	_glUniform1fv = glUniform1fv;
	_glUniform1i = glUniform1i;
	_glUniform1iv = glUniform1iv;
	_glUniform2f = glUniform2f;
	_glUniform2fv = glUniform2fv;
	_glUniform2i = glUniform2i;
	_glUniform2iv = glUniform2iv;
	_glUniform3f = glUniform3f;
	_glUniform3fv = glUniform3fv;
	_glUniform3i = glUniform3i;
	_glUniform3iv = glUniform3iv;
	_glUniform4f = glUniform4f;
	_glUniform4fv = glUniform4fv;
	_glUniform4i = glUniform4i;
	_glUniform4iv = glUniform4iv;
	_glUniformMatrix2fv = glUniformMatrix2fv;
	_glUniformMatrix3fv = glUniformMatrix3fv;
	_glUniformMatrix4fv = glUniformMatrix4fv;
	_glUseProgram = glUseProgram;
	_glValidateProgram = glValidateProgram;
	_glVertexAttrib1f = glVertexAttrib1f;
	_glVertexAttrib1fv = glVertexAttrib1fv;
	_glVertexAttrib2f = glVertexAttrib2f;
	_glVertexAttrib2fv = glVertexAttrib2fv;
	_glVertexAttrib3f = glVertexAttrib3f;
	_glVertexAttrib3fv = glVertexAttrib3fv;
	_glVertexAttrib4f = glVertexAttrib4f;
	_glVertexAttrib4fv = glVertexAttrib4fv;
	_glVertexAttribPointer = glVertexAttribPointer;
	_glViewport = glViewport;

	/* GL_ES_VERSION_3_0 */
	_glReadBuffer = glReadBuffer;
	_glDrawRangeElements = glDrawRangeElements;
	_glTexImage3D = glTexImage3D;
	_glTexSubImage3D = glTexSubImage3D;
	_glCopyTexSubImage3D = glCopyTexSubImage3D;
	_glCompressedTexImage3D = glCompressedTexImage3D;
	_glCompressedTexSubImage3D = glCompressedTexSubImage3D;
	_glGenQueries = glGenQueries;
	_glDeleteQueries = glDeleteQueries;
	_glIsQuery = glIsQuery;
	_glBeginQuery = glBeginQuery;
	_glEndQuery = glEndQuery;
	_glGetQueryiv = glGetQueryiv;
	_glGetQueryObjectuiv = glGetQueryObjectuiv;
	_glUnmapBuffer = glUnmapBuffer;
	_glGetBufferPointerv = glGetBufferPointerv;
	_glDrawBuffers = glDrawBuffers;
	_glUniformMatrix2x3fv = glUniformMatrix2x3fv;
	_glUniformMatrix3x2fv = glUniformMatrix3x2fv;
	_glUniformMatrix2x4fv = glUniformMatrix2x4fv;
	_glUniformMatrix4x2fv = glUniformMatrix4x2fv;
	_glUniformMatrix3x4fv = glUniformMatrix3x4fv;
	_glUniformMatrix4x3fv = glUniformMatrix4x3fv;
	_glBlitFramebuffer = glBlitFramebuffer;
	_glRenderbufferStorageMultisample = glRenderbufferStorageMultisample;
	_glFramebufferTextureLayer = glFramebufferTextureLayer;
	_glMapBufferRange = glMapBufferRange;
	_glFlushMappedBufferRange = glFlushMappedBufferRange;
	_glBindVertexArray = glBindVertexArray;
	_glDeleteVertexArrays = glDeleteVertexArrays;
	_glGenVertexArrays = glGenVertexArrays;
	_glIsVertexArray = glIsVertexArray;
	_glGetIntegeri_v = glGetIntegeri_v;
	_glBeginTransformFeedback = glBeginTransformFeedback;
	_glEndTransformFeedback = glEndTransformFeedback;
	_glBindBufferRange = glBindBufferRange;
	_glBindBufferBase = glBindBufferBase;
	_glTransformFeedbackVaryings = glTransformFeedbackVaryings;
	_glGetTransformFeedbackVarying = glGetTransformFeedbackVarying;
	_glVertexAttribIPointer = glVertexAttribIPointer;
	_glGetVertexAttribIiv = glGetVertexAttribIiv;
	_glGetVertexAttribIuiv = glGetVertexAttribIuiv;
	_glVertexAttribI4i = glVertexAttribI4i;
	_glVertexAttribI4ui = glVertexAttribI4ui;
	_glVertexAttribI4iv = glVertexAttribI4iv;
	_glVertexAttribI4uiv = glVertexAttribI4uiv;
	_glGetUniformuiv = glGetUniformuiv;
	_glGetFragDataLocation = glGetFragDataLocation;
	_glUniform1ui = glUniform1ui;
	_glUniform2ui = glUniform2ui;
	_glUniform3ui = glUniform3ui;
	_glUniform4ui = glUniform4ui;
	_glUniform1uiv = glUniform1uiv;
	_glUniform2uiv = glUniform2uiv;
	_glUniform3uiv = glUniform3uiv;
	_glUniform4uiv = glUniform4uiv;
	_glClearBufferiv = glClearBufferiv;
	_glClearBufferuiv = glClearBufferuiv;
	_glClearBufferfv = glClearBufferfv;
	_glClearBufferfi = glClearBufferfi;
	_glGetStringi = glGetStringi;
	_glCopyBufferSubData = glCopyBufferSubData;
	_glGetUniformIndices = glGetUniformIndices;
	_glGetActiveUniformsiv = glGetActiveUniformsiv;
	_glGetUniformBlockIndex = glGetUniformBlockIndex;
	_glGetActiveUniformBlockiv = glGetActiveUniformBlockiv;
	_glGetActiveUniformBlockName = glGetActiveUniformBlockName;
	_glUniformBlockBinding = glUniformBlockBinding;
	_glDrawArraysInstanced = glDrawArraysInstanced;
	_glDrawElementsInstanced = glDrawElementsInstanced;
	_glFenceSync = glFenceSync;
	_glIsSync = glIsSync;
	_glDeleteSync = glDeleteSync;
	_glClientWaitSync = glClientWaitSync;
	_glWaitSync = glWaitSync;
	_glGetInteger64v = glGetInteger64v;
	_glGetSynciv = glGetSynciv;
	_glGetInteger64i_v = glGetInteger64i_v;
	_glGetBufferParameteri64v = glGetBufferParameteri64v;
	_glGenSamplers = glGenSamplers;
	_glDeleteSamplers = glDeleteSamplers;
	_glIsSampler = glIsSampler;
	_glBindSampler = glBindSampler;
	_glSamplerParameteri = glSamplerParameteri;
	_glSamplerParameteriv = glSamplerParameteriv;
	_glSamplerParameterf = glSamplerParameterf;
	_glSamplerParameterfv = glSamplerParameterfv;
	_glGetSamplerParameteriv = glGetSamplerParameteriv;
	_glGetSamplerParameterfv = glGetSamplerParameterfv;
	_glVertexAttribDivisor = glVertexAttribDivisor;
	_glBindTransformFeedback = glBindTransformFeedback;
	_glDeleteTransformFeedbacks = glDeleteTransformFeedbacks;
	_glGenTransformFeedbacks = glGenTransformFeedbacks;
	_glIsTransformFeedback = glIsTransformFeedback;
	_glPauseTransformFeedback = glPauseTransformFeedback;
	_glResumeTransformFeedback = glResumeTransformFeedback;
	_glGetProgramBinary = glGetProgramBinary;
	_glProgramBinary = glProgramBinary;
	_glProgramParameteri = glProgramParameteri;
	_glInvalidateFramebuffer = glInvalidateFramebuffer;
	_glInvalidateSubFramebuffer = glInvalidateSubFramebuffer;
	_glTexStorage2D = glTexStorage2D;
	_glTexStorage3D = glTexStorage3D;
	_glGetInternalformativ = glGetInternalformativ;

#ifdef GL_AMD_performance_monitor
	_glGetPerfMonitorGroupsAMD = (PFNGLGETPERFMONITORGROUPSAMD)GPA(glGetPerfMonitorGroupsAMD);
	_glGetPerfMonitorCountersAMD = (PFNGLGETPERFMONITORCOUNTERSAMD)GPA(glGetPerfMonitorCountersAMD);
	_glGetPerfMonitorGroupStringAMD = (PFNGLGETPERFMONITORGROUPSTRINGAMD)GPA(glGetPerfMonitorGroupStringAMD);
	_glGetPerfMonitorCounterStringAMD = (PFNGLGETPERFMONITORCOUNTERSTRINGAMD)GPA(glGetPerfMonitorCounterStringAMD);
	_glGetPerfMonitorCounterInfoAMD = (PFNGLGETPERFMONITORCOUNTERINFOAMD)GPA(glGetPerfMonitorCounterInfoAMD);
	_glGenPerfMonitorsAMD = (PFNGLGENPERFMONITORSAMD)GPA(glGenPerfMonitorsAMD);
	_glDeletePerfMonitorsAMD = (PFNGLDELETEPERFMONITORSAMD)GPA(glDeletePerfMonitorsAMD);
	_glSelectPerfMonitorCountersAMD = (PFNGLSELECTPERFMONITORCOUNTERSAMD)GPA(glSelectPerfMonitorCountersAMD);
	_glBeginPerfMonitorAMD = (PFNGLBEGINPERFMONITORAMD)GPA(glBeginPerfMonitorAMD);
	_glEndPerfMonitorAMD = (PFNGLENDPERFMONITORAMD)GPA(glEndPerfMonitorAMD);
	_glGetPerfMonitorCounterDataAMD = (PFNGLGETPERFMONITORCOUNTERDATAAMD)GPA(glGetPerfMonitorCounterDataAMD);
#endif

#ifdef GL_ANGLE_framebuffer_blit
	_glBlitFramebufferANGLE = (PFNGLBLITFRAMEBUFFERANGLE)GPA(glBlitFramebufferANGLE);
#endif

#ifdef GL_ANGLE_framebuffer_multisample
	_glRenderbufferStorageMultisampleANGLE = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEANGLE)GPA(glRenderbufferStorageMultisampleANGLE);
#endif

#ifdef GL_ANGLE_instanced_arrays
	_glDrawArraysInstancedANGLE = (PFNGLDRAWARRAYSINSTANCEDANGLE)GPA(glDrawArraysInstancedANGLE);
	_glDrawElementsInstancedANGLE = (PFNGLDRAWELEMENTSINSTANCEDANGLE)GPA(glDrawElementsInstancedANGLE);
	_glVertexAttribDivisorANGLE = (PFNGLVERTEXATTRIBDIVISORANGLE)GPA(glVertexAttribDivisorANGLE);
#endif

#ifdef GL_ANGLE_translated_shader_source
	_glGetTranslatedShaderSourceANGLE = (PFNGLGETTRANSLATEDSHADERSOURCEANGLE)GPA(glGetTranslatedShaderSourceANGLE);
#endif

#ifdef GL_APPLE_copy_texture_levels
	_glCopyTextureLevelsAPPLE = (PFNGLCOPYTEXTURELEVELSAPPLE)GPA(glCopyTextureLevelsAPPLE);
#endif

#ifdef GL_APPLE_framebuffer_multisample
	_glRenderbufferStorageMultisampleAPPLE = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEAPPLE)GPA(glRenderbufferStorageMultisampleAPPLE);
	_glResolveMultisampleFramebufferAPPLE = (PFNGLRESOLVEMULTISAMPLEFRAMEBUFFERAPPLE)GPA(glResolveMultisampleFramebufferAPPLE);
#endif

#ifdef GL_APPLE_sync
	_glFenceSyncAPPLE = (PFNGLFENCESYNCAPPLE)GPA(glFenceSyncAPPLE);
	_glIsSyncAPPLE = (PFNGLISSYNCAPPLE)GPA(glIsSyncAPPLE);
	_glDeleteSyncAPPLE = (PFNGLDELETESYNCAPPLE)GPA(glDeleteSyncAPPLE);
	_glClientWaitSyncAPPLE = (PFNGLCLIENTWAITSYNCAPPLE)GPA(glClientWaitSyncAPPLE);
	_glWaitSyncAPPLE = (PFNGLWAITSYNCAPPLE)GPA(glWaitSyncAPPLE);
	_glGetInteger64vAPPLE = (PFNGLGETINTEGER64VAPPLE)GPA(glGetInteger64vAPPLE);
	_glGetSyncivAPPLE = (PFNGLGETSYNCIVAPPLE)GPA(glGetSyncivAPPLE);
#endif

#ifdef GL_EXT_base_instance
	_glDrawArraysInstancedBaseInstanceEXT = (PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEEXT)GPA(glDrawArraysInstancedBaseInstanceEXT);
	_glDrawElementsInstancedBaseInstanceEXT = (PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEEXT)GPA(glDrawElementsInstancedBaseInstanceEXT);
	_glDrawElementsInstancedBaseVertexBaseInstanceEXT = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEEXT)GPA(glDrawElementsInstancedBaseVertexBaseInstanceEXT);
#endif

#ifdef GL_EXT_blend_func_extended
	_glBindFragDataLocationIndexedEXT = (PFNGLBINDFRAGDATALOCATIONINDEXEDEXT)GPA(glBindFragDataLocationIndexedEXT);
	_glBindFragDataLocationEXT = (PFNGLBINDFRAGDATALOCATIONEXT)GPA(glBindFragDataLocationEXT);
	_glGetProgramResourceLocationIndexEXT = (PFNGLGETPROGRAMRESOURCELOCATIONINDEXEXT)GPA(glGetProgramResourceLocationIndexEXT);
	_glGetFragDataIndexEXT = (PFNGLGETFRAGDATAINDEXEXT)GPA(glGetFragDataIndexEXT);
#endif

#ifdef GL_EXT_buffer_storage
	_glBufferStorageEXT = (PFNGLBUFFERSTORAGEEXT)GPA(glBufferStorageEXT);
#endif

#ifdef GL_EXT_clear_texture
	_glClearTexImageEXT = (PFNGLCLEARTEXIMAGEEXT)GPA(glClearTexImageEXT);
	_glClearTexSubImageEXT = (PFNGLCLEARTEXSUBIMAGEEXT)GPA(glClearTexSubImageEXT);
#endif

#ifdef GL_EXT_copy_image
	_glCopyImageSubDataEXT = (PFNGLCOPYIMAGESUBDATAEXT)GPA(glCopyImageSubDataEXT);
#endif

#ifdef GL_EXT_debug_label
	_glLabelObjectEXT = (PFNGLLABELOBJECTEXT)GPA(glLabelObjectEXT);
	_glGetObjectLabelEXT = (PFNGLGETOBJECTLABELEXT)GPA(glGetObjectLabelEXT);
#endif

#ifdef GL_EXT_debug_marker
	_glInsertEventMarkerEXT = (PFNGLINSERTEVENTMARKEREXT)GPA(glInsertEventMarkerEXT);
	_glPushGroupMarkerEXT = (PFNGLPUSHGROUPMARKEREXT)GPA(glPushGroupMarkerEXT);
	_glPopGroupMarkerEXT = (PFNGLPOPGROUPMARKEREXT)GPA(glPopGroupMarkerEXT);
#endif

#ifdef GL_EXT_discard_framebuffer
	_glDiscardFramebufferEXT = (PFNGLDISCARDFRAMEBUFFEREXT)GPA(glDiscardFramebufferEXT);
#endif

#ifdef GL_EXT_disjoint_timer_query
	_glGenQueriesEXT = (PFNGLGENQUERIESEXT)GPA(glGenQueriesEXT);
	_glDeleteQueriesEXT = (PFNGLDELETEQUERIESEXT)GPA(glDeleteQueriesEXT);
	_glIsQueryEXT = (PFNGLISQUERYEXT)GPA(glIsQueryEXT);
	_glBeginQueryEXT = (PFNGLBEGINQUERYEXT)GPA(glBeginQueryEXT);
	_glEndQueryEXT = (PFNGLENDQUERYEXT)GPA(glEndQueryEXT);
	_glQueryCounterEXT = (PFNGLQUERYCOUNTEREXT)GPA(glQueryCounterEXT);
	_glGetQueryivEXT = (PFNGLGETQUERYIVEXT)GPA(glGetQueryivEXT);
	_glGetQueryObjectivEXT = (PFNGLGETQUERYOBJECTIVEXT)GPA(glGetQueryObjectivEXT);
	_glGetQueryObjectuivEXT = (PFNGLGETQUERYOBJECTUIVEXT)GPA(glGetQueryObjectuivEXT);
	_glGetQueryObjecti64vEXT = (PFNGLGETQUERYOBJECTI64VEXT)GPA(glGetQueryObjecti64vEXT);
	_glGetQueryObjectui64vEXT = (PFNGLGETQUERYOBJECTUI64VEXT)GPA(glGetQueryObjectui64vEXT);
#endif

#ifdef GL_EXT_draw_buffers
	_glDrawBuffersEXT = (PFNGLDRAWBUFFERSEXT)GPA(glDrawBuffersEXT);
#endif

#ifdef GL_EXT_draw_buffers_indexed
	_glEnableiEXT = (PFNGLENABLEIEXT)GPA(glEnableiEXT);
	_glDisableiEXT = (PFNGLDISABLEIEXT)GPA(glDisableiEXT);
	_glBlendEquationiEXT = (PFNGLBLENDEQUATIONIEXT)GPA(glBlendEquationiEXT);
	_glBlendEquationSeparateiEXT = (PFNGLBLENDEQUATIONSEPARATEIEXT)GPA(glBlendEquationSeparateiEXT);
	_glBlendFunciEXT = (PFNGLBLENDFUNCIEXT)GPA(glBlendFunciEXT);
	_glBlendFuncSeparateiEXT = (PFNGLBLENDFUNCSEPARATEIEXT)GPA(glBlendFuncSeparateiEXT);
	_glColorMaskiEXT = (PFNGLCOLORMASKIEXT)GPA(glColorMaskiEXT);
	_glIsEnablediEXT = (PFNGLISENABLEDIEXT)GPA(glIsEnablediEXT);
#endif

#ifdef GL_EXT_draw_elements_base_vertex
	_glDrawElementsBaseVertexEXT = (PFNGLDRAWELEMENTSBASEVERTEXEXT)GPA(glDrawElementsBaseVertexEXT);
	_glDrawRangeElementsBaseVertexEXT = (PFNGLDRAWRANGEELEMENTSBASEVERTEXEXT)GPA(glDrawRangeElementsBaseVertexEXT);
	_glDrawElementsInstancedBaseVertexEXT = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXEXT)GPA(glDrawElementsInstancedBaseVertexEXT);
	_glMultiDrawElementsBaseVertexEXT = (PFNGLMULTIDRAWELEMENTSBASEVERTEXEXT)GPA(glMultiDrawElementsBaseVertexEXT);
#endif

#ifdef GL_EXT_draw_instanced
	_glDrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXT)GPA(glDrawArraysInstancedEXT);
	_glDrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXT)GPA(glDrawElementsInstancedEXT);
#endif

#ifdef GL_EXT_draw_transform_feedback
	_glDrawTransformFeedbackEXT = (PFNGLDRAWTRANSFORMFEEDBACKEXT)GPA(glDrawTransformFeedbackEXT);
	_glDrawTransformFeedbackInstancedEXT = (PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDEXT)GPA(glDrawTransformFeedbackInstancedEXT);
#endif

#ifdef GL_EXT_geometry_shader
	_glFramebufferTextureEXT = (PFNGLFRAMEBUFFERTEXTUREEXT)GPA(glFramebufferTextureEXT);
#endif

#ifdef GL_EXT_instanced_arrays
	_glDrawArraysInstancedEXT = (PFNGLDRAWARRAYSINSTANCEDEXT)GPA(glDrawArraysInstancedEXT);
	_glDrawElementsInstancedEXT = (PFNGLDRAWELEMENTSINSTANCEDEXT)GPA(glDrawElementsInstancedEXT);
	_glVertexAttribDivisorEXT = (PFNGLVERTEXATTRIBDIVISOREXT)GPA(glVertexAttribDivisorEXT);
#endif

#ifdef GL_EXT_map_buffer_range
	_glMapBufferRangeEXT = (PFNGLMAPBUFFERRANGEEXT)GPA(glMapBufferRangeEXT);
	_glFlushMappedBufferRangeEXT = (PFNGLFLUSHMAPPEDBUFFERRANGEEXT)GPA(glFlushMappedBufferRangeEXT);
#endif

#ifdef GL_EXT_multi_draw_arrays
	_glMultiDrawArraysEXT = (PFNGLMULTIDRAWARRAYSEXT)GPA(glMultiDrawArraysEXT);
	_glMultiDrawElementsEXT = (PFNGLMULTIDRAWELEMENTSEXT)GPA(glMultiDrawElementsEXT);
#endif

#ifdef GL_EXT_multi_draw_indirect
	_glMultiDrawArraysIndirectEXT = (PFNGLMULTIDRAWARRAYSINDIRECTEXT)GPA(glMultiDrawArraysIndirectEXT);
	_glMultiDrawElementsIndirectEXT = (PFNGLMULTIDRAWELEMENTSINDIRECTEXT)GPA(glMultiDrawElementsIndirectEXT);
#endif

#ifdef GL_EXT_multisampled_render_to_texture
	_glRenderbufferStorageMultisampleEXT = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXT)GPA(glRenderbufferStorageMultisampleEXT);
	_glFramebufferTexture2DMultisampleEXT = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXT)GPA(glFramebufferTexture2DMultisampleEXT);
#endif

#ifdef GL_EXT_multiview_draw_buffers
	_glReadBufferIndexedEXT = (PFNGLREADBUFFERINDEXEDEXT)GPA(glReadBufferIndexedEXT);
	_glDrawBuffersIndexedEXT = (PFNGLDRAWBUFFERSINDEXEDEXT)GPA(glDrawBuffersIndexedEXT);
	_glGetIntegeri_vEXT = (PFNGLGETINTEGERI_VEXT)GPA(glGetIntegeri_vEXT);
#endif

#ifdef GL_EXT_occlusion_query_boolean
	_glGenQueriesEXT = (PFNGLGENQUERIESEXT)GPA(glGenQueriesEXT);
	_glDeleteQueriesEXT = (PFNGLDELETEQUERIESEXT)GPA(glDeleteQueriesEXT);
	_glIsQueryEXT = (PFNGLISQUERYEXT)GPA(glIsQueryEXT);
	_glBeginQueryEXT = (PFNGLBEGINQUERYEXT)GPA(glBeginQueryEXT);
	_glEndQueryEXT = (PFNGLENDQUERYEXT)GPA(glEndQueryEXT);
	_glGetQueryivEXT = (PFNGLGETQUERYIVEXT)GPA(glGetQueryivEXT);
	_glGetQueryObjectuivEXT = (PFNGLGETQUERYOBJECTUIVEXT)GPA(glGetQueryObjectuivEXT);
#endif

#ifdef GL_EXT_polygon_offset_clamp
	_glPolygonOffsetClampEXT = (PFNGLPOLYGONOFFSETCLAMPEXT)GPA(glPolygonOffsetClampEXT);
#endif

#ifdef GL_EXT_primitive_bounding_box
	_glPrimitiveBoundingBoxEXT = (PFNGLPRIMITIVEBOUNDINGBOXEXT)GPA(glPrimitiveBoundingBoxEXT);
#endif

#ifdef GL_EXT_raster_multisample
	_glRasterSamplesEXT = (PFNGLRASTERSAMPLESEXT)GPA(glRasterSamplesEXT);
#endif

#ifdef GL_EXT_robustness
	_glGetGraphicsResetStatusEXT = (PFNGLGETGRAPHICSRESETSTATUSEXT)GPA(glGetGraphicsResetStatusEXT);
	_glReadnPixelsEXT = (PFNGLREADNPIXELSEXT)GPA(glReadnPixelsEXT);
	_glGetnUniformfvEXT = (PFNGLGETNUNIFORMFVEXT)GPA(glGetnUniformfvEXT);
	_glGetnUniformivEXT = (PFNGLGETNUNIFORMIVEXT)GPA(glGetnUniformivEXT);
#endif

#ifdef GL_EXT_separate_shader_objects
	_glActiveShaderProgramEXT = (PFNGLACTIVESHADERPROGRAMEXT)GPA(glActiveShaderProgramEXT);
	_glBindProgramPipelineEXT = (PFNGLBINDPROGRAMPIPELINEEXT)GPA(glBindProgramPipelineEXT);
	_glCreateShaderProgramvEXT = (PFNGLCREATESHADERPROGRAMVEXT)GPA(glCreateShaderProgramvEXT);
	_glDeleteProgramPipelinesEXT = (PFNGLDELETEPROGRAMPIPELINESEXT)GPA(glDeleteProgramPipelinesEXT);
	_glGenProgramPipelinesEXT = (PFNGLGENPROGRAMPIPELINESEXT)GPA(glGenProgramPipelinesEXT);
	_glGetProgramPipelineInfoLogEXT = (PFNGLGETPROGRAMPIPELINEINFOLOGEXT)GPA(glGetProgramPipelineInfoLogEXT);
	_glGetProgramPipelineivEXT = (PFNGLGETPROGRAMPIPELINEIVEXT)GPA(glGetProgramPipelineivEXT);
	_glIsProgramPipelineEXT = (PFNGLISPROGRAMPIPELINEEXT)GPA(glIsProgramPipelineEXT);
	_glProgramParameteriEXT = (PFNGLPROGRAMPARAMETERIEXT)GPA(glProgramParameteriEXT);
	_glProgramUniform1fEXT = (PFNGLPROGRAMUNIFORM1FEXT)GPA(glProgramUniform1fEXT);
	_glProgramUniform1fvEXT = (PFNGLPROGRAMUNIFORM1FVEXT)GPA(glProgramUniform1fvEXT);
	_glProgramUniform1iEXT = (PFNGLPROGRAMUNIFORM1IEXT)GPA(glProgramUniform1iEXT);
	_glProgramUniform1ivEXT = (PFNGLPROGRAMUNIFORM1IVEXT)GPA(glProgramUniform1ivEXT);
	_glProgramUniform2fEXT = (PFNGLPROGRAMUNIFORM2FEXT)GPA(glProgramUniform2fEXT);
	_glProgramUniform2fvEXT = (PFNGLPROGRAMUNIFORM2FVEXT)GPA(glProgramUniform2fvEXT);
	_glProgramUniform2iEXT = (PFNGLPROGRAMUNIFORM2IEXT)GPA(glProgramUniform2iEXT);
	_glProgramUniform2ivEXT = (PFNGLPROGRAMUNIFORM2IVEXT)GPA(glProgramUniform2ivEXT);
	_glProgramUniform3fEXT = (PFNGLPROGRAMUNIFORM3FEXT)GPA(glProgramUniform3fEXT);
	_glProgramUniform3fvEXT = (PFNGLPROGRAMUNIFORM3FVEXT)GPA(glProgramUniform3fvEXT);
	_glProgramUniform3iEXT = (PFNGLPROGRAMUNIFORM3IEXT)GPA(glProgramUniform3iEXT);
	_glProgramUniform3ivEXT = (PFNGLPROGRAMUNIFORM3IVEXT)GPA(glProgramUniform3ivEXT);
	_glProgramUniform4fEXT = (PFNGLPROGRAMUNIFORM4FEXT)GPA(glProgramUniform4fEXT);
	_glProgramUniform4fvEXT = (PFNGLPROGRAMUNIFORM4FVEXT)GPA(glProgramUniform4fvEXT);
	_glProgramUniform4iEXT = (PFNGLPROGRAMUNIFORM4IEXT)GPA(glProgramUniform4iEXT);
	_glProgramUniform4ivEXT = (PFNGLPROGRAMUNIFORM4IVEXT)GPA(glProgramUniform4ivEXT);
	_glProgramUniformMatrix2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2FVEXT)GPA(glProgramUniformMatrix2fvEXT);
	_glProgramUniformMatrix3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3FVEXT)GPA(glProgramUniformMatrix3fvEXT);
	_glProgramUniformMatrix4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4FVEXT)GPA(glProgramUniformMatrix4fvEXT);
	_glUseProgramStagesEXT = (PFNGLUSEPROGRAMSTAGESEXT)GPA(glUseProgramStagesEXT);
	_glValidateProgramPipelineEXT = (PFNGLVALIDATEPROGRAMPIPELINEEXT)GPA(glValidateProgramPipelineEXT);
	_glProgramUniform1uiEXT = (PFNGLPROGRAMUNIFORM1UIEXT)GPA(glProgramUniform1uiEXT);
	_glProgramUniform2uiEXT = (PFNGLPROGRAMUNIFORM2UIEXT)GPA(glProgramUniform2uiEXT);
	_glProgramUniform3uiEXT = (PFNGLPROGRAMUNIFORM3UIEXT)GPA(glProgramUniform3uiEXT);
	_glProgramUniform4uiEXT = (PFNGLPROGRAMUNIFORM4UIEXT)GPA(glProgramUniform4uiEXT);
	_glProgramUniform1uivEXT = (PFNGLPROGRAMUNIFORM1UIVEXT)GPA(glProgramUniform1uivEXT);
	_glProgramUniform2uivEXT = (PFNGLPROGRAMUNIFORM2UIVEXT)GPA(glProgramUniform2uivEXT);
	_glProgramUniform3uivEXT = (PFNGLPROGRAMUNIFORM3UIVEXT)GPA(glProgramUniform3uivEXT);
	_glProgramUniform4uivEXT = (PFNGLPROGRAMUNIFORM4UIVEXT)GPA(glProgramUniform4uivEXT);
	_glProgramUniformMatrix4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4FVEXT)GPA(glProgramUniformMatrix4fvEXT);
	_glProgramUniformMatrix2x3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X3FVEXT)GPA(glProgramUniformMatrix2x3fvEXT);
	_glProgramUniformMatrix3x2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X2FVEXT)GPA(glProgramUniformMatrix3x2fvEXT);
	_glProgramUniformMatrix2x4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX2X4FVEXT)GPA(glProgramUniformMatrix2x4fvEXT);
	_glProgramUniformMatrix4x2fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X2FVEXT)GPA(glProgramUniformMatrix4x2fvEXT);
	_glProgramUniformMatrix3x4fvEXT = (PFNGLPROGRAMUNIFORMMATRIX3X4FVEXT)GPA(glProgramUniformMatrix3x4fvEXT);
	_glProgramUniformMatrix4x3fvEXT = (PFNGLPROGRAMUNIFORMMATRIX4X3FVEXT)GPA(glProgramUniformMatrix4x3fvEXT);
#endif

#ifdef GL_EXT_shader_pixel_local_storage2
	_glFramebufferPixelLocalStorageSizeEXT = (PFNGLFRAMEBUFFERPIXELLOCALSTORAGESIZEEXT)GPA(glFramebufferPixelLocalStorageSizeEXT);
	_glGetFramebufferPixelLocalStorageSizeEXT = (PFNGLGETFRAMEBUFFERPIXELLOCALSTORAGESIZEEXT)GPA(glGetFramebufferPixelLocalStorageSizeEXT);
	_glClearPixelLocalStorageuiEXT = (PFNGLCLEARPIXELLOCALSTORAGEUIEXT)GPA(glClearPixelLocalStorageuiEXT);
#endif

#ifdef GL_EXT_sparse_texture
	_glTexPageCommitmentEXT = (PFNGLTEXPAGECOMMITMENTEXT)GPA(glTexPageCommitmentEXT);
#endif

#ifdef GL_EXT_tessellation_shader
	_glPatchParameteriEXT = (PFNGLPATCHPARAMETERIEXT)GPA(glPatchParameteriEXT);
#endif

#ifdef GL_EXT_texture_border_clamp
	_glTexParameterIivEXT = (PFNGLTEXPARAMETERIIVEXT)GPA(glTexParameterIivEXT);
	_glTexParameterIuivEXT = (PFNGLTEXPARAMETERIUIVEXT)GPA(glTexParameterIuivEXT);
	_glGetTexParameterIivEXT = (PFNGLGETTEXPARAMETERIIVEXT)GPA(glGetTexParameterIivEXT);
	_glGetTexParameterIuivEXT = (PFNGLGETTEXPARAMETERIUIVEXT)GPA(glGetTexParameterIuivEXT);
	_glSamplerParameterIivEXT = (PFNGLSAMPLERPARAMETERIIVEXT)GPA(glSamplerParameterIivEXT);
	_glSamplerParameterIuivEXT = (PFNGLSAMPLERPARAMETERIUIVEXT)GPA(glSamplerParameterIuivEXT);
	_glGetSamplerParameterIivEXT = (PFNGLGETSAMPLERPARAMETERIIVEXT)GPA(glGetSamplerParameterIivEXT);
	_glGetSamplerParameterIuivEXT = (PFNGLGETSAMPLERPARAMETERIUIVEXT)GPA(glGetSamplerParameterIuivEXT);
#endif

#ifdef GL_EXT_texture_buffer
	_glTexBufferEXT = (PFNGLTEXBUFFEREXT)GPA(glTexBufferEXT);
	_glTexBufferRangeEXT = (PFNGLTEXBUFFERRANGEEXT)GPA(glTexBufferRangeEXT);
#endif

#ifdef GL_EXT_texture_filter_minmax
	_glRasterSamplesEXT = (PFNGLRASTERSAMPLESEXT)GPA(glRasterSamplesEXT);
#endif

#ifdef GL_EXT_texture_storage
	_glTexStorage1DEXT = (PFNGLTEXSTORAGE1DEXT)GPA(glTexStorage1DEXT);
	_glTexStorage2DEXT = (PFNGLTEXSTORAGE2DEXT)GPA(glTexStorage2DEXT);
	_glTexStorage3DEXT = (PFNGLTEXSTORAGE3DEXT)GPA(glTexStorage3DEXT);
	_glTextureStorage1DEXT = (PFNGLTEXTURESTORAGE1DEXT)GPA(glTextureStorage1DEXT);
	_glTextureStorage2DEXT = (PFNGLTEXTURESTORAGE2DEXT)GPA(glTextureStorage2DEXT);
	_glTextureStorage3DEXT = (PFNGLTEXTURESTORAGE3DEXT)GPA(glTextureStorage3DEXT);
#endif

#ifdef GL_EXT_texture_view
	_glTextureViewEXT = (PFNGLTEXTUREVIEWEXT)GPA(glTextureViewEXT);
#endif

#ifdef GL_EXT_window_rectangles
	_glWindowRectanglesEXT = (PFNGLWINDOWRECTANGLESEXT)GPA(glWindowRectanglesEXT);
#endif

#ifdef GL_IMG_bindless_texture
	_glGetTextureHandleIMG = (PFNGLGETTEXTUREHANDLEIMG)GPA(glGetTextureHandleIMG);
	_glGetTextureSamplerHandleIMG = (PFNGLGETTEXTURESAMPLERHANDLEIMG)GPA(glGetTextureSamplerHandleIMG);
	_glUniformHandleui64IMG = (PFNGLUNIFORMHANDLEUI64IMG)GPA(glUniformHandleui64IMG);
	_glUniformHandleui64vIMG = (PFNGLUNIFORMHANDLEUI64VIMG)GPA(glUniformHandleui64vIMG);
	_glProgramUniformHandleui64IMG = (PFNGLPROGRAMUNIFORMHANDLEUI64IMG)GPA(glProgramUniformHandleui64IMG);
	_glProgramUniformHandleui64vIMG = (PFNGLPROGRAMUNIFORMHANDLEUI64VIMG)GPA(glProgramUniformHandleui64vIMG);
#endif

#ifdef GL_IMG_framebuffer_downsample
	_glFramebufferTexture2DDownsampleIMG = (PFNGLFRAMEBUFFERTEXTURE2DDOWNSAMPLEIMG)GPA(glFramebufferTexture2DDownsampleIMG);
	_glFramebufferTextureLayerDownsampleIMG = (PFNGLFRAMEBUFFERTEXTURELAYERDOWNSAMPLEIMG)GPA(glFramebufferTextureLayerDownsampleIMG);
#endif

#ifdef GL_IMG_multisampled_render_to_texture
	_glRenderbufferStorageMultisampleIMG = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG)GPA(glRenderbufferStorageMultisampleIMG);
	_glFramebufferTexture2DMultisampleIMG = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG)GPA(glFramebufferTexture2DMultisampleIMG);
#endif

#ifdef GL_INTEL_framebuffer_CMAA
	_glApplyFramebufferAttachmentCMAAINTEL = (PFNGLAPPLYFRAMEBUFFERATTACHMENTCMAAINTEL)GPA(glApplyFramebufferAttachmentCMAAINTEL);
#endif

#ifdef GL_INTEL_performance_query
	_glBeginPerfQueryINTEL = (PFNGLBEGINPERFQUERYINTEL)GPA(glBeginPerfQueryINTEL);
	_glCreatePerfQueryINTEL = (PFNGLCREATEPERFQUERYINTEL)GPA(glCreatePerfQueryINTEL);
	_glDeletePerfQueryINTEL = (PFNGLDELETEPERFQUERYINTEL)GPA(glDeletePerfQueryINTEL);
	_glEndPerfQueryINTEL = (PFNGLENDPERFQUERYINTEL)GPA(glEndPerfQueryINTEL);
	_glGetFirstPerfQueryIdINTEL = (PFNGLGETFIRSTPERFQUERYIDINTEL)GPA(glGetFirstPerfQueryIdINTEL);
	_glGetNextPerfQueryIdINTEL = (PFNGLGETNEXTPERFQUERYIDINTEL)GPA(glGetNextPerfQueryIdINTEL);
	_glGetPerfCounterInfoINTEL = (PFNGLGETPERFCOUNTERINFOINTEL)GPA(glGetPerfCounterInfoINTEL);
	_glGetPerfQueryDataINTEL = (PFNGLGETPERFQUERYDATAINTEL)GPA(glGetPerfQueryDataINTEL);
	_glGetPerfQueryIdByNameINTEL = (PFNGLGETPERFQUERYIDBYNAMEINTEL)GPA(glGetPerfQueryIdByNameINTEL);
	_glGetPerfQueryInfoINTEL = (PFNGLGETPERFQUERYINFOINTEL)GPA(glGetPerfQueryInfoINTEL);
#endif

#ifdef GL_KHR_blend_equation_advanced
	_glBlendBarrierKHR = (PFNGLBLENDBARRIERKHR)GPA(glBlendBarrierKHR);
#endif

#ifdef GL_KHR_debug
	_glDebugMessageControlKHR = (PFNGLDEBUGMESSAGECONTROLKHR)GPA(glDebugMessageControlKHR);
	_glDebugMessageInsertKHR = (PFNGLDEBUGMESSAGEINSERTKHR)GPA(glDebugMessageInsertKHR);
	_glDebugMessageCallbackKHR = (PFNGLDEBUGMESSAGECALLBACKKHR)GPA(glDebugMessageCallbackKHR);
	_glGetDebugMessageLogKHR = (PFNGLGETDEBUGMESSAGELOGKHR)GPA(glGetDebugMessageLogKHR);
	_glPushDebugGroupKHR = (PFNGLPUSHDEBUGGROUPKHR)GPA(glPushDebugGroupKHR);
	_glPopDebugGroupKHR = (PFNGLPOPDEBUGGROUPKHR)GPA(glPopDebugGroupKHR);
	_glObjectLabelKHR = (PFNGLOBJECTLABELKHR)GPA(glObjectLabelKHR);
	_glGetObjectLabelKHR = (PFNGLGETOBJECTLABELKHR)GPA(glGetObjectLabelKHR);
	_glObjectPtrLabelKHR = (PFNGLOBJECTPTRLABELKHR)GPA(glObjectPtrLabelKHR);
	_glGetObjectPtrLabelKHR = (PFNGLGETOBJECTPTRLABELKHR)GPA(glGetObjectPtrLabelKHR);
	_glGetPointervKHR = (PFNGLGETPOINTERVKHR)GPA(glGetPointervKHR);
#endif

#ifdef GL_KHR_robustness
	_glGetGraphicsResetStatusKHR = (PFNGLGETGRAPHICSRESETSTATUSKHR)GPA(glGetGraphicsResetStatusKHR);
	_glReadnPixelsKHR = (PFNGLREADNPIXELSKHR)GPA(glReadnPixelsKHR);
	_glGetnUniformfvKHR = (PFNGLGETNUNIFORMFVKHR)GPA(glGetnUniformfvKHR);
	_glGetnUniformivKHR = (PFNGLGETNUNIFORMIVKHR)GPA(glGetnUniformivKHR);
	_glGetnUniformuivKHR = (PFNGLGETNUNIFORMUIVKHR)GPA(glGetnUniformuivKHR);
#endif

#ifdef GL_NV_bindless_texture
	_glGetTextureHandleNV = (PFNGLGETTEXTUREHANDLENV)GPA(glGetTextureHandleNV);
	_glGetTextureSamplerHandleNV = (PFNGLGETTEXTURESAMPLERHANDLENV)GPA(glGetTextureSamplerHandleNV);
	_glMakeTextureHandleResidentNV = (PFNGLMAKETEXTUREHANDLERESIDENTNV)GPA(glMakeTextureHandleResidentNV);
	_glMakeTextureHandleNonResidentNV = (PFNGLMAKETEXTUREHANDLENONRESIDENTNV)GPA(glMakeTextureHandleNonResidentNV);
	_glGetImageHandleNV = (PFNGLGETIMAGEHANDLENV)GPA(glGetImageHandleNV);
	_glMakeImageHandleResidentNV = (PFNGLMAKEIMAGEHANDLERESIDENTNV)GPA(glMakeImageHandleResidentNV);
	_glMakeImageHandleNonResidentNV = (PFNGLMAKEIMAGEHANDLENONRESIDENTNV)GPA(glMakeImageHandleNonResidentNV);
	_glUniformHandleui64NV = (PFNGLUNIFORMHANDLEUI64NV)GPA(glUniformHandleui64NV);
	_glUniformHandleui64vNV = (PFNGLUNIFORMHANDLEUI64VNV)GPA(glUniformHandleui64vNV);
	_glProgramUniformHandleui64NV = (PFNGLPROGRAMUNIFORMHANDLEUI64NV)GPA(glProgramUniformHandleui64NV);
	_glProgramUniformHandleui64vNV = (PFNGLPROGRAMUNIFORMHANDLEUI64VNV)GPA(glProgramUniformHandleui64vNV);
	_glIsTextureHandleResidentNV = (PFNGLISTEXTUREHANDLERESIDENTNV)GPA(glIsTextureHandleResidentNV);
	_glIsImageHandleResidentNV = (PFNGLISIMAGEHANDLERESIDENTNV)GPA(glIsImageHandleResidentNV);
#endif

#ifdef GL_NV_blend_equation_advanced
	_glBlendParameteriNV = (PFNGLBLENDPARAMETERINV)GPA(glBlendParameteriNV);
	_glBlendBarrierNV = (PFNGLBLENDBARRIERNV)GPA(glBlendBarrierNV);
#endif

#ifdef GL_NV_conditional_render
	_glBeginConditionalRenderNV = (PFNGLBEGINCONDITIONALRENDERNV)GPA(glBeginConditionalRenderNV);
	_glEndConditionalRenderNV = (PFNGLENDCONDITIONALRENDERNV)GPA(glEndConditionalRenderNV);
#endif

#ifdef GL_NV_conservative_raster
	_glSubpixelPrecisionBiasNV = (PFNGLSUBPIXELPRECISIONBIASNV)GPA(glSubpixelPrecisionBiasNV);
#endif

#ifdef GL_NV_conservative_raster_pre_snap_triangles
	_glConservativeRasterParameteriNV = (PFNGLCONSERVATIVERASTERPARAMETERINV)GPA(glConservativeRasterParameteriNV);
#endif

#ifdef GL_NV_copy_buffer
	_glCopyBufferSubDataNV = (PFNGLCOPYBUFFERSUBDATANV)GPA(glCopyBufferSubDataNV);
#endif

#ifdef GL_NV_coverage_sample
	_glCoverageMaskNV = (PFNGLCOVERAGEMASKNV)GPA(glCoverageMaskNV);
	_glCoverageOperationNV = (PFNGLCOVERAGEOPERATIONNV)GPA(glCoverageOperationNV);
#endif

#ifdef GL_NV_draw_buffers
	_glDrawBuffersNV = (PFNGLDRAWBUFFERSNV)GPA(glDrawBuffersNV);
#endif

#ifdef GL_NV_draw_instanced
	_glDrawArraysInstancedNV = (PFNGLDRAWARRAYSINSTANCEDNV)GPA(glDrawArraysInstancedNV);
	_glDrawElementsInstancedNV = (PFNGLDRAWELEMENTSINSTANCEDNV)GPA(glDrawElementsInstancedNV);
#endif

#ifdef GL_NV_fence
	_glDeleteFencesNV = (PFNGLDELETEFENCESNV)GPA(glDeleteFencesNV);
	_glGenFencesNV = (PFNGLGENFENCESNV)GPA(glGenFencesNV);
	_glIsFenceNV = (PFNGLISFENCENV)GPA(glIsFenceNV);
	_glTestFenceNV = (PFNGLTESTFENCENV)GPA(glTestFenceNV);
	_glGetFenceivNV = (PFNGLGETFENCEIVNV)GPA(glGetFenceivNV);
	_glFinishFenceNV = (PFNGLFINISHFENCENV)GPA(glFinishFenceNV);
	_glSetFenceNV = (PFNGLSETFENCENV)GPA(glSetFenceNV);
#endif

#ifdef GL_NV_fragment_coverage_to_color
	_glFragmentCoverageColorNV = (PFNGLFRAGMENTCOVERAGECOLORNV)GPA(glFragmentCoverageColorNV);
#endif

#ifdef GL_NV_framebuffer_blit
	_glBlitFramebufferNV = (PFNGLBLITFRAMEBUFFERNV)GPA(glBlitFramebufferNV);
#endif

#ifdef GL_NV_framebuffer_mixed_samples
	_glRasterSamplesEXT = (PFNGLRASTERSAMPLESEXT)GPA(glRasterSamplesEXT);
	_glCoverageModulationTableNV = (PFNGLCOVERAGEMODULATIONTABLENV)GPA(glCoverageModulationTableNV);
	_glGetCoverageModulationTableNV = (PFNGLGETCOVERAGEMODULATIONTABLENV)GPA(glGetCoverageModulationTableNV);
	_glCoverageModulationNV = (PFNGLCOVERAGEMODULATIONNV)GPA(glCoverageModulationNV);
#endif

#ifdef GL_NV_framebuffer_multisample
	_glRenderbufferStorageMultisampleNV = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLENV)GPA(glRenderbufferStorageMultisampleNV);
#endif

#ifdef GL_NV_gpu_shader5
	_glUniform1i64NV = (PFNGLUNIFORM1I64NV)GPA(glUniform1i64NV);
	_glUniform2i64NV = (PFNGLUNIFORM2I64NV)GPA(glUniform2i64NV);
	_glUniform3i64NV = (PFNGLUNIFORM3I64NV)GPA(glUniform3i64NV);
	_glUniform4i64NV = (PFNGLUNIFORM4I64NV)GPA(glUniform4i64NV);
	_glUniform1i64vNV = (PFNGLUNIFORM1I64VNV)GPA(glUniform1i64vNV);
	_glUniform2i64vNV = (PFNGLUNIFORM2I64VNV)GPA(glUniform2i64vNV);
	_glUniform3i64vNV = (PFNGLUNIFORM3I64VNV)GPA(glUniform3i64vNV);
	_glUniform4i64vNV = (PFNGLUNIFORM4I64VNV)GPA(glUniform4i64vNV);
	_glUniform1ui64NV = (PFNGLUNIFORM1UI64NV)GPA(glUniform1ui64NV);
	_glUniform2ui64NV = (PFNGLUNIFORM2UI64NV)GPA(glUniform2ui64NV);
	_glUniform3ui64NV = (PFNGLUNIFORM3UI64NV)GPA(glUniform3ui64NV);
	_glUniform4ui64NV = (PFNGLUNIFORM4UI64NV)GPA(glUniform4ui64NV);
	_glUniform1ui64vNV = (PFNGLUNIFORM1UI64VNV)GPA(glUniform1ui64vNV);
	_glUniform2ui64vNV = (PFNGLUNIFORM2UI64VNV)GPA(glUniform2ui64vNV);
	_glUniform3ui64vNV = (PFNGLUNIFORM3UI64VNV)GPA(glUniform3ui64vNV);
	_glUniform4ui64vNV = (PFNGLUNIFORM4UI64VNV)GPA(glUniform4ui64vNV);
	_glGetUniformi64vNV = (PFNGLGETUNIFORMI64VNV)GPA(glGetUniformi64vNV);
	_glProgramUniform1i64NV = (PFNGLPROGRAMUNIFORM1I64NV)GPA(glProgramUniform1i64NV);
	_glProgramUniform2i64NV = (PFNGLPROGRAMUNIFORM2I64NV)GPA(glProgramUniform2i64NV);
	_glProgramUniform3i64NV = (PFNGLPROGRAMUNIFORM3I64NV)GPA(glProgramUniform3i64NV);
	_glProgramUniform4i64NV = (PFNGLPROGRAMUNIFORM4I64NV)GPA(glProgramUniform4i64NV);
	_glProgramUniform1i64vNV = (PFNGLPROGRAMUNIFORM1I64VNV)GPA(glProgramUniform1i64vNV);
	_glProgramUniform2i64vNV = (PFNGLPROGRAMUNIFORM2I64VNV)GPA(glProgramUniform2i64vNV);
	_glProgramUniform3i64vNV = (PFNGLPROGRAMUNIFORM3I64VNV)GPA(glProgramUniform3i64vNV);
	_glProgramUniform4i64vNV = (PFNGLPROGRAMUNIFORM4I64VNV)GPA(glProgramUniform4i64vNV);
	_glProgramUniform1ui64NV = (PFNGLPROGRAMUNIFORM1UI64NV)GPA(glProgramUniform1ui64NV);
	_glProgramUniform2ui64NV = (PFNGLPROGRAMUNIFORM2UI64NV)GPA(glProgramUniform2ui64NV);
	_glProgramUniform3ui64NV = (PFNGLPROGRAMUNIFORM3UI64NV)GPA(glProgramUniform3ui64NV);
	_glProgramUniform4ui64NV = (PFNGLPROGRAMUNIFORM4UI64NV)GPA(glProgramUniform4ui64NV);
	_glProgramUniform1ui64vNV = (PFNGLPROGRAMUNIFORM1UI64VNV)GPA(glProgramUniform1ui64vNV);
	_glProgramUniform2ui64vNV = (PFNGLPROGRAMUNIFORM2UI64VNV)GPA(glProgramUniform2ui64vNV);
	_glProgramUniform3ui64vNV = (PFNGLPROGRAMUNIFORM3UI64VNV)GPA(glProgramUniform3ui64vNV);
	_glProgramUniform4ui64vNV = (PFNGLPROGRAMUNIFORM4UI64VNV)GPA(glProgramUniform4ui64vNV);
#endif

#ifdef GL_NV_instanced_arrays
	_glVertexAttribDivisorNV = (PFNGLVERTEXATTRIBDIVISORNV)GPA(glVertexAttribDivisorNV);
#endif

#ifdef GL_NV_internalformat_sample_query
	_glGetInternalformatSampleivNV = (PFNGLGETINTERNALFORMATSAMPLEIVNV)GPA(glGetInternalformatSampleivNV);
#endif

#ifdef GL_NV_non_square_matrices
	_glUniformMatrix2x3fvNV = (PFNGLUNIFORMMATRIX2X3FVNV)GPA(glUniformMatrix2x3fvNV);
	_glUniformMatrix3x2fvNV = (PFNGLUNIFORMMATRIX3X2FVNV)GPA(glUniformMatrix3x2fvNV);
	_glUniformMatrix2x4fvNV = (PFNGLUNIFORMMATRIX2X4FVNV)GPA(glUniformMatrix2x4fvNV);
	_glUniformMatrix4x2fvNV = (PFNGLUNIFORMMATRIX4X2FVNV)GPA(glUniformMatrix4x2fvNV);
	_glUniformMatrix3x4fvNV = (PFNGLUNIFORMMATRIX3X4FVNV)GPA(glUniformMatrix3x4fvNV);
	_glUniformMatrix4x3fvNV = (PFNGLUNIFORMMATRIX4X3FVNV)GPA(glUniformMatrix4x3fvNV);
#endif

#ifdef GL_NV_path_rendering
	_glGenPathsNV = (PFNGLGENPATHSNV)GPA(glGenPathsNV);
	_glDeletePathsNV = (PFNGLDELETEPATHSNV)GPA(glDeletePathsNV);
	_glIsPathNV = (PFNGLISPATHNV)GPA(glIsPathNV);
	_glPathCommandsNV = (PFNGLPATHCOMMANDSNV)GPA(glPathCommandsNV);
	_glPathCoordsNV = (PFNGLPATHCOORDSNV)GPA(glPathCoordsNV);
	_glPathSubCommandsNV = (PFNGLPATHSUBCOMMANDSNV)GPA(glPathSubCommandsNV);
	_glPathSubCoordsNV = (PFNGLPATHSUBCOORDSNV)GPA(glPathSubCoordsNV);
	_glPathStringNV = (PFNGLPATHSTRINGNV)GPA(glPathStringNV);
	_glPathGlyphsNV = (PFNGLPATHGLYPHSNV)GPA(glPathGlyphsNV);
	_glPathGlyphRangeNV = (PFNGLPATHGLYPHRANGENV)GPA(glPathGlyphRangeNV);
	_glWeightPathsNV = (PFNGLWEIGHTPATHSNV)GPA(glWeightPathsNV);
	_glCopyPathNV = (PFNGLCOPYPATHNV)GPA(glCopyPathNV);
	_glInterpolatePathsNV = (PFNGLINTERPOLATEPATHSNV)GPA(glInterpolatePathsNV);
	_glTransformPathNV = (PFNGLTRANSFORMPATHNV)GPA(glTransformPathNV);
	_glPathParameterivNV = (PFNGLPATHPARAMETERIVNV)GPA(glPathParameterivNV);
	_glPathParameteriNV = (PFNGLPATHPARAMETERINV)GPA(glPathParameteriNV);
	_glPathParameterfvNV = (PFNGLPATHPARAMETERFVNV)GPA(glPathParameterfvNV);
	_glPathParameterfNV = (PFNGLPATHPARAMETERFNV)GPA(glPathParameterfNV);
	_glPathDashArrayNV = (PFNGLPATHDASHARRAYNV)GPA(glPathDashArrayNV);
	_glPathStencilFuncNV = (PFNGLPATHSTENCILFUNCNV)GPA(glPathStencilFuncNV);
	_glPathStencilDepthOffsetNV = (PFNGLPATHSTENCILDEPTHOFFSETNV)GPA(glPathStencilDepthOffsetNV);
	_glStencilFillPathNV = (PFNGLSTENCILFILLPATHNV)GPA(glStencilFillPathNV);
	_glStencilStrokePathNV = (PFNGLSTENCILSTROKEPATHNV)GPA(glStencilStrokePathNV);
	_glStencilFillPathInstancedNV = (PFNGLSTENCILFILLPATHINSTANCEDNV)GPA(glStencilFillPathInstancedNV);
	_glStencilStrokePathInstancedNV = (PFNGLSTENCILSTROKEPATHINSTANCEDNV)GPA(glStencilStrokePathInstancedNV);
	_glPathCoverDepthFuncNV = (PFNGLPATHCOVERDEPTHFUNCNV)GPA(glPathCoverDepthFuncNV);
	_glCoverFillPathNV = (PFNGLCOVERFILLPATHNV)GPA(glCoverFillPathNV);
	_glCoverStrokePathNV = (PFNGLCOVERSTROKEPATHNV)GPA(glCoverStrokePathNV);
	_glCoverFillPathInstancedNV = (PFNGLCOVERFILLPATHINSTANCEDNV)GPA(glCoverFillPathInstancedNV);
	_glCoverStrokePathInstancedNV = (PFNGLCOVERSTROKEPATHINSTANCEDNV)GPA(glCoverStrokePathInstancedNV);
	_glGetPathParameterivNV = (PFNGLGETPATHPARAMETERIVNV)GPA(glGetPathParameterivNV);
	_glGetPathParameterfvNV = (PFNGLGETPATHPARAMETERFVNV)GPA(glGetPathParameterfvNV);
	_glGetPathCommandsNV = (PFNGLGETPATHCOMMANDSNV)GPA(glGetPathCommandsNV);
	_glGetPathCoordsNV = (PFNGLGETPATHCOORDSNV)GPA(glGetPathCoordsNV);
	_glGetPathDashArrayNV = (PFNGLGETPATHDASHARRAYNV)GPA(glGetPathDashArrayNV);
	_glGetPathMetricsNV = (PFNGLGETPATHMETRICSNV)GPA(glGetPathMetricsNV);
	_glGetPathMetricRangeNV = (PFNGLGETPATHMETRICRANGENV)GPA(glGetPathMetricRangeNV);
	_glGetPathSpacingNV = (PFNGLGETPATHSPACINGNV)GPA(glGetPathSpacingNV);
	_glIsPointInFillPathNV = (PFNGLISPOINTINFILLPATHNV)GPA(glIsPointInFillPathNV);
	_glIsPointInStrokePathNV = (PFNGLISPOINTINSTROKEPATHNV)GPA(glIsPointInStrokePathNV);
	_glGetPathLengthNV = (PFNGLGETPATHLENGTHNV)GPA(glGetPathLengthNV);
	_glPointAlongPathNV = (PFNGLPOINTALONGPATHNV)GPA(glPointAlongPathNV);
	_glMatrixLoad3x2fNV = (PFNGLMATRIXLOAD3X2FNV)GPA(glMatrixLoad3x2fNV);
	_glMatrixLoad3x3fNV = (PFNGLMATRIXLOAD3X3FNV)GPA(glMatrixLoad3x3fNV);
	_glMatrixLoadTranspose3x3fNV = (PFNGLMATRIXLOADTRANSPOSE3X3FNV)GPA(glMatrixLoadTranspose3x3fNV);
	_glMatrixMult3x2fNV = (PFNGLMATRIXMULT3X2FNV)GPA(glMatrixMult3x2fNV);
	_glMatrixMult3x3fNV = (PFNGLMATRIXMULT3X3FNV)GPA(glMatrixMult3x3fNV);
	_glMatrixMultTranspose3x3fNV = (PFNGLMATRIXMULTTRANSPOSE3X3FNV)GPA(glMatrixMultTranspose3x3fNV);
	_glStencilThenCoverFillPathNV = (PFNGLSTENCILTHENCOVERFILLPATHNV)GPA(glStencilThenCoverFillPathNV);
	_glStencilThenCoverStrokePathNV = (PFNGLSTENCILTHENCOVERSTROKEPATHNV)GPA(glStencilThenCoverStrokePathNV);
	_glStencilThenCoverFillPathInstancedNV = (PFNGLSTENCILTHENCOVERFILLPATHINSTANCEDNV)GPA(glStencilThenCoverFillPathInstancedNV);
	_glStencilThenCoverStrokePathInstancedNV = (PFNGLSTENCILTHENCOVERSTROKEPATHINSTANCEDNV)GPA(glStencilThenCoverStrokePathInstancedNV);
	_glPathGlyphIndexRangeNV = (PFNGLPATHGLYPHINDEXRANGENV)GPA(glPathGlyphIndexRangeNV);
	_glPathGlyphIndexArrayNV = (PFNGLPATHGLYPHINDEXARRAYNV)GPA(glPathGlyphIndexArrayNV);
	_glPathMemoryGlyphIndexArrayNV = (PFNGLPATHMEMORYGLYPHINDEXARRAYNV)GPA(glPathMemoryGlyphIndexArrayNV);
	_glProgramPathFragmentInputGenNV = (PFNGLPROGRAMPATHFRAGMENTINPUTGENNV)GPA(glProgramPathFragmentInputGenNV);
	_glGetProgramResourcefvNV = (PFNGLGETPROGRAMRESOURCEFVNV)GPA(glGetProgramResourcefvNV);
#endif

#ifdef GL_NV_polygon_mode
	_glPolygonModeNV = (PFNGLPOLYGONMODENV)GPA(glPolygonModeNV);
#endif

#ifdef GL_NV_read_buffer
	_glReadBufferNV = (PFNGLREADBUFFERNV)GPA(glReadBufferNV);
#endif

#ifdef GL_NV_sample_locations
	_glFramebufferSampleLocationsfvNV = (PFNGLFRAMEBUFFERSAMPLELOCATIONSFVNV)GPA(glFramebufferSampleLocationsfvNV);
	_glNamedFramebufferSampleLocationsfvNV = (PFNGLNAMEDFRAMEBUFFERSAMPLELOCATIONSFVNV)GPA(glNamedFramebufferSampleLocationsfvNV);
	_glResolveDepthValuesNV = (PFNGLRESOLVEDEPTHVALUESNV)GPA(glResolveDepthValuesNV);
#endif

#ifdef GL_NV_viewport_array
	_glViewportArrayvNV = (PFNGLVIEWPORTARRAYVNV)GPA(glViewportArrayvNV);
	_glViewportIndexedfNV = (PFNGLVIEWPORTINDEXEDFNV)GPA(glViewportIndexedfNV);
	_glViewportIndexedfvNV = (PFNGLVIEWPORTINDEXEDFVNV)GPA(glViewportIndexedfvNV);
	_glScissorArrayvNV = (PFNGLSCISSORARRAYVNV)GPA(glScissorArrayvNV);
	_glScissorIndexedNV = (PFNGLSCISSORINDEXEDNV)GPA(glScissorIndexedNV);
	_glScissorIndexedvNV = (PFNGLSCISSORINDEXEDVNV)GPA(glScissorIndexedvNV);
	_glDepthRangeArrayfvNV = (PFNGLDEPTHRANGEARRAYFVNV)GPA(glDepthRangeArrayfvNV);
	_glDepthRangeIndexedfNV = (PFNGLDEPTHRANGEINDEXEDFNV)GPA(glDepthRangeIndexedfNV);
	_glGetFloati_vNV = (PFNGLGETFLOATI_VNV)GPA(glGetFloati_vNV);
	_glEnableiNV = (PFNGLENABLEINV)GPA(glEnableiNV);
	_glDisableiNV = (PFNGLDISABLEINV)GPA(glDisableiNV);
	_glIsEnablediNV = (PFNGLISENABLEDINV)GPA(glIsEnablediNV);
#endif

#ifdef GL_NV_viewport_swizzle
	_glViewportSwizzleNV = (PFNGLVIEWPORTSWIZZLENV)GPA(glViewportSwizzleNV);
#endif

#ifdef GL_OES_EGL_image
	_glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOES)GPA(glEGLImageTargetTexture2DOES);
	_glEGLImageTargetRenderbufferStorageOES = (PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOES)GPA(glEGLImageTargetRenderbufferStorageOES);
#endif

#ifdef GL_OES_copy_image
	_glCopyImageSubDataOES = (PFNGLCOPYIMAGESUBDATAOES)GPA(glCopyImageSubDataOES);
#endif

#ifdef GL_OES_draw_buffers_indexed
	_glEnableiOES = (PFNGLENABLEIOES)GPA(glEnableiOES);
	_glDisableiOES = (PFNGLDISABLEIOES)GPA(glDisableiOES);
	_glBlendEquationiOES = (PFNGLBLENDEQUATIONIOES)GPA(glBlendEquationiOES);
	_glBlendEquationSeparateiOES = (PFNGLBLENDEQUATIONSEPARATEIOES)GPA(glBlendEquationSeparateiOES);
	_glBlendFunciOES = (PFNGLBLENDFUNCIOES)GPA(glBlendFunciOES);
	_glBlendFuncSeparateiOES = (PFNGLBLENDFUNCSEPARATEIOES)GPA(glBlendFuncSeparateiOES);
	_glColorMaskiOES = (PFNGLCOLORMASKIOES)GPA(glColorMaskiOES);
	_glIsEnablediOES = (PFNGLISENABLEDIOES)GPA(glIsEnablediOES);
#endif

#ifdef GL_OES_draw_elements_base_vertex
	_glDrawElementsBaseVertexOES = (PFNGLDRAWELEMENTSBASEVERTEXOES)GPA(glDrawElementsBaseVertexOES);
	_glDrawRangeElementsBaseVertexOES = (PFNGLDRAWRANGEELEMENTSBASEVERTEXOES)GPA(glDrawRangeElementsBaseVertexOES);
	_glDrawElementsInstancedBaseVertexOES = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXOES)GPA(glDrawElementsInstancedBaseVertexOES);
	_glMultiDrawElementsBaseVertexOES = (PFNGLMULTIDRAWELEMENTSBASEVERTEXOES)GPA(glMultiDrawElementsBaseVertexOES);
#endif

#ifdef GL_OES_geometry_shader
	_glFramebufferTextureOES = (PFNGLFRAMEBUFFERTEXTUREOES)GPA(glFramebufferTextureOES);
#endif

#ifdef GL_OES_get_program_binary
	_glGetProgramBinaryOES = (PFNGLGETPROGRAMBINARYOES)GPA(glGetProgramBinaryOES);
	_glProgramBinaryOES = (PFNGLPROGRAMBINARYOES)GPA(glProgramBinaryOES);
#endif

#ifdef GL_OES_mapbuffer
	_glMapBufferOES = (PFNGLMAPBUFFEROES)GPA(glMapBufferOES);
	_glUnmapBufferOES = (PFNGLUNMAPBUFFEROES)GPA(glUnmapBufferOES);
	_glGetBufferPointervOES = (PFNGLGETBUFFERPOINTERVOES)GPA(glGetBufferPointervOES);
#endif

#ifdef GL_OES_primitive_bounding_box
	_glPrimitiveBoundingBoxOES = (PFNGLPRIMITIVEBOUNDINGBOXOES)GPA(glPrimitiveBoundingBoxOES);
#endif

#ifdef GL_OES_sample_shading
	_glMinSampleShadingOES = (PFNGLMINSAMPLESHADINGOES)GPA(glMinSampleShadingOES);
#endif

#ifdef GL_OES_tessellation_shader
	_glPatchParameteriOES = (PFNGLPATCHPARAMETERIOES)GPA(glPatchParameteriOES);
#endif

#ifdef GL_OES_texture_3D
	_glTexImage3DOES = (PFNGLTEXIMAGE3DOES)GPA(glTexImage3DOES);
	_glTexSubImage3DOES = (PFNGLTEXSUBIMAGE3DOES)GPA(glTexSubImage3DOES);
	_glCopyTexSubImage3DOES = (PFNGLCOPYTEXSUBIMAGE3DOES)GPA(glCopyTexSubImage3DOES);
	_glCompressedTexImage3DOES = (PFNGLCOMPRESSEDTEXIMAGE3DOES)GPA(glCompressedTexImage3DOES);
	_glCompressedTexSubImage3DOES = (PFNGLCOMPRESSEDTEXSUBIMAGE3DOES)GPA(glCompressedTexSubImage3DOES);
	_glFramebufferTexture3DOES = (PFNGLFRAMEBUFFERTEXTURE3DOES)GPA(glFramebufferTexture3DOES);
#endif

#ifdef GL_OES_texture_border_clamp
	_glTexParameterIivOES = (PFNGLTEXPARAMETERIIVOES)GPA(glTexParameterIivOES);
	_glTexParameterIuivOES = (PFNGLTEXPARAMETERIUIVOES)GPA(glTexParameterIuivOES);
	_glGetTexParameterIivOES = (PFNGLGETTEXPARAMETERIIVOES)GPA(glGetTexParameterIivOES);
	_glGetTexParameterIuivOES = (PFNGLGETTEXPARAMETERIUIVOES)GPA(glGetTexParameterIuivOES);
	_glSamplerParameterIivOES = (PFNGLSAMPLERPARAMETERIIVOES)GPA(glSamplerParameterIivOES);
	_glSamplerParameterIuivOES = (PFNGLSAMPLERPARAMETERIUIVOES)GPA(glSamplerParameterIuivOES);
	_glGetSamplerParameterIivOES = (PFNGLGETSAMPLERPARAMETERIIVOES)GPA(glGetSamplerParameterIivOES);
	_glGetSamplerParameterIuivOES = (PFNGLGETSAMPLERPARAMETERIUIVOES)GPA(glGetSamplerParameterIuivOES);
#endif

#ifdef GL_OES_texture_buffer
	_glTexBufferOES = (PFNGLTEXBUFFEROES)GPA(glTexBufferOES);
	_glTexBufferRangeOES = (PFNGLTEXBUFFERRANGEOES)GPA(glTexBufferRangeOES);
#endif

#ifdef GL_OES_texture_storage_multisample_2d_array
	_glTexStorage3DMultisampleOES = (PFNGLTEXSTORAGE3DMULTISAMPLEOES)GPA(glTexStorage3DMultisampleOES);
#endif

#ifdef GL_OES_texture_view
	_glTextureViewOES = (PFNGLTEXTUREVIEWOES)GPA(glTextureViewOES);
#endif

#ifdef GL_OES_vertex_array_object
	_glBindVertexArrayOES = (PFNGLBINDVERTEXARRAYOES)GPA(glBindVertexArrayOES);
	_glDeleteVertexArraysOES = (PFNGLDELETEVERTEXARRAYSOES)GPA(glDeleteVertexArraysOES);
	_glGenVertexArraysOES = (PFNGLGENVERTEXARRAYSOES)GPA(glGenVertexArraysOES);
	_glIsVertexArrayOES = (PFNGLISVERTEXARRAYOES)GPA(glIsVertexArrayOES);
#endif

#ifdef GL_OES_viewport_array
	_glViewportArrayvOES = (PFNGLVIEWPORTARRAYVOES)GPA(glViewportArrayvOES);
	_glViewportIndexedfOES = (PFNGLVIEWPORTINDEXEDFOES)GPA(glViewportIndexedfOES);
	_glViewportIndexedfvOES = (PFNGLVIEWPORTINDEXEDFVOES)GPA(glViewportIndexedfvOES);
	_glScissorArrayvOES = (PFNGLSCISSORARRAYVOES)GPA(glScissorArrayvOES);
	_glScissorIndexedOES = (PFNGLSCISSORINDEXEDOES)GPA(glScissorIndexedOES);
	_glScissorIndexedvOES = (PFNGLSCISSORINDEXEDVOES)GPA(glScissorIndexedvOES);
	_glDepthRangeArrayfvOES = (PFNGLDEPTHRANGEARRAYFVOES)GPA(glDepthRangeArrayfvOES);
	_glDepthRangeIndexedfOES = (PFNGLDEPTHRANGEINDEXEDFOES)GPA(glDepthRangeIndexedfOES);
	_glGetFloati_vOES = (PFNGLGETFLOATI_VOES)GPA(glGetFloati_vOES);
	_glEnableiOES = (PFNGLENABLEIOES)GPA(glEnableiOES);
	_glDisableiOES = (PFNGLDISABLEIOES)GPA(glDisableiOES);
	_glIsEnablediOES = (PFNGLISENABLEDIOES)GPA(glIsEnablediOES);
#endif

#ifdef GL_OVR_multiview
	_glFramebufferTextureMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVR)GPA(glFramebufferTextureMultiviewOVR);
#endif

#ifdef GL_OVR_multiview_multisampled_render_to_texture
	_glFramebufferTextureMultisampleMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVR)GPA(glFramebufferTextureMultisampleMultiviewOVR);
#endif

#ifdef GL_QCOM_alpha_test
	_glAlphaFuncQCOM = (PFNGLALPHAFUNCQCOM)GPA(glAlphaFuncQCOM);
#endif

#ifdef GL_QCOM_driver_control
	_glGetDriverControlsQCOM = (PFNGLGETDRIVERCONTROLSQCOM)GPA(glGetDriverControlsQCOM);
	_glGetDriverControlStringQCOM = (PFNGLGETDRIVERCONTROLSTRINGQCOM)GPA(glGetDriverControlStringQCOM);
	_glEnableDriverControlQCOM = (PFNGLENABLEDRIVERCONTROLQCOM)GPA(glEnableDriverControlQCOM);
	_glDisableDriverControlQCOM = (PFNGLDISABLEDRIVERCONTROLQCOM)GPA(glDisableDriverControlQCOM);
#endif

#ifdef GL_QCOM_extended_get
	_glExtGetTexturesQCOM = (PFNGLEXTGETTEXTURESQCOM)GPA(glExtGetTexturesQCOM);
	_glExtGetBuffersQCOM = (PFNGLEXTGETBUFFERSQCOM)GPA(glExtGetBuffersQCOM);
	_glExtGetRenderbuffersQCOM = (PFNGLEXTGETRENDERBUFFERSQCOM)GPA(glExtGetRenderbuffersQCOM);
	_glExtGetFramebuffersQCOM = (PFNGLEXTGETFRAMEBUFFERSQCOM)GPA(glExtGetFramebuffersQCOM);
	_glExtGetTexLevelParameterivQCOM = (PFNGLEXTGETTEXLEVELPARAMETERIVQCOM)GPA(glExtGetTexLevelParameterivQCOM);
	_glExtTexObjectStateOverrideiQCOM = (PFNGLEXTTEXOBJECTSTATEOVERRIDEIQCOM)GPA(glExtTexObjectStateOverrideiQCOM);
	_glExtGetTexSubImageQCOM = (PFNGLEXTGETTEXSUBIMAGEQCOM)GPA(glExtGetTexSubImageQCOM);
	_glExtGetBufferPointervQCOM = (PFNGLEXTGETBUFFERPOINTERVQCOM)GPA(glExtGetBufferPointervQCOM);
#endif

#ifdef GL_QCOM_extended_get2
	_glExtGetShadersQCOM = (PFNGLEXTGETSHADERSQCOM)GPA(glExtGetShadersQCOM);
	_glExtGetProgramsQCOM = (PFNGLEXTGETPROGRAMSQCOM)GPA(glExtGetProgramsQCOM);
	_glExtIsProgramBinaryQCOM = (PFNGLEXTISPROGRAMBINARYQCOM)GPA(glExtIsProgramBinaryQCOM);
	_glExtGetProgramBinarySourceQCOM = (PFNGLEXTGETPROGRAMBINARYSOURCEQCOM)GPA(glExtGetProgramBinarySourceQCOM);
#endif

#ifdef GL_QCOM_tiled_rendering
	_glStartTilingQCOM = (PFNGLSTARTTILINGQCOM)GPA(glStartTilingQCOM);
	_glEndTilingQCOM = (PFNGLENDTILINGQCOM)GPA(glEndTilingQCOM);
#endif

	ggl_rebind(enableDebug);

	_glGetIntegerv(GL_NUM_EXTENSIONS, &gglext_count);
	for (int i = 0; i < gglext_count; i++) {
		gglext_strings[i] = (const char *)_glGetStringi(GL_EXTENSIONS, i);
	}
	memset(&gglext, 0, sizeof(gglext));
	if (ggl_check_extension("GL_AMD_compressed_3DC_texture")) gglext._GL_AMD_compressed_3DC_texture = 1;
	if (ggl_check_extension("GL_AMD_compressed_ATC_texture")) gglext._GL_AMD_compressed_ATC_texture = 1;
	if (ggl_check_extension("GL_AMD_performance_monitor")) gglext._GL_AMD_performance_monitor = 1;
	if (ggl_check_extension("GL_AMD_program_binary_Z400")) gglext._GL_AMD_program_binary_Z400 = 1;
	if (ggl_check_extension("GL_ANDROID_extension_pack_es31a")) gglext._GL_ANDROID_extension_pack_es31a = 1;
	if (ggl_check_extension("GL_ANGLE_depth_texture")) gglext._GL_ANGLE_depth_texture = 1;
	if (ggl_check_extension("GL_ANGLE_framebuffer_blit")) gglext._GL_ANGLE_framebuffer_blit = 1;
	if (ggl_check_extension("GL_ANGLE_framebuffer_multisample")) gglext._GL_ANGLE_framebuffer_multisample = 1;
	if (ggl_check_extension("GL_ANGLE_instanced_arrays")) gglext._GL_ANGLE_instanced_arrays = 1;
	if (ggl_check_extension("GL_ANGLE_pack_reverse_row_order")) gglext._GL_ANGLE_pack_reverse_row_order = 1;
	if (ggl_check_extension("GL_ANGLE_program_binary")) gglext._GL_ANGLE_program_binary = 1;
	if (ggl_check_extension("GL_ANGLE_texture_compression_dxt3")) gglext._GL_ANGLE_texture_compression_dxt3 = 1;
	if (ggl_check_extension("GL_ANGLE_texture_compression_dxt5")) gglext._GL_ANGLE_texture_compression_dxt5 = 1;
	if (ggl_check_extension("GL_ANGLE_texture_usage")) gglext._GL_ANGLE_texture_usage = 1;
	if (ggl_check_extension("GL_ANGLE_translated_shader_source")) gglext._GL_ANGLE_translated_shader_source = 1;
	if (ggl_check_extension("GL_APPLE_clip_distance")) gglext._GL_APPLE_clip_distance = 1;
	if (ggl_check_extension("GL_APPLE_color_buffer_packed_float")) gglext._GL_APPLE_color_buffer_packed_float = 1;
	if (ggl_check_extension("GL_APPLE_copy_texture_levels")) gglext._GL_APPLE_copy_texture_levels = 1;
	if (ggl_check_extension("GL_APPLE_framebuffer_multisample")) gglext._GL_APPLE_framebuffer_multisample = 1;
	if (ggl_check_extension("GL_APPLE_rgb_422")) gglext._GL_APPLE_rgb_422 = 1;
	if (ggl_check_extension("GL_APPLE_sync")) gglext._GL_APPLE_sync = 1;
	if (ggl_check_extension("GL_APPLE_texture_format_BGRA8888")) gglext._GL_APPLE_texture_format_BGRA8888 = 1;
	if (ggl_check_extension("GL_APPLE_texture_max_level")) gglext._GL_APPLE_texture_max_level = 1;
	if (ggl_check_extension("GL_APPLE_texture_packed_float")) gglext._GL_APPLE_texture_packed_float = 1;
	if (ggl_check_extension("GL_ARB_sparse_texture2")) gglext._GL_ARB_sparse_texture2 = 1;
	if (ggl_check_extension("GL_ARM_mali_program_binary")) gglext._GL_ARM_mali_program_binary = 1;
	if (ggl_check_extension("GL_ARM_mali_shader_binary")) gglext._GL_ARM_mali_shader_binary = 1;
	if (ggl_check_extension("GL_ARM_rgba8")) gglext._GL_ARM_rgba8 = 1;
	if (ggl_check_extension("GL_ARM_shader_framebuffer_fetch")) gglext._GL_ARM_shader_framebuffer_fetch = 1;
	if (ggl_check_extension("GL_ARM_shader_framebuffer_fetch_depth_stencil")) gglext._GL_ARM_shader_framebuffer_fetch_depth_stencil = 1;
	if (ggl_check_extension("GL_DMP_program_binary")) gglext._GL_DMP_program_binary = 1;
	if (ggl_check_extension("GL_DMP_shader_binary")) gglext._GL_DMP_shader_binary = 1;
	if (ggl_check_extension("GL_EXT_YUV_target")) gglext._GL_EXT_YUV_target = 1;
	if (ggl_check_extension("GL_EXT_base_instance")) gglext._GL_EXT_base_instance = 1;
	if (ggl_check_extension("GL_EXT_blend_func_extended")) gglext._GL_EXT_blend_func_extended = 1;
	if (ggl_check_extension("GL_EXT_blend_minmax")) gglext._GL_EXT_blend_minmax = 1;
	if (ggl_check_extension("GL_EXT_buffer_storage")) gglext._GL_EXT_buffer_storage = 1;
	if (ggl_check_extension("GL_EXT_clear_texture")) gglext._GL_EXT_clear_texture = 1;
	if (ggl_check_extension("GL_EXT_clip_cull_distance")) gglext._GL_EXT_clip_cull_distance = 1;
	if (ggl_check_extension("GL_EXT_color_buffer_float")) gglext._GL_EXT_color_buffer_float = 1;
	if (ggl_check_extension("GL_EXT_color_buffer_half_float")) gglext._GL_EXT_color_buffer_half_float = 1;
	if (ggl_check_extension("GL_EXT_conservative_depth")) gglext._GL_EXT_conservative_depth = 1;
	if (ggl_check_extension("GL_EXT_copy_image")) gglext._GL_EXT_copy_image = 1;
	if (ggl_check_extension("GL_EXT_debug_label")) gglext._GL_EXT_debug_label = 1;
	if (ggl_check_extension("GL_EXT_debug_marker")) gglext._GL_EXT_debug_marker = 1;
	if (ggl_check_extension("GL_EXT_discard_framebuffer")) gglext._GL_EXT_discard_framebuffer = 1;
	if (ggl_check_extension("GL_EXT_disjoint_timer_query")) gglext._GL_EXT_disjoint_timer_query = 1;
	if (ggl_check_extension("GL_EXT_draw_buffers")) gglext._GL_EXT_draw_buffers = 1;
	if (ggl_check_extension("GL_EXT_draw_buffers_indexed")) gglext._GL_EXT_draw_buffers_indexed = 1;
	if (ggl_check_extension("GL_EXT_draw_elements_base_vertex")) gglext._GL_EXT_draw_elements_base_vertex = 1;
	if (ggl_check_extension("GL_EXT_draw_instanced")) gglext._GL_EXT_draw_instanced = 1;
	if (ggl_check_extension("GL_EXT_draw_transform_feedback")) gglext._GL_EXT_draw_transform_feedback = 1;
	if (ggl_check_extension("GL_EXT_float_blend")) gglext._GL_EXT_float_blend = 1;
	if (ggl_check_extension("GL_EXT_geometry_point_size")) gglext._GL_EXT_geometry_point_size = 1;
	if (ggl_check_extension("GL_EXT_geometry_shader")) gglext._GL_EXT_geometry_shader = 1;
	if (ggl_check_extension("GL_EXT_gpu_shader5")) gglext._GL_EXT_gpu_shader5 = 1;
	if (ggl_check_extension("GL_EXT_instanced_arrays")) gglext._GL_EXT_instanced_arrays = 1;
	if (ggl_check_extension("GL_EXT_map_buffer_range")) gglext._GL_EXT_map_buffer_range = 1;
	if (ggl_check_extension("GL_EXT_multi_draw_arrays")) gglext._GL_EXT_multi_draw_arrays = 1;
	if (ggl_check_extension("GL_EXT_multi_draw_indirect")) gglext._GL_EXT_multi_draw_indirect = 1;
	if (ggl_check_extension("GL_EXT_multisampled_compatibility")) gglext._GL_EXT_multisampled_compatibility = 1;
	if (ggl_check_extension("GL_EXT_multisampled_render_to_texture")) gglext._GL_EXT_multisampled_render_to_texture = 1;
	if (ggl_check_extension("GL_EXT_multiview_draw_buffers")) gglext._GL_EXT_multiview_draw_buffers = 1;
	if (ggl_check_extension("GL_EXT_occlusion_query_boolean")) gglext._GL_EXT_occlusion_query_boolean = 1;
	if (ggl_check_extension("GL_EXT_polygon_offset_clamp")) gglext._GL_EXT_polygon_offset_clamp = 1;
	if (ggl_check_extension("GL_EXT_post_depth_coverage")) gglext._GL_EXT_post_depth_coverage = 1;
	if (ggl_check_extension("GL_EXT_primitive_bounding_box")) gglext._GL_EXT_primitive_bounding_box = 1;
	if (ggl_check_extension("GL_EXT_protected_textures")) gglext._GL_EXT_protected_textures = 1;
	if (ggl_check_extension("GL_EXT_pvrtc_sRGB")) gglext._GL_EXT_pvrtc_sRGB = 1;
	if (ggl_check_extension("GL_EXT_raster_multisample")) gglext._GL_EXT_raster_multisample = 1;
	if (ggl_check_extension("GL_EXT_read_format_bgra")) gglext._GL_EXT_read_format_bgra = 1;
	if (ggl_check_extension("GL_EXT_render_snorm")) gglext._GL_EXT_render_snorm = 1;
	if (ggl_check_extension("GL_EXT_robustness")) gglext._GL_EXT_robustness = 1;
	if (ggl_check_extension("GL_EXT_sRGB")) gglext._GL_EXT_sRGB = 1;
	if (ggl_check_extension("GL_EXT_sRGB_write_control")) gglext._GL_EXT_sRGB_write_control = 1;
	if (ggl_check_extension("GL_EXT_separate_shader_objects")) gglext._GL_EXT_separate_shader_objects = 1;
	if (ggl_check_extension("GL_EXT_shader_framebuffer_fetch")) gglext._GL_EXT_shader_framebuffer_fetch = 1;
	if (ggl_check_extension("GL_EXT_shader_group_vote")) gglext._GL_EXT_shader_group_vote = 1;
	if (ggl_check_extension("GL_EXT_shader_implicit_conversions")) gglext._GL_EXT_shader_implicit_conversions = 1;
	if (ggl_check_extension("GL_EXT_shader_integer_mix")) gglext._GL_EXT_shader_integer_mix = 1;
	if (ggl_check_extension("GL_EXT_shader_io_blocks")) gglext._GL_EXT_shader_io_blocks = 1;
	if (ggl_check_extension("GL_EXT_shader_non_constant_global_initializers")) gglext._GL_EXT_shader_non_constant_global_initializers = 1;
	if (ggl_check_extension("GL_EXT_shader_pixel_local_storage")) gglext._GL_EXT_shader_pixel_local_storage = 1;
	if (ggl_check_extension("GL_EXT_shader_pixel_local_storage2")) gglext._GL_EXT_shader_pixel_local_storage2 = 1;
	if (ggl_check_extension("GL_EXT_shader_texture_lod")) gglext._GL_EXT_shader_texture_lod = 1;
	if (ggl_check_extension("GL_EXT_shadow_samplers")) gglext._GL_EXT_shadow_samplers = 1;
	if (ggl_check_extension("GL_EXT_sparse_texture")) gglext._GL_EXT_sparse_texture = 1;
	if (ggl_check_extension("GL_EXT_tessellation_point_size")) gglext._GL_EXT_tessellation_point_size = 1;
	if (ggl_check_extension("GL_EXT_tessellation_shader")) gglext._GL_EXT_tessellation_shader = 1;
	if (ggl_check_extension("GL_EXT_texture_border_clamp")) gglext._GL_EXT_texture_border_clamp = 1;
	if (ggl_check_extension("GL_EXT_texture_buffer")) gglext._GL_EXT_texture_buffer = 1;
	if (ggl_check_extension("GL_EXT_texture_compression_dxt1")) gglext._GL_EXT_texture_compression_dxt1 = 1;
	if (ggl_check_extension("GL_EXT_texture_compression_s3tc")) gglext._GL_EXT_texture_compression_s3tc = 1;
	if (ggl_check_extension("GL_EXT_texture_cube_map_array")) gglext._GL_EXT_texture_cube_map_array = 1;
	if (ggl_check_extension("GL_EXT_texture_filter_anisotropic")) gglext._GL_EXT_texture_filter_anisotropic = 1;
	if (ggl_check_extension("GL_EXT_texture_filter_minmax")) gglext._GL_EXT_texture_filter_minmax = 1;
	if (ggl_check_extension("GL_EXT_texture_format_BGRA8888")) gglext._GL_EXT_texture_format_BGRA8888 = 1;
	if (ggl_check_extension("GL_EXT_texture_norm16")) gglext._GL_EXT_texture_norm16 = 1;
	if (ggl_check_extension("GL_EXT_texture_rg")) gglext._GL_EXT_texture_rg = 1;
	if (ggl_check_extension("GL_EXT_texture_sRGB_R8")) gglext._GL_EXT_texture_sRGB_R8 = 1;
	if (ggl_check_extension("GL_EXT_texture_sRGB_RG8")) gglext._GL_EXT_texture_sRGB_RG8 = 1;
	if (ggl_check_extension("GL_EXT_texture_sRGB_decode")) gglext._GL_EXT_texture_sRGB_decode = 1;
	if (ggl_check_extension("GL_EXT_texture_storage")) gglext._GL_EXT_texture_storage = 1;
	if (ggl_check_extension("GL_EXT_texture_type_2_10_10_10_REV")) gglext._GL_EXT_texture_type_2_10_10_10_REV = 1;
	if (ggl_check_extension("GL_EXT_texture_view")) gglext._GL_EXT_texture_view = 1;
	if (ggl_check_extension("GL_EXT_unpack_subimage")) gglext._GL_EXT_unpack_subimage = 1;
	if (ggl_check_extension("GL_EXT_window_rectangles")) gglext._GL_EXT_window_rectangles = 1;
	if (ggl_check_extension("GL_FJ_shader_binary_GCCSO")) gglext._GL_FJ_shader_binary_GCCSO = 1;
	if (ggl_check_extension("GL_IMG_bindless_texture")) gglext._GL_IMG_bindless_texture = 1;
	if (ggl_check_extension("GL_IMG_framebuffer_downsample")) gglext._GL_IMG_framebuffer_downsample = 1;
	if (ggl_check_extension("GL_IMG_multisampled_render_to_texture")) gglext._GL_IMG_multisampled_render_to_texture = 1;
	if (ggl_check_extension("GL_IMG_program_binary")) gglext._GL_IMG_program_binary = 1;
	if (ggl_check_extension("GL_IMG_read_format")) gglext._GL_IMG_read_format = 1;
	if (ggl_check_extension("GL_IMG_shader_binary")) gglext._GL_IMG_shader_binary = 1;
	if (ggl_check_extension("GL_IMG_texture_compression_pvrtc")) gglext._GL_IMG_texture_compression_pvrtc = 1;
	if (ggl_check_extension("GL_IMG_texture_compression_pvrtc2")) gglext._GL_IMG_texture_compression_pvrtc2 = 1;
	if (ggl_check_extension("GL_IMG_texture_filter_cubic")) gglext._GL_IMG_texture_filter_cubic = 1;
	if (ggl_check_extension("GL_INTEL_conservative_rasterization")) gglext._GL_INTEL_conservative_rasterization = 1;
	if (ggl_check_extension("GL_INTEL_framebuffer_CMAA")) gglext._GL_INTEL_framebuffer_CMAA = 1;
	if (ggl_check_extension("GL_INTEL_performance_query")) gglext._GL_INTEL_performance_query = 1;
	if (ggl_check_extension("GL_KHR_blend_equation_advanced")) gglext._GL_KHR_blend_equation_advanced = 1;
	if (ggl_check_extension("GL_KHR_blend_equation_advanced_coherent")) gglext._GL_KHR_blend_equation_advanced_coherent = 1;
	if (ggl_check_extension("GL_KHR_context_flush_control")) gglext._GL_KHR_context_flush_control = 1;
	if (ggl_check_extension("GL_KHR_debug")) gglext._GL_KHR_debug = 1;
	if (ggl_check_extension("GL_KHR_no_error")) gglext._GL_KHR_no_error = 1;
	if (ggl_check_extension("GL_KHR_robust_buffer_access_behavior")) gglext._GL_KHR_robust_buffer_access_behavior = 1;
	if (ggl_check_extension("GL_KHR_robustness")) gglext._GL_KHR_robustness = 1;
	if (ggl_check_extension("GL_KHR_texture_compression_astc_hdr")) gglext._GL_KHR_texture_compression_astc_hdr = 1;
	if (ggl_check_extension("GL_KHR_texture_compression_astc_ldr")) gglext._GL_KHR_texture_compression_astc_ldr = 1;
	if (ggl_check_extension("GL_KHR_texture_compression_astc_sliced_3d")) gglext._GL_KHR_texture_compression_astc_sliced_3d = 1;
	if (ggl_check_extension("GL_NV_bindless_texture")) gglext._GL_NV_bindless_texture = 1;
	if (ggl_check_extension("GL_NV_blend_equation_advanced")) gglext._GL_NV_blend_equation_advanced = 1;
	if (ggl_check_extension("GL_NV_blend_equation_advanced_coherent")) gglext._GL_NV_blend_equation_advanced_coherent = 1;
	if (ggl_check_extension("GL_NV_conditional_render")) gglext._GL_NV_conditional_render = 1;
	if (ggl_check_extension("GL_NV_conservative_raster")) gglext._GL_NV_conservative_raster = 1;
	if (ggl_check_extension("GL_NV_conservative_raster_pre_snap_triangles")) gglext._GL_NV_conservative_raster_pre_snap_triangles = 1;
	if (ggl_check_extension("GL_NV_copy_buffer")) gglext._GL_NV_copy_buffer = 1;
	if (ggl_check_extension("GL_NV_coverage_sample")) gglext._GL_NV_coverage_sample = 1;
	if (ggl_check_extension("GL_NV_depth_nonlinear")) gglext._GL_NV_depth_nonlinear = 1;
	if (ggl_check_extension("GL_NV_draw_buffers")) gglext._GL_NV_draw_buffers = 1;
	if (ggl_check_extension("GL_NV_draw_instanced")) gglext._GL_NV_draw_instanced = 1;
	if (ggl_check_extension("GL_NV_explicit_attrib_location")) gglext._GL_NV_explicit_attrib_location = 1;
	if (ggl_check_extension("GL_NV_fbo_color_attachments")) gglext._GL_NV_fbo_color_attachments = 1;
	if (ggl_check_extension("GL_NV_fence")) gglext._GL_NV_fence = 1;
	if (ggl_check_extension("GL_NV_fill_rectangle")) gglext._GL_NV_fill_rectangle = 1;
	if (ggl_check_extension("GL_NV_fragment_coverage_to_color")) gglext._GL_NV_fragment_coverage_to_color = 1;
	if (ggl_check_extension("GL_NV_fragment_shader_interlock")) gglext._GL_NV_fragment_shader_interlock = 1;
	if (ggl_check_extension("GL_NV_framebuffer_blit")) gglext._GL_NV_framebuffer_blit = 1;
	if (ggl_check_extension("GL_NV_framebuffer_mixed_samples")) gglext._GL_NV_framebuffer_mixed_samples = 1;
	if (ggl_check_extension("GL_NV_framebuffer_multisample")) gglext._GL_NV_framebuffer_multisample = 1;
	if (ggl_check_extension("GL_NV_generate_mipmap_sRGB")) gglext._GL_NV_generate_mipmap_sRGB = 1;
	if (ggl_check_extension("GL_NV_geometry_shader_passthrough")) gglext._GL_NV_geometry_shader_passthrough = 1;
	if (ggl_check_extension("GL_NV_gpu_shader5")) gglext._GL_NV_gpu_shader5 = 1;
	if (ggl_check_extension("GL_NV_image_formats")) gglext._GL_NV_image_formats = 1;
	if (ggl_check_extension("GL_NV_instanced_arrays")) gglext._GL_NV_instanced_arrays = 1;
	if (ggl_check_extension("GL_NV_internalformat_sample_query")) gglext._GL_NV_internalformat_sample_query = 1;
	if (ggl_check_extension("GL_NV_non_square_matrices")) gglext._GL_NV_non_square_matrices = 1;
	if (ggl_check_extension("GL_NV_path_rendering")) gglext._GL_NV_path_rendering = 1;
	if (ggl_check_extension("GL_NV_path_rendering_shared_edge")) gglext._GL_NV_path_rendering_shared_edge = 1;
	if (ggl_check_extension("GL_NV_polygon_mode")) gglext._GL_NV_polygon_mode = 1;
	if (ggl_check_extension("GL_NV_read_buffer")) gglext._GL_NV_read_buffer = 1;
	if (ggl_check_extension("GL_NV_read_buffer_front")) gglext._GL_NV_read_buffer_front = 1;
	if (ggl_check_extension("GL_NV_read_depth")) gglext._GL_NV_read_depth = 1;
	if (ggl_check_extension("GL_NV_read_depth_stencil")) gglext._GL_NV_read_depth_stencil = 1;
	if (ggl_check_extension("GL_NV_read_stencil")) gglext._GL_NV_read_stencil = 1;
	if (ggl_check_extension("GL_NV_sRGB_formats")) gglext._GL_NV_sRGB_formats = 1;
	if (ggl_check_extension("GL_NV_sample_locations")) gglext._GL_NV_sample_locations = 1;
	if (ggl_check_extension("GL_NV_sample_mask_override_coverage")) gglext._GL_NV_sample_mask_override_coverage = 1;
	if (ggl_check_extension("GL_NV_shader_atomic_fp16_vector")) gglext._GL_NV_shader_atomic_fp16_vector = 1;
	if (ggl_check_extension("GL_NV_shader_noperspective_interpolation")) gglext._GL_NV_shader_noperspective_interpolation = 1;
	if (ggl_check_extension("GL_NV_shadow_samplers_array")) gglext._GL_NV_shadow_samplers_array = 1;
	if (ggl_check_extension("GL_NV_shadow_samplers_cube")) gglext._GL_NV_shadow_samplers_cube = 1;
	if (ggl_check_extension("GL_NV_texture_border_clamp")) gglext._GL_NV_texture_border_clamp = 1;
	if (ggl_check_extension("GL_NV_texture_compression_s3tc_update")) gglext._GL_NV_texture_compression_s3tc_update = 1;
	if (ggl_check_extension("GL_NV_texture_npot_2D_mipmap")) gglext._GL_NV_texture_npot_2D_mipmap = 1;
	if (ggl_check_extension("GL_NV_viewport_array")) gglext._GL_NV_viewport_array = 1;
	if (ggl_check_extension("GL_NV_viewport_array2")) gglext._GL_NV_viewport_array2 = 1;
	if (ggl_check_extension("GL_NV_viewport_swizzle")) gglext._GL_NV_viewport_swizzle = 1;
	if (ggl_check_extension("GL_OES_EGL_image")) gglext._GL_OES_EGL_image = 1;
	if (ggl_check_extension("GL_OES_EGL_image_external")) gglext._GL_OES_EGL_image_external = 1;
	if (ggl_check_extension("GL_OES_EGL_image_external_essl3")) gglext._GL_OES_EGL_image_external_essl3 = 1;
	if (ggl_check_extension("GL_OES_compressed_ETC1_RGB8_sub_texture")) gglext._GL_OES_compressed_ETC1_RGB8_sub_texture = 1;
	if (ggl_check_extension("GL_OES_compressed_ETC1_RGB8_texture")) gglext._GL_OES_compressed_ETC1_RGB8_texture = 1;
	if (ggl_check_extension("GL_OES_compressed_paletted_texture")) gglext._GL_OES_compressed_paletted_texture = 1;
	if (ggl_check_extension("GL_OES_copy_image")) gglext._GL_OES_copy_image = 1;
	if (ggl_check_extension("GL_OES_depth24")) gglext._GL_OES_depth24 = 1;
	if (ggl_check_extension("GL_OES_depth32")) gglext._GL_OES_depth32 = 1;
	if (ggl_check_extension("GL_OES_depth_texture")) gglext._GL_OES_depth_texture = 1;
	if (ggl_check_extension("GL_OES_draw_buffers_indexed")) gglext._GL_OES_draw_buffers_indexed = 1;
	if (ggl_check_extension("GL_OES_draw_elements_base_vertex")) gglext._GL_OES_draw_elements_base_vertex = 1;
	if (ggl_check_extension("GL_OES_element_index_uint")) gglext._GL_OES_element_index_uint = 1;
	if (ggl_check_extension("GL_OES_fbo_render_mipmap")) gglext._GL_OES_fbo_render_mipmap = 1;
	if (ggl_check_extension("GL_OES_fragment_precision_high")) gglext._GL_OES_fragment_precision_high = 1;
	if (ggl_check_extension("GL_OES_geometry_point_size")) gglext._GL_OES_geometry_point_size = 1;
	if (ggl_check_extension("GL_OES_geometry_shader")) gglext._GL_OES_geometry_shader = 1;
	if (ggl_check_extension("GL_OES_get_program_binary")) gglext._GL_OES_get_program_binary = 1;
	if (ggl_check_extension("GL_OES_gpu_shader5")) gglext._GL_OES_gpu_shader5 = 1;
	if (ggl_check_extension("GL_OES_mapbuffer")) gglext._GL_OES_mapbuffer = 1;
	if (ggl_check_extension("GL_OES_packed_depth_stencil")) gglext._GL_OES_packed_depth_stencil = 1;
	if (ggl_check_extension("GL_OES_primitive_bounding_box")) gglext._GL_OES_primitive_bounding_box = 1;
	if (ggl_check_extension("GL_OES_required_internalformat")) gglext._GL_OES_required_internalformat = 1;
	if (ggl_check_extension("GL_OES_rgb8_rgba8")) gglext._GL_OES_rgb8_rgba8 = 1;
	if (ggl_check_extension("GL_OES_sample_shading")) gglext._GL_OES_sample_shading = 1;
	if (ggl_check_extension("GL_OES_sample_variables")) gglext._GL_OES_sample_variables = 1;
	if (ggl_check_extension("GL_OES_shader_image_atomic")) gglext._GL_OES_shader_image_atomic = 1;
	if (ggl_check_extension("GL_OES_shader_io_blocks")) gglext._GL_OES_shader_io_blocks = 1;
	if (ggl_check_extension("GL_OES_shader_multisample_interpolation")) gglext._GL_OES_shader_multisample_interpolation = 1;
	if (ggl_check_extension("GL_OES_standard_derivatives")) gglext._GL_OES_standard_derivatives = 1;
	if (ggl_check_extension("GL_OES_stencil1")) gglext._GL_OES_stencil1 = 1;
	if (ggl_check_extension("GL_OES_stencil4")) gglext._GL_OES_stencil4 = 1;
	if (ggl_check_extension("GL_OES_surfaceless_context")) gglext._GL_OES_surfaceless_context = 1;
	if (ggl_check_extension("GL_OES_tessellation_point_size")) gglext._GL_OES_tessellation_point_size = 1;
	if (ggl_check_extension("GL_OES_tessellation_shader")) gglext._GL_OES_tessellation_shader = 1;
	if (ggl_check_extension("GL_OES_texture_3D")) gglext._GL_OES_texture_3D = 1;
	if (ggl_check_extension("GL_OES_texture_border_clamp")) gglext._GL_OES_texture_border_clamp = 1;
	if (ggl_check_extension("GL_OES_texture_buffer")) gglext._GL_OES_texture_buffer = 1;
	if (ggl_check_extension("GL_OES_texture_compression_astc")) gglext._GL_OES_texture_compression_astc = 1;
	if (ggl_check_extension("GL_OES_texture_cube_map_array")) gglext._GL_OES_texture_cube_map_array = 1;
	if (ggl_check_extension("GL_OES_texture_float")) gglext._GL_OES_texture_float = 1;
	if (ggl_check_extension("GL_OES_texture_float_linear")) gglext._GL_OES_texture_float_linear = 1;
	if (ggl_check_extension("GL_OES_texture_half_float")) gglext._GL_OES_texture_half_float = 1;
	if (ggl_check_extension("GL_OES_texture_half_float_linear")) gglext._GL_OES_texture_half_float_linear = 1;
	if (ggl_check_extension("GL_OES_texture_npot")) gglext._GL_OES_texture_npot = 1;
	if (ggl_check_extension("GL_OES_texture_stencil8")) gglext._GL_OES_texture_stencil8 = 1;
	if (ggl_check_extension("GL_OES_texture_storage_multisample_2d_array")) gglext._GL_OES_texture_storage_multisample_2d_array = 1;
	if (ggl_check_extension("GL_OES_texture_view")) gglext._GL_OES_texture_view = 1;
	if (ggl_check_extension("GL_OES_vertex_array_object")) gglext._GL_OES_vertex_array_object = 1;
	if (ggl_check_extension("GL_OES_vertex_half_float")) gglext._GL_OES_vertex_half_float = 1;
	if (ggl_check_extension("GL_OES_vertex_type_10_10_10_2")) gglext._GL_OES_vertex_type_10_10_10_2 = 1;
	if (ggl_check_extension("GL_OES_viewport_array")) gglext._GL_OES_viewport_array = 1;
	if (ggl_check_extension("GL_OVR_multiview")) gglext._GL_OVR_multiview = 1;
	if (ggl_check_extension("GL_OVR_multiview2")) gglext._GL_OVR_multiview2 = 1;
	if (ggl_check_extension("GL_OVR_multiview_multisampled_render_to_texture")) gglext._GL_OVR_multiview_multisampled_render_to_texture = 1;
	if (ggl_check_extension("GL_QCOM_alpha_test")) gglext._GL_QCOM_alpha_test = 1;
	if (ggl_check_extension("GL_QCOM_binning_control")) gglext._GL_QCOM_binning_control = 1;
	if (ggl_check_extension("GL_QCOM_driver_control")) gglext._GL_QCOM_driver_control = 1;
	if (ggl_check_extension("GL_QCOM_extended_get")) gglext._GL_QCOM_extended_get = 1;
	if (ggl_check_extension("GL_QCOM_extended_get2")) gglext._GL_QCOM_extended_get2 = 1;
	if (ggl_check_extension("GL_QCOM_perfmon_global_mode")) gglext._GL_QCOM_perfmon_global_mode = 1;
	if (ggl_check_extension("GL_QCOM_tiled_rendering")) gglext._GL_QCOM_tiled_rendering = 1;
	if (ggl_check_extension("GL_QCOM_writeonly_rendering")) gglext._GL_QCOM_writeonly_rendering = 1;
	if (ggl_check_extension("GL_VIV_shader_binary")) gglext._GL_VIV_shader_binary = 1;
}

void ggl_rebind(int enableDebug) {
	if (!enableDebug) {
		gglActiveShaderProgramEXT = _glActiveShaderProgramEXT;
		gglActiveTexture = _glActiveTexture;
		gglAlphaFuncQCOM = _glAlphaFuncQCOM;
		gglApplyFramebufferAttachmentCMAAINTEL = _glApplyFramebufferAttachmentCMAAINTEL;
		gglAttachShader = _glAttachShader;
		gglBeginConditionalRenderNV = _glBeginConditionalRenderNV;
		gglBeginPerfMonitorAMD = _glBeginPerfMonitorAMD;
		gglBeginPerfQueryINTEL = _glBeginPerfQueryINTEL;
		gglBeginQuery = _glBeginQuery;
		gglBeginQueryEXT = _glBeginQueryEXT;
		gglBeginTransformFeedback = _glBeginTransformFeedback;
		gglBindAttribLocation = _glBindAttribLocation;
		gglBindBuffer = _glBindBuffer;
		gglBindBufferBase = _glBindBufferBase;
		gglBindBufferRange = _glBindBufferRange;
		gglBindFragDataLocationEXT = _glBindFragDataLocationEXT;
		gglBindFragDataLocationIndexedEXT = _glBindFragDataLocationIndexedEXT;
		gglBindFramebuffer = _glBindFramebuffer;
		gglBindProgramPipelineEXT = _glBindProgramPipelineEXT;
		gglBindRenderbuffer = _glBindRenderbuffer;
		gglBindSampler = _glBindSampler;
		gglBindTexture = _glBindTexture;
		gglBindTransformFeedback = _glBindTransformFeedback;
		gglBindVertexArray = _glBindVertexArray;
		gglBindVertexArrayOES = _glBindVertexArrayOES;
		gglBlendBarrierKHR = _glBlendBarrierKHR;
		gglBlendBarrierNV = _glBlendBarrierNV;
		gglBlendColor = _glBlendColor;
		gglBlendEquation = _glBlendEquation;
		gglBlendEquationSeparate = _glBlendEquationSeparate;
		gglBlendEquationSeparateiEXT = _glBlendEquationSeparateiEXT;
		gglBlendEquationSeparateiOES = _glBlendEquationSeparateiOES;
		gglBlendEquationiEXT = _glBlendEquationiEXT;
		gglBlendEquationiOES = _glBlendEquationiOES;
		gglBlendFunc = _glBlendFunc;
		gglBlendFuncSeparate = _glBlendFuncSeparate;
		gglBlendFuncSeparateiEXT = _glBlendFuncSeparateiEXT;
		gglBlendFuncSeparateiOES = _glBlendFuncSeparateiOES;
		gglBlendFunciEXT = _glBlendFunciEXT;
		gglBlendFunciOES = _glBlendFunciOES;
		gglBlendParameteriNV = _glBlendParameteriNV;
		gglBlitFramebuffer = _glBlitFramebuffer;
		gglBlitFramebufferANGLE = _glBlitFramebufferANGLE;
		gglBlitFramebufferNV = _glBlitFramebufferNV;
		gglBufferData = _glBufferData;
		gglBufferStorageEXT = _glBufferStorageEXT;
		gglBufferSubData = _glBufferSubData;
		gglCheckFramebufferStatus = _glCheckFramebufferStatus;
		gglClear = _glClear;
		gglClearBufferfi = _glClearBufferfi;
		gglClearBufferfv = _glClearBufferfv;
		gglClearBufferiv = _glClearBufferiv;
		gglClearBufferuiv = _glClearBufferuiv;
		gglClearColor = _glClearColor;
		gglClearDepthf = _glClearDepthf;
		gglClearPixelLocalStorageuiEXT = _glClearPixelLocalStorageuiEXT;
		gglClearStencil = _glClearStencil;
		gglClearTexImageEXT = _glClearTexImageEXT;
		gglClearTexSubImageEXT = _glClearTexSubImageEXT;
		gglClientWaitSync = _glClientWaitSync;
		gglClientWaitSyncAPPLE = _glClientWaitSyncAPPLE;
		gglColorMask = _glColorMask;
		gglColorMaskiEXT = _glColorMaskiEXT;
		gglColorMaskiOES = _glColorMaskiOES;
		gglCompileShader = _glCompileShader;
		gglCompressedTexImage2D = _glCompressedTexImage2D;
		gglCompressedTexImage3D = _glCompressedTexImage3D;
		gglCompressedTexImage3DOES = _glCompressedTexImage3DOES;
		gglCompressedTexSubImage2D = _glCompressedTexSubImage2D;
		gglCompressedTexSubImage3D = _glCompressedTexSubImage3D;
		gglCompressedTexSubImage3DOES = _glCompressedTexSubImage3DOES;
		gglConservativeRasterParameteriNV = _glConservativeRasterParameteriNV;
		gglCopyBufferSubData = _glCopyBufferSubData;
		gglCopyBufferSubDataNV = _glCopyBufferSubDataNV;
		gglCopyImageSubDataEXT = _glCopyImageSubDataEXT;
		gglCopyImageSubDataOES = _glCopyImageSubDataOES;
		gglCopyPathNV = _glCopyPathNV;
		gglCopyTexImage2D = _glCopyTexImage2D;
		gglCopyTexSubImage2D = _glCopyTexSubImage2D;
		gglCopyTexSubImage3D = _glCopyTexSubImage3D;
		gglCopyTexSubImage3DOES = _glCopyTexSubImage3DOES;
		gglCopyTextureLevelsAPPLE = _glCopyTextureLevelsAPPLE;
		gglCoverFillPathInstancedNV = _glCoverFillPathInstancedNV;
		gglCoverFillPathNV = _glCoverFillPathNV;
		gglCoverStrokePathInstancedNV = _glCoverStrokePathInstancedNV;
		gglCoverStrokePathNV = _glCoverStrokePathNV;
		gglCoverageMaskNV = _glCoverageMaskNV;
		gglCoverageModulationNV = _glCoverageModulationNV;
		gglCoverageModulationTableNV = _glCoverageModulationTableNV;
		gglCoverageOperationNV = _glCoverageOperationNV;
		gglCreatePerfQueryINTEL = _glCreatePerfQueryINTEL;
		gglCreateProgram = _glCreateProgram;
		gglCreateShader = _glCreateShader;
		gglCreateShaderProgramvEXT = _glCreateShaderProgramvEXT;
		gglCullFace = _glCullFace;
		gglDebugMessageCallbackKHR = _glDebugMessageCallbackKHR;
		gglDebugMessageControlKHR = _glDebugMessageControlKHR;
		gglDebugMessageInsertKHR = _glDebugMessageInsertKHR;
		gglDeleteBuffers = _glDeleteBuffers;
		gglDeleteFencesNV = _glDeleteFencesNV;
		gglDeleteFramebuffers = _glDeleteFramebuffers;
		gglDeletePathsNV = _glDeletePathsNV;
		gglDeletePerfMonitorsAMD = _glDeletePerfMonitorsAMD;
		gglDeletePerfQueryINTEL = _glDeletePerfQueryINTEL;
		gglDeleteProgram = _glDeleteProgram;
		gglDeleteProgramPipelinesEXT = _glDeleteProgramPipelinesEXT;
		gglDeleteQueries = _glDeleteQueries;
		gglDeleteQueriesEXT = _glDeleteQueriesEXT;
		gglDeleteRenderbuffers = _glDeleteRenderbuffers;
		gglDeleteSamplers = _glDeleteSamplers;
		gglDeleteShader = _glDeleteShader;
		gglDeleteSync = _glDeleteSync;
		gglDeleteSyncAPPLE = _glDeleteSyncAPPLE;
		gglDeleteTextures = _glDeleteTextures;
		gglDeleteTransformFeedbacks = _glDeleteTransformFeedbacks;
		gglDeleteVertexArrays = _glDeleteVertexArrays;
		gglDeleteVertexArraysOES = _glDeleteVertexArraysOES;
		gglDepthFunc = _glDepthFunc;
		gglDepthMask = _glDepthMask;
		gglDepthRangeArrayfvNV = _glDepthRangeArrayfvNV;
		gglDepthRangeArrayfvOES = _glDepthRangeArrayfvOES;
		gglDepthRangeIndexedfNV = _glDepthRangeIndexedfNV;
		gglDepthRangeIndexedfOES = _glDepthRangeIndexedfOES;
		gglDepthRangef = _glDepthRangef;
		gglDetachShader = _glDetachShader;
		gglDisable = _glDisable;
		gglDisableDriverControlQCOM = _glDisableDriverControlQCOM;
		gglDisableVertexAttribArray = _glDisableVertexAttribArray;
		gglDisableiEXT = _glDisableiEXT;
		gglDisableiNV = _glDisableiNV;
		gglDisableiOES = _glDisableiOES;
		gglDiscardFramebufferEXT = _glDiscardFramebufferEXT;
		gglDrawArrays = _glDrawArrays;
		gglDrawArraysInstanced = _glDrawArraysInstanced;
		gglDrawArraysInstancedANGLE = _glDrawArraysInstancedANGLE;
		gglDrawArraysInstancedBaseInstanceEXT = _glDrawArraysInstancedBaseInstanceEXT;
		gglDrawArraysInstancedEXT = _glDrawArraysInstancedEXT;
		gglDrawArraysInstancedNV = _glDrawArraysInstancedNV;
		gglDrawBuffers = _glDrawBuffers;
		gglDrawBuffersEXT = _glDrawBuffersEXT;
		gglDrawBuffersIndexedEXT = _glDrawBuffersIndexedEXT;
		gglDrawBuffersNV = _glDrawBuffersNV;
		gglDrawElements = _glDrawElements;
		gglDrawElementsBaseVertexEXT = _glDrawElementsBaseVertexEXT;
		gglDrawElementsBaseVertexOES = _glDrawElementsBaseVertexOES;
		gglDrawElementsInstanced = _glDrawElementsInstanced;
		gglDrawElementsInstancedANGLE = _glDrawElementsInstancedANGLE;
		gglDrawElementsInstancedBaseInstanceEXT = _glDrawElementsInstancedBaseInstanceEXT;
		gglDrawElementsInstancedBaseVertexBaseInstanceEXT = _glDrawElementsInstancedBaseVertexBaseInstanceEXT;
		gglDrawElementsInstancedBaseVertexEXT = _glDrawElementsInstancedBaseVertexEXT;
		gglDrawElementsInstancedBaseVertexOES = _glDrawElementsInstancedBaseVertexOES;
		gglDrawElementsInstancedEXT = _glDrawElementsInstancedEXT;
		gglDrawElementsInstancedNV = _glDrawElementsInstancedNV;
		gglDrawRangeElements = _glDrawRangeElements;
		gglDrawRangeElementsBaseVertexEXT = _glDrawRangeElementsBaseVertexEXT;
		gglDrawRangeElementsBaseVertexOES = _glDrawRangeElementsBaseVertexOES;
		gglDrawTransformFeedbackEXT = _glDrawTransformFeedbackEXT;
		gglDrawTransformFeedbackInstancedEXT = _glDrawTransformFeedbackInstancedEXT;
		gglEGLImageTargetRenderbufferStorageOES = _glEGLImageTargetRenderbufferStorageOES;
		gglEGLImageTargetTexture2DOES = _glEGLImageTargetTexture2DOES;
		gglEnable = _glEnable;
		gglEnableDriverControlQCOM = _glEnableDriverControlQCOM;
		gglEnableVertexAttribArray = _glEnableVertexAttribArray;
		gglEnableiEXT = _glEnableiEXT;
		gglEnableiNV = _glEnableiNV;
		gglEnableiOES = _glEnableiOES;
		gglEndConditionalRenderNV = _glEndConditionalRenderNV;
		gglEndPerfMonitorAMD = _glEndPerfMonitorAMD;
		gglEndPerfQueryINTEL = _glEndPerfQueryINTEL;
		gglEndQuery = _glEndQuery;
		gglEndQueryEXT = _glEndQueryEXT;
		gglEndTilingQCOM = _glEndTilingQCOM;
		gglEndTransformFeedback = _glEndTransformFeedback;
		gglExtGetBufferPointervQCOM = _glExtGetBufferPointervQCOM;
		gglExtGetBuffersQCOM = _glExtGetBuffersQCOM;
		gglExtGetFramebuffersQCOM = _glExtGetFramebuffersQCOM;
		gglExtGetProgramBinarySourceQCOM = _glExtGetProgramBinarySourceQCOM;
		gglExtGetProgramsQCOM = _glExtGetProgramsQCOM;
		gglExtGetRenderbuffersQCOM = _glExtGetRenderbuffersQCOM;
		gglExtGetShadersQCOM = _glExtGetShadersQCOM;
		gglExtGetTexLevelParameterivQCOM = _glExtGetTexLevelParameterivQCOM;
		gglExtGetTexSubImageQCOM = _glExtGetTexSubImageQCOM;
		gglExtGetTexturesQCOM = _glExtGetTexturesQCOM;
		gglExtIsProgramBinaryQCOM = _glExtIsProgramBinaryQCOM;
		gglExtTexObjectStateOverrideiQCOM = _glExtTexObjectStateOverrideiQCOM;
		gglFenceSync = _glFenceSync;
		gglFenceSyncAPPLE = _glFenceSyncAPPLE;
		gglFinish = _glFinish;
		gglFinishFenceNV = _glFinishFenceNV;
		gglFlush = _glFlush;
		gglFlushMappedBufferRange = _glFlushMappedBufferRange;
		gglFlushMappedBufferRangeEXT = _glFlushMappedBufferRangeEXT;
		gglFragmentCoverageColorNV = _glFragmentCoverageColorNV;
		gglFramebufferPixelLocalStorageSizeEXT = _glFramebufferPixelLocalStorageSizeEXT;
		gglFramebufferRenderbuffer = _glFramebufferRenderbuffer;
		gglFramebufferSampleLocationsfvNV = _glFramebufferSampleLocationsfvNV;
		gglFramebufferTexture2D = _glFramebufferTexture2D;
		gglFramebufferTexture2DDownsampleIMG = _glFramebufferTexture2DDownsampleIMG;
		gglFramebufferTexture2DMultisampleEXT = _glFramebufferTexture2DMultisampleEXT;
		gglFramebufferTexture2DMultisampleIMG = _glFramebufferTexture2DMultisampleIMG;
		gglFramebufferTexture3DOES = _glFramebufferTexture3DOES;
		gglFramebufferTextureEXT = _glFramebufferTextureEXT;
		gglFramebufferTextureLayer = _glFramebufferTextureLayer;
		gglFramebufferTextureLayerDownsampleIMG = _glFramebufferTextureLayerDownsampleIMG;
		gglFramebufferTextureMultisampleMultiviewOVR = _glFramebufferTextureMultisampleMultiviewOVR;
		gglFramebufferTextureMultiviewOVR = _glFramebufferTextureMultiviewOVR;
		gglFramebufferTextureOES = _glFramebufferTextureOES;
		gglFrontFace = _glFrontFace;
		gglGenBuffers = _glGenBuffers;
		gglGenFencesNV = _glGenFencesNV;
		gglGenFramebuffers = _glGenFramebuffers;
		gglGenPathsNV = _glGenPathsNV;
		gglGenPerfMonitorsAMD = _glGenPerfMonitorsAMD;
		gglGenProgramPipelinesEXT = _glGenProgramPipelinesEXT;
		gglGenQueries = _glGenQueries;
		gglGenQueriesEXT = _glGenQueriesEXT;
		gglGenRenderbuffers = _glGenRenderbuffers;
		gglGenSamplers = _glGenSamplers;
		gglGenTextures = _glGenTextures;
		gglGenTransformFeedbacks = _glGenTransformFeedbacks;
		gglGenVertexArrays = _glGenVertexArrays;
		gglGenVertexArraysOES = _glGenVertexArraysOES;
		gglGenerateMipmap = _glGenerateMipmap;
		gglGetActiveAttrib = _glGetActiveAttrib;
		gglGetActiveUniform = _glGetActiveUniform;
		gglGetActiveUniformBlockName = _glGetActiveUniformBlockName;
		gglGetActiveUniformBlockiv = _glGetActiveUniformBlockiv;
		gglGetActiveUniformsiv = _glGetActiveUniformsiv;
		gglGetAttachedShaders = _glGetAttachedShaders;
		gglGetAttribLocation = _glGetAttribLocation;
		gglGetBooleanv = _glGetBooleanv;
		gglGetBufferParameteri64v = _glGetBufferParameteri64v;
		gglGetBufferParameteriv = _glGetBufferParameteriv;
		gglGetBufferPointerv = _glGetBufferPointerv;
		gglGetBufferPointervOES = _glGetBufferPointervOES;
		gglGetCoverageModulationTableNV = _glGetCoverageModulationTableNV;
		gglGetDebugMessageLogKHR = _glGetDebugMessageLogKHR;
		gglGetDriverControlStringQCOM = _glGetDriverControlStringQCOM;
		gglGetDriverControlsQCOM = _glGetDriverControlsQCOM;
		gglGetError = _glGetError;
		gglGetFenceivNV = _glGetFenceivNV;
		gglGetFirstPerfQueryIdINTEL = _glGetFirstPerfQueryIdINTEL;
		gglGetFloati_vNV = _glGetFloati_vNV;
		gglGetFloati_vOES = _glGetFloati_vOES;
		gglGetFloatv = _glGetFloatv;
		gglGetFragDataIndexEXT = _glGetFragDataIndexEXT;
		gglGetFragDataLocation = _glGetFragDataLocation;
		gglGetFramebufferAttachmentParameteriv = _glGetFramebufferAttachmentParameteriv;
		gglGetFramebufferPixelLocalStorageSizeEXT = _glGetFramebufferPixelLocalStorageSizeEXT;
		gglGetGraphicsResetStatusEXT = _glGetGraphicsResetStatusEXT;
		gglGetGraphicsResetStatusKHR = _glGetGraphicsResetStatusKHR;
		gglGetImageHandleNV = _glGetImageHandleNV;
		gglGetInteger64i_v = _glGetInteger64i_v;
		gglGetInteger64v = _glGetInteger64v;
		gglGetInteger64vAPPLE = _glGetInteger64vAPPLE;
		gglGetIntegeri_v = _glGetIntegeri_v;
		gglGetIntegeri_vEXT = _glGetIntegeri_vEXT;
		gglGetIntegerv = _glGetIntegerv;
		gglGetInternalformatSampleivNV = _glGetInternalformatSampleivNV;
		gglGetInternalformativ = _glGetInternalformativ;
		gglGetNextPerfQueryIdINTEL = _glGetNextPerfQueryIdINTEL;
		gglGetObjectLabelEXT = _glGetObjectLabelEXT;
		gglGetObjectLabelKHR = _glGetObjectLabelKHR;
		gglGetObjectPtrLabelKHR = _glGetObjectPtrLabelKHR;
		gglGetPathCommandsNV = _glGetPathCommandsNV;
		gglGetPathCoordsNV = _glGetPathCoordsNV;
		gglGetPathDashArrayNV = _glGetPathDashArrayNV;
		gglGetPathLengthNV = _glGetPathLengthNV;
		gglGetPathMetricRangeNV = _glGetPathMetricRangeNV;
		gglGetPathMetricsNV = _glGetPathMetricsNV;
		gglGetPathParameterfvNV = _glGetPathParameterfvNV;
		gglGetPathParameterivNV = _glGetPathParameterivNV;
		gglGetPathSpacingNV = _glGetPathSpacingNV;
		gglGetPerfCounterInfoINTEL = _glGetPerfCounterInfoINTEL;
		gglGetPerfMonitorCounterDataAMD = _glGetPerfMonitorCounterDataAMD;
		gglGetPerfMonitorCounterInfoAMD = _glGetPerfMonitorCounterInfoAMD;
		gglGetPerfMonitorCounterStringAMD = _glGetPerfMonitorCounterStringAMD;
		gglGetPerfMonitorCountersAMD = _glGetPerfMonitorCountersAMD;
		gglGetPerfMonitorGroupStringAMD = _glGetPerfMonitorGroupStringAMD;
		gglGetPerfMonitorGroupsAMD = _glGetPerfMonitorGroupsAMD;
		gglGetPerfQueryDataINTEL = _glGetPerfQueryDataINTEL;
		gglGetPerfQueryIdByNameINTEL = _glGetPerfQueryIdByNameINTEL;
		gglGetPerfQueryInfoINTEL = _glGetPerfQueryInfoINTEL;
		gglGetPointervKHR = _glGetPointervKHR;
		gglGetProgramBinary = _glGetProgramBinary;
		gglGetProgramBinaryOES = _glGetProgramBinaryOES;
		gglGetProgramInfoLog = _glGetProgramInfoLog;
		gglGetProgramPipelineInfoLogEXT = _glGetProgramPipelineInfoLogEXT;
		gglGetProgramPipelineivEXT = _glGetProgramPipelineivEXT;
		gglGetProgramResourceLocationIndexEXT = _glGetProgramResourceLocationIndexEXT;
		gglGetProgramResourcefvNV = _glGetProgramResourcefvNV;
		gglGetProgramiv = _glGetProgramiv;
		gglGetQueryObjecti64vEXT = _glGetQueryObjecti64vEXT;
		gglGetQueryObjectivEXT = _glGetQueryObjectivEXT;
		gglGetQueryObjectui64vEXT = _glGetQueryObjectui64vEXT;
		gglGetQueryObjectuiv = _glGetQueryObjectuiv;
		gglGetQueryObjectuivEXT = _glGetQueryObjectuivEXT;
		gglGetQueryiv = _glGetQueryiv;
		gglGetQueryivEXT = _glGetQueryivEXT;
		gglGetRenderbufferParameteriv = _glGetRenderbufferParameteriv;
		gglGetSamplerParameterIivEXT = _glGetSamplerParameterIivEXT;
		gglGetSamplerParameterIivOES = _glGetSamplerParameterIivOES;
		gglGetSamplerParameterIuivEXT = _glGetSamplerParameterIuivEXT;
		gglGetSamplerParameterIuivOES = _glGetSamplerParameterIuivOES;
		gglGetSamplerParameterfv = _glGetSamplerParameterfv;
		gglGetSamplerParameteriv = _glGetSamplerParameteriv;
		gglGetShaderInfoLog = _glGetShaderInfoLog;
		gglGetShaderPrecisionFormat = _glGetShaderPrecisionFormat;
		gglGetShaderSource = _glGetShaderSource;
		gglGetShaderiv = _glGetShaderiv;
		gglGetString = _glGetString;
		gglGetStringi = _glGetStringi;
		gglGetSynciv = _glGetSynciv;
		gglGetSyncivAPPLE = _glGetSyncivAPPLE;
		gglGetTexParameterIivEXT = _glGetTexParameterIivEXT;
		gglGetTexParameterIivOES = _glGetTexParameterIivOES;
		gglGetTexParameterIuivEXT = _glGetTexParameterIuivEXT;
		gglGetTexParameterIuivOES = _glGetTexParameterIuivOES;
		gglGetTexParameterfv = _glGetTexParameterfv;
		gglGetTexParameteriv = _glGetTexParameteriv;
		gglGetTextureHandleIMG = _glGetTextureHandleIMG;
		gglGetTextureHandleNV = _glGetTextureHandleNV;
		gglGetTextureSamplerHandleIMG = _glGetTextureSamplerHandleIMG;
		gglGetTextureSamplerHandleNV = _glGetTextureSamplerHandleNV;
		gglGetTransformFeedbackVarying = _glGetTransformFeedbackVarying;
		gglGetTranslatedShaderSourceANGLE = _glGetTranslatedShaderSourceANGLE;
		gglGetUniformBlockIndex = _glGetUniformBlockIndex;
		gglGetUniformIndices = _glGetUniformIndices;
		gglGetUniformLocation = _glGetUniformLocation;
		gglGetUniformfv = _glGetUniformfv;
		gglGetUniformi64vNV = _glGetUniformi64vNV;
		gglGetUniformiv = _glGetUniformiv;
		gglGetUniformuiv = _glGetUniformuiv;
		gglGetVertexAttribIiv = _glGetVertexAttribIiv;
		gglGetVertexAttribIuiv = _glGetVertexAttribIuiv;
		gglGetVertexAttribPointerv = _glGetVertexAttribPointerv;
		gglGetVertexAttribfv = _glGetVertexAttribfv;
		gglGetVertexAttribiv = _glGetVertexAttribiv;
		gglGetnUniformfvEXT = _glGetnUniformfvEXT;
		gglGetnUniformfvKHR = _glGetnUniformfvKHR;
		gglGetnUniformivEXT = _glGetnUniformivEXT;
		gglGetnUniformivKHR = _glGetnUniformivKHR;
		gglGetnUniformuivKHR = _glGetnUniformuivKHR;
		gglHint = _glHint;
		gglInsertEventMarkerEXT = _glInsertEventMarkerEXT;
		gglInterpolatePathsNV = _glInterpolatePathsNV;
		gglInvalidateFramebuffer = _glInvalidateFramebuffer;
		gglInvalidateSubFramebuffer = _glInvalidateSubFramebuffer;
		gglIsBuffer = _glIsBuffer;
		gglIsEnabled = _glIsEnabled;
		gglIsEnablediEXT = _glIsEnablediEXT;
		gglIsEnablediNV = _glIsEnablediNV;
		gglIsEnablediOES = _glIsEnablediOES;
		gglIsFenceNV = _glIsFenceNV;
		gglIsFramebuffer = _glIsFramebuffer;
		gglIsImageHandleResidentNV = _glIsImageHandleResidentNV;
		gglIsPathNV = _glIsPathNV;
		gglIsPointInFillPathNV = _glIsPointInFillPathNV;
		gglIsPointInStrokePathNV = _glIsPointInStrokePathNV;
		gglIsProgram = _glIsProgram;
		gglIsProgramPipelineEXT = _glIsProgramPipelineEXT;
		gglIsQuery = _glIsQuery;
		gglIsQueryEXT = _glIsQueryEXT;
		gglIsRenderbuffer = _glIsRenderbuffer;
		gglIsSampler = _glIsSampler;
		gglIsShader = _glIsShader;
		gglIsSync = _glIsSync;
		gglIsSyncAPPLE = _glIsSyncAPPLE;
		gglIsTexture = _glIsTexture;
		gglIsTextureHandleResidentNV = _glIsTextureHandleResidentNV;
		gglIsTransformFeedback = _glIsTransformFeedback;
		gglIsVertexArray = _glIsVertexArray;
		gglIsVertexArrayOES = _glIsVertexArrayOES;
		gglLabelObjectEXT = _glLabelObjectEXT;
		gglLineWidth = _glLineWidth;
		gglLinkProgram = _glLinkProgram;
		gglMakeImageHandleNonResidentNV = _glMakeImageHandleNonResidentNV;
		gglMakeImageHandleResidentNV = _glMakeImageHandleResidentNV;
		gglMakeTextureHandleNonResidentNV = _glMakeTextureHandleNonResidentNV;
		gglMakeTextureHandleResidentNV = _glMakeTextureHandleResidentNV;
		gglMapBufferOES = _glMapBufferOES;
		gglMapBufferRange = _glMapBufferRange;
		gglMapBufferRangeEXT = _glMapBufferRangeEXT;
		gglMatrixLoad3x2fNV = _glMatrixLoad3x2fNV;
		gglMatrixLoad3x3fNV = _glMatrixLoad3x3fNV;
		gglMatrixLoadTranspose3x3fNV = _glMatrixLoadTranspose3x3fNV;
		gglMatrixMult3x2fNV = _glMatrixMult3x2fNV;
		gglMatrixMult3x3fNV = _glMatrixMult3x3fNV;
		gglMatrixMultTranspose3x3fNV = _glMatrixMultTranspose3x3fNV;
		gglMinSampleShadingOES = _glMinSampleShadingOES;
		gglMultiDrawArraysEXT = _glMultiDrawArraysEXT;
		gglMultiDrawArraysIndirectEXT = _glMultiDrawArraysIndirectEXT;
		gglMultiDrawElementsBaseVertexEXT = _glMultiDrawElementsBaseVertexEXT;
		gglMultiDrawElementsBaseVertexOES = _glMultiDrawElementsBaseVertexOES;
		gglMultiDrawElementsEXT = _glMultiDrawElementsEXT;
		gglMultiDrawElementsIndirectEXT = _glMultiDrawElementsIndirectEXT;
		gglNamedFramebufferSampleLocationsfvNV = _glNamedFramebufferSampleLocationsfvNV;
		gglObjectLabelKHR = _glObjectLabelKHR;
		gglObjectPtrLabelKHR = _glObjectPtrLabelKHR;
		gglPatchParameteriEXT = _glPatchParameteriEXT;
		gglPatchParameteriOES = _glPatchParameteriOES;
		gglPathCommandsNV = _glPathCommandsNV;
		gglPathCoordsNV = _glPathCoordsNV;
		gglPathCoverDepthFuncNV = _glPathCoverDepthFuncNV;
		gglPathDashArrayNV = _glPathDashArrayNV;
		gglPathGlyphIndexArrayNV = _glPathGlyphIndexArrayNV;
		gglPathGlyphIndexRangeNV = _glPathGlyphIndexRangeNV;
		gglPathGlyphRangeNV = _glPathGlyphRangeNV;
		gglPathGlyphsNV = _glPathGlyphsNV;
		gglPathMemoryGlyphIndexArrayNV = _glPathMemoryGlyphIndexArrayNV;
		gglPathParameterfNV = _glPathParameterfNV;
		gglPathParameterfvNV = _glPathParameterfvNV;
		gglPathParameteriNV = _glPathParameteriNV;
		gglPathParameterivNV = _glPathParameterivNV;
		gglPathStencilDepthOffsetNV = _glPathStencilDepthOffsetNV;
		gglPathStencilFuncNV = _glPathStencilFuncNV;
		gglPathStringNV = _glPathStringNV;
		gglPathSubCommandsNV = _glPathSubCommandsNV;
		gglPathSubCoordsNV = _glPathSubCoordsNV;
		gglPauseTransformFeedback = _glPauseTransformFeedback;
		gglPixelStorei = _glPixelStorei;
		gglPointAlongPathNV = _glPointAlongPathNV;
		gglPolygonModeNV = _glPolygonModeNV;
		gglPolygonOffset = _glPolygonOffset;
		gglPolygonOffsetClampEXT = _glPolygonOffsetClampEXT;
		gglPopDebugGroupKHR = _glPopDebugGroupKHR;
		gglPopGroupMarkerEXT = _glPopGroupMarkerEXT;
		gglPrimitiveBoundingBoxEXT = _glPrimitiveBoundingBoxEXT;
		gglPrimitiveBoundingBoxOES = _glPrimitiveBoundingBoxOES;
		gglProgramBinary = _glProgramBinary;
		gglProgramBinaryOES = _glProgramBinaryOES;
		gglProgramParameteri = _glProgramParameteri;
		gglProgramParameteriEXT = _glProgramParameteriEXT;
		gglProgramPathFragmentInputGenNV = _glProgramPathFragmentInputGenNV;
		gglProgramUniform1fEXT = _glProgramUniform1fEXT;
		gglProgramUniform1fvEXT = _glProgramUniform1fvEXT;
		gglProgramUniform1i64NV = _glProgramUniform1i64NV;
		gglProgramUniform1i64vNV = _glProgramUniform1i64vNV;
		gglProgramUniform1iEXT = _glProgramUniform1iEXT;
		gglProgramUniform1ivEXT = _glProgramUniform1ivEXT;
		gglProgramUniform1ui64NV = _glProgramUniform1ui64NV;
		gglProgramUniform1ui64vNV = _glProgramUniform1ui64vNV;
		gglProgramUniform1uiEXT = _glProgramUniform1uiEXT;
		gglProgramUniform1uivEXT = _glProgramUniform1uivEXT;
		gglProgramUniform2fEXT = _glProgramUniform2fEXT;
		gglProgramUniform2fvEXT = _glProgramUniform2fvEXT;
		gglProgramUniform2i64NV = _glProgramUniform2i64NV;
		gglProgramUniform2i64vNV = _glProgramUniform2i64vNV;
		gglProgramUniform2iEXT = _glProgramUniform2iEXT;
		gglProgramUniform2ivEXT = _glProgramUniform2ivEXT;
		gglProgramUniform2ui64NV = _glProgramUniform2ui64NV;
		gglProgramUniform2ui64vNV = _glProgramUniform2ui64vNV;
		gglProgramUniform2uiEXT = _glProgramUniform2uiEXT;
		gglProgramUniform2uivEXT = _glProgramUniform2uivEXT;
		gglProgramUniform3fEXT = _glProgramUniform3fEXT;
		gglProgramUniform3fvEXT = _glProgramUniform3fvEXT;
		gglProgramUniform3i64NV = _glProgramUniform3i64NV;
		gglProgramUniform3i64vNV = _glProgramUniform3i64vNV;
		gglProgramUniform3iEXT = _glProgramUniform3iEXT;
		gglProgramUniform3ivEXT = _glProgramUniform3ivEXT;
		gglProgramUniform3ui64NV = _glProgramUniform3ui64NV;
		gglProgramUniform3ui64vNV = _glProgramUniform3ui64vNV;
		gglProgramUniform3uiEXT = _glProgramUniform3uiEXT;
		gglProgramUniform3uivEXT = _glProgramUniform3uivEXT;
		gglProgramUniform4fEXT = _glProgramUniform4fEXT;
		gglProgramUniform4fvEXT = _glProgramUniform4fvEXT;
		gglProgramUniform4i64NV = _glProgramUniform4i64NV;
		gglProgramUniform4i64vNV = _glProgramUniform4i64vNV;
		gglProgramUniform4iEXT = _glProgramUniform4iEXT;
		gglProgramUniform4ivEXT = _glProgramUniform4ivEXT;
		gglProgramUniform4ui64NV = _glProgramUniform4ui64NV;
		gglProgramUniform4ui64vNV = _glProgramUniform4ui64vNV;
		gglProgramUniform4uiEXT = _glProgramUniform4uiEXT;
		gglProgramUniform4uivEXT = _glProgramUniform4uivEXT;
		gglProgramUniformHandleui64IMG = _glProgramUniformHandleui64IMG;
		gglProgramUniformHandleui64NV = _glProgramUniformHandleui64NV;
		gglProgramUniformHandleui64vIMG = _glProgramUniformHandleui64vIMG;
		gglProgramUniformHandleui64vNV = _glProgramUniformHandleui64vNV;
		gglProgramUniformMatrix2fvEXT = _glProgramUniformMatrix2fvEXT;
		gglProgramUniformMatrix2x3fvEXT = _glProgramUniformMatrix2x3fvEXT;
		gglProgramUniformMatrix2x4fvEXT = _glProgramUniformMatrix2x4fvEXT;
		gglProgramUniformMatrix3fvEXT = _glProgramUniformMatrix3fvEXT;
		gglProgramUniformMatrix3x2fvEXT = _glProgramUniformMatrix3x2fvEXT;
		gglProgramUniformMatrix3x4fvEXT = _glProgramUniformMatrix3x4fvEXT;
		gglProgramUniformMatrix4fvEXT = _glProgramUniformMatrix4fvEXT;
		gglProgramUniformMatrix4x2fvEXT = _glProgramUniformMatrix4x2fvEXT;
		gglProgramUniformMatrix4x3fvEXT = _glProgramUniformMatrix4x3fvEXT;
		gglPushDebugGroupKHR = _glPushDebugGroupKHR;
		gglPushGroupMarkerEXT = _glPushGroupMarkerEXT;
		gglQueryCounterEXT = _glQueryCounterEXT;
		gglRasterSamplesEXT = _glRasterSamplesEXT;
		gglReadBuffer = _glReadBuffer;
		gglReadBufferIndexedEXT = _glReadBufferIndexedEXT;
		gglReadBufferNV = _glReadBufferNV;
		gglReadPixels = _glReadPixels;
		gglReadnPixelsEXT = _glReadnPixelsEXT;
		gglReadnPixelsKHR = _glReadnPixelsKHR;
		gglReleaseShaderCompiler = _glReleaseShaderCompiler;
		gglRenderbufferStorage = _glRenderbufferStorage;
		gglRenderbufferStorageMultisample = _glRenderbufferStorageMultisample;
		gglRenderbufferStorageMultisampleANGLE = _glRenderbufferStorageMultisampleANGLE;
		gglRenderbufferStorageMultisampleAPPLE = _glRenderbufferStorageMultisampleAPPLE;
		gglRenderbufferStorageMultisampleEXT = _glRenderbufferStorageMultisampleEXT;
		gglRenderbufferStorageMultisampleIMG = _glRenderbufferStorageMultisampleIMG;
		gglRenderbufferStorageMultisampleNV = _glRenderbufferStorageMultisampleNV;
		gglResolveDepthValuesNV = _glResolveDepthValuesNV;
		gglResolveMultisampleFramebufferAPPLE = _glResolveMultisampleFramebufferAPPLE;
		gglResumeTransformFeedback = _glResumeTransformFeedback;
		gglSampleCoverage = _glSampleCoverage;
		gglSamplerParameterIivEXT = _glSamplerParameterIivEXT;
		gglSamplerParameterIivOES = _glSamplerParameterIivOES;
		gglSamplerParameterIuivEXT = _glSamplerParameterIuivEXT;
		gglSamplerParameterIuivOES = _glSamplerParameterIuivOES;
		gglSamplerParameterf = _glSamplerParameterf;
		gglSamplerParameterfv = _glSamplerParameterfv;
		gglSamplerParameteri = _glSamplerParameteri;
		gglSamplerParameteriv = _glSamplerParameteriv;
		gglScissor = _glScissor;
		gglScissorArrayvNV = _glScissorArrayvNV;
		gglScissorArrayvOES = _glScissorArrayvOES;
		gglScissorIndexedNV = _glScissorIndexedNV;
		gglScissorIndexedOES = _glScissorIndexedOES;
		gglScissorIndexedvNV = _glScissorIndexedvNV;
		gglScissorIndexedvOES = _glScissorIndexedvOES;
		gglSelectPerfMonitorCountersAMD = _glSelectPerfMonitorCountersAMD;
		gglSetFenceNV = _glSetFenceNV;
		gglShaderBinary = _glShaderBinary;
		gglShaderSource = _glShaderSource;
		gglStartTilingQCOM = _glStartTilingQCOM;
		gglStencilFillPathInstancedNV = _glStencilFillPathInstancedNV;
		gglStencilFillPathNV = _glStencilFillPathNV;
		gglStencilFunc = _glStencilFunc;
		gglStencilFuncSeparate = _glStencilFuncSeparate;
		gglStencilMask = _glStencilMask;
		gglStencilMaskSeparate = _glStencilMaskSeparate;
		gglStencilOp = _glStencilOp;
		gglStencilOpSeparate = _glStencilOpSeparate;
		gglStencilStrokePathInstancedNV = _glStencilStrokePathInstancedNV;
		gglStencilStrokePathNV = _glStencilStrokePathNV;
		gglStencilThenCoverFillPathInstancedNV = _glStencilThenCoverFillPathInstancedNV;
		gglStencilThenCoverFillPathNV = _glStencilThenCoverFillPathNV;
		gglStencilThenCoverStrokePathInstancedNV = _glStencilThenCoverStrokePathInstancedNV;
		gglStencilThenCoverStrokePathNV = _glStencilThenCoverStrokePathNV;
		gglSubpixelPrecisionBiasNV = _glSubpixelPrecisionBiasNV;
		gglTestFenceNV = _glTestFenceNV;
		gglTexBufferEXT = _glTexBufferEXT;
		gglTexBufferOES = _glTexBufferOES;
		gglTexBufferRangeEXT = _glTexBufferRangeEXT;
		gglTexBufferRangeOES = _glTexBufferRangeOES;
		gglTexImage2D = _glTexImage2D;
		gglTexImage3D = _glTexImage3D;
		gglTexImage3DOES = _glTexImage3DOES;
		gglTexPageCommitmentEXT = _glTexPageCommitmentEXT;
		gglTexParameterIivEXT = _glTexParameterIivEXT;
		gglTexParameterIivOES = _glTexParameterIivOES;
		gglTexParameterIuivEXT = _glTexParameterIuivEXT;
		gglTexParameterIuivOES = _glTexParameterIuivOES;
		gglTexParameterf = _glTexParameterf;
		gglTexParameterfv = _glTexParameterfv;
		gglTexParameteri = _glTexParameteri;
		gglTexParameteriv = _glTexParameteriv;
		gglTexStorage1DEXT = _glTexStorage1DEXT;
		gglTexStorage2D = _glTexStorage2D;
		gglTexStorage2DEXT = _glTexStorage2DEXT;
		gglTexStorage3D = _glTexStorage3D;
		gglTexStorage3DEXT = _glTexStorage3DEXT;
		gglTexStorage3DMultisampleOES = _glTexStorage3DMultisampleOES;
		gglTexSubImage2D = _glTexSubImage2D;
		gglTexSubImage3D = _glTexSubImage3D;
		gglTexSubImage3DOES = _glTexSubImage3DOES;
		gglTextureStorage1DEXT = _glTextureStorage1DEXT;
		gglTextureStorage2DEXT = _glTextureStorage2DEXT;
		gglTextureStorage3DEXT = _glTextureStorage3DEXT;
		gglTextureViewEXT = _glTextureViewEXT;
		gglTextureViewOES = _glTextureViewOES;
		gglTransformFeedbackVaryings = _glTransformFeedbackVaryings;
		gglTransformPathNV = _glTransformPathNV;
		gglUniform1f = _glUniform1f;
		gglUniform1fv = _glUniform1fv;
		gglUniform1i = _glUniform1i;
		gglUniform1i64NV = _glUniform1i64NV;
		gglUniform1i64vNV = _glUniform1i64vNV;
		gglUniform1iv = _glUniform1iv;
		gglUniform1ui = _glUniform1ui;
		gglUniform1ui64NV = _glUniform1ui64NV;
		gglUniform1ui64vNV = _glUniform1ui64vNV;
		gglUniform1uiv = _glUniform1uiv;
		gglUniform2f = _glUniform2f;
		gglUniform2fv = _glUniform2fv;
		gglUniform2i = _glUniform2i;
		gglUniform2i64NV = _glUniform2i64NV;
		gglUniform2i64vNV = _glUniform2i64vNV;
		gglUniform2iv = _glUniform2iv;
		gglUniform2ui = _glUniform2ui;
		gglUniform2ui64NV = _glUniform2ui64NV;
		gglUniform2ui64vNV = _glUniform2ui64vNV;
		gglUniform2uiv = _glUniform2uiv;
		gglUniform3f = _glUniform3f;
		gglUniform3fv = _glUniform3fv;
		gglUniform3i = _glUniform3i;
		gglUniform3i64NV = _glUniform3i64NV;
		gglUniform3i64vNV = _glUniform3i64vNV;
		gglUniform3iv = _glUniform3iv;
		gglUniform3ui = _glUniform3ui;
		gglUniform3ui64NV = _glUniform3ui64NV;
		gglUniform3ui64vNV = _glUniform3ui64vNV;
		gglUniform3uiv = _glUniform3uiv;
		gglUniform4f = _glUniform4f;
		gglUniform4fv = _glUniform4fv;
		gglUniform4i = _glUniform4i;
		gglUniform4i64NV = _glUniform4i64NV;
		gglUniform4i64vNV = _glUniform4i64vNV;
		gglUniform4iv = _glUniform4iv;
		gglUniform4ui = _glUniform4ui;
		gglUniform4ui64NV = _glUniform4ui64NV;
		gglUniform4ui64vNV = _glUniform4ui64vNV;
		gglUniform4uiv = _glUniform4uiv;
		gglUniformBlockBinding = _glUniformBlockBinding;
		gglUniformHandleui64IMG = _glUniformHandleui64IMG;
		gglUniformHandleui64NV = _glUniformHandleui64NV;
		gglUniformHandleui64vIMG = _glUniformHandleui64vIMG;
		gglUniformHandleui64vNV = _glUniformHandleui64vNV;
		gglUniformMatrix2fv = _glUniformMatrix2fv;
		gglUniformMatrix2x3fv = _glUniformMatrix2x3fv;
		gglUniformMatrix2x3fvNV = _glUniformMatrix2x3fvNV;
		gglUniformMatrix2x4fv = _glUniformMatrix2x4fv;
		gglUniformMatrix2x4fvNV = _glUniformMatrix2x4fvNV;
		gglUniformMatrix3fv = _glUniformMatrix3fv;
		gglUniformMatrix3x2fv = _glUniformMatrix3x2fv;
		gglUniformMatrix3x2fvNV = _glUniformMatrix3x2fvNV;
		gglUniformMatrix3x4fv = _glUniformMatrix3x4fv;
		gglUniformMatrix3x4fvNV = _glUniformMatrix3x4fvNV;
		gglUniformMatrix4fv = _glUniformMatrix4fv;
		gglUniformMatrix4x2fv = _glUniformMatrix4x2fv;
		gglUniformMatrix4x2fvNV = _glUniformMatrix4x2fvNV;
		gglUniformMatrix4x3fv = _glUniformMatrix4x3fv;
		gglUniformMatrix4x3fvNV = _glUniformMatrix4x3fvNV;
		gglUnmapBuffer = _glUnmapBuffer;
		gglUnmapBufferOES = _glUnmapBufferOES;
		gglUseProgram = _glUseProgram;
		gglUseProgramStagesEXT = _glUseProgramStagesEXT;
		gglValidateProgram = _glValidateProgram;
		gglValidateProgramPipelineEXT = _glValidateProgramPipelineEXT;
		gglVertexAttrib1f = _glVertexAttrib1f;
		gglVertexAttrib1fv = _glVertexAttrib1fv;
		gglVertexAttrib2f = _glVertexAttrib2f;
		gglVertexAttrib2fv = _glVertexAttrib2fv;
		gglVertexAttrib3f = _glVertexAttrib3f;
		gglVertexAttrib3fv = _glVertexAttrib3fv;
		gglVertexAttrib4f = _glVertexAttrib4f;
		gglVertexAttrib4fv = _glVertexAttrib4fv;
		gglVertexAttribDivisor = _glVertexAttribDivisor;
		gglVertexAttribDivisorANGLE = _glVertexAttribDivisorANGLE;
		gglVertexAttribDivisorEXT = _glVertexAttribDivisorEXT;
		gglVertexAttribDivisorNV = _glVertexAttribDivisorNV;
		gglVertexAttribI4i = _glVertexAttribI4i;
		gglVertexAttribI4iv = _glVertexAttribI4iv;
		gglVertexAttribI4ui = _glVertexAttribI4ui;
		gglVertexAttribI4uiv = _glVertexAttribI4uiv;
		gglVertexAttribIPointer = _glVertexAttribIPointer;
		gglVertexAttribPointer = _glVertexAttribPointer;
		gglViewport = _glViewport;
		gglViewportArrayvNV = _glViewportArrayvNV;
		gglViewportArrayvOES = _glViewportArrayvOES;
		gglViewportIndexedfOES = _glViewportIndexedfOES;
		gglViewportIndexedfNV = _glViewportIndexedfNV;
		gglViewportIndexedfvOES = _glViewportIndexedfvOES;
		gglViewportIndexedfvNV = _glViewportIndexedfvNV;
		gglViewportSwizzleNV = _glViewportSwizzleNV;
		gglWaitSync = _glWaitSync;
		gglWaitSyncAPPLE = _glWaitSyncAPPLE;
		gglWeightPathsNV = _glWeightPathsNV;
		gglWindowRectanglesEXT = _glWindowRectanglesEXT;
	} else {
		gglActiveShaderProgramEXT = d_glActiveShaderProgramEXT;
		gglActiveTexture = d_glActiveTexture;
		gglAlphaFuncQCOM = d_glAlphaFuncQCOM;
		gglApplyFramebufferAttachmentCMAAINTEL = d_glApplyFramebufferAttachmentCMAAINTEL;
		gglAttachShader = d_glAttachShader;
		gglBeginConditionalRenderNV = d_glBeginConditionalRenderNV;
		gglBeginPerfMonitorAMD = d_glBeginPerfMonitorAMD;
		gglBeginPerfQueryINTEL = d_glBeginPerfQueryINTEL;
		gglBeginQuery = d_glBeginQuery;
		gglBeginQueryEXT = d_glBeginQueryEXT;
		gglBeginTransformFeedback = d_glBeginTransformFeedback;
		gglBindAttribLocation = d_glBindAttribLocation;
		gglBindBuffer = d_glBindBuffer;
		gglBindBufferBase = d_glBindBufferBase;
		gglBindBufferRange = d_glBindBufferRange;
		gglBindFragDataLocationEXT = d_glBindFragDataLocationEXT;
		gglBindFragDataLocationIndexedEXT = d_glBindFragDataLocationIndexedEXT;
		gglBindFramebuffer = d_glBindFramebuffer;
		gglBindProgramPipelineEXT = d_glBindProgramPipelineEXT;
		gglBindRenderbuffer = d_glBindRenderbuffer;
		gglBindSampler = d_glBindSampler;
		gglBindTexture = d_glBindTexture;
		gglBindTransformFeedback = d_glBindTransformFeedback;
		gglBindVertexArray = d_glBindVertexArray;
		gglBindVertexArrayOES = d_glBindVertexArrayOES;
		gglBlendBarrierKHR = d_glBlendBarrierKHR;
		gglBlendBarrierNV = d_glBlendBarrierNV;
		gglBlendColor = d_glBlendColor;
		gglBlendEquation = d_glBlendEquation;
		gglBlendEquationSeparate = d_glBlendEquationSeparate;
		gglBlendEquationSeparateiEXT = d_glBlendEquationSeparateiEXT;
		gglBlendEquationSeparateiOES = d_glBlendEquationSeparateiOES;
		gglBlendEquationiEXT = d_glBlendEquationiEXT;
		gglBlendEquationiOES = d_glBlendEquationiOES;
		gglBlendFunc = d_glBlendFunc;
		gglBlendFuncSeparate = d_glBlendFuncSeparate;
		gglBlendFuncSeparateiEXT = d_glBlendFuncSeparateiEXT;
		gglBlendFuncSeparateiOES = d_glBlendFuncSeparateiOES;
		gglBlendFunciEXT = d_glBlendFunciEXT;
		gglBlendFunciOES = d_glBlendFunciOES;
		gglBlendParameteriNV = d_glBlendParameteriNV;
		gglBlitFramebuffer = d_glBlitFramebuffer;
		gglBlitFramebufferANGLE = d_glBlitFramebufferANGLE;
		gglBlitFramebufferNV = d_glBlitFramebufferNV;
		gglBufferData = d_glBufferData;
		gglBufferStorageEXT = d_glBufferStorageEXT;
		gglBufferSubData = d_glBufferSubData;
		gglCheckFramebufferStatus = d_glCheckFramebufferStatus;
		gglClear = d_glClear;
		gglClearBufferfi = d_glClearBufferfi;
		gglClearBufferfv = d_glClearBufferfv;
		gglClearBufferiv = d_glClearBufferiv;
		gglClearBufferuiv = d_glClearBufferuiv;
		gglClearColor = d_glClearColor;
		gglClearDepthf = d_glClearDepthf;
		gglClearPixelLocalStorageuiEXT = d_glClearPixelLocalStorageuiEXT;
		gglClearStencil = d_glClearStencil;
		gglClearTexImageEXT = d_glClearTexImageEXT;
		gglClearTexSubImageEXT = d_glClearTexSubImageEXT;
		gglClientWaitSync = d_glClientWaitSync;
		gglClientWaitSyncAPPLE = d_glClientWaitSyncAPPLE;
		gglColorMask = d_glColorMask;
		gglColorMaskiEXT = d_glColorMaskiEXT;
		gglColorMaskiOES = d_glColorMaskiOES;
		gglCompileShader = d_glCompileShader;
		gglCompressedTexImage2D = d_glCompressedTexImage2D;
		gglCompressedTexImage3D = d_glCompressedTexImage3D;
		gglCompressedTexImage3DOES = d_glCompressedTexImage3DOES;
		gglCompressedTexSubImage2D = d_glCompressedTexSubImage2D;
		gglCompressedTexSubImage3D = d_glCompressedTexSubImage3D;
		gglCompressedTexSubImage3DOES = d_glCompressedTexSubImage3DOES;
		gglConservativeRasterParameteriNV = d_glConservativeRasterParameteriNV;
		gglCopyBufferSubData = d_glCopyBufferSubData;
		gglCopyBufferSubDataNV = d_glCopyBufferSubDataNV;
		gglCopyImageSubDataEXT = d_glCopyImageSubDataEXT;
		gglCopyImageSubDataOES = d_glCopyImageSubDataOES;
		gglCopyPathNV = d_glCopyPathNV;
		gglCopyTexImage2D = d_glCopyTexImage2D;
		gglCopyTexSubImage2D = d_glCopyTexSubImage2D;
		gglCopyTexSubImage3D = d_glCopyTexSubImage3D;
		gglCopyTexSubImage3DOES = d_glCopyTexSubImage3DOES;
		gglCopyTextureLevelsAPPLE = d_glCopyTextureLevelsAPPLE;
		gglCoverFillPathInstancedNV = d_glCoverFillPathInstancedNV;
		gglCoverFillPathNV = d_glCoverFillPathNV;
		gglCoverStrokePathInstancedNV = d_glCoverStrokePathInstancedNV;
		gglCoverStrokePathNV = d_glCoverStrokePathNV;
		gglCoverageMaskNV = d_glCoverageMaskNV;
		gglCoverageModulationNV = d_glCoverageModulationNV;
		gglCoverageModulationTableNV = d_glCoverageModulationTableNV;
		gglCoverageOperationNV = d_glCoverageOperationNV;
		gglCreatePerfQueryINTEL = d_glCreatePerfQueryINTEL;
		gglCreateProgram = d_glCreateProgram;
		gglCreateShader = d_glCreateShader;
		gglCreateShaderProgramvEXT = d_glCreateShaderProgramvEXT;
		gglCullFace = d_glCullFace;
		gglDebugMessageCallbackKHR = d_glDebugMessageCallbackKHR;
		gglDebugMessageControlKHR = d_glDebugMessageControlKHR;
		gglDebugMessageInsertKHR = d_glDebugMessageInsertKHR;
		gglDeleteBuffers = d_glDeleteBuffers;
		gglDeleteFencesNV = d_glDeleteFencesNV;
		gglDeleteFramebuffers = d_glDeleteFramebuffers;
		gglDeletePathsNV = d_glDeletePathsNV;
		gglDeletePerfMonitorsAMD = d_glDeletePerfMonitorsAMD;
		gglDeletePerfQueryINTEL = d_glDeletePerfQueryINTEL;
		gglDeleteProgram = d_glDeleteProgram;
		gglDeleteProgramPipelinesEXT = d_glDeleteProgramPipelinesEXT;
		gglDeleteQueries = d_glDeleteQueries;
		gglDeleteQueriesEXT = d_glDeleteQueriesEXT;
		gglDeleteRenderbuffers = d_glDeleteRenderbuffers;
		gglDeleteSamplers = d_glDeleteSamplers;
		gglDeleteShader = d_glDeleteShader;
		gglDeleteSync = d_glDeleteSync;
		gglDeleteSyncAPPLE = d_glDeleteSyncAPPLE;
		gglDeleteTextures = d_glDeleteTextures;
		gglDeleteTransformFeedbacks = d_glDeleteTransformFeedbacks;
		gglDeleteVertexArrays = d_glDeleteVertexArrays;
		gglDeleteVertexArraysOES = d_glDeleteVertexArraysOES;
		gglDepthFunc = d_glDepthFunc;
		gglDepthMask = d_glDepthMask;
		gglDepthRangeArrayfvNV = d_glDepthRangeArrayfvNV;
		gglDepthRangeArrayfvOES = d_glDepthRangeArrayfvOES;
		gglDepthRangeIndexedfNV = d_glDepthRangeIndexedfNV;
		gglDepthRangeIndexedfOES = d_glDepthRangeIndexedfOES;
		gglDepthRangef = d_glDepthRangef;
		gglDetachShader = d_glDetachShader;
		gglDisable = d_glDisable;
		gglDisableDriverControlQCOM = d_glDisableDriverControlQCOM;
		gglDisableVertexAttribArray = d_glDisableVertexAttribArray;
		gglDisableiEXT = d_glDisableiEXT;
		gglDisableiNV = d_glDisableiNV;
		gglDisableiOES = d_glDisableiOES;
		gglDiscardFramebufferEXT = d_glDiscardFramebufferEXT;
		gglDrawArrays = d_glDrawArrays;
		gglDrawArraysInstanced = d_glDrawArraysInstanced;
		gglDrawArraysInstancedANGLE = d_glDrawArraysInstancedANGLE;
		gglDrawArraysInstancedBaseInstanceEXT = d_glDrawArraysInstancedBaseInstanceEXT;
		gglDrawArraysInstancedEXT = d_glDrawArraysInstancedEXT;
		gglDrawArraysInstancedNV = d_glDrawArraysInstancedNV;
		gglDrawBuffers = d_glDrawBuffers;
		gglDrawBuffersEXT = d_glDrawBuffersEXT;
		gglDrawBuffersIndexedEXT = d_glDrawBuffersIndexedEXT;
		gglDrawBuffersNV = d_glDrawBuffersNV;
		gglDrawElements = d_glDrawElements;
		gglDrawElementsBaseVertexEXT = d_glDrawElementsBaseVertexEXT;
		gglDrawElementsBaseVertexOES = d_glDrawElementsBaseVertexOES;
		gglDrawElementsInstanced = d_glDrawElementsInstanced;
		gglDrawElementsInstancedANGLE = d_glDrawElementsInstancedANGLE;
		gglDrawElementsInstancedBaseInstanceEXT = d_glDrawElementsInstancedBaseInstanceEXT;
		gglDrawElementsInstancedBaseVertexBaseInstanceEXT = d_glDrawElementsInstancedBaseVertexBaseInstanceEXT;
		gglDrawElementsInstancedBaseVertexEXT = d_glDrawElementsInstancedBaseVertexEXT;
		gglDrawElementsInstancedBaseVertexOES = d_glDrawElementsInstancedBaseVertexOES;
		gglDrawElementsInstancedEXT = d_glDrawElementsInstancedEXT;
		gglDrawElementsInstancedNV = d_glDrawElementsInstancedNV;
		gglDrawRangeElements = d_glDrawRangeElements;
		gglDrawRangeElementsBaseVertexEXT = d_glDrawRangeElementsBaseVertexEXT;
		gglDrawRangeElementsBaseVertexOES = d_glDrawRangeElementsBaseVertexOES;
		gglDrawTransformFeedbackEXT = d_glDrawTransformFeedbackEXT;
		gglDrawTransformFeedbackInstancedEXT = d_glDrawTransformFeedbackInstancedEXT;
		gglEGLImageTargetRenderbufferStorageOES = d_glEGLImageTargetRenderbufferStorageOES;
		gglEGLImageTargetTexture2DOES = d_glEGLImageTargetTexture2DOES;
		gglEnable = d_glEnable;
		gglEnableDriverControlQCOM = d_glEnableDriverControlQCOM;
		gglEnableVertexAttribArray = d_glEnableVertexAttribArray;
		gglEnableiEXT = d_glEnableiEXT;
		gglEnableiNV = d_glEnableiNV;
		gglEnableiOES = d_glEnableiOES;
		gglEndConditionalRenderNV = d_glEndConditionalRenderNV;
		gglEndPerfMonitorAMD = d_glEndPerfMonitorAMD;
		gglEndPerfQueryINTEL = d_glEndPerfQueryINTEL;
		gglEndQuery = d_glEndQuery;
		gglEndQueryEXT = d_glEndQueryEXT;
		gglEndTilingQCOM = d_glEndTilingQCOM;
		gglEndTransformFeedback = d_glEndTransformFeedback;
		gglExtGetBufferPointervQCOM = d_glExtGetBufferPointervQCOM;
		gglExtGetBuffersQCOM = d_glExtGetBuffersQCOM;
		gglExtGetFramebuffersQCOM = d_glExtGetFramebuffersQCOM;
		gglExtGetProgramBinarySourceQCOM = d_glExtGetProgramBinarySourceQCOM;
		gglExtGetProgramsQCOM = d_glExtGetProgramsQCOM;
		gglExtGetRenderbuffersQCOM = d_glExtGetRenderbuffersQCOM;
		gglExtGetShadersQCOM = d_glExtGetShadersQCOM;
		gglExtGetTexLevelParameterivQCOM = d_glExtGetTexLevelParameterivQCOM;
		gglExtGetTexSubImageQCOM = d_glExtGetTexSubImageQCOM;
		gglExtGetTexturesQCOM = d_glExtGetTexturesQCOM;
		gglExtIsProgramBinaryQCOM = d_glExtIsProgramBinaryQCOM;
		gglExtTexObjectStateOverrideiQCOM = d_glExtTexObjectStateOverrideiQCOM;
		gglFenceSync = d_glFenceSync;
		gglFenceSyncAPPLE = d_glFenceSyncAPPLE;
		gglFinish = d_glFinish;
		gglFinishFenceNV = d_glFinishFenceNV;
		gglFlush = d_glFlush;
		gglFlushMappedBufferRange = d_glFlushMappedBufferRange;
		gglFlushMappedBufferRangeEXT = d_glFlushMappedBufferRangeEXT;
		gglFragmentCoverageColorNV = d_glFragmentCoverageColorNV;
		gglFramebufferPixelLocalStorageSizeEXT = d_glFramebufferPixelLocalStorageSizeEXT;
		gglFramebufferRenderbuffer = d_glFramebufferRenderbuffer;
		gglFramebufferSampleLocationsfvNV = d_glFramebufferSampleLocationsfvNV;
		gglFramebufferTexture2D = d_glFramebufferTexture2D;
		gglFramebufferTexture2DDownsampleIMG = d_glFramebufferTexture2DDownsampleIMG;
		gglFramebufferTexture2DMultisampleEXT = d_glFramebufferTexture2DMultisampleEXT;
		gglFramebufferTexture2DMultisampleIMG = d_glFramebufferTexture2DMultisampleIMG;
		gglFramebufferTexture3DOES = d_glFramebufferTexture3DOES;
		gglFramebufferTextureEXT = d_glFramebufferTextureEXT;
		gglFramebufferTextureLayer = d_glFramebufferTextureLayer;
		gglFramebufferTextureLayerDownsampleIMG = d_glFramebufferTextureLayerDownsampleIMG;
		gglFramebufferTextureMultisampleMultiviewOVR = d_glFramebufferTextureMultisampleMultiviewOVR;
		gglFramebufferTextureMultiviewOVR = d_glFramebufferTextureMultiviewOVR;
		gglFramebufferTextureOES = d_glFramebufferTextureOES;
		gglFrontFace = d_glFrontFace;
		gglGenBuffers = d_glGenBuffers;
		gglGenFencesNV = d_glGenFencesNV;
		gglGenFramebuffers = d_glGenFramebuffers;
		gglGenPathsNV = d_glGenPathsNV;
		gglGenPerfMonitorsAMD = d_glGenPerfMonitorsAMD;
		gglGenProgramPipelinesEXT = d_glGenProgramPipelinesEXT;
		gglGenQueries = d_glGenQueries;
		gglGenQueriesEXT = d_glGenQueriesEXT;
		gglGenRenderbuffers = d_glGenRenderbuffers;
		gglGenSamplers = d_glGenSamplers;
		gglGenTextures = d_glGenTextures;
		gglGenTransformFeedbacks = d_glGenTransformFeedbacks;
		gglGenVertexArrays = d_glGenVertexArrays;
		gglGenVertexArraysOES = d_glGenVertexArraysOES;
		gglGenerateMipmap = d_glGenerateMipmap;
		gglGetActiveAttrib = d_glGetActiveAttrib;
		gglGetActiveUniform = d_glGetActiveUniform;
		gglGetActiveUniformBlockName = d_glGetActiveUniformBlockName;
		gglGetActiveUniformBlockiv = d_glGetActiveUniformBlockiv;
		gglGetActiveUniformsiv = d_glGetActiveUniformsiv;
		gglGetAttachedShaders = d_glGetAttachedShaders;
		gglGetAttribLocation = d_glGetAttribLocation;
		gglGetBooleanv = d_glGetBooleanv;
		gglGetBufferParameteri64v = d_glGetBufferParameteri64v;
		gglGetBufferParameteriv = d_glGetBufferParameteriv;
		gglGetBufferPointerv = d_glGetBufferPointerv;
		gglGetBufferPointervOES = d_glGetBufferPointervOES;
		gglGetCoverageModulationTableNV = d_glGetCoverageModulationTableNV;
		gglGetDebugMessageLogKHR = d_glGetDebugMessageLogKHR;
		gglGetDriverControlStringQCOM = d_glGetDriverControlStringQCOM;
		gglGetDriverControlsQCOM = d_glGetDriverControlsQCOM;
		gglGetError = d_glGetError;
		gglGetFenceivNV = d_glGetFenceivNV;
		gglGetFirstPerfQueryIdINTEL = d_glGetFirstPerfQueryIdINTEL;
		gglGetFloati_vNV = d_glGetFloati_vNV;
		gglGetFloati_vOES = d_glGetFloati_vOES;
		gglGetFloatv = d_glGetFloatv;
		gglGetFragDataIndexEXT = d_glGetFragDataIndexEXT;
		gglGetFragDataLocation = d_glGetFragDataLocation;
		gglGetFramebufferAttachmentParameteriv = d_glGetFramebufferAttachmentParameteriv;
		gglGetFramebufferPixelLocalStorageSizeEXT = d_glGetFramebufferPixelLocalStorageSizeEXT;
		gglGetGraphicsResetStatusEXT = d_glGetGraphicsResetStatusEXT;
		gglGetGraphicsResetStatusKHR = d_glGetGraphicsResetStatusKHR;
		gglGetImageHandleNV = d_glGetImageHandleNV;
		gglGetInteger64i_v = d_glGetInteger64i_v;
		gglGetInteger64v = d_glGetInteger64v;
		gglGetInteger64vAPPLE = d_glGetInteger64vAPPLE;
		gglGetIntegeri_v = d_glGetIntegeri_v;
		gglGetIntegeri_vEXT = d_glGetIntegeri_vEXT;
		gglGetIntegerv = d_glGetIntegerv;
		gglGetInternalformatSampleivNV = d_glGetInternalformatSampleivNV;
		gglGetInternalformativ = d_glGetInternalformativ;
		gglGetNextPerfQueryIdINTEL = d_glGetNextPerfQueryIdINTEL;
		gglGetObjectLabelEXT = d_glGetObjectLabelEXT;
		gglGetObjectLabelKHR = d_glGetObjectLabelKHR;
		gglGetObjectPtrLabelKHR = d_glGetObjectPtrLabelKHR;
		gglGetPathCommandsNV = d_glGetPathCommandsNV;
		gglGetPathCoordsNV = d_glGetPathCoordsNV;
		gglGetPathDashArrayNV = d_glGetPathDashArrayNV;
		gglGetPathLengthNV = d_glGetPathLengthNV;
		gglGetPathMetricRangeNV = d_glGetPathMetricRangeNV;
		gglGetPathMetricsNV = d_glGetPathMetricsNV;
		gglGetPathParameterfvNV = d_glGetPathParameterfvNV;
		gglGetPathParameterivNV = d_glGetPathParameterivNV;
		gglGetPathSpacingNV = d_glGetPathSpacingNV;
		gglGetPerfCounterInfoINTEL = d_glGetPerfCounterInfoINTEL;
		gglGetPerfMonitorCounterDataAMD = d_glGetPerfMonitorCounterDataAMD;
		gglGetPerfMonitorCounterInfoAMD = d_glGetPerfMonitorCounterInfoAMD;
		gglGetPerfMonitorCounterStringAMD = d_glGetPerfMonitorCounterStringAMD;
		gglGetPerfMonitorCountersAMD = d_glGetPerfMonitorCountersAMD;
		gglGetPerfMonitorGroupStringAMD = d_glGetPerfMonitorGroupStringAMD;
		gglGetPerfMonitorGroupsAMD = d_glGetPerfMonitorGroupsAMD;
		gglGetPerfQueryDataINTEL = d_glGetPerfQueryDataINTEL;
		gglGetPerfQueryIdByNameINTEL = d_glGetPerfQueryIdByNameINTEL;
		gglGetPerfQueryInfoINTEL = d_glGetPerfQueryInfoINTEL;
		gglGetPointervKHR = d_glGetPointervKHR;
		gglGetProgramBinary = d_glGetProgramBinary;
		gglGetProgramBinaryOES = d_glGetProgramBinaryOES;
		gglGetProgramInfoLog = d_glGetProgramInfoLog;
		gglGetProgramPipelineInfoLogEXT = d_glGetProgramPipelineInfoLogEXT;
		gglGetProgramPipelineivEXT = d_glGetProgramPipelineivEXT;
		gglGetProgramResourceLocationIndexEXT = d_glGetProgramResourceLocationIndexEXT;
		gglGetProgramResourcefvNV = d_glGetProgramResourcefvNV;
		gglGetProgramiv = d_glGetProgramiv;
		gglGetQueryObjecti64vEXT = d_glGetQueryObjecti64vEXT;
		gglGetQueryObjectivEXT = d_glGetQueryObjectivEXT;
		gglGetQueryObjectui64vEXT = d_glGetQueryObjectui64vEXT;
		gglGetQueryObjectuiv = d_glGetQueryObjectuiv;
		gglGetQueryObjectuivEXT = d_glGetQueryObjectuivEXT;
		gglGetQueryiv = d_glGetQueryiv;
		gglGetQueryivEXT = d_glGetQueryivEXT;
		gglGetRenderbufferParameteriv = d_glGetRenderbufferParameteriv;
		gglGetSamplerParameterIivEXT = d_glGetSamplerParameterIivEXT;
		gglGetSamplerParameterIivOES = d_glGetSamplerParameterIivOES;
		gglGetSamplerParameterIuivEXT = d_glGetSamplerParameterIuivEXT;
		gglGetSamplerParameterIuivOES = d_glGetSamplerParameterIuivOES;
		gglGetSamplerParameterfv = d_glGetSamplerParameterfv;
		gglGetSamplerParameteriv = d_glGetSamplerParameteriv;
		gglGetShaderInfoLog = d_glGetShaderInfoLog;
		gglGetShaderPrecisionFormat = d_glGetShaderPrecisionFormat;
		gglGetShaderSource = d_glGetShaderSource;
		gglGetShaderiv = d_glGetShaderiv;
		gglGetString = d_glGetString;
		gglGetStringi = d_glGetStringi;
		gglGetSynciv = d_glGetSynciv;
		gglGetSyncivAPPLE = d_glGetSyncivAPPLE;
		gglGetTexParameterIivEXT = d_glGetTexParameterIivEXT;
		gglGetTexParameterIivOES = d_glGetTexParameterIivOES;
		gglGetTexParameterIuivEXT = d_glGetTexParameterIuivEXT;
		gglGetTexParameterIuivOES = d_glGetTexParameterIuivOES;
		gglGetTexParameterfv = d_glGetTexParameterfv;
		gglGetTexParameteriv = d_glGetTexParameteriv;
		gglGetTextureHandleIMG = d_glGetTextureHandleIMG;
		gglGetTextureHandleNV = d_glGetTextureHandleNV;
		gglGetTextureSamplerHandleIMG = d_glGetTextureSamplerHandleIMG;
		gglGetTextureSamplerHandleNV = d_glGetTextureSamplerHandleNV;
		gglGetTransformFeedbackVarying = d_glGetTransformFeedbackVarying;
		gglGetTranslatedShaderSourceANGLE = d_glGetTranslatedShaderSourceANGLE;
		gglGetUniformBlockIndex = d_glGetUniformBlockIndex;
		gglGetUniformIndices = d_glGetUniformIndices;
		gglGetUniformLocation = d_glGetUniformLocation;
		gglGetUniformfv = d_glGetUniformfv;
		gglGetUniformi64vNV = d_glGetUniformi64vNV;
		gglGetUniformiv = d_glGetUniformiv;
		gglGetUniformuiv = d_glGetUniformuiv;
		gglGetVertexAttribIiv = d_glGetVertexAttribIiv;
		gglGetVertexAttribIuiv = d_glGetVertexAttribIuiv;
		gglGetVertexAttribPointerv = d_glGetVertexAttribPointerv;
		gglGetVertexAttribfv = d_glGetVertexAttribfv;
		gglGetVertexAttribiv = d_glGetVertexAttribiv;
		gglGetnUniformfvEXT = d_glGetnUniformfvEXT;
		gglGetnUniformfvKHR = d_glGetnUniformfvKHR;
		gglGetnUniformivEXT = d_glGetnUniformivEXT;
		gglGetnUniformivKHR = d_glGetnUniformivKHR;
		gglGetnUniformuivKHR = d_glGetnUniformuivKHR;
		gglHint = d_glHint;
		gglInsertEventMarkerEXT = d_glInsertEventMarkerEXT;
		gglInterpolatePathsNV = d_glInterpolatePathsNV;
		gglInvalidateFramebuffer = d_glInvalidateFramebuffer;
		gglInvalidateSubFramebuffer = d_glInvalidateSubFramebuffer;
		gglIsBuffer = d_glIsBuffer;
		gglIsEnabled = d_glIsEnabled;
		gglIsEnablediEXT = d_glIsEnablediEXT;
		gglIsEnablediNV = d_glIsEnablediNV;
		gglIsEnablediOES = d_glIsEnablediOES;
		gglIsFenceNV = d_glIsFenceNV;
		gglIsFramebuffer = d_glIsFramebuffer;
		gglIsImageHandleResidentNV = d_glIsImageHandleResidentNV;
		gglIsPathNV = d_glIsPathNV;
		gglIsPointInFillPathNV = d_glIsPointInFillPathNV;
		gglIsPointInStrokePathNV = d_glIsPointInStrokePathNV;
		gglIsProgram = d_glIsProgram;
		gglIsProgramPipelineEXT = d_glIsProgramPipelineEXT;
		gglIsQuery = d_glIsQuery;
		gglIsQueryEXT = d_glIsQueryEXT;
		gglIsRenderbuffer = d_glIsRenderbuffer;
		gglIsSampler = d_glIsSampler;
		gglIsShader = d_glIsShader;
		gglIsSync = d_glIsSync;
		gglIsSyncAPPLE = d_glIsSyncAPPLE;
		gglIsTexture = d_glIsTexture;
		gglIsTextureHandleResidentNV = d_glIsTextureHandleResidentNV;
		gglIsTransformFeedback = d_glIsTransformFeedback;
		gglIsVertexArray = d_glIsVertexArray;
		gglIsVertexArrayOES = d_glIsVertexArrayOES;
		gglLabelObjectEXT = d_glLabelObjectEXT;
		gglLineWidth = d_glLineWidth;
		gglLinkProgram = d_glLinkProgram;
		gglMakeImageHandleNonResidentNV = d_glMakeImageHandleNonResidentNV;
		gglMakeImageHandleResidentNV = d_glMakeImageHandleResidentNV;
		gglMakeTextureHandleNonResidentNV = d_glMakeTextureHandleNonResidentNV;
		gglMakeTextureHandleResidentNV = d_glMakeTextureHandleResidentNV;
		gglMapBufferOES = d_glMapBufferOES;
		gglMapBufferRange = d_glMapBufferRange;
		gglMapBufferRangeEXT = d_glMapBufferRangeEXT;
		gglMatrixLoad3x2fNV = d_glMatrixLoad3x2fNV;
		gglMatrixLoad3x3fNV = d_glMatrixLoad3x3fNV;
		gglMatrixLoadTranspose3x3fNV = d_glMatrixLoadTranspose3x3fNV;
		gglMatrixMult3x2fNV = d_glMatrixMult3x2fNV;
		gglMatrixMult3x3fNV = d_glMatrixMult3x3fNV;
		gglMatrixMultTranspose3x3fNV = d_glMatrixMultTranspose3x3fNV;
		gglMinSampleShadingOES = d_glMinSampleShadingOES;
		gglMultiDrawArraysEXT = d_glMultiDrawArraysEXT;
		gglMultiDrawArraysIndirectEXT = d_glMultiDrawArraysIndirectEXT;
		gglMultiDrawElementsBaseVertexEXT = d_glMultiDrawElementsBaseVertexEXT;
		gglMultiDrawElementsBaseVertexOES = d_glMultiDrawElementsBaseVertexOES;
		gglMultiDrawElementsEXT = d_glMultiDrawElementsEXT;
		gglMultiDrawElementsIndirectEXT = d_glMultiDrawElementsIndirectEXT;
		gglNamedFramebufferSampleLocationsfvNV = d_glNamedFramebufferSampleLocationsfvNV;
		gglObjectLabelKHR = d_glObjectLabelKHR;
		gglObjectPtrLabelKHR = d_glObjectPtrLabelKHR;
		gglPatchParameteriEXT = d_glPatchParameteriEXT;
		gglPatchParameteriOES = d_glPatchParameteriOES;
		gglPathCommandsNV = d_glPathCommandsNV;
		gglPathCoordsNV = d_glPathCoordsNV;
		gglPathCoverDepthFuncNV = d_glPathCoverDepthFuncNV;
		gglPathDashArrayNV = d_glPathDashArrayNV;
		gglPathGlyphIndexArrayNV = d_glPathGlyphIndexArrayNV;
		gglPathGlyphIndexRangeNV = d_glPathGlyphIndexRangeNV;
		gglPathGlyphRangeNV = d_glPathGlyphRangeNV;
		gglPathGlyphsNV = d_glPathGlyphsNV;
		gglPathMemoryGlyphIndexArrayNV = d_glPathMemoryGlyphIndexArrayNV;
		gglPathParameterfNV = d_glPathParameterfNV;
		gglPathParameterfvNV = d_glPathParameterfvNV;
		gglPathParameteriNV = d_glPathParameteriNV;
		gglPathParameterivNV = d_glPathParameterivNV;
		gglPathStencilDepthOffsetNV = d_glPathStencilDepthOffsetNV;
		gglPathStencilFuncNV = d_glPathStencilFuncNV;
		gglPathStringNV = d_glPathStringNV;
		gglPathSubCommandsNV = d_glPathSubCommandsNV;
		gglPathSubCoordsNV = d_glPathSubCoordsNV;
		gglPauseTransformFeedback = d_glPauseTransformFeedback;
		gglPixelStorei = d_glPixelStorei;
		gglPointAlongPathNV = d_glPointAlongPathNV;
		gglPolygonModeNV = d_glPolygonModeNV;
		gglPolygonOffset = d_glPolygonOffset;
		gglPolygonOffsetClampEXT = d_glPolygonOffsetClampEXT;
		gglPopDebugGroupKHR = d_glPopDebugGroupKHR;
		gglPopGroupMarkerEXT = d_glPopGroupMarkerEXT;
		gglPrimitiveBoundingBoxEXT = d_glPrimitiveBoundingBoxEXT;
		gglPrimitiveBoundingBoxOES = d_glPrimitiveBoundingBoxOES;
		gglProgramBinary = d_glProgramBinary;
		gglProgramBinaryOES = d_glProgramBinaryOES;
		gglProgramParameteri = d_glProgramParameteri;
		gglProgramParameteriEXT = d_glProgramParameteriEXT;
		gglProgramPathFragmentInputGenNV = d_glProgramPathFragmentInputGenNV;
		gglProgramUniform1fEXT = d_glProgramUniform1fEXT;
		gglProgramUniform1fvEXT = d_glProgramUniform1fvEXT;
		gglProgramUniform1i64NV = d_glProgramUniform1i64NV;
		gglProgramUniform1i64vNV = d_glProgramUniform1i64vNV;
		gglProgramUniform1iEXT = d_glProgramUniform1iEXT;
		gglProgramUniform1ivEXT = d_glProgramUniform1ivEXT;
		gglProgramUniform1ui64NV = d_glProgramUniform1ui64NV;
		gglProgramUniform1ui64vNV = d_glProgramUniform1ui64vNV;
		gglProgramUniform1uiEXT = d_glProgramUniform1uiEXT;
		gglProgramUniform1uivEXT = d_glProgramUniform1uivEXT;
		gglProgramUniform2fEXT = d_glProgramUniform2fEXT;
		gglProgramUniform2fvEXT = d_glProgramUniform2fvEXT;
		gglProgramUniform2i64NV = d_glProgramUniform2i64NV;
		gglProgramUniform2i64vNV = d_glProgramUniform2i64vNV;
		gglProgramUniform2iEXT = d_glProgramUniform2iEXT;
		gglProgramUniform2ivEXT = d_glProgramUniform2ivEXT;
		gglProgramUniform2ui64NV = d_glProgramUniform2ui64NV;
		gglProgramUniform2ui64vNV = d_glProgramUniform2ui64vNV;
		gglProgramUniform2uiEXT = d_glProgramUniform2uiEXT;
		gglProgramUniform2uivEXT = d_glProgramUniform2uivEXT;
		gglProgramUniform3fEXT = d_glProgramUniform3fEXT;
		gglProgramUniform3fvEXT = d_glProgramUniform3fvEXT;
		gglProgramUniform3i64NV = d_glProgramUniform3i64NV;
		gglProgramUniform3i64vNV = d_glProgramUniform3i64vNV;
		gglProgramUniform3iEXT = d_glProgramUniform3iEXT;
		gglProgramUniform3ivEXT = d_glProgramUniform3ivEXT;
		gglProgramUniform3ui64NV = d_glProgramUniform3ui64NV;
		gglProgramUniform3ui64vNV = d_glProgramUniform3ui64vNV;
		gglProgramUniform3uiEXT = d_glProgramUniform3uiEXT;
		gglProgramUniform3uivEXT = d_glProgramUniform3uivEXT;
		gglProgramUniform4fEXT = d_glProgramUniform4fEXT;
		gglProgramUniform4fvEXT = d_glProgramUniform4fvEXT;
		gglProgramUniform4i64NV = d_glProgramUniform4i64NV;
		gglProgramUniform4i64vNV = d_glProgramUniform4i64vNV;
		gglProgramUniform4iEXT = d_glProgramUniform4iEXT;
		gglProgramUniform4ivEXT = d_glProgramUniform4ivEXT;
		gglProgramUniform4ui64NV = d_glProgramUniform4ui64NV;
		gglProgramUniform4ui64vNV = d_glProgramUniform4ui64vNV;
		gglProgramUniform4uiEXT = d_glProgramUniform4uiEXT;
		gglProgramUniform4uivEXT = d_glProgramUniform4uivEXT;
		gglProgramUniformHandleui64IMG = d_glProgramUniformHandleui64IMG;
		gglProgramUniformHandleui64NV = d_glProgramUniformHandleui64NV;
		gglProgramUniformHandleui64vIMG = d_glProgramUniformHandleui64vIMG;
		gglProgramUniformHandleui64vNV = d_glProgramUniformHandleui64vNV;
		gglProgramUniformMatrix2fvEXT = d_glProgramUniformMatrix2fvEXT;
		gglProgramUniformMatrix2x3fvEXT = d_glProgramUniformMatrix2x3fvEXT;
		gglProgramUniformMatrix2x4fvEXT = d_glProgramUniformMatrix2x4fvEXT;
		gglProgramUniformMatrix3fvEXT = d_glProgramUniformMatrix3fvEXT;
		gglProgramUniformMatrix3x2fvEXT = d_glProgramUniformMatrix3x2fvEXT;
		gglProgramUniformMatrix3x4fvEXT = d_glProgramUniformMatrix3x4fvEXT;
		gglProgramUniformMatrix4fvEXT = d_glProgramUniformMatrix4fvEXT;
		gglProgramUniformMatrix4x2fvEXT = d_glProgramUniformMatrix4x2fvEXT;
		gglProgramUniformMatrix4x3fvEXT = d_glProgramUniformMatrix4x3fvEXT;
		gglPushDebugGroupKHR = d_glPushDebugGroupKHR;
		gglPushGroupMarkerEXT = d_glPushGroupMarkerEXT;
		gglQueryCounterEXT = d_glQueryCounterEXT;
		gglRasterSamplesEXT = d_glRasterSamplesEXT;
		gglReadBuffer = d_glReadBuffer;
		gglReadBufferIndexedEXT = d_glReadBufferIndexedEXT;
		gglReadBufferNV = d_glReadBufferNV;
		gglReadPixels = d_glReadPixels;
		gglReadnPixelsEXT = d_glReadnPixelsEXT;
		gglReadnPixelsKHR = d_glReadnPixelsKHR;
		gglReleaseShaderCompiler = d_glReleaseShaderCompiler;
		gglRenderbufferStorage = d_glRenderbufferStorage;
		gglRenderbufferStorageMultisample = d_glRenderbufferStorageMultisample;
		gglRenderbufferStorageMultisampleANGLE = d_glRenderbufferStorageMultisampleANGLE;
		gglRenderbufferStorageMultisampleAPPLE = d_glRenderbufferStorageMultisampleAPPLE;
		gglRenderbufferStorageMultisampleEXT = d_glRenderbufferStorageMultisampleEXT;
		gglRenderbufferStorageMultisampleIMG = d_glRenderbufferStorageMultisampleIMG;
		gglRenderbufferStorageMultisampleNV = d_glRenderbufferStorageMultisampleNV;
		gglResolveDepthValuesNV = d_glResolveDepthValuesNV;
		gglResolveMultisampleFramebufferAPPLE = d_glResolveMultisampleFramebufferAPPLE;
		gglResumeTransformFeedback = d_glResumeTransformFeedback;
		gglSampleCoverage = d_glSampleCoverage;
		gglSamplerParameterIivEXT = d_glSamplerParameterIivEXT;
		gglSamplerParameterIivOES = d_glSamplerParameterIivOES;
		gglSamplerParameterIuivEXT = d_glSamplerParameterIuivEXT;
		gglSamplerParameterIuivOES = d_glSamplerParameterIuivOES;
		gglSamplerParameterf = d_glSamplerParameterf;
		gglSamplerParameterfv = d_glSamplerParameterfv;
		gglSamplerParameteri = d_glSamplerParameteri;
		gglSamplerParameteriv = d_glSamplerParameteriv;
		gglScissor = d_glScissor;
		gglScissorArrayvNV = d_glScissorArrayvNV;
		gglScissorArrayvOES = d_glScissorArrayvOES;
		gglScissorIndexedNV = d_glScissorIndexedNV;
		gglScissorIndexedOES = d_glScissorIndexedOES;
		gglScissorIndexedvNV = d_glScissorIndexedvNV;
		gglScissorIndexedvOES = d_glScissorIndexedvOES;
		gglSelectPerfMonitorCountersAMD = d_glSelectPerfMonitorCountersAMD;
		gglSetFenceNV = d_glSetFenceNV;
		gglShaderBinary = d_glShaderBinary;
		gglShaderSource = d_glShaderSource;
		gglStartTilingQCOM = d_glStartTilingQCOM;
		gglStencilFillPathInstancedNV = d_glStencilFillPathInstancedNV;
		gglStencilFillPathNV = d_glStencilFillPathNV;
		gglStencilFunc = d_glStencilFunc;
		gglStencilFuncSeparate = d_glStencilFuncSeparate;
		gglStencilMask = d_glStencilMask;
		gglStencilMaskSeparate = d_glStencilMaskSeparate;
		gglStencilOp = d_glStencilOp;
		gglStencilOpSeparate = d_glStencilOpSeparate;
		gglStencilStrokePathInstancedNV = d_glStencilStrokePathInstancedNV;
		gglStencilStrokePathNV = d_glStencilStrokePathNV;
		gglStencilThenCoverFillPathInstancedNV = d_glStencilThenCoverFillPathInstancedNV;
		gglStencilThenCoverFillPathNV = d_glStencilThenCoverFillPathNV;
		gglStencilThenCoverStrokePathInstancedNV = d_glStencilThenCoverStrokePathInstancedNV;
		gglStencilThenCoverStrokePathNV = d_glStencilThenCoverStrokePathNV;
		gglSubpixelPrecisionBiasNV = d_glSubpixelPrecisionBiasNV;
		gglTestFenceNV = d_glTestFenceNV;
		gglTexBufferEXT = d_glTexBufferEXT;
		gglTexBufferOES = d_glTexBufferOES;
		gglTexBufferRangeEXT = d_glTexBufferRangeEXT;
		gglTexBufferRangeOES = d_glTexBufferRangeOES;
		gglTexImage2D = d_glTexImage2D;
		gglTexImage3D = d_glTexImage3D;
		gglTexImage3DOES = d_glTexImage3DOES;
		gglTexPageCommitmentEXT = d_glTexPageCommitmentEXT;
		gglTexParameterIivEXT = d_glTexParameterIivEXT;
		gglTexParameterIivOES = d_glTexParameterIivOES;
		gglTexParameterIuivEXT = d_glTexParameterIuivEXT;
		gglTexParameterIuivOES = d_glTexParameterIuivOES;
		gglTexParameterf = d_glTexParameterf;
		gglTexParameterfv = d_glTexParameterfv;
		gglTexParameteri = d_glTexParameteri;
		gglTexParameteriv = d_glTexParameteriv;
		gglTexStorage1DEXT = d_glTexStorage1DEXT;
		gglTexStorage2D = d_glTexStorage2D;
		gglTexStorage2DEXT = d_glTexStorage2DEXT;
		gglTexStorage3D = d_glTexStorage3D;
		gglTexStorage3DEXT = d_glTexStorage3DEXT;
		gglTexStorage3DMultisampleOES = d_glTexStorage3DMultisampleOES;
		gglTexSubImage2D = d_glTexSubImage2D;
		gglTexSubImage3D = d_glTexSubImage3D;
		gglTexSubImage3DOES = d_glTexSubImage3DOES;
		gglTextureStorage1DEXT = d_glTextureStorage1DEXT;
		gglTextureStorage2DEXT = d_glTextureStorage2DEXT;
		gglTextureStorage3DEXT = d_glTextureStorage3DEXT;
		gglTextureViewEXT = d_glTextureViewEXT;
		gglTextureViewOES = d_glTextureViewOES;
		gglTransformFeedbackVaryings = d_glTransformFeedbackVaryings;
		gglTransformPathNV = d_glTransformPathNV;
		gglUniform1f = d_glUniform1f;
		gglUniform1fv = d_glUniform1fv;
		gglUniform1i = d_glUniform1i;
		gglUniform1i64NV = d_glUniform1i64NV;
		gglUniform1i64vNV = d_glUniform1i64vNV;
		gglUniform1iv = d_glUniform1iv;
		gglUniform1ui = d_glUniform1ui;
		gglUniform1ui64NV = d_glUniform1ui64NV;
		gglUniform1ui64vNV = d_glUniform1ui64vNV;
		gglUniform1uiv = d_glUniform1uiv;
		gglUniform2f = d_glUniform2f;
		gglUniform2fv = d_glUniform2fv;
		gglUniform2i = d_glUniform2i;
		gglUniform2i64NV = d_glUniform2i64NV;
		gglUniform2i64vNV = d_glUniform2i64vNV;
		gglUniform2iv = d_glUniform2iv;
		gglUniform2ui = d_glUniform2ui;
		gglUniform2ui64NV = d_glUniform2ui64NV;
		gglUniform2ui64vNV = d_glUniform2ui64vNV;
		gglUniform2uiv = d_glUniform2uiv;
		gglUniform3f = d_glUniform3f;
		gglUniform3fv = d_glUniform3fv;
		gglUniform3i = d_glUniform3i;
		gglUniform3i64NV = d_glUniform3i64NV;
		gglUniform3i64vNV = d_glUniform3i64vNV;
		gglUniform3iv = d_glUniform3iv;
		gglUniform3ui = d_glUniform3ui;
		gglUniform3ui64NV = d_glUniform3ui64NV;
		gglUniform3ui64vNV = d_glUniform3ui64vNV;
		gglUniform3uiv = d_glUniform3uiv;
		gglUniform4f = d_glUniform4f;
		gglUniform4fv = d_glUniform4fv;
		gglUniform4i = d_glUniform4i;
		gglUniform4i64NV = d_glUniform4i64NV;
		gglUniform4i64vNV = d_glUniform4i64vNV;
		gglUniform4iv = d_glUniform4iv;
		gglUniform4ui = d_glUniform4ui;
		gglUniform4ui64NV = d_glUniform4ui64NV;
		gglUniform4ui64vNV = d_glUniform4ui64vNV;
		gglUniform4uiv = d_glUniform4uiv;
		gglUniformBlockBinding = d_glUniformBlockBinding;
		gglUniformHandleui64IMG = d_glUniformHandleui64IMG;
		gglUniformHandleui64NV = d_glUniformHandleui64NV;
		gglUniformHandleui64vIMG = d_glUniformHandleui64vIMG;
		gglUniformHandleui64vNV = d_glUniformHandleui64vNV;
		gglUniformMatrix2fv = d_glUniformMatrix2fv;
		gglUniformMatrix2x3fv = d_glUniformMatrix2x3fv;
		gglUniformMatrix2x3fvNV = d_glUniformMatrix2x3fvNV;
		gglUniformMatrix2x4fv = d_glUniformMatrix2x4fv;
		gglUniformMatrix2x4fvNV = d_glUniformMatrix2x4fvNV;
		gglUniformMatrix3fv = d_glUniformMatrix3fv;
		gglUniformMatrix3x2fv = d_glUniformMatrix3x2fv;
		gglUniformMatrix3x2fvNV = d_glUniformMatrix3x2fvNV;
		gglUniformMatrix3x4fv = d_glUniformMatrix3x4fv;
		gglUniformMatrix3x4fvNV = d_glUniformMatrix3x4fvNV;
		gglUniformMatrix4fv = d_glUniformMatrix4fv;
		gglUniformMatrix4x2fv = d_glUniformMatrix4x2fv;
		gglUniformMatrix4x2fvNV = d_glUniformMatrix4x2fvNV;
		gglUniformMatrix4x3fv = d_glUniformMatrix4x3fv;
		gglUniformMatrix4x3fvNV = d_glUniformMatrix4x3fvNV;
		gglUnmapBuffer = d_glUnmapBuffer;
		gglUnmapBufferOES = d_glUnmapBufferOES;
		gglUseProgram = d_glUseProgram;
		gglUseProgramStagesEXT = d_glUseProgramStagesEXT;
		gglValidateProgram = d_glValidateProgram;
		gglValidateProgramPipelineEXT = d_glValidateProgramPipelineEXT;
		gglVertexAttrib1f = d_glVertexAttrib1f;
		gglVertexAttrib1fv = d_glVertexAttrib1fv;
		gglVertexAttrib2f = d_glVertexAttrib2f;
		gglVertexAttrib2fv = d_glVertexAttrib2fv;
		gglVertexAttrib3f = d_glVertexAttrib3f;
		gglVertexAttrib3fv = d_glVertexAttrib3fv;
		gglVertexAttrib4f = d_glVertexAttrib4f;
		gglVertexAttrib4fv = d_glVertexAttrib4fv;
		gglVertexAttribDivisor = d_glVertexAttribDivisor;
		gglVertexAttribDivisorANGLE = d_glVertexAttribDivisorANGLE;
		gglVertexAttribDivisorEXT = d_glVertexAttribDivisorEXT;
		gglVertexAttribDivisorNV = d_glVertexAttribDivisorNV;
		gglVertexAttribI4i = d_glVertexAttribI4i;
		gglVertexAttribI4iv = d_glVertexAttribI4iv;
		gglVertexAttribI4ui = d_glVertexAttribI4ui;
		gglVertexAttribI4uiv = d_glVertexAttribI4uiv;
		gglVertexAttribIPointer = d_glVertexAttribIPointer;
		gglVertexAttribPointer = d_glVertexAttribPointer;
		gglViewport = d_glViewport;
		gglViewportArrayvNV = d_glViewportArrayvNV;
		gglViewportArrayvOES = d_glViewportArrayvOES;
		gglViewportIndexedfOES = d_glViewportIndexedfOES;
		gglViewportIndexedfNV = d_glViewportIndexedfNV;
		gglViewportIndexedfvOES = d_glViewportIndexedfvOES;
		gglViewportIndexedfvNV = d_glViewportIndexedfvNV;
		gglViewportSwizzleNV = d_glViewportSwizzleNV;
		gglWaitSync = d_glWaitSync;
		gglWaitSyncAPPLE = d_glWaitSyncAPPLE;
		gglWeightPathsNV = d_glWeightPathsNV;
		gglWindowRectanglesEXT = d_glWindowRectanglesEXT;
	}
}
