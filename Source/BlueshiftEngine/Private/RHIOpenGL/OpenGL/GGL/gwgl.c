/*********************************************************************************************
 *
 * gwgl.c
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

#include "gwgl.h"
#include <string.h>

extern void CheckGLError(const char *msg);

typedef int (APIENTRYP PFNCHOOSEPIXELFORMAT)(HDC hDc, const PIXELFORMATDESCRIPTOR *pPfd);
PFNCHOOSEPIXELFORMAT gChoosePixelFormat;
static PFNCHOOSEPIXELFORMAT _ChoosePixelFormat;
static int APIENTRY d_ChoosePixelFormat(HDC hDc, const PIXELFORMATDESCRIPTOR *pPfd) {
	int ret = _ChoosePixelFormat(hDc, pPfd);
	CheckGLError("ChoosePixelFormat");
	return ret;
}
typedef int (APIENTRYP PFNDESCRIBEPIXELFORMAT)(HDC hdc, int ipfd, UINT cjpfd, const PIXELFORMATDESCRIPTOR *ppfd);
PFNDESCRIBEPIXELFORMAT gDescribePixelFormat;
static PFNDESCRIBEPIXELFORMAT _DescribePixelFormat;
static int APIENTRY d_DescribePixelFormat(HDC hdc, int ipfd, UINT cjpfd, const PIXELFORMATDESCRIPTOR *ppfd) {
	int ret = _DescribePixelFormat(hdc, ipfd, cjpfd, ppfd);
	CheckGLError("DescribePixelFormat");
	return ret;
}
typedef int (APIENTRYP PFNGETPIXELFORMAT)(HDC hdc);
PFNGETPIXELFORMAT gGetPixelFormat;
static PFNGETPIXELFORMAT _GetPixelFormat;
static int APIENTRY d_GetPixelFormat(HDC hdc) {
	int ret = _GetPixelFormat(hdc);
	CheckGLError("GetPixelFormat");
	return ret;
}
typedef BOOL (APIENTRYP PFNSETPIXELFORMAT)(HDC hdc, int ipfd, const PIXELFORMATDESCRIPTOR *ppfd);
PFNSETPIXELFORMAT gSetPixelFormat;
static PFNSETPIXELFORMAT _SetPixelFormat;
static BOOL APIENTRY d_SetPixelFormat(HDC hdc, int ipfd, const PIXELFORMATDESCRIPTOR *ppfd) {
	BOOL ret = _SetPixelFormat(hdc, ipfd, ppfd);
	CheckGLError("SetPixelFormat");
	return ret;
}
typedef BOOL (APIENTRYP PFNSWAPBUFFERS)(HDC hdc);
PFNSWAPBUFFERS gSwapBuffers;
static PFNSWAPBUFFERS _SwapBuffers;
static BOOL APIENTRY d_SwapBuffers(HDC hdc) {
	BOOL ret = _SwapBuffers(hdc);
	CheckGLError("SwapBuffers");
	return ret;
}
typedef void * (APIENTRYP PFNWGLALLOCATEMEMORYNV)(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
PFNWGLALLOCATEMEMORYNV gwglAllocateMemoryNV;
static PFNWGLALLOCATEMEMORYNV _wglAllocateMemoryNV;
static void * APIENTRY d_wglAllocateMemoryNV(GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority) {
	void * ret = _wglAllocateMemoryNV(size, readfreq, writefreq, priority);
	CheckGLError("wglAllocateMemoryNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLASSOCIATEIMAGEBUFFEREVENTSI3D)(HDC hDC, const HANDLE *pEvent, const LPVOID *pAddress, const DWORD *pSize, UINT count);
PFNWGLASSOCIATEIMAGEBUFFEREVENTSI3D gwglAssociateImageBufferEventsI3D;
static PFNWGLASSOCIATEIMAGEBUFFEREVENTSI3D _wglAssociateImageBufferEventsI3D;
static BOOL APIENTRY d_wglAssociateImageBufferEventsI3D(HDC hDC, const HANDLE *pEvent, const LPVOID *pAddress, const DWORD *pSize, UINT count) {
	BOOL ret = _wglAssociateImageBufferEventsI3D(hDC, pEvent, pAddress, pSize, count);
	CheckGLError("wglAssociateImageBufferEventsI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLBEGINFRAMETRACKINGI3D)();
PFNWGLBEGINFRAMETRACKINGI3D gwglBeginFrameTrackingI3D;
static PFNWGLBEGINFRAMETRACKINGI3D _wglBeginFrameTrackingI3D;
static BOOL APIENTRY d_wglBeginFrameTrackingI3D() {
	BOOL ret = _wglBeginFrameTrackingI3D();
	CheckGLError("wglBeginFrameTrackingI3D");
	return ret;
}
typedef GLboolean (APIENTRYP PFNWGLBINDDISPLAYCOLORTABLEEXT)(GLushort id);
PFNWGLBINDDISPLAYCOLORTABLEEXT gwglBindDisplayColorTableEXT;
static PFNWGLBINDDISPLAYCOLORTABLEEXT _wglBindDisplayColorTableEXT;
static GLboolean APIENTRY d_wglBindDisplayColorTableEXT(GLushort id) {
	GLboolean ret = _wglBindDisplayColorTableEXT(id);
	CheckGLError("wglBindDisplayColorTableEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLBINDSWAPBARRIERNV)(GLuint group, GLuint barrier);
PFNWGLBINDSWAPBARRIERNV gwglBindSwapBarrierNV;
static PFNWGLBINDSWAPBARRIERNV _wglBindSwapBarrierNV;
static BOOL APIENTRY d_wglBindSwapBarrierNV(GLuint group, GLuint barrier) {
	BOOL ret = _wglBindSwapBarrierNV(group, barrier);
	CheckGLError("wglBindSwapBarrierNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLBINDTEXIMAGEARB)(HPBUFFERARB hPbuffer, int iBuffer);
PFNWGLBINDTEXIMAGEARB gwglBindTexImageARB;
static PFNWGLBINDTEXIMAGEARB _wglBindTexImageARB;
static BOOL APIENTRY d_wglBindTexImageARB(HPBUFFERARB hPbuffer, int iBuffer) {
	BOOL ret = _wglBindTexImageARB(hPbuffer, iBuffer);
	CheckGLError("wglBindTexImageARB");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLBINDVIDEOCAPTUREDEVICENV)(UINT uVideoSlot, HVIDEOINPUTDEVICENV hDevice);
PFNWGLBINDVIDEOCAPTUREDEVICENV gwglBindVideoCaptureDeviceNV;
static PFNWGLBINDVIDEOCAPTUREDEVICENV _wglBindVideoCaptureDeviceNV;
static BOOL APIENTRY d_wglBindVideoCaptureDeviceNV(UINT uVideoSlot, HVIDEOINPUTDEVICENV hDevice) {
	BOOL ret = _wglBindVideoCaptureDeviceNV(uVideoSlot, hDevice);
	CheckGLError("wglBindVideoCaptureDeviceNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLBINDVIDEODEVICENV)(HDC hDC, unsigned int uVideoSlot, HVIDEOOUTPUTDEVICENV hVideoDevice, const int *piAttribList);
PFNWGLBINDVIDEODEVICENV gwglBindVideoDeviceNV;
static PFNWGLBINDVIDEODEVICENV _wglBindVideoDeviceNV;
static BOOL APIENTRY d_wglBindVideoDeviceNV(HDC hDC, unsigned int uVideoSlot, HVIDEOOUTPUTDEVICENV hVideoDevice, const int *piAttribList) {
	BOOL ret = _wglBindVideoDeviceNV(hDC, uVideoSlot, hVideoDevice, piAttribList);
	CheckGLError("wglBindVideoDeviceNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLBINDVIDEOIMAGENV)(HPVIDEODEV hVideoDevice, HPBUFFERARB hPbuffer, int iVideoBuffer);
PFNWGLBINDVIDEOIMAGENV gwglBindVideoImageNV;
static PFNWGLBINDVIDEOIMAGENV _wglBindVideoImageNV;
static BOOL APIENTRY d_wglBindVideoImageNV(HPVIDEODEV hVideoDevice, HPBUFFERARB hPbuffer, int iVideoBuffer) {
	BOOL ret = _wglBindVideoImageNV(hVideoDevice, hPbuffer, iVideoBuffer);
	CheckGLError("wglBindVideoImageNV");
	return ret;
}
typedef VOID (APIENTRYP PFNWGLBLITCONTEXTFRAMEBUFFERAMD)(HGLRC dstCtx, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
PFNWGLBLITCONTEXTFRAMEBUFFERAMD gwglBlitContextFramebufferAMD;
static PFNWGLBLITCONTEXTFRAMEBUFFERAMD _wglBlitContextFramebufferAMD;
static VOID APIENTRY d_wglBlitContextFramebufferAMD(HGLRC dstCtx, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
	_wglBlitContextFramebufferAMD(dstCtx, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	CheckGLError("wglBlitContextFramebufferAMD");
}
typedef BOOL (APIENTRYP PFNWGLCHOOSEPIXELFORMATARB)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
PFNWGLCHOOSEPIXELFORMATARB gwglChoosePixelFormatARB;
static PFNWGLCHOOSEPIXELFORMATARB _wglChoosePixelFormatARB;
static BOOL APIENTRY d_wglChoosePixelFormatARB(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats) {
	BOOL ret = _wglChoosePixelFormatARB(hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats);
	CheckGLError("wglChoosePixelFormatARB");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLCHOOSEPIXELFORMATEXT)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
PFNWGLCHOOSEPIXELFORMATEXT gwglChoosePixelFormatEXT;
static PFNWGLCHOOSEPIXELFORMATEXT _wglChoosePixelFormatEXT;
static BOOL APIENTRY d_wglChoosePixelFormatEXT(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats) {
	BOOL ret = _wglChoosePixelFormatEXT(hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats);
	CheckGLError("wglChoosePixelFormatEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLCOPYCONTEXT)(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask);
PFNWGLCOPYCONTEXT gwglCopyContext;
static PFNWGLCOPYCONTEXT _wglCopyContext;
static BOOL APIENTRY d_wglCopyContext(HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask) {
	BOOL ret = _wglCopyContext(hglrcSrc, hglrcDst, mask);
	CheckGLError("wglCopyContext");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLCOPYIMAGESUBDATANV)(HGLRC hSrcRC, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, HGLRC hDstRC, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth);
PFNWGLCOPYIMAGESUBDATANV gwglCopyImageSubDataNV;
static PFNWGLCOPYIMAGESUBDATANV _wglCopyImageSubDataNV;
static BOOL APIENTRY d_wglCopyImageSubDataNV(HGLRC hSrcRC, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, HGLRC hDstRC, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth) {
	BOOL ret = _wglCopyImageSubDataNV(hSrcRC, srcName, srcTarget, srcLevel, srcX, srcY, srcZ, hDstRC, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, width, height, depth);
	CheckGLError("wglCopyImageSubDataNV");
	return ret;
}
typedef HDC (APIENTRYP PFNWGLCREATEAFFINITYDCNV)(const HGPUNV *phGpuList);
PFNWGLCREATEAFFINITYDCNV gwglCreateAffinityDCNV;
static PFNWGLCREATEAFFINITYDCNV _wglCreateAffinityDCNV;
static HDC APIENTRY d_wglCreateAffinityDCNV(const HGPUNV *phGpuList) {
	HDC ret = _wglCreateAffinityDCNV(phGpuList);
	CheckGLError("wglCreateAffinityDCNV");
	return ret;
}
typedef HGLRC (APIENTRYP PFNWGLCREATEASSOCIATEDCONTEXTAMD)(UINT id);
PFNWGLCREATEASSOCIATEDCONTEXTAMD gwglCreateAssociatedContextAMD;
static PFNWGLCREATEASSOCIATEDCONTEXTAMD _wglCreateAssociatedContextAMD;
static HGLRC APIENTRY d_wglCreateAssociatedContextAMD(UINT id) {
	HGLRC ret = _wglCreateAssociatedContextAMD(id);
	CheckGLError("wglCreateAssociatedContextAMD");
	return ret;
}
typedef HGLRC (APIENTRYP PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMD)(UINT id, HGLRC hShareContext, const int *attribList);
PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMD gwglCreateAssociatedContextAttribsAMD;
static PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMD _wglCreateAssociatedContextAttribsAMD;
static HGLRC APIENTRY d_wglCreateAssociatedContextAttribsAMD(UINT id, HGLRC hShareContext, const int *attribList) {
	HGLRC ret = _wglCreateAssociatedContextAttribsAMD(id, hShareContext, attribList);
	CheckGLError("wglCreateAssociatedContextAttribsAMD");
	return ret;
}
typedef HANDLE (APIENTRYP PFNWGLCREATEBUFFERREGIONARB)(HDC hDC, int iLayerPlane, UINT uType);
PFNWGLCREATEBUFFERREGIONARB gwglCreateBufferRegionARB;
static PFNWGLCREATEBUFFERREGIONARB _wglCreateBufferRegionARB;
static HANDLE APIENTRY d_wglCreateBufferRegionARB(HDC hDC, int iLayerPlane, UINT uType) {
	HANDLE ret = _wglCreateBufferRegionARB(hDC, iLayerPlane, uType);
	CheckGLError("wglCreateBufferRegionARB");
	return ret;
}
typedef HGLRC (APIENTRYP PFNWGLCREATECONTEXT)(HDC hDc);
PFNWGLCREATECONTEXT gwglCreateContext;
static PFNWGLCREATECONTEXT _wglCreateContext;
static HGLRC APIENTRY d_wglCreateContext(HDC hDc) {
	HGLRC ret = _wglCreateContext(hDc);
	CheckGLError("wglCreateContext");
	return ret;
}
typedef HGLRC (APIENTRYP PFNWGLCREATECONTEXTATTRIBSARB)(HDC hDC, HGLRC hShareContext, const int *attribList);
PFNWGLCREATECONTEXTATTRIBSARB gwglCreateContextAttribsARB;
static PFNWGLCREATECONTEXTATTRIBSARB _wglCreateContextAttribsARB;
static HGLRC APIENTRY d_wglCreateContextAttribsARB(HDC hDC, HGLRC hShareContext, const int *attribList) {
	HGLRC ret = _wglCreateContextAttribsARB(hDC, hShareContext, attribList);
	CheckGLError("wglCreateContextAttribsARB");
	return ret;
}
typedef GLboolean (APIENTRYP PFNWGLCREATEDISPLAYCOLORTABLEEXT)(GLushort id);
PFNWGLCREATEDISPLAYCOLORTABLEEXT gwglCreateDisplayColorTableEXT;
static PFNWGLCREATEDISPLAYCOLORTABLEEXT _wglCreateDisplayColorTableEXT;
static GLboolean APIENTRY d_wglCreateDisplayColorTableEXT(GLushort id) {
	GLboolean ret = _wglCreateDisplayColorTableEXT(id);
	CheckGLError("wglCreateDisplayColorTableEXT");
	return ret;
}
typedef LPVOID (APIENTRYP PFNWGLCREATEIMAGEBUFFERI3D)(HDC hDC, DWORD dwSize, UINT uFlags);
PFNWGLCREATEIMAGEBUFFERI3D gwglCreateImageBufferI3D;
static PFNWGLCREATEIMAGEBUFFERI3D _wglCreateImageBufferI3D;
static LPVOID APIENTRY d_wglCreateImageBufferI3D(HDC hDC, DWORD dwSize, UINT uFlags) {
	LPVOID ret = _wglCreateImageBufferI3D(hDC, dwSize, uFlags);
	CheckGLError("wglCreateImageBufferI3D");
	return ret;
}
typedef HGLRC (APIENTRYP PFNWGLCREATELAYERCONTEXT)(HDC hDc, int level);
PFNWGLCREATELAYERCONTEXT gwglCreateLayerContext;
static PFNWGLCREATELAYERCONTEXT _wglCreateLayerContext;
static HGLRC APIENTRY d_wglCreateLayerContext(HDC hDc, int level) {
	HGLRC ret = _wglCreateLayerContext(hDc, level);
	CheckGLError("wglCreateLayerContext");
	return ret;
}
typedef HPBUFFERARB (APIENTRYP PFNWGLCREATEPBUFFERARB)(HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
PFNWGLCREATEPBUFFERARB gwglCreatePbufferARB;
static PFNWGLCREATEPBUFFERARB _wglCreatePbufferARB;
static HPBUFFERARB APIENTRY d_wglCreatePbufferARB(HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList) {
	HPBUFFERARB ret = _wglCreatePbufferARB(hDC, iPixelFormat, iWidth, iHeight, piAttribList);
	CheckGLError("wglCreatePbufferARB");
	return ret;
}
typedef HPBUFFEREXT (APIENTRYP PFNWGLCREATEPBUFFEREXT)(HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
PFNWGLCREATEPBUFFEREXT gwglCreatePbufferEXT;
static PFNWGLCREATEPBUFFEREXT _wglCreatePbufferEXT;
static HPBUFFEREXT APIENTRY d_wglCreatePbufferEXT(HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList) {
	HPBUFFEREXT ret = _wglCreatePbufferEXT(hDC, iPixelFormat, iWidth, iHeight, piAttribList);
	CheckGLError("wglCreatePbufferEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDELAYBEFORESWAPNV)(HDC hDC, GLfloat seconds);
PFNWGLDELAYBEFORESWAPNV gwglDelayBeforeSwapNV;
static PFNWGLDELAYBEFORESWAPNV _wglDelayBeforeSwapNV;
static BOOL APIENTRY d_wglDelayBeforeSwapNV(HDC hDC, GLfloat seconds) {
	BOOL ret = _wglDelayBeforeSwapNV(hDC, seconds);
	CheckGLError("wglDelayBeforeSwapNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDELETEASSOCIATEDCONTEXTAMD)(HGLRC hglrc);
PFNWGLDELETEASSOCIATEDCONTEXTAMD gwglDeleteAssociatedContextAMD;
static PFNWGLDELETEASSOCIATEDCONTEXTAMD _wglDeleteAssociatedContextAMD;
static BOOL APIENTRY d_wglDeleteAssociatedContextAMD(HGLRC hglrc) {
	BOOL ret = _wglDeleteAssociatedContextAMD(hglrc);
	CheckGLError("wglDeleteAssociatedContextAMD");
	return ret;
}
typedef VOID (APIENTRYP PFNWGLDELETEBUFFERREGIONARB)(HANDLE hRegion);
PFNWGLDELETEBUFFERREGIONARB gwglDeleteBufferRegionARB;
static PFNWGLDELETEBUFFERREGIONARB _wglDeleteBufferRegionARB;
static VOID APIENTRY d_wglDeleteBufferRegionARB(HANDLE hRegion) {
	_wglDeleteBufferRegionARB(hRegion);
	CheckGLError("wglDeleteBufferRegionARB");
}
typedef BOOL (APIENTRYP PFNWGLDELETECONTEXT)(HGLRC oldContext);
PFNWGLDELETECONTEXT gwglDeleteContext;
static PFNWGLDELETECONTEXT _wglDeleteContext;
static BOOL APIENTRY d_wglDeleteContext(HGLRC oldContext) {
	BOOL ret = _wglDeleteContext(oldContext);
	CheckGLError("wglDeleteContext");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDELETEDCNV)(HDC hdc);
PFNWGLDELETEDCNV gwglDeleteDCNV;
static PFNWGLDELETEDCNV _wglDeleteDCNV;
static BOOL APIENTRY d_wglDeleteDCNV(HDC hdc) {
	BOOL ret = _wglDeleteDCNV(hdc);
	CheckGLError("wglDeleteDCNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDESCRIBELAYERPLANE)(HDC hDc, int pixelFormat, int layerPlane, UINT nBytes, const LAYERPLANEDESCRIPTOR *plpd);
PFNWGLDESCRIBELAYERPLANE gwglDescribeLayerPlane;
static PFNWGLDESCRIBELAYERPLANE _wglDescribeLayerPlane;
static BOOL APIENTRY d_wglDescribeLayerPlane(HDC hDc, int pixelFormat, int layerPlane, UINT nBytes, const LAYERPLANEDESCRIPTOR *plpd) {
	BOOL ret = _wglDescribeLayerPlane(hDc, pixelFormat, layerPlane, nBytes, plpd);
	CheckGLError("wglDescribeLayerPlane");
	return ret;
}
typedef VOID (APIENTRYP PFNWGLDESTROYDISPLAYCOLORTABLEEXT)(GLushort id);
PFNWGLDESTROYDISPLAYCOLORTABLEEXT gwglDestroyDisplayColorTableEXT;
static PFNWGLDESTROYDISPLAYCOLORTABLEEXT _wglDestroyDisplayColorTableEXT;
static VOID APIENTRY d_wglDestroyDisplayColorTableEXT(GLushort id) {
	_wglDestroyDisplayColorTableEXT(id);
	CheckGLError("wglDestroyDisplayColorTableEXT");
}
typedef BOOL (APIENTRYP PFNWGLDESTROYIMAGEBUFFERI3D)(HDC hDC, LPVOID pAddress);
PFNWGLDESTROYIMAGEBUFFERI3D gwglDestroyImageBufferI3D;
static PFNWGLDESTROYIMAGEBUFFERI3D _wglDestroyImageBufferI3D;
static BOOL APIENTRY d_wglDestroyImageBufferI3D(HDC hDC, LPVOID pAddress) {
	BOOL ret = _wglDestroyImageBufferI3D(hDC, pAddress);
	CheckGLError("wglDestroyImageBufferI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDESTROYPBUFFERARB)(HPBUFFERARB hPbuffer);
PFNWGLDESTROYPBUFFERARB gwglDestroyPbufferARB;
static PFNWGLDESTROYPBUFFERARB _wglDestroyPbufferARB;
static BOOL APIENTRY d_wglDestroyPbufferARB(HPBUFFERARB hPbuffer) {
	BOOL ret = _wglDestroyPbufferARB(hPbuffer);
	CheckGLError("wglDestroyPbufferARB");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDESTROYPBUFFEREXT)(HPBUFFEREXT hPbuffer);
PFNWGLDESTROYPBUFFEREXT gwglDestroyPbufferEXT;
static PFNWGLDESTROYPBUFFEREXT _wglDestroyPbufferEXT;
static BOOL APIENTRY d_wglDestroyPbufferEXT(HPBUFFEREXT hPbuffer) {
	BOOL ret = _wglDestroyPbufferEXT(hPbuffer);
	CheckGLError("wglDestroyPbufferEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDISABLEFRAMELOCKI3D)();
PFNWGLDISABLEFRAMELOCKI3D gwglDisableFrameLockI3D;
static PFNWGLDISABLEFRAMELOCKI3D _wglDisableFrameLockI3D;
static BOOL APIENTRY d_wglDisableFrameLockI3D() {
	BOOL ret = _wglDisableFrameLockI3D();
	CheckGLError("wglDisableFrameLockI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDISABLEGENLOCKI3D)(HDC hDC);
PFNWGLDISABLEGENLOCKI3D gwglDisableGenlockI3D;
static PFNWGLDISABLEGENLOCKI3D _wglDisableGenlockI3D;
static BOOL APIENTRY d_wglDisableGenlockI3D(HDC hDC) {
	BOOL ret = _wglDisableGenlockI3D(hDC);
	CheckGLError("wglDisableGenlockI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDXCLOSEDEVICENV)(HANDLE hDevice);
PFNWGLDXCLOSEDEVICENV gwglDXCloseDeviceNV;
static PFNWGLDXCLOSEDEVICENV _wglDXCloseDeviceNV;
static BOOL APIENTRY d_wglDXCloseDeviceNV(HANDLE hDevice) {
	BOOL ret = _wglDXCloseDeviceNV(hDevice);
	CheckGLError("wglDXCloseDeviceNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDXLOCKOBJECTSNV)(HANDLE hDevice, GLint count, HANDLE *hObjects);
PFNWGLDXLOCKOBJECTSNV gwglDXLockObjectsNV;
static PFNWGLDXLOCKOBJECTSNV _wglDXLockObjectsNV;
static BOOL APIENTRY d_wglDXLockObjectsNV(HANDLE hDevice, GLint count, HANDLE *hObjects) {
	BOOL ret = _wglDXLockObjectsNV(hDevice, count, hObjects);
	CheckGLError("wglDXLockObjectsNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDXOBJECTACCESSNV)(HANDLE hObject, GLenum access);
PFNWGLDXOBJECTACCESSNV gwglDXObjectAccessNV;
static PFNWGLDXOBJECTACCESSNV _wglDXObjectAccessNV;
static BOOL APIENTRY d_wglDXObjectAccessNV(HANDLE hObject, GLenum access) {
	BOOL ret = _wglDXObjectAccessNV(hObject, access);
	CheckGLError("wglDXObjectAccessNV");
	return ret;
}
typedef HANDLE (APIENTRYP PFNWGLDXOPENDEVICENV)(void *dxDevice);
PFNWGLDXOPENDEVICENV gwglDXOpenDeviceNV;
static PFNWGLDXOPENDEVICENV _wglDXOpenDeviceNV;
static HANDLE APIENTRY d_wglDXOpenDeviceNV(void *dxDevice) {
	HANDLE ret = _wglDXOpenDeviceNV(dxDevice);
	CheckGLError("wglDXOpenDeviceNV");
	return ret;
}
typedef HANDLE (APIENTRYP PFNWGLDXREGISTEROBJECTNV)(HANDLE hDevice, void *dxObject, GLuint name, GLenum type, GLenum access);
PFNWGLDXREGISTEROBJECTNV gwglDXRegisterObjectNV;
static PFNWGLDXREGISTEROBJECTNV _wglDXRegisterObjectNV;
static HANDLE APIENTRY d_wglDXRegisterObjectNV(HANDLE hDevice, void *dxObject, GLuint name, GLenum type, GLenum access) {
	HANDLE ret = _wglDXRegisterObjectNV(hDevice, dxObject, name, type, access);
	CheckGLError("wglDXRegisterObjectNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDXSETRESOURCESHAREHANDLENV)(void *dxObject, HANDLE shareHandle);
PFNWGLDXSETRESOURCESHAREHANDLENV gwglDXSetResourceShareHandleNV;
static PFNWGLDXSETRESOURCESHAREHANDLENV _wglDXSetResourceShareHandleNV;
static BOOL APIENTRY d_wglDXSetResourceShareHandleNV(void *dxObject, HANDLE shareHandle) {
	BOOL ret = _wglDXSetResourceShareHandleNV(dxObject, shareHandle);
	CheckGLError("wglDXSetResourceShareHandleNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDXUNLOCKOBJECTSNV)(HANDLE hDevice, GLint count, HANDLE *hObjects);
PFNWGLDXUNLOCKOBJECTSNV gwglDXUnlockObjectsNV;
static PFNWGLDXUNLOCKOBJECTSNV _wglDXUnlockObjectsNV;
static BOOL APIENTRY d_wglDXUnlockObjectsNV(HANDLE hDevice, GLint count, HANDLE *hObjects) {
	BOOL ret = _wglDXUnlockObjectsNV(hDevice, count, hObjects);
	CheckGLError("wglDXUnlockObjectsNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLDXUNREGISTEROBJECTNV)(HANDLE hDevice, HANDLE hObject);
PFNWGLDXUNREGISTEROBJECTNV gwglDXUnregisterObjectNV;
static PFNWGLDXUNREGISTEROBJECTNV _wglDXUnregisterObjectNV;
static BOOL APIENTRY d_wglDXUnregisterObjectNV(HANDLE hDevice, HANDLE hObject) {
	BOOL ret = _wglDXUnregisterObjectNV(hDevice, hObject);
	CheckGLError("wglDXUnregisterObjectNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLENABLEFRAMELOCKI3D)();
PFNWGLENABLEFRAMELOCKI3D gwglEnableFrameLockI3D;
static PFNWGLENABLEFRAMELOCKI3D _wglEnableFrameLockI3D;
static BOOL APIENTRY d_wglEnableFrameLockI3D() {
	BOOL ret = _wglEnableFrameLockI3D();
	CheckGLError("wglEnableFrameLockI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLENABLEGENLOCKI3D)(HDC hDC);
PFNWGLENABLEGENLOCKI3D gwglEnableGenlockI3D;
static PFNWGLENABLEGENLOCKI3D _wglEnableGenlockI3D;
static BOOL APIENTRY d_wglEnableGenlockI3D(HDC hDC) {
	BOOL ret = _wglEnableGenlockI3D(hDC);
	CheckGLError("wglEnableGenlockI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLENDFRAMETRACKINGI3D)();
PFNWGLENDFRAMETRACKINGI3D gwglEndFrameTrackingI3D;
static PFNWGLENDFRAMETRACKINGI3D _wglEndFrameTrackingI3D;
static BOOL APIENTRY d_wglEndFrameTrackingI3D() {
	BOOL ret = _wglEndFrameTrackingI3D();
	CheckGLError("wglEndFrameTrackingI3D");
	return ret;
}
typedef UINT (APIENTRYP PFNWGLENUMERATEVIDEOCAPTUREDEVICESNV)(HDC hDc, HVIDEOINPUTDEVICENV *phDeviceList);
PFNWGLENUMERATEVIDEOCAPTUREDEVICESNV gwglEnumerateVideoCaptureDevicesNV;
static PFNWGLENUMERATEVIDEOCAPTUREDEVICESNV _wglEnumerateVideoCaptureDevicesNV;
static UINT APIENTRY d_wglEnumerateVideoCaptureDevicesNV(HDC hDc, HVIDEOINPUTDEVICENV *phDeviceList) {
	UINT ret = _wglEnumerateVideoCaptureDevicesNV(hDc, phDeviceList);
	CheckGLError("wglEnumerateVideoCaptureDevicesNV");
	return ret;
}
typedef int (APIENTRYP PFNWGLENUMERATEVIDEODEVICESNV)(HDC hDC, HVIDEOOUTPUTDEVICENV *phDeviceList);
PFNWGLENUMERATEVIDEODEVICESNV gwglEnumerateVideoDevicesNV;
static PFNWGLENUMERATEVIDEODEVICESNV _wglEnumerateVideoDevicesNV;
static int APIENTRY d_wglEnumerateVideoDevicesNV(HDC hDC, HVIDEOOUTPUTDEVICENV *phDeviceList) {
	int ret = _wglEnumerateVideoDevicesNV(hDC, phDeviceList);
	CheckGLError("wglEnumerateVideoDevicesNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLENUMGPUDEVICESNV)(HGPUNV hGpu, UINT iDeviceIndex, PGPU_DEVICE lpGpuDevice);
PFNWGLENUMGPUDEVICESNV gwglEnumGpuDevicesNV;
static PFNWGLENUMGPUDEVICESNV _wglEnumGpuDevicesNV;
static BOOL APIENTRY d_wglEnumGpuDevicesNV(HGPUNV hGpu, UINT iDeviceIndex, PGPU_DEVICE lpGpuDevice) {
	BOOL ret = _wglEnumGpuDevicesNV(hGpu, iDeviceIndex, lpGpuDevice);
	CheckGLError("wglEnumGpuDevicesNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLENUMGPUSFROMAFFINITYDCNV)(HDC hAffinityDC, UINT iGpuIndex, HGPUNV *hGpu);
PFNWGLENUMGPUSFROMAFFINITYDCNV gwglEnumGpusFromAffinityDCNV;
static PFNWGLENUMGPUSFROMAFFINITYDCNV _wglEnumGpusFromAffinityDCNV;
static BOOL APIENTRY d_wglEnumGpusFromAffinityDCNV(HDC hAffinityDC, UINT iGpuIndex, HGPUNV *hGpu) {
	BOOL ret = _wglEnumGpusFromAffinityDCNV(hAffinityDC, iGpuIndex, hGpu);
	CheckGLError("wglEnumGpusFromAffinityDCNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLENUMGPUSNV)(UINT iGpuIndex, HGPUNV *phGpu);
PFNWGLENUMGPUSNV gwglEnumGpusNV;
static PFNWGLENUMGPUSNV _wglEnumGpusNV;
static BOOL APIENTRY d_wglEnumGpusNV(UINT iGpuIndex, HGPUNV *phGpu) {
	BOOL ret = _wglEnumGpusNV(iGpuIndex, phGpu);
	CheckGLError("wglEnumGpusNV");
	return ret;
}
typedef void (APIENTRYP PFNWGLFREEMEMORYNV)(void *pointer);
PFNWGLFREEMEMORYNV gwglFreeMemoryNV;
static PFNWGLFREEMEMORYNV _wglFreeMemoryNV;
static void APIENTRY d_wglFreeMemoryNV(void *pointer) {
	_wglFreeMemoryNV(pointer);
	CheckGLError("wglFreeMemoryNV");
}
typedef BOOL (APIENTRYP PFNWGLGENLOCKSAMPLERATEI3D)(HDC hDC, UINT uRate);
PFNWGLGENLOCKSAMPLERATEI3D gwglGenlockSampleRateI3D;
static PFNWGLGENLOCKSAMPLERATEI3D _wglGenlockSampleRateI3D;
static BOOL APIENTRY d_wglGenlockSampleRateI3D(HDC hDC, UINT uRate) {
	BOOL ret = _wglGenlockSampleRateI3D(hDC, uRate);
	CheckGLError("wglGenlockSampleRateI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGENLOCKSOURCEDELAYI3D)(HDC hDC, UINT uDelay);
PFNWGLGENLOCKSOURCEDELAYI3D gwglGenlockSourceDelayI3D;
static PFNWGLGENLOCKSOURCEDELAYI3D _wglGenlockSourceDelayI3D;
static BOOL APIENTRY d_wglGenlockSourceDelayI3D(HDC hDC, UINT uDelay) {
	BOOL ret = _wglGenlockSourceDelayI3D(hDC, uDelay);
	CheckGLError("wglGenlockSourceDelayI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGENLOCKSOURCEEDGEI3D)(HDC hDC, UINT uEdge);
PFNWGLGENLOCKSOURCEEDGEI3D gwglGenlockSourceEdgeI3D;
static PFNWGLGENLOCKSOURCEEDGEI3D _wglGenlockSourceEdgeI3D;
static BOOL APIENTRY d_wglGenlockSourceEdgeI3D(HDC hDC, UINT uEdge) {
	BOOL ret = _wglGenlockSourceEdgeI3D(hDC, uEdge);
	CheckGLError("wglGenlockSourceEdgeI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGENLOCKSOURCEI3D)(HDC hDC, UINT uSource);
PFNWGLGENLOCKSOURCEI3D gwglGenlockSourceI3D;
static PFNWGLGENLOCKSOURCEI3D _wglGenlockSourceI3D;
static BOOL APIENTRY d_wglGenlockSourceI3D(HDC hDC, UINT uSource) {
	BOOL ret = _wglGenlockSourceI3D(hDC, uSource);
	CheckGLError("wglGenlockSourceI3D");
	return ret;
}
typedef UINT (APIENTRYP PFNWGLGETCONTEXTGPUIDAMD)(HGLRC hglrc);
PFNWGLGETCONTEXTGPUIDAMD gwglGetContextGPUIDAMD;
static PFNWGLGETCONTEXTGPUIDAMD _wglGetContextGPUIDAMD;
static UINT APIENTRY d_wglGetContextGPUIDAMD(HGLRC hglrc) {
	UINT ret = _wglGetContextGPUIDAMD(hglrc);
	CheckGLError("wglGetContextGPUIDAMD");
	return ret;
}
typedef HGLRC (APIENTRYP PFNWGLGETCURRENTASSOCIATEDCONTEXTAMD)();
PFNWGLGETCURRENTASSOCIATEDCONTEXTAMD gwglGetCurrentAssociatedContextAMD;
static PFNWGLGETCURRENTASSOCIATEDCONTEXTAMD _wglGetCurrentAssociatedContextAMD;
static HGLRC APIENTRY d_wglGetCurrentAssociatedContextAMD() {
	HGLRC ret = _wglGetCurrentAssociatedContextAMD();
	CheckGLError("wglGetCurrentAssociatedContextAMD");
	return ret;
}
typedef HGLRC (APIENTRYP PFNWGLGETCURRENTCONTEXT)();
PFNWGLGETCURRENTCONTEXT gwglGetCurrentContext;
static PFNWGLGETCURRENTCONTEXT _wglGetCurrentContext;
static HGLRC APIENTRY d_wglGetCurrentContext() {
	HGLRC ret = _wglGetCurrentContext();
	CheckGLError("wglGetCurrentContext");
	return ret;
}
typedef HDC (APIENTRYP PFNWGLGETCURRENTDC)();
PFNWGLGETCURRENTDC gwglGetCurrentDC;
static PFNWGLGETCURRENTDC _wglGetCurrentDC;
static HDC APIENTRY d_wglGetCurrentDC() {
	HDC ret = _wglGetCurrentDC();
	CheckGLError("wglGetCurrentDC");
	return ret;
}
typedef HDC (APIENTRYP PFNWGLGETCURRENTREADDCARB)();
PFNWGLGETCURRENTREADDCARB gwglGetCurrentReadDCARB;
static PFNWGLGETCURRENTREADDCARB _wglGetCurrentReadDCARB;
static HDC APIENTRY d_wglGetCurrentReadDCARB() {
	HDC ret = _wglGetCurrentReadDCARB();
	CheckGLError("wglGetCurrentReadDCARB");
	return ret;
}
typedef HDC (APIENTRYP PFNWGLGETCURRENTREADDCEXT)();
PFNWGLGETCURRENTREADDCEXT gwglGetCurrentReadDCEXT;
static PFNWGLGETCURRENTREADDCEXT _wglGetCurrentReadDCEXT;
static HDC APIENTRY d_wglGetCurrentReadDCEXT() {
	HDC ret = _wglGetCurrentReadDCEXT();
	CheckGLError("wglGetCurrentReadDCEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETDIGITALVIDEOPARAMETERSI3D)(HDC hDC, int iAttribute, int *piValue);
PFNWGLGETDIGITALVIDEOPARAMETERSI3D gwglGetDigitalVideoParametersI3D;
static PFNWGLGETDIGITALVIDEOPARAMETERSI3D _wglGetDigitalVideoParametersI3D;
static BOOL APIENTRY d_wglGetDigitalVideoParametersI3D(HDC hDC, int iAttribute, int *piValue) {
	BOOL ret = _wglGetDigitalVideoParametersI3D(hDC, iAttribute, piValue);
	CheckGLError("wglGetDigitalVideoParametersI3D");
	return ret;
}
typedef UINT (APIENTRYP PFNGETENHMETAFILEPIXELFORMAT)(HENHMETAFILE hemf, const PIXELFORMATDESCRIPTOR *ppfd);
PFNGETENHMETAFILEPIXELFORMAT gGetEnhMetaFilePixelFormat;
static PFNGETENHMETAFILEPIXELFORMAT _GetEnhMetaFilePixelFormat;
static UINT APIENTRY d_GetEnhMetaFilePixelFormat(HENHMETAFILE hemf, const PIXELFORMATDESCRIPTOR *ppfd) {
	UINT ret = _GetEnhMetaFilePixelFormat(hemf, ppfd);
	CheckGLError("GetEnhMetaFilePixelFormat");
	return ret;
}
typedef const char * (APIENTRYP PFNWGLGETEXTENSIONSSTRINGARB)(HDC hdc);
PFNWGLGETEXTENSIONSSTRINGARB gwglGetExtensionsStringARB;
static PFNWGLGETEXTENSIONSSTRINGARB _wglGetExtensionsStringARB;
static const char * APIENTRY d_wglGetExtensionsStringARB(HDC hdc) {
	const char * ret = _wglGetExtensionsStringARB(hdc);
	CheckGLError("wglGetExtensionsStringARB");
	return ret;
}
typedef const char * (APIENTRYP PFNWGLGETEXTENSIONSSTRINGEXT)();
PFNWGLGETEXTENSIONSSTRINGEXT gwglGetExtensionsStringEXT;
static PFNWGLGETEXTENSIONSSTRINGEXT _wglGetExtensionsStringEXT;
static const char * APIENTRY d_wglGetExtensionsStringEXT() {
	const char * ret = _wglGetExtensionsStringEXT();
	CheckGLError("wglGetExtensionsStringEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETFRAMEUSAGEI3D)(float *pUsage);
PFNWGLGETFRAMEUSAGEI3D gwglGetFrameUsageI3D;
static PFNWGLGETFRAMEUSAGEI3D _wglGetFrameUsageI3D;
static BOOL APIENTRY d_wglGetFrameUsageI3D(float *pUsage) {
	BOOL ret = _wglGetFrameUsageI3D(pUsage);
	CheckGLError("wglGetFrameUsageI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETGAMMATABLEI3D)(HDC hDC, int iEntries, USHORT *puRed, USHORT *puGreen, USHORT *puBlue);
PFNWGLGETGAMMATABLEI3D gwglGetGammaTableI3D;
static PFNWGLGETGAMMATABLEI3D _wglGetGammaTableI3D;
static BOOL APIENTRY d_wglGetGammaTableI3D(HDC hDC, int iEntries, USHORT *puRed, USHORT *puGreen, USHORT *puBlue) {
	BOOL ret = _wglGetGammaTableI3D(hDC, iEntries, puRed, puGreen, puBlue);
	CheckGLError("wglGetGammaTableI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETGAMMATABLEPARAMETERSI3D)(HDC hDC, int iAttribute, int *piValue);
PFNWGLGETGAMMATABLEPARAMETERSI3D gwglGetGammaTableParametersI3D;
static PFNWGLGETGAMMATABLEPARAMETERSI3D _wglGetGammaTableParametersI3D;
static BOOL APIENTRY d_wglGetGammaTableParametersI3D(HDC hDC, int iAttribute, int *piValue) {
	BOOL ret = _wglGetGammaTableParametersI3D(hDC, iAttribute, piValue);
	CheckGLError("wglGetGammaTableParametersI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETGENLOCKSAMPLERATEI3D)(HDC hDC, UINT *uRate);
PFNWGLGETGENLOCKSAMPLERATEI3D gwglGetGenlockSampleRateI3D;
static PFNWGLGETGENLOCKSAMPLERATEI3D _wglGetGenlockSampleRateI3D;
static BOOL APIENTRY d_wglGetGenlockSampleRateI3D(HDC hDC, UINT *uRate) {
	BOOL ret = _wglGetGenlockSampleRateI3D(hDC, uRate);
	CheckGLError("wglGetGenlockSampleRateI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETGENLOCKSOURCEDELAYI3D)(HDC hDC, UINT *uDelay);
PFNWGLGETGENLOCKSOURCEDELAYI3D gwglGetGenlockSourceDelayI3D;
static PFNWGLGETGENLOCKSOURCEDELAYI3D _wglGetGenlockSourceDelayI3D;
static BOOL APIENTRY d_wglGetGenlockSourceDelayI3D(HDC hDC, UINT *uDelay) {
	BOOL ret = _wglGetGenlockSourceDelayI3D(hDC, uDelay);
	CheckGLError("wglGetGenlockSourceDelayI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETGENLOCKSOURCEEDGEI3D)(HDC hDC, UINT *uEdge);
PFNWGLGETGENLOCKSOURCEEDGEI3D gwglGetGenlockSourceEdgeI3D;
static PFNWGLGETGENLOCKSOURCEEDGEI3D _wglGetGenlockSourceEdgeI3D;
static BOOL APIENTRY d_wglGetGenlockSourceEdgeI3D(HDC hDC, UINT *uEdge) {
	BOOL ret = _wglGetGenlockSourceEdgeI3D(hDC, uEdge);
	CheckGLError("wglGetGenlockSourceEdgeI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETGENLOCKSOURCEI3D)(HDC hDC, UINT *uSource);
PFNWGLGETGENLOCKSOURCEI3D gwglGetGenlockSourceI3D;
static PFNWGLGETGENLOCKSOURCEI3D _wglGetGenlockSourceI3D;
static BOOL APIENTRY d_wglGetGenlockSourceI3D(HDC hDC, UINT *uSource) {
	BOOL ret = _wglGetGenlockSourceI3D(hDC, uSource);
	CheckGLError("wglGetGenlockSourceI3D");
	return ret;
}
typedef UINT (APIENTRYP PFNWGLGETGPUIDSAMD)(UINT maxCount, UINT *ids);
PFNWGLGETGPUIDSAMD gwglGetGPUIDsAMD;
static PFNWGLGETGPUIDSAMD _wglGetGPUIDsAMD;
static UINT APIENTRY d_wglGetGPUIDsAMD(UINT maxCount, UINT *ids) {
	UINT ret = _wglGetGPUIDsAMD(maxCount, ids);
	CheckGLError("wglGetGPUIDsAMD");
	return ret;
}
typedef INT (APIENTRYP PFNWGLGETGPUINFOAMD)(UINT id, int property, GLenum dataType, UINT size, void *data);
PFNWGLGETGPUINFOAMD gwglGetGPUInfoAMD;
static PFNWGLGETGPUINFOAMD _wglGetGPUInfoAMD;
static INT APIENTRY d_wglGetGPUInfoAMD(UINT id, int property, GLenum dataType, UINT size, void *data) {
	INT ret = _wglGetGPUInfoAMD(id, property, dataType, size, data);
	CheckGLError("wglGetGPUInfoAMD");
	return ret;
}
typedef int (APIENTRYP PFNWGLGETLAYERPALETTEENTRIES)(HDC hdc, int iLayerPlane, int iStart, int cEntries, const COLORREF *pcr);
PFNWGLGETLAYERPALETTEENTRIES gwglGetLayerPaletteEntries;
static PFNWGLGETLAYERPALETTEENTRIES _wglGetLayerPaletteEntries;
static int APIENTRY d_wglGetLayerPaletteEntries(HDC hdc, int iLayerPlane, int iStart, int cEntries, const COLORREF *pcr) {
	int ret = _wglGetLayerPaletteEntries(hdc, iLayerPlane, iStart, cEntries, pcr);
	CheckGLError("wglGetLayerPaletteEntries");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETMSCRATEOML)(HDC hdc, INT32 *numerator, INT32 *denominator);
PFNWGLGETMSCRATEOML gwglGetMscRateOML;
static PFNWGLGETMSCRATEOML _wglGetMscRateOML;
static BOOL APIENTRY d_wglGetMscRateOML(HDC hdc, INT32 *numerator, INT32 *denominator) {
	BOOL ret = _wglGetMscRateOML(hdc, numerator, denominator);
	CheckGLError("wglGetMscRateOML");
	return ret;
}
typedef HDC (APIENTRYP PFNWGLGETPBUFFERDCARB)(HPBUFFERARB hPbuffer);
PFNWGLGETPBUFFERDCARB gwglGetPbufferDCARB;
static PFNWGLGETPBUFFERDCARB _wglGetPbufferDCARB;
static HDC APIENTRY d_wglGetPbufferDCARB(HPBUFFERARB hPbuffer) {
	HDC ret = _wglGetPbufferDCARB(hPbuffer);
	CheckGLError("wglGetPbufferDCARB");
	return ret;
}
typedef HDC (APIENTRYP PFNWGLGETPBUFFERDCEXT)(HPBUFFEREXT hPbuffer);
PFNWGLGETPBUFFERDCEXT gwglGetPbufferDCEXT;
static PFNWGLGETPBUFFERDCEXT _wglGetPbufferDCEXT;
static HDC APIENTRY d_wglGetPbufferDCEXT(HPBUFFEREXT hPbuffer) {
	HDC ret = _wglGetPbufferDCEXT(hPbuffer);
	CheckGLError("wglGetPbufferDCEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETPIXELFORMATATTRIBFVARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
PFNWGLGETPIXELFORMATATTRIBFVARB gwglGetPixelFormatAttribfvARB;
static PFNWGLGETPIXELFORMATATTRIBFVARB _wglGetPixelFormatAttribfvARB;
static BOOL APIENTRY d_wglGetPixelFormatAttribfvARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues) {
	BOOL ret = _wglGetPixelFormatAttribfvARB(hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues);
	CheckGLError("wglGetPixelFormatAttribfvARB");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETPIXELFORMATATTRIBFVEXT)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, FLOAT *pfValues);
PFNWGLGETPIXELFORMATATTRIBFVEXT gwglGetPixelFormatAttribfvEXT;
static PFNWGLGETPIXELFORMATATTRIBFVEXT _wglGetPixelFormatAttribfvEXT;
static BOOL APIENTRY d_wglGetPixelFormatAttribfvEXT(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, FLOAT *pfValues) {
	BOOL ret = _wglGetPixelFormatAttribfvEXT(hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues);
	CheckGLError("wglGetPixelFormatAttribfvEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETPIXELFORMATATTRIBIVARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
PFNWGLGETPIXELFORMATATTRIBIVARB gwglGetPixelFormatAttribivARB;
static PFNWGLGETPIXELFORMATATTRIBIVARB _wglGetPixelFormatAttribivARB;
static BOOL APIENTRY d_wglGetPixelFormatAttribivARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues) {
	BOOL ret = _wglGetPixelFormatAttribivARB(hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues);
	CheckGLError("wglGetPixelFormatAttribivARB");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETPIXELFORMATATTRIBIVEXT)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, int *piValues);
PFNWGLGETPIXELFORMATATTRIBIVEXT gwglGetPixelFormatAttribivEXT;
static PFNWGLGETPIXELFORMATATTRIBIVEXT _wglGetPixelFormatAttribivEXT;
static BOOL APIENTRY d_wglGetPixelFormatAttribivEXT(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, int *piValues) {
	BOOL ret = _wglGetPixelFormatAttribivEXT(hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues);
	CheckGLError("wglGetPixelFormatAttribivEXT");
	return ret;
}
typedef PROC (APIENTRYP PFNWGLGETPROCADDRESS)(LPCSTR lpszProc);
PFNWGLGETPROCADDRESS gwglGetProcAddress;
static PFNWGLGETPROCADDRESS _wglGetProcAddress;
static PROC APIENTRY d_wglGetProcAddress(LPCSTR lpszProc) {
	PROC ret = _wglGetProcAddress(lpszProc);
	CheckGLError("wglGetProcAddress");
	return ret;
}
typedef int (APIENTRYP PFNWGLGETSWAPINTERVALEXT)();
PFNWGLGETSWAPINTERVALEXT gwglGetSwapIntervalEXT;
static PFNWGLGETSWAPINTERVALEXT _wglGetSwapIntervalEXT;
static int APIENTRY d_wglGetSwapIntervalEXT() {
	int ret = _wglGetSwapIntervalEXT();
	CheckGLError("wglGetSwapIntervalEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETSYNCVALUESOML)(HDC hdc, INT64 *ust, INT64 *msc, INT64 *sbc);
PFNWGLGETSYNCVALUESOML gwglGetSyncValuesOML;
static PFNWGLGETSYNCVALUESOML _wglGetSyncValuesOML;
static BOOL APIENTRY d_wglGetSyncValuesOML(HDC hdc, INT64 *ust, INT64 *msc, INT64 *sbc) {
	BOOL ret = _wglGetSyncValuesOML(hdc, ust, msc, sbc);
	CheckGLError("wglGetSyncValuesOML");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETVIDEODEVICENV)(HDC hDC, int numDevices, HPVIDEODEV *hVideoDevice);
PFNWGLGETVIDEODEVICENV gwglGetVideoDeviceNV;
static PFNWGLGETVIDEODEVICENV _wglGetVideoDeviceNV;
static BOOL APIENTRY d_wglGetVideoDeviceNV(HDC hDC, int numDevices, HPVIDEODEV *hVideoDevice) {
	BOOL ret = _wglGetVideoDeviceNV(hDC, numDevices, hVideoDevice);
	CheckGLError("wglGetVideoDeviceNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLGETVIDEOINFONV)(HPVIDEODEV hpVideoDevice, unsigned long *pulCounterOutputPbuffer, unsigned long *pulCounterOutputVideo);
PFNWGLGETVIDEOINFONV gwglGetVideoInfoNV;
static PFNWGLGETVIDEOINFONV _wglGetVideoInfoNV;
static BOOL APIENTRY d_wglGetVideoInfoNV(HPVIDEODEV hpVideoDevice, unsigned long *pulCounterOutputPbuffer, unsigned long *pulCounterOutputVideo) {
	BOOL ret = _wglGetVideoInfoNV(hpVideoDevice, pulCounterOutputPbuffer, pulCounterOutputVideo);
	CheckGLError("wglGetVideoInfoNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLISENABLEDFRAMELOCKI3D)(BOOL *pFlag);
PFNWGLISENABLEDFRAMELOCKI3D gwglIsEnabledFrameLockI3D;
static PFNWGLISENABLEDFRAMELOCKI3D _wglIsEnabledFrameLockI3D;
static BOOL APIENTRY d_wglIsEnabledFrameLockI3D(BOOL *pFlag) {
	BOOL ret = _wglIsEnabledFrameLockI3D(pFlag);
	CheckGLError("wglIsEnabledFrameLockI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLISENABLEDGENLOCKI3D)(HDC hDC, BOOL *pFlag);
PFNWGLISENABLEDGENLOCKI3D gwglIsEnabledGenlockI3D;
static PFNWGLISENABLEDGENLOCKI3D _wglIsEnabledGenlockI3D;
static BOOL APIENTRY d_wglIsEnabledGenlockI3D(HDC hDC, BOOL *pFlag) {
	BOOL ret = _wglIsEnabledGenlockI3D(hDC, pFlag);
	CheckGLError("wglIsEnabledGenlockI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLJOINSWAPGROUPNV)(HDC hDC, GLuint group);
PFNWGLJOINSWAPGROUPNV gwglJoinSwapGroupNV;
static PFNWGLJOINSWAPGROUPNV _wglJoinSwapGroupNV;
static BOOL APIENTRY d_wglJoinSwapGroupNV(HDC hDC, GLuint group) {
	BOOL ret = _wglJoinSwapGroupNV(hDC, group);
	CheckGLError("wglJoinSwapGroupNV");
	return ret;
}
typedef GLboolean (APIENTRYP PFNWGLLOADDISPLAYCOLORTABLEEXT)(const GLushort *table, GLuint length);
PFNWGLLOADDISPLAYCOLORTABLEEXT gwglLoadDisplayColorTableEXT;
static PFNWGLLOADDISPLAYCOLORTABLEEXT _wglLoadDisplayColorTableEXT;
static GLboolean APIENTRY d_wglLoadDisplayColorTableEXT(const GLushort *table, GLuint length) {
	GLboolean ret = _wglLoadDisplayColorTableEXT(table, length);
	CheckGLError("wglLoadDisplayColorTableEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLLOCKVIDEOCAPTUREDEVICENV)(HDC hDc, HVIDEOINPUTDEVICENV hDevice);
PFNWGLLOCKVIDEOCAPTUREDEVICENV gwglLockVideoCaptureDeviceNV;
static PFNWGLLOCKVIDEOCAPTUREDEVICENV _wglLockVideoCaptureDeviceNV;
static BOOL APIENTRY d_wglLockVideoCaptureDeviceNV(HDC hDc, HVIDEOINPUTDEVICENV hDevice) {
	BOOL ret = _wglLockVideoCaptureDeviceNV(hDc, hDevice);
	CheckGLError("wglLockVideoCaptureDeviceNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMD)(HGLRC hglrc);
PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMD gwglMakeAssociatedContextCurrentAMD;
static PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMD _wglMakeAssociatedContextCurrentAMD;
static BOOL APIENTRY d_wglMakeAssociatedContextCurrentAMD(HGLRC hglrc) {
	BOOL ret = _wglMakeAssociatedContextCurrentAMD(hglrc);
	CheckGLError("wglMakeAssociatedContextCurrentAMD");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLMAKECONTEXTCURRENTARB)(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
PFNWGLMAKECONTEXTCURRENTARB gwglMakeContextCurrentARB;
static PFNWGLMAKECONTEXTCURRENTARB _wglMakeContextCurrentARB;
static BOOL APIENTRY d_wglMakeContextCurrentARB(HDC hDrawDC, HDC hReadDC, HGLRC hglrc) {
	BOOL ret = _wglMakeContextCurrentARB(hDrawDC, hReadDC, hglrc);
	CheckGLError("wglMakeContextCurrentARB");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLMAKECONTEXTCURRENTEXT)(HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
PFNWGLMAKECONTEXTCURRENTEXT gwglMakeContextCurrentEXT;
static PFNWGLMAKECONTEXTCURRENTEXT _wglMakeContextCurrentEXT;
static BOOL APIENTRY d_wglMakeContextCurrentEXT(HDC hDrawDC, HDC hReadDC, HGLRC hglrc) {
	BOOL ret = _wglMakeContextCurrentEXT(hDrawDC, hReadDC, hglrc);
	CheckGLError("wglMakeContextCurrentEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLMAKECURRENT)(HDC hDc, HGLRC newContext);
PFNWGLMAKECURRENT gwglMakeCurrent;
static PFNWGLMAKECURRENT _wglMakeCurrent;
static BOOL APIENTRY d_wglMakeCurrent(HDC hDc, HGLRC newContext) {
	BOOL ret = _wglMakeCurrent(hDc, newContext);
	CheckGLError("wglMakeCurrent");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLQUERYCURRENTCONTEXTNV)(int iAttribute, int *piValue);
PFNWGLQUERYCURRENTCONTEXTNV gwglQueryCurrentContextNV;
static PFNWGLQUERYCURRENTCONTEXTNV _wglQueryCurrentContextNV;
static BOOL APIENTRY d_wglQueryCurrentContextNV(int iAttribute, int *piValue) {
	BOOL ret = _wglQueryCurrentContextNV(iAttribute, piValue);
	CheckGLError("wglQueryCurrentContextNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLQUERYFRAMECOUNTNV)(HDC hDC, GLuint *count);
PFNWGLQUERYFRAMECOUNTNV gwglQueryFrameCountNV;
static PFNWGLQUERYFRAMECOUNTNV _wglQueryFrameCountNV;
static BOOL APIENTRY d_wglQueryFrameCountNV(HDC hDC, GLuint *count) {
	BOOL ret = _wglQueryFrameCountNV(hDC, count);
	CheckGLError("wglQueryFrameCountNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLQUERYFRAMELOCKMASTERI3D)(BOOL *pFlag);
PFNWGLQUERYFRAMELOCKMASTERI3D gwglQueryFrameLockMasterI3D;
static PFNWGLQUERYFRAMELOCKMASTERI3D _wglQueryFrameLockMasterI3D;
static BOOL APIENTRY d_wglQueryFrameLockMasterI3D(BOOL *pFlag) {
	BOOL ret = _wglQueryFrameLockMasterI3D(pFlag);
	CheckGLError("wglQueryFrameLockMasterI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLQUERYFRAMETRACKINGI3D)(DWORD *pFrameCount, DWORD *pMissedFrames, float *pLastMissedUsage);
PFNWGLQUERYFRAMETRACKINGI3D gwglQueryFrameTrackingI3D;
static PFNWGLQUERYFRAMETRACKINGI3D _wglQueryFrameTrackingI3D;
static BOOL APIENTRY d_wglQueryFrameTrackingI3D(DWORD *pFrameCount, DWORD *pMissedFrames, float *pLastMissedUsage) {
	BOOL ret = _wglQueryFrameTrackingI3D(pFrameCount, pMissedFrames, pLastMissedUsage);
	CheckGLError("wglQueryFrameTrackingI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLQUERYGENLOCKMAXSOURCEDELAYI3D)(HDC hDC, UINT *uMaxLineDelay, UINT *uMaxPixelDelay);
PFNWGLQUERYGENLOCKMAXSOURCEDELAYI3D gwglQueryGenlockMaxSourceDelayI3D;
static PFNWGLQUERYGENLOCKMAXSOURCEDELAYI3D _wglQueryGenlockMaxSourceDelayI3D;
static BOOL APIENTRY d_wglQueryGenlockMaxSourceDelayI3D(HDC hDC, UINT *uMaxLineDelay, UINT *uMaxPixelDelay) {
	BOOL ret = _wglQueryGenlockMaxSourceDelayI3D(hDC, uMaxLineDelay, uMaxPixelDelay);
	CheckGLError("wglQueryGenlockMaxSourceDelayI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLQUERYMAXSWAPGROUPSNV)(HDC hDC, GLuint *maxGroups, GLuint *maxBarriers);
PFNWGLQUERYMAXSWAPGROUPSNV gwglQueryMaxSwapGroupsNV;
static PFNWGLQUERYMAXSWAPGROUPSNV _wglQueryMaxSwapGroupsNV;
static BOOL APIENTRY d_wglQueryMaxSwapGroupsNV(HDC hDC, GLuint *maxGroups, GLuint *maxBarriers) {
	BOOL ret = _wglQueryMaxSwapGroupsNV(hDC, maxGroups, maxBarriers);
	CheckGLError("wglQueryMaxSwapGroupsNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLQUERYPBUFFERARB)(HPBUFFERARB hPbuffer, int iAttribute, int *piValue);
PFNWGLQUERYPBUFFERARB gwglQueryPbufferARB;
static PFNWGLQUERYPBUFFERARB _wglQueryPbufferARB;
static BOOL APIENTRY d_wglQueryPbufferARB(HPBUFFERARB hPbuffer, int iAttribute, int *piValue) {
	BOOL ret = _wglQueryPbufferARB(hPbuffer, iAttribute, piValue);
	CheckGLError("wglQueryPbufferARB");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLQUERYPBUFFEREXT)(HPBUFFEREXT hPbuffer, int iAttribute, int *piValue);
PFNWGLQUERYPBUFFEREXT gwglQueryPbufferEXT;
static PFNWGLQUERYPBUFFEREXT _wglQueryPbufferEXT;
static BOOL APIENTRY d_wglQueryPbufferEXT(HPBUFFEREXT hPbuffer, int iAttribute, int *piValue) {
	BOOL ret = _wglQueryPbufferEXT(hPbuffer, iAttribute, piValue);
	CheckGLError("wglQueryPbufferEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLQUERYSWAPGROUPNV)(HDC hDC, GLuint *group, GLuint *barrier);
PFNWGLQUERYSWAPGROUPNV gwglQuerySwapGroupNV;
static PFNWGLQUERYSWAPGROUPNV _wglQuerySwapGroupNV;
static BOOL APIENTRY d_wglQuerySwapGroupNV(HDC hDC, GLuint *group, GLuint *barrier) {
	BOOL ret = _wglQuerySwapGroupNV(hDC, group, barrier);
	CheckGLError("wglQuerySwapGroupNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLQUERYVIDEOCAPTUREDEVICENV)(HDC hDc, HVIDEOINPUTDEVICENV hDevice, int iAttribute, int *piValue);
PFNWGLQUERYVIDEOCAPTUREDEVICENV gwglQueryVideoCaptureDeviceNV;
static PFNWGLQUERYVIDEOCAPTUREDEVICENV _wglQueryVideoCaptureDeviceNV;
static BOOL APIENTRY d_wglQueryVideoCaptureDeviceNV(HDC hDc, HVIDEOINPUTDEVICENV hDevice, int iAttribute, int *piValue) {
	BOOL ret = _wglQueryVideoCaptureDeviceNV(hDc, hDevice, iAttribute, piValue);
	CheckGLError("wglQueryVideoCaptureDeviceNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLREALIZELAYERPALETTE)(HDC hdc, int iLayerPlane, BOOL bRealize);
PFNWGLREALIZELAYERPALETTE gwglRealizeLayerPalette;
static PFNWGLREALIZELAYERPALETTE _wglRealizeLayerPalette;
static BOOL APIENTRY d_wglRealizeLayerPalette(HDC hdc, int iLayerPlane, BOOL bRealize) {
	BOOL ret = _wglRealizeLayerPalette(hdc, iLayerPlane, bRealize);
	CheckGLError("wglRealizeLayerPalette");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLRELEASEIMAGEBUFFEREVENTSI3D)(HDC hDC, const LPVOID *pAddress, UINT count);
PFNWGLRELEASEIMAGEBUFFEREVENTSI3D gwglReleaseImageBufferEventsI3D;
static PFNWGLRELEASEIMAGEBUFFEREVENTSI3D _wglReleaseImageBufferEventsI3D;
static BOOL APIENTRY d_wglReleaseImageBufferEventsI3D(HDC hDC, const LPVOID *pAddress, UINT count) {
	BOOL ret = _wglReleaseImageBufferEventsI3D(hDC, pAddress, count);
	CheckGLError("wglReleaseImageBufferEventsI3D");
	return ret;
}
typedef int (APIENTRYP PFNWGLRELEASEPBUFFERDCARB)(HPBUFFERARB hPbuffer, HDC hDC);
PFNWGLRELEASEPBUFFERDCARB gwglReleasePbufferDCARB;
static PFNWGLRELEASEPBUFFERDCARB _wglReleasePbufferDCARB;
static int APIENTRY d_wglReleasePbufferDCARB(HPBUFFERARB hPbuffer, HDC hDC) {
	int ret = _wglReleasePbufferDCARB(hPbuffer, hDC);
	CheckGLError("wglReleasePbufferDCARB");
	return ret;
}
typedef int (APIENTRYP PFNWGLRELEASEPBUFFERDCEXT)(HPBUFFEREXT hPbuffer, HDC hDC);
PFNWGLRELEASEPBUFFERDCEXT gwglReleasePbufferDCEXT;
static PFNWGLRELEASEPBUFFERDCEXT _wglReleasePbufferDCEXT;
static int APIENTRY d_wglReleasePbufferDCEXT(HPBUFFEREXT hPbuffer, HDC hDC) {
	int ret = _wglReleasePbufferDCEXT(hPbuffer, hDC);
	CheckGLError("wglReleasePbufferDCEXT");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLRELEASETEXIMAGEARB)(HPBUFFERARB hPbuffer, int iBuffer);
PFNWGLRELEASETEXIMAGEARB gwglReleaseTexImageARB;
static PFNWGLRELEASETEXIMAGEARB _wglReleaseTexImageARB;
static BOOL APIENTRY d_wglReleaseTexImageARB(HPBUFFERARB hPbuffer, int iBuffer) {
	BOOL ret = _wglReleaseTexImageARB(hPbuffer, iBuffer);
	CheckGLError("wglReleaseTexImageARB");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLRELEASEVIDEOCAPTUREDEVICENV)(HDC hDc, HVIDEOINPUTDEVICENV hDevice);
PFNWGLRELEASEVIDEOCAPTUREDEVICENV gwglReleaseVideoCaptureDeviceNV;
static PFNWGLRELEASEVIDEOCAPTUREDEVICENV _wglReleaseVideoCaptureDeviceNV;
static BOOL APIENTRY d_wglReleaseVideoCaptureDeviceNV(HDC hDc, HVIDEOINPUTDEVICENV hDevice) {
	BOOL ret = _wglReleaseVideoCaptureDeviceNV(hDc, hDevice);
	CheckGLError("wglReleaseVideoCaptureDeviceNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLRELEASEVIDEODEVICENV)(HPVIDEODEV hVideoDevice);
PFNWGLRELEASEVIDEODEVICENV gwglReleaseVideoDeviceNV;
static PFNWGLRELEASEVIDEODEVICENV _wglReleaseVideoDeviceNV;
static BOOL APIENTRY d_wglReleaseVideoDeviceNV(HPVIDEODEV hVideoDevice) {
	BOOL ret = _wglReleaseVideoDeviceNV(hVideoDevice);
	CheckGLError("wglReleaseVideoDeviceNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLRELEASEVIDEOIMAGENV)(HPBUFFERARB hPbuffer, int iVideoBuffer);
PFNWGLRELEASEVIDEOIMAGENV gwglReleaseVideoImageNV;
static PFNWGLRELEASEVIDEOIMAGENV _wglReleaseVideoImageNV;
static BOOL APIENTRY d_wglReleaseVideoImageNV(HPBUFFERARB hPbuffer, int iVideoBuffer) {
	BOOL ret = _wglReleaseVideoImageNV(hPbuffer, iVideoBuffer);
	CheckGLError("wglReleaseVideoImageNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLRESETFRAMECOUNTNV)(HDC hDC);
PFNWGLRESETFRAMECOUNTNV gwglResetFrameCountNV;
static PFNWGLRESETFRAMECOUNTNV _wglResetFrameCountNV;
static BOOL APIENTRY d_wglResetFrameCountNV(HDC hDC) {
	BOOL ret = _wglResetFrameCountNV(hDC);
	CheckGLError("wglResetFrameCountNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLRESTOREBUFFERREGIONARB)(HANDLE hRegion, int x, int y, int width, int height, int xSrc, int ySrc);
PFNWGLRESTOREBUFFERREGIONARB gwglRestoreBufferRegionARB;
static PFNWGLRESTOREBUFFERREGIONARB _wglRestoreBufferRegionARB;
static BOOL APIENTRY d_wglRestoreBufferRegionARB(HANDLE hRegion, int x, int y, int width, int height, int xSrc, int ySrc) {
	BOOL ret = _wglRestoreBufferRegionARB(hRegion, x, y, width, height, xSrc, ySrc);
	CheckGLError("wglRestoreBufferRegionARB");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLSAVEBUFFERREGIONARB)(HANDLE hRegion, int x, int y, int width, int height);
PFNWGLSAVEBUFFERREGIONARB gwglSaveBufferRegionARB;
static PFNWGLSAVEBUFFERREGIONARB _wglSaveBufferRegionARB;
static BOOL APIENTRY d_wglSaveBufferRegionARB(HANDLE hRegion, int x, int y, int width, int height) {
	BOOL ret = _wglSaveBufferRegionARB(hRegion, x, y, width, height);
	CheckGLError("wglSaveBufferRegionARB");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLSENDPBUFFERTOVIDEONV)(HPBUFFERARB hPbuffer, int iBufferType, unsigned long *pulCounterPbuffer, BOOL bBlock);
PFNWGLSENDPBUFFERTOVIDEONV gwglSendPbufferToVideoNV;
static PFNWGLSENDPBUFFERTOVIDEONV _wglSendPbufferToVideoNV;
static BOOL APIENTRY d_wglSendPbufferToVideoNV(HPBUFFERARB hPbuffer, int iBufferType, unsigned long *pulCounterPbuffer, BOOL bBlock) {
	BOOL ret = _wglSendPbufferToVideoNV(hPbuffer, iBufferType, pulCounterPbuffer, bBlock);
	CheckGLError("wglSendPbufferToVideoNV");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLSETDIGITALVIDEOPARAMETERSI3D)(HDC hDC, int iAttribute, const int *piValue);
PFNWGLSETDIGITALVIDEOPARAMETERSI3D gwglSetDigitalVideoParametersI3D;
static PFNWGLSETDIGITALVIDEOPARAMETERSI3D _wglSetDigitalVideoParametersI3D;
static BOOL APIENTRY d_wglSetDigitalVideoParametersI3D(HDC hDC, int iAttribute, const int *piValue) {
	BOOL ret = _wglSetDigitalVideoParametersI3D(hDC, iAttribute, piValue);
	CheckGLError("wglSetDigitalVideoParametersI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLSETGAMMATABLEI3D)(HDC hDC, int iEntries, const USHORT *puRed, const USHORT *puGreen, const USHORT *puBlue);
PFNWGLSETGAMMATABLEI3D gwglSetGammaTableI3D;
static PFNWGLSETGAMMATABLEI3D _wglSetGammaTableI3D;
static BOOL APIENTRY d_wglSetGammaTableI3D(HDC hDC, int iEntries, const USHORT *puRed, const USHORT *puGreen, const USHORT *puBlue) {
	BOOL ret = _wglSetGammaTableI3D(hDC, iEntries, puRed, puGreen, puBlue);
	CheckGLError("wglSetGammaTableI3D");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLSETGAMMATABLEPARAMETERSI3D)(HDC hDC, int iAttribute, const int *piValue);
PFNWGLSETGAMMATABLEPARAMETERSI3D gwglSetGammaTableParametersI3D;
static PFNWGLSETGAMMATABLEPARAMETERSI3D _wglSetGammaTableParametersI3D;
static BOOL APIENTRY d_wglSetGammaTableParametersI3D(HDC hDC, int iAttribute, const int *piValue) {
	BOOL ret = _wglSetGammaTableParametersI3D(hDC, iAttribute, piValue);
	CheckGLError("wglSetGammaTableParametersI3D");
	return ret;
}
typedef int (APIENTRYP PFNWGLSETLAYERPALETTEENTRIES)(HDC hdc, int iLayerPlane, int iStart, int cEntries, const COLORREF *pcr);
PFNWGLSETLAYERPALETTEENTRIES gwglSetLayerPaletteEntries;
static PFNWGLSETLAYERPALETTEENTRIES _wglSetLayerPaletteEntries;
static int APIENTRY d_wglSetLayerPaletteEntries(HDC hdc, int iLayerPlane, int iStart, int cEntries, const COLORREF *pcr) {
	int ret = _wglSetLayerPaletteEntries(hdc, iLayerPlane, iStart, cEntries, pcr);
	CheckGLError("wglSetLayerPaletteEntries");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLSETPBUFFERATTRIBARB)(HPBUFFERARB hPbuffer, const int *piAttribList);
PFNWGLSETPBUFFERATTRIBARB gwglSetPbufferAttribARB;
static PFNWGLSETPBUFFERATTRIBARB _wglSetPbufferAttribARB;
static BOOL APIENTRY d_wglSetPbufferAttribARB(HPBUFFERARB hPbuffer, const int *piAttribList) {
	BOOL ret = _wglSetPbufferAttribARB(hPbuffer, piAttribList);
	CheckGLError("wglSetPbufferAttribARB");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLSETSTEREOEMITTERSTATE3DL)(HDC hDC, UINT uState);
PFNWGLSETSTEREOEMITTERSTATE3DL gwglSetStereoEmitterState3DL;
static PFNWGLSETSTEREOEMITTERSTATE3DL _wglSetStereoEmitterState3DL;
static BOOL APIENTRY d_wglSetStereoEmitterState3DL(HDC hDC, UINT uState) {
	BOOL ret = _wglSetStereoEmitterState3DL(hDC, uState);
	CheckGLError("wglSetStereoEmitterState3DL");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLSHARELISTS)(HGLRC hrcSrvShare, HGLRC hrcSrvSource);
PFNWGLSHARELISTS gwglShareLists;
static PFNWGLSHARELISTS _wglShareLists;
static BOOL APIENTRY d_wglShareLists(HGLRC hrcSrvShare, HGLRC hrcSrvSource) {
	BOOL ret = _wglShareLists(hrcSrvShare, hrcSrvSource);
	CheckGLError("wglShareLists");
	return ret;
}
typedef INT64 (APIENTRYP PFNWGLSWAPBUFFERSMSCOML)(HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder);
PFNWGLSWAPBUFFERSMSCOML gwglSwapBuffersMscOML;
static PFNWGLSWAPBUFFERSMSCOML _wglSwapBuffersMscOML;
static INT64 APIENTRY d_wglSwapBuffersMscOML(HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder) {
	INT64 ret = _wglSwapBuffersMscOML(hdc, target_msc, divisor, remainder);
	CheckGLError("wglSwapBuffersMscOML");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLSWAPLAYERBUFFERS)(HDC hdc, UINT fuFlags);
PFNWGLSWAPLAYERBUFFERS gwglSwapLayerBuffers;
static PFNWGLSWAPLAYERBUFFERS _wglSwapLayerBuffers;
static BOOL APIENTRY d_wglSwapLayerBuffers(HDC hdc, UINT fuFlags) {
	BOOL ret = _wglSwapLayerBuffers(hdc, fuFlags);
	CheckGLError("wglSwapLayerBuffers");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLSWAPINTERVALEXT)(int interval);
PFNWGLSWAPINTERVALEXT gwglSwapIntervalEXT;
static PFNWGLSWAPINTERVALEXT _wglSwapIntervalEXT;
static BOOL APIENTRY d_wglSwapIntervalEXT(int interval) {
	BOOL ret = _wglSwapIntervalEXT(interval);
	CheckGLError("wglSwapIntervalEXT");
	return ret;
}
typedef INT64 (APIENTRYP PFNWGLSWAPLAYERBUFFERSMSCOML)(HDC hdc, int fuPlanes, INT64 target_msc, INT64 divisor, INT64 remainder);
PFNWGLSWAPLAYERBUFFERSMSCOML gwglSwapLayerBuffersMscOML;
static PFNWGLSWAPLAYERBUFFERSMSCOML _wglSwapLayerBuffersMscOML;
static INT64 APIENTRY d_wglSwapLayerBuffersMscOML(HDC hdc, int fuPlanes, INT64 target_msc, INT64 divisor, INT64 remainder) {
	INT64 ret = _wglSwapLayerBuffersMscOML(hdc, fuPlanes, target_msc, divisor, remainder);
	CheckGLError("wglSwapLayerBuffersMscOML");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLUSEFONTBITMAPS)(HDC hDC, DWORD first, DWORD count, DWORD listBase);
PFNWGLUSEFONTBITMAPS gwglUseFontBitmaps;
static PFNWGLUSEFONTBITMAPS _wglUseFontBitmaps;
static BOOL APIENTRY d_wglUseFontBitmaps(HDC hDC, DWORD first, DWORD count, DWORD listBase) {
	BOOL ret = _wglUseFontBitmaps(hDC, first, count, listBase);
	CheckGLError("wglUseFontBitmaps");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLUSEFONTBITMAPSA)(HDC hDC, DWORD first, DWORD count, DWORD listBase);
PFNWGLUSEFONTBITMAPSA gwglUseFontBitmapsA;
static PFNWGLUSEFONTBITMAPSA _wglUseFontBitmapsA;
static BOOL APIENTRY d_wglUseFontBitmapsA(HDC hDC, DWORD first, DWORD count, DWORD listBase) {
	BOOL ret = _wglUseFontBitmapsA(hDC, first, count, listBase);
	CheckGLError("wglUseFontBitmapsA");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLUSEFONTBITMAPSW)(HDC hDC, DWORD first, DWORD count, DWORD listBase);
PFNWGLUSEFONTBITMAPSW gwglUseFontBitmapsW;
static PFNWGLUSEFONTBITMAPSW _wglUseFontBitmapsW;
static BOOL APIENTRY d_wglUseFontBitmapsW(HDC hDC, DWORD first, DWORD count, DWORD listBase) {
	BOOL ret = _wglUseFontBitmapsW(hDC, first, count, listBase);
	CheckGLError("wglUseFontBitmapsW");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLUSEFONTOUTLINES)(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf);
PFNWGLUSEFONTOUTLINES gwglUseFontOutlines;
static PFNWGLUSEFONTOUTLINES _wglUseFontOutlines;
static BOOL APIENTRY d_wglUseFontOutlines(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf) {
	BOOL ret = _wglUseFontOutlines(hDC, first, count, listBase, deviation, extrusion, format, lpgmf);
	CheckGLError("wglUseFontOutlines");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLUSEFONTOUTLINESA)(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf);
PFNWGLUSEFONTOUTLINESA gwglUseFontOutlinesA;
static PFNWGLUSEFONTOUTLINESA _wglUseFontOutlinesA;
static BOOL APIENTRY d_wglUseFontOutlinesA(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf) {
	BOOL ret = _wglUseFontOutlinesA(hDC, first, count, listBase, deviation, extrusion, format, lpgmf);
	CheckGLError("wglUseFontOutlinesA");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLUSEFONTOUTLINESW)(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf);
PFNWGLUSEFONTOUTLINESW gwglUseFontOutlinesW;
static PFNWGLUSEFONTOUTLINESW _wglUseFontOutlinesW;
static BOOL APIENTRY d_wglUseFontOutlinesW(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf) {
	BOOL ret = _wglUseFontOutlinesW(hDC, first, count, listBase, deviation, extrusion, format, lpgmf);
	CheckGLError("wglUseFontOutlinesW");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLWAITFORMSCOML)(HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder, INT64 *ust, INT64 *msc, INT64 *sbc);
PFNWGLWAITFORMSCOML gwglWaitForMscOML;
static PFNWGLWAITFORMSCOML _wglWaitForMscOML;
static BOOL APIENTRY d_wglWaitForMscOML(HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder, INT64 *ust, INT64 *msc, INT64 *sbc) {
	BOOL ret = _wglWaitForMscOML(hdc, target_msc, divisor, remainder, ust, msc, sbc);
	CheckGLError("wglWaitForMscOML");
	return ret;
}
typedef BOOL (APIENTRYP PFNWGLWAITFORSBCOML)(HDC hdc, INT64 target_sbc, INT64 *ust, INT64 *msc, INT64 *sbc);
PFNWGLWAITFORSBCOML gwglWaitForSbcOML;
static PFNWGLWAITFORSBCOML _wglWaitForSbcOML;
static BOOL APIENTRY d_wglWaitForSbcOML(HDC hdc, INT64 target_sbc, INT64 *ust, INT64 *msc, INT64 *sbc) {
	BOOL ret = _wglWaitForSbcOML(hdc, target_sbc, ust, msc, sbc);
	CheckGLError("wglWaitForSbcOML");
	return ret;
}

#ifdef _WIN32
#define GPA(a) wglGetProcAddress(#a)
#elif defined(__APPLE__)
#define GPA(a) nsglGetProcAddress(#a)
#include <CoreFoundation/CoreFoundation.h>
void *nsglGetProcAddress(const char *procname) {
    static CFBundleRef openGLFramework = nil;
    if (openGLFramework == nil) {
        openGLFramework = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));
    }

    CFStringRef symbolName = CFStringCreateWithCString(kCFAllocatorDefault, procname, kCFStringEncodingASCII);
    void *symbol = CFBundleGetFunctionPointerForName(openGLFramework, symbolName);
    CFRelease(symbolName);
    
    return symbol;
}
#elif defined(__linux__)
#define GPA(a) glXGetProcAddressARB((const GLubyte *)#a)
#endif

gwglext_t gwglext;
static const char *gwglext_str = NULL;

static int gwgl_check_extension(const char *ext) {
	return strstr(gwglext_str, ext) ? 1 : 0;
}

void gwgl_init(HDC hdc, int enableDebug) {
	/* WGL_VERSION_1_0 */
	_ChoosePixelFormat = (PFNCHOOSEPIXELFORMAT)GPA(ChoosePixelFormat);
	_DescribePixelFormat = (PFNDESCRIBEPIXELFORMAT)GPA(DescribePixelFormat);
	_GetEnhMetaFilePixelFormat = (PFNGETENHMETAFILEPIXELFORMAT)GPA(GetEnhMetaFilePixelFormat);
	_GetPixelFormat = (PFNGETPIXELFORMAT)GPA(GetPixelFormat);
	_SetPixelFormat = (PFNSETPIXELFORMAT)GPA(SetPixelFormat);
	_SwapBuffers = (PFNSWAPBUFFERS)GPA(SwapBuffers);
	_wglCopyContext = (PFNWGLCOPYCONTEXT)GPA(wglCopyContext);
	_wglCreateContext = (PFNWGLCREATECONTEXT)GPA(wglCreateContext);
	_wglCreateLayerContext = (PFNWGLCREATELAYERCONTEXT)GPA(wglCreateLayerContext);
	_wglDeleteContext = (PFNWGLDELETECONTEXT)GPA(wglDeleteContext);
	_wglDescribeLayerPlane = (PFNWGLDESCRIBELAYERPLANE)GPA(wglDescribeLayerPlane);
	_wglGetCurrentContext = (PFNWGLGETCURRENTCONTEXT)GPA(wglGetCurrentContext);
	_wglGetCurrentDC = (PFNWGLGETCURRENTDC)GPA(wglGetCurrentDC);
	_wglGetLayerPaletteEntries = (PFNWGLGETLAYERPALETTEENTRIES)GPA(wglGetLayerPaletteEntries);
	_wglGetProcAddress = (PFNWGLGETPROCADDRESS)GPA(wglGetProcAddress);
	_wglMakeCurrent = (PFNWGLMAKECURRENT)GPA(wglMakeCurrent);
	_wglRealizeLayerPalette = (PFNWGLREALIZELAYERPALETTE)GPA(wglRealizeLayerPalette);
	_wglSetLayerPaletteEntries = (PFNWGLSETLAYERPALETTEENTRIES)GPA(wglSetLayerPaletteEntries);
	_wglShareLists = (PFNWGLSHARELISTS)GPA(wglShareLists);
	_wglSwapLayerBuffers = (PFNWGLSWAPLAYERBUFFERS)GPA(wglSwapLayerBuffers);
	_wglUseFontBitmaps = (PFNWGLUSEFONTBITMAPS)GPA(wglUseFontBitmaps);
	_wglUseFontBitmapsA = (PFNWGLUSEFONTBITMAPSA)GPA(wglUseFontBitmapsA);
	_wglUseFontBitmapsW = (PFNWGLUSEFONTBITMAPSW)GPA(wglUseFontBitmapsW);
	_wglUseFontOutlines = (PFNWGLUSEFONTOUTLINES)GPA(wglUseFontOutlines);
	_wglUseFontOutlinesA = (PFNWGLUSEFONTOUTLINESA)GPA(wglUseFontOutlinesA);
	_wglUseFontOutlinesW = (PFNWGLUSEFONTOUTLINESW)GPA(wglUseFontOutlinesW);

	/* WGL_3DL_stereo_control */
	_wglSetStereoEmitterState3DL = (PFNWGLSETSTEREOEMITTERSTATE3DL)GPA(wglSetStereoEmitterState3DL);

	/* WGL_AMD_gpu_association */
	_wglGetGPUIDsAMD = (PFNWGLGETGPUIDSAMD)GPA(wglGetGPUIDsAMD);
	_wglGetGPUInfoAMD = (PFNWGLGETGPUINFOAMD)GPA(wglGetGPUInfoAMD);
	_wglGetContextGPUIDAMD = (PFNWGLGETCONTEXTGPUIDAMD)GPA(wglGetContextGPUIDAMD);
	_wglCreateAssociatedContextAMD = (PFNWGLCREATEASSOCIATEDCONTEXTAMD)GPA(wglCreateAssociatedContextAMD);
	_wglCreateAssociatedContextAttribsAMD = (PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMD)GPA(wglCreateAssociatedContextAttribsAMD);
	_wglDeleteAssociatedContextAMD = (PFNWGLDELETEASSOCIATEDCONTEXTAMD)GPA(wglDeleteAssociatedContextAMD);
	_wglMakeAssociatedContextCurrentAMD = (PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMD)GPA(wglMakeAssociatedContextCurrentAMD);
	_wglGetCurrentAssociatedContextAMD = (PFNWGLGETCURRENTASSOCIATEDCONTEXTAMD)GPA(wglGetCurrentAssociatedContextAMD);
	_wglBlitContextFramebufferAMD = (PFNWGLBLITCONTEXTFRAMEBUFFERAMD)GPA(wglBlitContextFramebufferAMD);

	/* WGL_ARB_buffer_region */
	_wglCreateBufferRegionARB = (PFNWGLCREATEBUFFERREGIONARB)GPA(wglCreateBufferRegionARB);
	_wglDeleteBufferRegionARB = (PFNWGLDELETEBUFFERREGIONARB)GPA(wglDeleteBufferRegionARB);
	_wglSaveBufferRegionARB = (PFNWGLSAVEBUFFERREGIONARB)GPA(wglSaveBufferRegionARB);
	_wglRestoreBufferRegionARB = (PFNWGLRESTOREBUFFERREGIONARB)GPA(wglRestoreBufferRegionARB);

	/* WGL_ARB_create_context */
	_wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARB)GPA(wglCreateContextAttribsARB);

	/* WGL_ARB_extensions_string */
	_wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARB)GPA(wglGetExtensionsStringARB);

	/* WGL_ARB_make_current_read */
	_wglMakeContextCurrentARB = (PFNWGLMAKECONTEXTCURRENTARB)GPA(wglMakeContextCurrentARB);
	_wglGetCurrentReadDCARB = (PFNWGLGETCURRENTREADDCARB)GPA(wglGetCurrentReadDCARB);

	/* WGL_ARB_pbuffer */
	_wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARB)GPA(wglCreatePbufferARB);
	_wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARB)GPA(wglGetPbufferDCARB);
	_wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARB)GPA(wglReleasePbufferDCARB);
	_wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARB)GPA(wglDestroyPbufferARB);
	_wglQueryPbufferARB = (PFNWGLQUERYPBUFFERARB)GPA(wglQueryPbufferARB);

	/* WGL_ARB_pixel_format */
	_wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARB)GPA(wglGetPixelFormatAttribivARB);
	_wglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARB)GPA(wglGetPixelFormatAttribfvARB);
	_wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARB)GPA(wglChoosePixelFormatARB);

	/* WGL_ARB_render_texture */
	_wglBindTexImageARB = (PFNWGLBINDTEXIMAGEARB)GPA(wglBindTexImageARB);
	_wglReleaseTexImageARB = (PFNWGLRELEASETEXIMAGEARB)GPA(wglReleaseTexImageARB);
	_wglSetPbufferAttribARB = (PFNWGLSETPBUFFERATTRIBARB)GPA(wglSetPbufferAttribARB);

	/* WGL_EXT_display_color_table */
	_wglCreateDisplayColorTableEXT = (PFNWGLCREATEDISPLAYCOLORTABLEEXT)GPA(wglCreateDisplayColorTableEXT);
	_wglLoadDisplayColorTableEXT = (PFNWGLLOADDISPLAYCOLORTABLEEXT)GPA(wglLoadDisplayColorTableEXT);
	_wglBindDisplayColorTableEXT = (PFNWGLBINDDISPLAYCOLORTABLEEXT)GPA(wglBindDisplayColorTableEXT);
	_wglDestroyDisplayColorTableEXT = (PFNWGLDESTROYDISPLAYCOLORTABLEEXT)GPA(wglDestroyDisplayColorTableEXT);

	/* WGL_EXT_extensions_string */
	_wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXT)GPA(wglGetExtensionsStringEXT);

	/* WGL_EXT_make_current_read */
	_wglMakeContextCurrentEXT = (PFNWGLMAKECONTEXTCURRENTEXT)GPA(wglMakeContextCurrentEXT);
	_wglGetCurrentReadDCEXT = (PFNWGLGETCURRENTREADDCEXT)GPA(wglGetCurrentReadDCEXT);

	/* WGL_EXT_pbuffer */
	_wglCreatePbufferEXT = (PFNWGLCREATEPBUFFEREXT)GPA(wglCreatePbufferEXT);
	_wglGetPbufferDCEXT = (PFNWGLGETPBUFFERDCEXT)GPA(wglGetPbufferDCEXT);
	_wglReleasePbufferDCEXT = (PFNWGLRELEASEPBUFFERDCEXT)GPA(wglReleasePbufferDCEXT);
	_wglDestroyPbufferEXT = (PFNWGLDESTROYPBUFFEREXT)GPA(wglDestroyPbufferEXT);
	_wglQueryPbufferEXT = (PFNWGLQUERYPBUFFEREXT)GPA(wglQueryPbufferEXT);

	/* WGL_EXT_pixel_format */
	_wglGetPixelFormatAttribivEXT = (PFNWGLGETPIXELFORMATATTRIBIVEXT)GPA(wglGetPixelFormatAttribivEXT);
	_wglGetPixelFormatAttribfvEXT = (PFNWGLGETPIXELFORMATATTRIBFVEXT)GPA(wglGetPixelFormatAttribfvEXT);
	_wglChoosePixelFormatEXT = (PFNWGLCHOOSEPIXELFORMATEXT)GPA(wglChoosePixelFormatEXT);

	/* WGL_EXT_swap_control */
	_wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXT)GPA(wglSwapIntervalEXT);
	_wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXT)GPA(wglGetSwapIntervalEXT);

	/* WGL_I3D_digital_video_control */
	_wglGetDigitalVideoParametersI3D = (PFNWGLGETDIGITALVIDEOPARAMETERSI3D)GPA(wglGetDigitalVideoParametersI3D);
	_wglSetDigitalVideoParametersI3D = (PFNWGLSETDIGITALVIDEOPARAMETERSI3D)GPA(wglSetDigitalVideoParametersI3D);

	/* WGL_I3D_gamma */
	_wglGetGammaTableParametersI3D = (PFNWGLGETGAMMATABLEPARAMETERSI3D)GPA(wglGetGammaTableParametersI3D);
	_wglSetGammaTableParametersI3D = (PFNWGLSETGAMMATABLEPARAMETERSI3D)GPA(wglSetGammaTableParametersI3D);
	_wglGetGammaTableI3D = (PFNWGLGETGAMMATABLEI3D)GPA(wglGetGammaTableI3D);
	_wglSetGammaTableI3D = (PFNWGLSETGAMMATABLEI3D)GPA(wglSetGammaTableI3D);

	/* WGL_I3D_genlock */
	_wglEnableGenlockI3D = (PFNWGLENABLEGENLOCKI3D)GPA(wglEnableGenlockI3D);
	_wglDisableGenlockI3D = (PFNWGLDISABLEGENLOCKI3D)GPA(wglDisableGenlockI3D);
	_wglIsEnabledGenlockI3D = (PFNWGLISENABLEDGENLOCKI3D)GPA(wglIsEnabledGenlockI3D);
	_wglGenlockSourceI3D = (PFNWGLGENLOCKSOURCEI3D)GPA(wglGenlockSourceI3D);
	_wglGetGenlockSourceI3D = (PFNWGLGETGENLOCKSOURCEI3D)GPA(wglGetGenlockSourceI3D);
	_wglGenlockSourceEdgeI3D = (PFNWGLGENLOCKSOURCEEDGEI3D)GPA(wglGenlockSourceEdgeI3D);
	_wglGetGenlockSourceEdgeI3D = (PFNWGLGETGENLOCKSOURCEEDGEI3D)GPA(wglGetGenlockSourceEdgeI3D);
	_wglGenlockSampleRateI3D = (PFNWGLGENLOCKSAMPLERATEI3D)GPA(wglGenlockSampleRateI3D);
	_wglGetGenlockSampleRateI3D = (PFNWGLGETGENLOCKSAMPLERATEI3D)GPA(wglGetGenlockSampleRateI3D);
	_wglGenlockSourceDelayI3D = (PFNWGLGENLOCKSOURCEDELAYI3D)GPA(wglGenlockSourceDelayI3D);
	_wglGetGenlockSourceDelayI3D = (PFNWGLGETGENLOCKSOURCEDELAYI3D)GPA(wglGetGenlockSourceDelayI3D);
	_wglQueryGenlockMaxSourceDelayI3D = (PFNWGLQUERYGENLOCKMAXSOURCEDELAYI3D)GPA(wglQueryGenlockMaxSourceDelayI3D);

	/* WGL_I3D_image_buffer */
	_wglCreateImageBufferI3D = (PFNWGLCREATEIMAGEBUFFERI3D)GPA(wglCreateImageBufferI3D);
	_wglDestroyImageBufferI3D = (PFNWGLDESTROYIMAGEBUFFERI3D)GPA(wglDestroyImageBufferI3D);
	_wglAssociateImageBufferEventsI3D = (PFNWGLASSOCIATEIMAGEBUFFEREVENTSI3D)GPA(wglAssociateImageBufferEventsI3D);
	_wglReleaseImageBufferEventsI3D = (PFNWGLRELEASEIMAGEBUFFEREVENTSI3D)GPA(wglReleaseImageBufferEventsI3D);

	/* WGL_I3D_swap_frame_lock */
	_wglEnableFrameLockI3D = (PFNWGLENABLEFRAMELOCKI3D)GPA(wglEnableFrameLockI3D);
	_wglDisableFrameLockI3D = (PFNWGLDISABLEFRAMELOCKI3D)GPA(wglDisableFrameLockI3D);
	_wglIsEnabledFrameLockI3D = (PFNWGLISENABLEDFRAMELOCKI3D)GPA(wglIsEnabledFrameLockI3D);
	_wglQueryFrameLockMasterI3D = (PFNWGLQUERYFRAMELOCKMASTERI3D)GPA(wglQueryFrameLockMasterI3D);

	/* WGL_I3D_swap_frame_usage */
	_wglGetFrameUsageI3D = (PFNWGLGETFRAMEUSAGEI3D)GPA(wglGetFrameUsageI3D);
	_wglBeginFrameTrackingI3D = (PFNWGLBEGINFRAMETRACKINGI3D)GPA(wglBeginFrameTrackingI3D);
	_wglEndFrameTrackingI3D = (PFNWGLENDFRAMETRACKINGI3D)GPA(wglEndFrameTrackingI3D);
	_wglQueryFrameTrackingI3D = (PFNWGLQUERYFRAMETRACKINGI3D)GPA(wglQueryFrameTrackingI3D);

	/* WGL_NV_copy_image */
	_wglCopyImageSubDataNV = (PFNWGLCOPYIMAGESUBDATANV)GPA(wglCopyImageSubDataNV);

	/* WGL_NV_delay_before_swap */
	_wglDelayBeforeSwapNV = (PFNWGLDELAYBEFORESWAPNV)GPA(wglDelayBeforeSwapNV);

	/* WGL_NV_DX_interop */
	_wglDXSetResourceShareHandleNV = (PFNWGLDXSETRESOURCESHAREHANDLENV)GPA(wglDXSetResourceShareHandleNV);
	_wglDXOpenDeviceNV = (PFNWGLDXOPENDEVICENV)GPA(wglDXOpenDeviceNV);
	_wglDXCloseDeviceNV = (PFNWGLDXCLOSEDEVICENV)GPA(wglDXCloseDeviceNV);
	_wglDXRegisterObjectNV = (PFNWGLDXREGISTEROBJECTNV)GPA(wglDXRegisterObjectNV);
	_wglDXUnregisterObjectNV = (PFNWGLDXUNREGISTEROBJECTNV)GPA(wglDXUnregisterObjectNV);
	_wglDXObjectAccessNV = (PFNWGLDXOBJECTACCESSNV)GPA(wglDXObjectAccessNV);
	_wglDXLockObjectsNV = (PFNWGLDXLOCKOBJECTSNV)GPA(wglDXLockObjectsNV);
	_wglDXUnlockObjectsNV = (PFNWGLDXUNLOCKOBJECTSNV)GPA(wglDXUnlockObjectsNV);

	/* WGL_NV_gpu_affinity */
	_wglEnumGpusNV = (PFNWGLENUMGPUSNV)GPA(wglEnumGpusNV);
	_wglEnumGpuDevicesNV = (PFNWGLENUMGPUDEVICESNV)GPA(wglEnumGpuDevicesNV);
	_wglCreateAffinityDCNV = (PFNWGLCREATEAFFINITYDCNV)GPA(wglCreateAffinityDCNV);
	_wglEnumGpusFromAffinityDCNV = (PFNWGLENUMGPUSFROMAFFINITYDCNV)GPA(wglEnumGpusFromAffinityDCNV);
	_wglDeleteDCNV = (PFNWGLDELETEDCNV)GPA(wglDeleteDCNV);

	/* WGL_NV_present_video */
	_wglEnumerateVideoDevicesNV = (PFNWGLENUMERATEVIDEODEVICESNV)GPA(wglEnumerateVideoDevicesNV);
	_wglBindVideoDeviceNV = (PFNWGLBINDVIDEODEVICENV)GPA(wglBindVideoDeviceNV);
	_wglQueryCurrentContextNV = (PFNWGLQUERYCURRENTCONTEXTNV)GPA(wglQueryCurrentContextNV);

	/* WGL_NV_swap_group */
	_wglJoinSwapGroupNV = (PFNWGLJOINSWAPGROUPNV)GPA(wglJoinSwapGroupNV);
	_wglBindSwapBarrierNV = (PFNWGLBINDSWAPBARRIERNV)GPA(wglBindSwapBarrierNV);
	_wglQuerySwapGroupNV = (PFNWGLQUERYSWAPGROUPNV)GPA(wglQuerySwapGroupNV);
	_wglQueryMaxSwapGroupsNV = (PFNWGLQUERYMAXSWAPGROUPSNV)GPA(wglQueryMaxSwapGroupsNV);
	_wglQueryFrameCountNV = (PFNWGLQUERYFRAMECOUNTNV)GPA(wglQueryFrameCountNV);
	_wglResetFrameCountNV = (PFNWGLRESETFRAMECOUNTNV)GPA(wglResetFrameCountNV);

	/* WGL_NV_video_capture */
	_wglBindVideoCaptureDeviceNV = (PFNWGLBINDVIDEOCAPTUREDEVICENV)GPA(wglBindVideoCaptureDeviceNV);
	_wglEnumerateVideoCaptureDevicesNV = (PFNWGLENUMERATEVIDEOCAPTUREDEVICESNV)GPA(wglEnumerateVideoCaptureDevicesNV);
	_wglLockVideoCaptureDeviceNV = (PFNWGLLOCKVIDEOCAPTUREDEVICENV)GPA(wglLockVideoCaptureDeviceNV);
	_wglQueryVideoCaptureDeviceNV = (PFNWGLQUERYVIDEOCAPTUREDEVICENV)GPA(wglQueryVideoCaptureDeviceNV);
	_wglReleaseVideoCaptureDeviceNV = (PFNWGLRELEASEVIDEOCAPTUREDEVICENV)GPA(wglReleaseVideoCaptureDeviceNV);

	/* WGL_NV_video_output */
	_wglGetVideoDeviceNV = (PFNWGLGETVIDEODEVICENV)GPA(wglGetVideoDeviceNV);
	_wglReleaseVideoDeviceNV = (PFNWGLRELEASEVIDEODEVICENV)GPA(wglReleaseVideoDeviceNV);
	_wglBindVideoImageNV = (PFNWGLBINDVIDEOIMAGENV)GPA(wglBindVideoImageNV);
	_wglReleaseVideoImageNV = (PFNWGLRELEASEVIDEOIMAGENV)GPA(wglReleaseVideoImageNV);
	_wglSendPbufferToVideoNV = (PFNWGLSENDPBUFFERTOVIDEONV)GPA(wglSendPbufferToVideoNV);
	_wglGetVideoInfoNV = (PFNWGLGETVIDEOINFONV)GPA(wglGetVideoInfoNV);

	/* WGL_NV_vertex_array_range */
	_wglAllocateMemoryNV = (PFNWGLALLOCATEMEMORYNV)GPA(wglAllocateMemoryNV);
	_wglFreeMemoryNV = (PFNWGLFREEMEMORYNV)GPA(wglFreeMemoryNV);

	/* WGL_OML_sync_control */
	_wglGetSyncValuesOML = (PFNWGLGETSYNCVALUESOML)GPA(wglGetSyncValuesOML);
	_wglGetMscRateOML = (PFNWGLGETMSCRATEOML)GPA(wglGetMscRateOML);
	_wglSwapBuffersMscOML = (PFNWGLSWAPBUFFERSMSCOML)GPA(wglSwapBuffersMscOML);
	_wglSwapLayerBuffersMscOML = (PFNWGLSWAPLAYERBUFFERSMSCOML)GPA(wglSwapLayerBuffersMscOML);
	_wglWaitForMscOML = (PFNWGLWAITFORMSCOML)GPA(wglWaitForMscOML);
	_wglWaitForSbcOML = (PFNWGLWAITFORSBCOML)GPA(wglWaitForSbcOML);

	gwgl_rebind(enableDebug);

	gwglext_str = (const char *)_wglGetExtensionsStringARB(hdc);
	memset(&gwglext, 0, sizeof(gwglext));
	if (gwgl_check_extension("WGL_3DFX_multisample")) gwglext._WGL_3DFX_multisample = 1;
	if (gwgl_check_extension("WGL_3DL_stereo_control")) gwglext._WGL_3DL_stereo_control = 1;
	if (gwgl_check_extension("WGL_AMD_gpu_association")) gwglext._WGL_AMD_gpu_association = 1;
	if (gwgl_check_extension("WGL_ARB_buffer_region")) gwglext._WGL_ARB_buffer_region = 1;
	if (gwgl_check_extension("WGL_ARB_context_flush_control")) gwglext._WGL_ARB_context_flush_control = 1;
	if (gwgl_check_extension("WGL_ARB_create_context")) gwglext._WGL_ARB_create_context = 1;
	if (gwgl_check_extension("WGL_ARB_create_context_profile")) gwglext._WGL_ARB_create_context_profile = 1;
	if (gwgl_check_extension("WGL_ARB_create_context_robustness")) gwglext._WGL_ARB_create_context_robustness = 1;
	if (gwgl_check_extension("WGL_ARB_extensions_string")) gwglext._WGL_ARB_extensions_string = 1;
	if (gwgl_check_extension("WGL_ARB_framebuffer_sRGB")) gwglext._WGL_ARB_framebuffer_sRGB = 1;
	if (gwgl_check_extension("WGL_ARB_make_current_read")) gwglext._WGL_ARB_make_current_read = 1;
	if (gwgl_check_extension("WGL_ARB_multisample")) gwglext._WGL_ARB_multisample = 1;
	if (gwgl_check_extension("WGL_ARB_pbuffer")) gwglext._WGL_ARB_pbuffer = 1;
	if (gwgl_check_extension("WGL_ARB_pixel_format")) gwglext._WGL_ARB_pixel_format = 1;
	if (gwgl_check_extension("WGL_ARB_pixel_format_float")) gwglext._WGL_ARB_pixel_format_float = 1;
	if (gwgl_check_extension("WGL_ARB_render_texture")) gwglext._WGL_ARB_render_texture = 1;
	if (gwgl_check_extension("WGL_ARB_robustness_application_isolation")) gwglext._WGL_ARB_robustness_application_isolation = 1;
	if (gwgl_check_extension("WGL_ARB_robustness_share_group_isolation")) gwglext._WGL_ARB_robustness_share_group_isolation = 1;
	if (gwgl_check_extension("WGL_ATI_pixel_format_float")) gwglext._WGL_ATI_pixel_format_float = 1;
	if (gwgl_check_extension("WGL_EXT_colorspace")) gwglext._WGL_EXT_colorspace = 1;
	if (gwgl_check_extension("WGL_EXT_create_context_es_profile")) gwglext._WGL_EXT_create_context_es_profile = 1;
	if (gwgl_check_extension("WGL_EXT_create_context_es2_profile")) gwglext._WGL_EXT_create_context_es2_profile = 1;
	if (gwgl_check_extension("WGL_EXT_depth_float")) gwglext._WGL_EXT_depth_float = 1;
	if (gwgl_check_extension("WGL_EXT_display_color_table")) gwglext._WGL_EXT_display_color_table = 1;
	if (gwgl_check_extension("WGL_EXT_extensions_string")) gwglext._WGL_EXT_extensions_string = 1;
	if (gwgl_check_extension("WGL_EXT_framebuffer_sRGB")) gwglext._WGL_EXT_framebuffer_sRGB = 1;
	if (gwgl_check_extension("WGL_EXT_make_current_read")) gwglext._WGL_EXT_make_current_read = 1;
	if (gwgl_check_extension("WGL_EXT_multisample")) gwglext._WGL_EXT_multisample = 1;
	if (gwgl_check_extension("WGL_EXT_pbuffer")) gwglext._WGL_EXT_pbuffer = 1;
	if (gwgl_check_extension("WGL_EXT_pixel_format")) gwglext._WGL_EXT_pixel_format = 1;
	if (gwgl_check_extension("WGL_EXT_pixel_format_packed_float")) gwglext._WGL_EXT_pixel_format_packed_float = 1;
	if (gwgl_check_extension("WGL_EXT_swap_control")) gwglext._WGL_EXT_swap_control = 1;
	if (gwgl_check_extension("WGL_EXT_swap_control_tear")) gwglext._WGL_EXT_swap_control_tear = 1;
	if (gwgl_check_extension("WGL_I3D_digital_video_control")) gwglext._WGL_I3D_digital_video_control = 1;
	if (gwgl_check_extension("WGL_I3D_gamma")) gwglext._WGL_I3D_gamma = 1;
	if (gwgl_check_extension("WGL_I3D_genlock")) gwglext._WGL_I3D_genlock = 1;
	if (gwgl_check_extension("WGL_I3D_image_buffer")) gwglext._WGL_I3D_image_buffer = 1;
	if (gwgl_check_extension("WGL_I3D_swap_frame_lock")) gwglext._WGL_I3D_swap_frame_lock = 1;
	if (gwgl_check_extension("WGL_I3D_swap_frame_usage")) gwglext._WGL_I3D_swap_frame_usage = 1;
	if (gwgl_check_extension("WGL_NV_copy_image")) gwglext._WGL_NV_copy_image = 1;
	if (gwgl_check_extension("WGL_NV_delay_before_swap")) gwglext._WGL_NV_delay_before_swap = 1;
	if (gwgl_check_extension("WGL_NV_DX_interop")) gwglext._WGL_NV_DX_interop = 1;
	if (gwgl_check_extension("WGL_NV_DX_interop2")) gwglext._WGL_NV_DX_interop2 = 1;
	if (gwgl_check_extension("WGL_NV_float_buffer")) gwglext._WGL_NV_float_buffer = 1;
	if (gwgl_check_extension("WGL_NV_gpu_affinity")) gwglext._WGL_NV_gpu_affinity = 1;
	if (gwgl_check_extension("WGL_NV_multisample_coverage")) gwglext._WGL_NV_multisample_coverage = 1;
	if (gwgl_check_extension("WGL_NV_present_video")) gwglext._WGL_NV_present_video = 1;
	if (gwgl_check_extension("WGL_NV_render_depth_texture")) gwglext._WGL_NV_render_depth_texture = 1;
	if (gwgl_check_extension("WGL_NV_render_texture_rectangle")) gwglext._WGL_NV_render_texture_rectangle = 1;
	if (gwgl_check_extension("WGL_NV_swap_group")) gwglext._WGL_NV_swap_group = 1;
	if (gwgl_check_extension("WGL_NV_video_capture")) gwglext._WGL_NV_video_capture = 1;
	if (gwgl_check_extension("WGL_NV_video_output")) gwglext._WGL_NV_video_output = 1;
	if (gwgl_check_extension("WGL_NV_vertex_array_range")) gwglext._WGL_NV_vertex_array_range = 1;
	if (gwgl_check_extension("WGL_OML_sync_control")) gwglext._WGL_OML_sync_control = 1;
}

void gwgl_rebind(int enableDebug) {
	if (!enableDebug) {
		gChoosePixelFormat = _ChoosePixelFormat;
		gDescribePixelFormat = _DescribePixelFormat;
		gGetPixelFormat = _GetPixelFormat;
		gSetPixelFormat = _SetPixelFormat;
		gSwapBuffers = _SwapBuffers;
		gwglAllocateMemoryNV = _wglAllocateMemoryNV;
		gwglAssociateImageBufferEventsI3D = _wglAssociateImageBufferEventsI3D;
		gwglBeginFrameTrackingI3D = _wglBeginFrameTrackingI3D;
		gwglBindDisplayColorTableEXT = _wglBindDisplayColorTableEXT;
		gwglBindSwapBarrierNV = _wglBindSwapBarrierNV;
		gwglBindTexImageARB = _wglBindTexImageARB;
		gwglBindVideoCaptureDeviceNV = _wglBindVideoCaptureDeviceNV;
		gwglBindVideoDeviceNV = _wglBindVideoDeviceNV;
		gwglBindVideoImageNV = _wglBindVideoImageNV;
		gwglBlitContextFramebufferAMD = _wglBlitContextFramebufferAMD;
		gwglChoosePixelFormatARB = _wglChoosePixelFormatARB;
		gwglChoosePixelFormatEXT = _wglChoosePixelFormatEXT;
		gwglCopyContext = _wglCopyContext;
		gwglCopyImageSubDataNV = _wglCopyImageSubDataNV;
		gwglCreateAffinityDCNV = _wglCreateAffinityDCNV;
		gwglCreateAssociatedContextAMD = _wglCreateAssociatedContextAMD;
		gwglCreateAssociatedContextAttribsAMD = _wglCreateAssociatedContextAttribsAMD;
		gwglCreateBufferRegionARB = _wglCreateBufferRegionARB;
		gwglCreateContext = _wglCreateContext;
		gwglCreateContextAttribsARB = _wglCreateContextAttribsARB;
		gwglCreateDisplayColorTableEXT = _wglCreateDisplayColorTableEXT;
		gwglCreateImageBufferI3D = _wglCreateImageBufferI3D;
		gwglCreateLayerContext = _wglCreateLayerContext;
		gwglCreatePbufferARB = _wglCreatePbufferARB;
		gwglCreatePbufferEXT = _wglCreatePbufferEXT;
		gwglDelayBeforeSwapNV = _wglDelayBeforeSwapNV;
		gwglDeleteAssociatedContextAMD = _wglDeleteAssociatedContextAMD;
		gwglDeleteBufferRegionARB = _wglDeleteBufferRegionARB;
		gwglDeleteContext = _wglDeleteContext;
		gwglDeleteDCNV = _wglDeleteDCNV;
		gwglDescribeLayerPlane = _wglDescribeLayerPlane;
		gwglDestroyDisplayColorTableEXT = _wglDestroyDisplayColorTableEXT;
		gwglDestroyImageBufferI3D = _wglDestroyImageBufferI3D;
		gwglDestroyPbufferARB = _wglDestroyPbufferARB;
		gwglDestroyPbufferEXT = _wglDestroyPbufferEXT;
		gwglDisableFrameLockI3D = _wglDisableFrameLockI3D;
		gwglDisableGenlockI3D = _wglDisableGenlockI3D;
		gwglDXCloseDeviceNV = _wglDXCloseDeviceNV;
		gwglDXLockObjectsNV = _wglDXLockObjectsNV;
		gwglDXObjectAccessNV = _wglDXObjectAccessNV;
		gwglDXOpenDeviceNV = _wglDXOpenDeviceNV;
		gwglDXRegisterObjectNV = _wglDXRegisterObjectNV;
		gwglDXSetResourceShareHandleNV = _wglDXSetResourceShareHandleNV;
		gwglDXUnlockObjectsNV = _wglDXUnlockObjectsNV;
		gwglDXUnregisterObjectNV = _wglDXUnregisterObjectNV;
		gwglEnableFrameLockI3D = _wglEnableFrameLockI3D;
		gwglEnableGenlockI3D = _wglEnableGenlockI3D;
		gwglEndFrameTrackingI3D = _wglEndFrameTrackingI3D;
		gwglEnumerateVideoCaptureDevicesNV = _wglEnumerateVideoCaptureDevicesNV;
		gwglEnumerateVideoDevicesNV = _wglEnumerateVideoDevicesNV;
		gwglEnumGpuDevicesNV = _wglEnumGpuDevicesNV;
		gwglEnumGpusFromAffinityDCNV = _wglEnumGpusFromAffinityDCNV;
		gwglEnumGpusNV = _wglEnumGpusNV;
		gwglFreeMemoryNV = _wglFreeMemoryNV;
		gwglGenlockSampleRateI3D = _wglGenlockSampleRateI3D;
		gwglGenlockSourceDelayI3D = _wglGenlockSourceDelayI3D;
		gwglGenlockSourceEdgeI3D = _wglGenlockSourceEdgeI3D;
		gwglGenlockSourceI3D = _wglGenlockSourceI3D;
		gwglGetContextGPUIDAMD = _wglGetContextGPUIDAMD;
		gwglGetCurrentAssociatedContextAMD = _wglGetCurrentAssociatedContextAMD;
		gwglGetCurrentContext = _wglGetCurrentContext;
		gwglGetCurrentDC = _wglGetCurrentDC;
		gwglGetCurrentReadDCARB = _wglGetCurrentReadDCARB;
		gwglGetCurrentReadDCEXT = _wglGetCurrentReadDCEXT;
		gwglGetDigitalVideoParametersI3D = _wglGetDigitalVideoParametersI3D;
		gGetEnhMetaFilePixelFormat = _GetEnhMetaFilePixelFormat;
		gwglGetExtensionsStringARB = _wglGetExtensionsStringARB;
		gwglGetExtensionsStringEXT = _wglGetExtensionsStringEXT;
		gwglGetFrameUsageI3D = _wglGetFrameUsageI3D;
		gwglGetGammaTableI3D = _wglGetGammaTableI3D;
		gwglGetGammaTableParametersI3D = _wglGetGammaTableParametersI3D;
		gwglGetGenlockSampleRateI3D = _wglGetGenlockSampleRateI3D;
		gwglGetGenlockSourceDelayI3D = _wglGetGenlockSourceDelayI3D;
		gwglGetGenlockSourceEdgeI3D = _wglGetGenlockSourceEdgeI3D;
		gwglGetGenlockSourceI3D = _wglGetGenlockSourceI3D;
		gwglGetGPUIDsAMD = _wglGetGPUIDsAMD;
		gwglGetGPUInfoAMD = _wglGetGPUInfoAMD;
		gwglGetLayerPaletteEntries = _wglGetLayerPaletteEntries;
		gwglGetMscRateOML = _wglGetMscRateOML;
		gwglGetPbufferDCARB = _wglGetPbufferDCARB;
		gwglGetPbufferDCEXT = _wglGetPbufferDCEXT;
		gwglGetPixelFormatAttribfvARB = _wglGetPixelFormatAttribfvARB;
		gwglGetPixelFormatAttribfvEXT = _wglGetPixelFormatAttribfvEXT;
		gwglGetPixelFormatAttribivARB = _wglGetPixelFormatAttribivARB;
		gwglGetPixelFormatAttribivEXT = _wglGetPixelFormatAttribivEXT;
		gwglGetProcAddress = _wglGetProcAddress;
		gwglGetSwapIntervalEXT = _wglGetSwapIntervalEXT;
		gwglGetSyncValuesOML = _wglGetSyncValuesOML;
		gwglGetVideoDeviceNV = _wglGetVideoDeviceNV;
		gwglGetVideoInfoNV = _wglGetVideoInfoNV;
		gwglIsEnabledFrameLockI3D = _wglIsEnabledFrameLockI3D;
		gwglIsEnabledGenlockI3D = _wglIsEnabledGenlockI3D;
		gwglJoinSwapGroupNV = _wglJoinSwapGroupNV;
		gwglLoadDisplayColorTableEXT = _wglLoadDisplayColorTableEXT;
		gwglLockVideoCaptureDeviceNV = _wglLockVideoCaptureDeviceNV;
		gwglMakeAssociatedContextCurrentAMD = _wglMakeAssociatedContextCurrentAMD;
		gwglMakeContextCurrentARB = _wglMakeContextCurrentARB;
		gwglMakeContextCurrentEXT = _wglMakeContextCurrentEXT;
		gwglMakeCurrent = _wglMakeCurrent;
		gwglQueryCurrentContextNV = _wglQueryCurrentContextNV;
		gwglQueryFrameCountNV = _wglQueryFrameCountNV;
		gwglQueryFrameLockMasterI3D = _wglQueryFrameLockMasterI3D;
		gwglQueryFrameTrackingI3D = _wglQueryFrameTrackingI3D;
		gwglQueryGenlockMaxSourceDelayI3D = _wglQueryGenlockMaxSourceDelayI3D;
		gwglQueryMaxSwapGroupsNV = _wglQueryMaxSwapGroupsNV;
		gwglQueryPbufferARB = _wglQueryPbufferARB;
		gwglQueryPbufferEXT = _wglQueryPbufferEXT;
		gwglQuerySwapGroupNV = _wglQuerySwapGroupNV;
		gwglQueryVideoCaptureDeviceNV = _wglQueryVideoCaptureDeviceNV;
		gwglRealizeLayerPalette = _wglRealizeLayerPalette;
		gwglReleaseImageBufferEventsI3D = _wglReleaseImageBufferEventsI3D;
		gwglReleasePbufferDCARB = _wglReleasePbufferDCARB;
		gwglReleasePbufferDCEXT = _wglReleasePbufferDCEXT;
		gwglReleaseTexImageARB = _wglReleaseTexImageARB;
		gwglReleaseVideoCaptureDeviceNV = _wglReleaseVideoCaptureDeviceNV;
		gwglReleaseVideoDeviceNV = _wglReleaseVideoDeviceNV;
		gwglReleaseVideoImageNV = _wglReleaseVideoImageNV;
		gwglResetFrameCountNV = _wglResetFrameCountNV;
		gwglRestoreBufferRegionARB = _wglRestoreBufferRegionARB;
		gwglSaveBufferRegionARB = _wglSaveBufferRegionARB;
		gwglSendPbufferToVideoNV = _wglSendPbufferToVideoNV;
		gwglSetDigitalVideoParametersI3D = _wglSetDigitalVideoParametersI3D;
		gwglSetGammaTableI3D = _wglSetGammaTableI3D;
		gwglSetGammaTableParametersI3D = _wglSetGammaTableParametersI3D;
		gwglSetLayerPaletteEntries = _wglSetLayerPaletteEntries;
		gwglSetPbufferAttribARB = _wglSetPbufferAttribARB;
		gwglSetStereoEmitterState3DL = _wglSetStereoEmitterState3DL;
		gwglShareLists = _wglShareLists;
		gwglSwapBuffersMscOML = _wglSwapBuffersMscOML;
		gwglSwapLayerBuffers = _wglSwapLayerBuffers;
		gwglSwapIntervalEXT = _wglSwapIntervalEXT;
		gwglSwapLayerBuffersMscOML = _wglSwapLayerBuffersMscOML;
		gwglUseFontBitmaps = _wglUseFontBitmaps;
		gwglUseFontBitmapsA = _wglUseFontBitmapsA;
		gwglUseFontBitmapsW = _wglUseFontBitmapsW;
		gwglUseFontOutlines = _wglUseFontOutlines;
		gwglUseFontOutlinesA = _wglUseFontOutlinesA;
		gwglUseFontOutlinesW = _wglUseFontOutlinesW;
		gwglWaitForMscOML = _wglWaitForMscOML;
		gwglWaitForSbcOML = _wglWaitForSbcOML;
	} else {
		gChoosePixelFormat = d_ChoosePixelFormat;
		gDescribePixelFormat = d_DescribePixelFormat;
		gGetPixelFormat = d_GetPixelFormat;
		gSetPixelFormat = d_SetPixelFormat;
		gSwapBuffers = d_SwapBuffers;
		gwglAllocateMemoryNV = d_wglAllocateMemoryNV;
		gwglAssociateImageBufferEventsI3D = d_wglAssociateImageBufferEventsI3D;
		gwglBeginFrameTrackingI3D = d_wglBeginFrameTrackingI3D;
		gwglBindDisplayColorTableEXT = d_wglBindDisplayColorTableEXT;
		gwglBindSwapBarrierNV = d_wglBindSwapBarrierNV;
		gwglBindTexImageARB = d_wglBindTexImageARB;
		gwglBindVideoCaptureDeviceNV = d_wglBindVideoCaptureDeviceNV;
		gwglBindVideoDeviceNV = d_wglBindVideoDeviceNV;
		gwglBindVideoImageNV = d_wglBindVideoImageNV;
		gwglBlitContextFramebufferAMD = d_wglBlitContextFramebufferAMD;
		gwglChoosePixelFormatARB = d_wglChoosePixelFormatARB;
		gwglChoosePixelFormatEXT = d_wglChoosePixelFormatEXT;
		gwglCopyContext = d_wglCopyContext;
		gwglCopyImageSubDataNV = d_wglCopyImageSubDataNV;
		gwglCreateAffinityDCNV = d_wglCreateAffinityDCNV;
		gwglCreateAssociatedContextAMD = d_wglCreateAssociatedContextAMD;
		gwglCreateAssociatedContextAttribsAMD = d_wglCreateAssociatedContextAttribsAMD;
		gwglCreateBufferRegionARB = d_wglCreateBufferRegionARB;
		gwglCreateContext = d_wglCreateContext;
		gwglCreateContextAttribsARB = d_wglCreateContextAttribsARB;
		gwglCreateDisplayColorTableEXT = d_wglCreateDisplayColorTableEXT;
		gwglCreateImageBufferI3D = d_wglCreateImageBufferI3D;
		gwglCreateLayerContext = d_wglCreateLayerContext;
		gwglCreatePbufferARB = d_wglCreatePbufferARB;
		gwglCreatePbufferEXT = d_wglCreatePbufferEXT;
		gwglDelayBeforeSwapNV = d_wglDelayBeforeSwapNV;
		gwglDeleteAssociatedContextAMD = d_wglDeleteAssociatedContextAMD;
		gwglDeleteBufferRegionARB = d_wglDeleteBufferRegionARB;
		gwglDeleteContext = d_wglDeleteContext;
		gwglDeleteDCNV = d_wglDeleteDCNV;
		gwglDescribeLayerPlane = d_wglDescribeLayerPlane;
		gwglDestroyDisplayColorTableEXT = d_wglDestroyDisplayColorTableEXT;
		gwglDestroyImageBufferI3D = d_wglDestroyImageBufferI3D;
		gwglDestroyPbufferARB = d_wglDestroyPbufferARB;
		gwglDestroyPbufferEXT = d_wglDestroyPbufferEXT;
		gwglDisableFrameLockI3D = d_wglDisableFrameLockI3D;
		gwglDisableGenlockI3D = d_wglDisableGenlockI3D;
		gwglDXCloseDeviceNV = d_wglDXCloseDeviceNV;
		gwglDXLockObjectsNV = d_wglDXLockObjectsNV;
		gwglDXObjectAccessNV = d_wglDXObjectAccessNV;
		gwglDXOpenDeviceNV = d_wglDXOpenDeviceNV;
		gwglDXRegisterObjectNV = d_wglDXRegisterObjectNV;
		gwglDXSetResourceShareHandleNV = d_wglDXSetResourceShareHandleNV;
		gwglDXUnlockObjectsNV = d_wglDXUnlockObjectsNV;
		gwglDXUnregisterObjectNV = d_wglDXUnregisterObjectNV;
		gwglEnableFrameLockI3D = d_wglEnableFrameLockI3D;
		gwglEnableGenlockI3D = d_wglEnableGenlockI3D;
		gwglEndFrameTrackingI3D = d_wglEndFrameTrackingI3D;
		gwglEnumerateVideoCaptureDevicesNV = d_wglEnumerateVideoCaptureDevicesNV;
		gwglEnumerateVideoDevicesNV = d_wglEnumerateVideoDevicesNV;
		gwglEnumGpuDevicesNV = d_wglEnumGpuDevicesNV;
		gwglEnumGpusFromAffinityDCNV = d_wglEnumGpusFromAffinityDCNV;
		gwglEnumGpusNV = d_wglEnumGpusNV;
		gwglFreeMemoryNV = d_wglFreeMemoryNV;
		gwglGenlockSampleRateI3D = d_wglGenlockSampleRateI3D;
		gwglGenlockSourceDelayI3D = d_wglGenlockSourceDelayI3D;
		gwglGenlockSourceEdgeI3D = d_wglGenlockSourceEdgeI3D;
		gwglGenlockSourceI3D = d_wglGenlockSourceI3D;
		gwglGetContextGPUIDAMD = d_wglGetContextGPUIDAMD;
		gwglGetCurrentAssociatedContextAMD = d_wglGetCurrentAssociatedContextAMD;
		gwglGetCurrentContext = d_wglGetCurrentContext;
		gwglGetCurrentDC = d_wglGetCurrentDC;
		gwglGetCurrentReadDCARB = d_wglGetCurrentReadDCARB;
		gwglGetCurrentReadDCEXT = d_wglGetCurrentReadDCEXT;
		gwglGetDigitalVideoParametersI3D = d_wglGetDigitalVideoParametersI3D;
		gGetEnhMetaFilePixelFormat = d_GetEnhMetaFilePixelFormat;
		gwglGetExtensionsStringARB = d_wglGetExtensionsStringARB;
		gwglGetExtensionsStringEXT = d_wglGetExtensionsStringEXT;
		gwglGetFrameUsageI3D = d_wglGetFrameUsageI3D;
		gwglGetGammaTableI3D = d_wglGetGammaTableI3D;
		gwglGetGammaTableParametersI3D = d_wglGetGammaTableParametersI3D;
		gwglGetGenlockSampleRateI3D = d_wglGetGenlockSampleRateI3D;
		gwglGetGenlockSourceDelayI3D = d_wglGetGenlockSourceDelayI3D;
		gwglGetGenlockSourceEdgeI3D = d_wglGetGenlockSourceEdgeI3D;
		gwglGetGenlockSourceI3D = d_wglGetGenlockSourceI3D;
		gwglGetGPUIDsAMD = d_wglGetGPUIDsAMD;
		gwglGetGPUInfoAMD = d_wglGetGPUInfoAMD;
		gwglGetLayerPaletteEntries = d_wglGetLayerPaletteEntries;
		gwglGetMscRateOML = d_wglGetMscRateOML;
		gwglGetPbufferDCARB = d_wglGetPbufferDCARB;
		gwglGetPbufferDCEXT = d_wglGetPbufferDCEXT;
		gwglGetPixelFormatAttribfvARB = d_wglGetPixelFormatAttribfvARB;
		gwglGetPixelFormatAttribfvEXT = d_wglGetPixelFormatAttribfvEXT;
		gwglGetPixelFormatAttribivARB = d_wglGetPixelFormatAttribivARB;
		gwglGetPixelFormatAttribivEXT = d_wglGetPixelFormatAttribivEXT;
		gwglGetProcAddress = d_wglGetProcAddress;
		gwglGetSwapIntervalEXT = d_wglGetSwapIntervalEXT;
		gwglGetSyncValuesOML = d_wglGetSyncValuesOML;
		gwglGetVideoDeviceNV = d_wglGetVideoDeviceNV;
		gwglGetVideoInfoNV = d_wglGetVideoInfoNV;
		gwglIsEnabledFrameLockI3D = d_wglIsEnabledFrameLockI3D;
		gwglIsEnabledGenlockI3D = d_wglIsEnabledGenlockI3D;
		gwglJoinSwapGroupNV = d_wglJoinSwapGroupNV;
		gwglLoadDisplayColorTableEXT = d_wglLoadDisplayColorTableEXT;
		gwglLockVideoCaptureDeviceNV = d_wglLockVideoCaptureDeviceNV;
		gwglMakeAssociatedContextCurrentAMD = d_wglMakeAssociatedContextCurrentAMD;
		gwglMakeContextCurrentARB = d_wglMakeContextCurrentARB;
		gwglMakeContextCurrentEXT = d_wglMakeContextCurrentEXT;
		gwglMakeCurrent = d_wglMakeCurrent;
		gwglQueryCurrentContextNV = d_wglQueryCurrentContextNV;
		gwglQueryFrameCountNV = d_wglQueryFrameCountNV;
		gwglQueryFrameLockMasterI3D = d_wglQueryFrameLockMasterI3D;
		gwglQueryFrameTrackingI3D = d_wglQueryFrameTrackingI3D;
		gwglQueryGenlockMaxSourceDelayI3D = d_wglQueryGenlockMaxSourceDelayI3D;
		gwglQueryMaxSwapGroupsNV = d_wglQueryMaxSwapGroupsNV;
		gwglQueryPbufferARB = d_wglQueryPbufferARB;
		gwglQueryPbufferEXT = d_wglQueryPbufferEXT;
		gwglQuerySwapGroupNV = d_wglQuerySwapGroupNV;
		gwglQueryVideoCaptureDeviceNV = d_wglQueryVideoCaptureDeviceNV;
		gwglRealizeLayerPalette = d_wglRealizeLayerPalette;
		gwglReleaseImageBufferEventsI3D = d_wglReleaseImageBufferEventsI3D;
		gwglReleasePbufferDCARB = d_wglReleasePbufferDCARB;
		gwglReleasePbufferDCEXT = d_wglReleasePbufferDCEXT;
		gwglReleaseTexImageARB = d_wglReleaseTexImageARB;
		gwglReleaseVideoCaptureDeviceNV = d_wglReleaseVideoCaptureDeviceNV;
		gwglReleaseVideoDeviceNV = d_wglReleaseVideoDeviceNV;
		gwglReleaseVideoImageNV = d_wglReleaseVideoImageNV;
		gwglResetFrameCountNV = d_wglResetFrameCountNV;
		gwglRestoreBufferRegionARB = d_wglRestoreBufferRegionARB;
		gwglSaveBufferRegionARB = d_wglSaveBufferRegionARB;
		gwglSendPbufferToVideoNV = d_wglSendPbufferToVideoNV;
		gwglSetDigitalVideoParametersI3D = d_wglSetDigitalVideoParametersI3D;
		gwglSetGammaTableI3D = d_wglSetGammaTableI3D;
		gwglSetGammaTableParametersI3D = d_wglSetGammaTableParametersI3D;
		gwglSetLayerPaletteEntries = d_wglSetLayerPaletteEntries;
		gwglSetPbufferAttribARB = d_wglSetPbufferAttribARB;
		gwglSetStereoEmitterState3DL = d_wglSetStereoEmitterState3DL;
		gwglShareLists = d_wglShareLists;
		gwglSwapBuffersMscOML = d_wglSwapBuffersMscOML;
		gwglSwapLayerBuffers = d_wglSwapLayerBuffers;
		gwglSwapIntervalEXT = d_wglSwapIntervalEXT;
		gwglSwapLayerBuffersMscOML = d_wglSwapLayerBuffersMscOML;
		gwglUseFontBitmaps = d_wglUseFontBitmaps;
		gwglUseFontBitmapsA = d_wglUseFontBitmapsA;
		gwglUseFontBitmapsW = d_wglUseFontBitmapsW;
		gwglUseFontOutlines = d_wglUseFontOutlines;
		gwglUseFontOutlinesA = d_wglUseFontOutlinesA;
		gwglUseFontOutlinesW = d_wglUseFontOutlinesW;
		gwglWaitForMscOML = d_wglWaitForMscOML;
		gwglWaitForSbcOML = d_wglWaitForSbcOML;
	}
}
