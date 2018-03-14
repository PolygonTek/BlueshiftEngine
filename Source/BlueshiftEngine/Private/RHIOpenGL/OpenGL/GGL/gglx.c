/*********************************************************************************************
 *
 * gglx.c
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

#include "gglx.h"
#include <string.h>

extern void CheckGLError(const char *msg);

typedef Bool (APIENTRYP PFNGLXASSOCIATEDMPBUFFERSGIX)(Display *dpy, GLXPbufferSGIX pbuffer, DMparams *params, DMbuffer dmbuffer);
PFNGLXASSOCIATEDMPBUFFERSGIX gglXAssociateDMPbufferSGIX;
static PFNGLXASSOCIATEDMPBUFFERSGIX _glXAssociateDMPbufferSGIX;
static Bool APIENTRY d_glXAssociateDMPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuffer, DMparams *params, DMbuffer dmbuffer) {
	Bool ret = _glXAssociateDMPbufferSGIX(dpy, pbuffer, params, dmbuffer);
	CheckGLError("glXAssociateDMPbufferSGIX");
	return ret;
}
typedef int (APIENTRYP PFNGLXBINDCHANNELTOWINDOWSGIX)(Display *display, int screen, int channel, Window window);
PFNGLXBINDCHANNELTOWINDOWSGIX gglXBindChannelToWindowSGIX;
static PFNGLXBINDCHANNELTOWINDOWSGIX _glXBindChannelToWindowSGIX;
static int APIENTRY d_glXBindChannelToWindowSGIX(Display *display, int screen, int channel, Window window) {
	int ret = _glXBindChannelToWindowSGIX(display, screen, channel, window);
	CheckGLError("glXBindChannelToWindowSGIX");
	return ret;
}
typedef int (APIENTRYP PFNGLXBINDHYPERPIPESGIX)(Display *dpy, int hpId);
PFNGLXBINDHYPERPIPESGIX gglXBindHyperpipeSGIX;
static PFNGLXBINDHYPERPIPESGIX _glXBindHyperpipeSGIX;
static int APIENTRY d_glXBindHyperpipeSGIX(Display *dpy, int hpId) {
	int ret = _glXBindHyperpipeSGIX(dpy, hpId);
	CheckGLError("glXBindHyperpipeSGIX");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXBINDSWAPBARRIERNV)(Display *dpy, GLuint group, GLuint barrier);
PFNGLXBINDSWAPBARRIERNV gglXBindSwapBarrierNV;
static PFNGLXBINDSWAPBARRIERNV _glXBindSwapBarrierNV;
static Bool APIENTRY d_glXBindSwapBarrierNV(Display *dpy, GLuint group, GLuint barrier) {
	Bool ret = _glXBindSwapBarrierNV(dpy, group, barrier);
	CheckGLError("glXBindSwapBarrierNV");
	return ret;
}
typedef void (APIENTRYP PFNGLXBINDSWAPBARRIERSGIX)(Display *dpy, GLXDrawable drawable, int barrier);
PFNGLXBINDSWAPBARRIERSGIX gglXBindSwapBarrierSGIX;
static PFNGLXBINDSWAPBARRIERSGIX _glXBindSwapBarrierSGIX;
static void APIENTRY d_glXBindSwapBarrierSGIX(Display *dpy, GLXDrawable drawable, int barrier) {
	_glXBindSwapBarrierSGIX(dpy, drawable, barrier);
	CheckGLError("glXBindSwapBarrierSGIX");
}
typedef void (APIENTRYP PFNGLXBINDTEXIMAGEEXT)(Display *dpy, GLXDrawable drawable, int buffer, const int *attrib_list);
PFNGLXBINDTEXIMAGEEXT gglXBindTexImageEXT;
static PFNGLXBINDTEXIMAGEEXT _glXBindTexImageEXT;
static void APIENTRY d_glXBindTexImageEXT(Display *dpy, GLXDrawable drawable, int buffer, const int *attrib_list) {
	_glXBindTexImageEXT(dpy, drawable, buffer, attrib_list);
	CheckGLError("glXBindTexImageEXT");
}
typedef int (APIENTRYP PFNGLXBINDVIDEOCAPTUREDEVICENV)(Display *dpy, unsigned int video_capture_slot, GLXVideoCaptureDeviceNV device);
PFNGLXBINDVIDEOCAPTUREDEVICENV gglXBindVideoCaptureDeviceNV;
static PFNGLXBINDVIDEOCAPTUREDEVICENV _glXBindVideoCaptureDeviceNV;
static int APIENTRY d_glXBindVideoCaptureDeviceNV(Display *dpy, unsigned int video_capture_slot, GLXVideoCaptureDeviceNV device) {
	int ret = _glXBindVideoCaptureDeviceNV(dpy, video_capture_slot, device);
	CheckGLError("glXBindVideoCaptureDeviceNV");
	return ret;
}
typedef int (APIENTRYP PFNGLXBINDVIDEODEVICENV)(Display *dpy, unsigned int video_slot, unsigned int video_device, const int *attrib_list);
PFNGLXBINDVIDEODEVICENV gglXBindVideoDeviceNV;
static PFNGLXBINDVIDEODEVICENV _glXBindVideoDeviceNV;
static int APIENTRY d_glXBindVideoDeviceNV(Display *dpy, unsigned int video_slot, unsigned int video_device, const int *attrib_list) {
	int ret = _glXBindVideoDeviceNV(dpy, video_slot, video_device, attrib_list);
	CheckGLError("glXBindVideoDeviceNV");
	return ret;
}
typedef int (APIENTRYP PFNGLXBINDVIDEOIMAGENV)(Display *dpy, GLXVideoDeviceNV VideoDevice, GLXPbuffer pbuf, int iVideoBuffer);
PFNGLXBINDVIDEOIMAGENV gglXBindVideoImageNV;
static PFNGLXBINDVIDEOIMAGENV _glXBindVideoImageNV;
static int APIENTRY d_glXBindVideoImageNV(Display *dpy, GLXVideoDeviceNV VideoDevice, GLXPbuffer pbuf, int iVideoBuffer) {
	int ret = _glXBindVideoImageNV(dpy, VideoDevice, pbuf, iVideoBuffer);
	CheckGLError("glXBindVideoImageNV");
	return ret;
}
typedef void (APIENTRYP PFNGLXBLITCONTEXTFRAMEBUFFERAMD)(GLXContext dstCtx, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
PFNGLXBLITCONTEXTFRAMEBUFFERAMD gglXBlitContextFramebufferAMD;
static PFNGLXBLITCONTEXTFRAMEBUFFERAMD _glXBlitContextFramebufferAMD;
static void APIENTRY d_glXBlitContextFramebufferAMD(GLXContext dstCtx, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
	_glXBlitContextFramebufferAMD(dstCtx, srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	CheckGLError("glXBlitContextFramebufferAMD");
}
typedef int (APIENTRYP PFNGLXCHANNELRECTSGIX)(Display *display, int screen, int channel, int x, int y, int w, int h);
PFNGLXCHANNELRECTSGIX gglXChannelRectSGIX;
static PFNGLXCHANNELRECTSGIX _glXChannelRectSGIX;
static int APIENTRY d_glXChannelRectSGIX(Display *display, int screen, int channel, int x, int y, int w, int h) {
	int ret = _glXChannelRectSGIX(display, screen, channel, x, y, w, h);
	CheckGLError("glXChannelRectSGIX");
	return ret;
}
typedef int (APIENTRYP PFNGLXCHANNELRECTSYNCSGIX)(Display *display, int screen, int channel, GLenum synctype);
PFNGLXCHANNELRECTSYNCSGIX gglXChannelRectSyncSGIX;
static PFNGLXCHANNELRECTSYNCSGIX _glXChannelRectSyncSGIX;
static int APIENTRY d_glXChannelRectSyncSGIX(Display *display, int screen, int channel, GLenum synctype) {
	int ret = _glXChannelRectSyncSGIX(display, screen, channel, synctype);
	CheckGLError("glXChannelRectSyncSGIX");
	return ret;
}
typedef GLXFBConfig * (APIENTRYP PFNGLXCHOOSEFBCONFIG)(Display *dpy, int screen, const int *attrib_list, int *nelements);
PFNGLXCHOOSEFBCONFIG gglXChooseFBConfig;
static PFNGLXCHOOSEFBCONFIG _glXChooseFBConfig;
static GLXFBConfig * APIENTRY d_glXChooseFBConfig(Display *dpy, int screen, const int *attrib_list, int *nelements) {
	GLXFBConfig * ret = _glXChooseFBConfig(dpy, screen, attrib_list, nelements);
	CheckGLError("glXChooseFBConfig");
	return ret;
}
typedef GLXFBConfigSGIX * (APIENTRYP PFNGLXCHOOSEFBCONFIGSGIX)(Display *dpy, int screen, int *attrib_list, int *nelements);
PFNGLXCHOOSEFBCONFIGSGIX gglXChooseFBConfigSGIX;
static PFNGLXCHOOSEFBCONFIGSGIX _glXChooseFBConfigSGIX;
static GLXFBConfigSGIX * APIENTRY d_glXChooseFBConfigSGIX(Display *dpy, int screen, int *attrib_list, int *nelements) {
	GLXFBConfigSGIX * ret = _glXChooseFBConfigSGIX(dpy, screen, attrib_list, nelements);
	CheckGLError("glXChooseFBConfigSGIX");
	return ret;
}
typedef XVisualInfo * (APIENTRYP PFNGLXCHOOSEVISUAL)(Display *dpy, int screen, int *attribList);
PFNGLXCHOOSEVISUAL gglXChooseVisual;
static PFNGLXCHOOSEVISUAL _glXChooseVisual;
static XVisualInfo * APIENTRY d_glXChooseVisual(Display *dpy, int screen, int *attribList) {
	XVisualInfo * ret = _glXChooseVisual(dpy, screen, attribList);
	CheckGLError("glXChooseVisual");
	return ret;
}
typedef void (APIENTRYP PFNGLXCOPYBUFFERSUBDATANV)(Display *dpy, GLXContext readCtx, GLXContext writeCtx, GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
PFNGLXCOPYBUFFERSUBDATANV gglXCopyBufferSubDataNV;
static PFNGLXCOPYBUFFERSUBDATANV _glXCopyBufferSubDataNV;
static void APIENTRY d_glXCopyBufferSubDataNV(Display *dpy, GLXContext readCtx, GLXContext writeCtx, GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
	_glXCopyBufferSubDataNV(dpy, readCtx, writeCtx, readTarget, writeTarget, readOffset, writeOffset, size);
	CheckGLError("glXCopyBufferSubDataNV");
}
typedef void (APIENTRYP PFNGLXNAMEDCOPYBUFFERSUBDATANV)(Display *dpy, GLXContext readCtx, GLXContext writeCtx, GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
PFNGLXNAMEDCOPYBUFFERSUBDATANV gglXNamedCopyBufferSubDataNV;
static PFNGLXNAMEDCOPYBUFFERSUBDATANV _glXNamedCopyBufferSubDataNV;
static void APIENTRY d_glXNamedCopyBufferSubDataNV(Display *dpy, GLXContext readCtx, GLXContext writeCtx, GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size) {
	_glXNamedCopyBufferSubDataNV(dpy, readCtx, writeCtx, readBuffer, writeBuffer, readOffset, writeOffset, size);
	CheckGLError("glXNamedCopyBufferSubDataNV");
}
typedef void (APIENTRYP PFNGLXCOPYCONTEXT)(Display *dpy, GLXContext src, GLXContext dst, unsigned long mask);
PFNGLXCOPYCONTEXT gglXCopyContext;
static PFNGLXCOPYCONTEXT _glXCopyContext;
static void APIENTRY d_glXCopyContext(Display *dpy, GLXContext src, GLXContext dst, unsigned long mask) {
	_glXCopyContext(dpy, src, dst, mask);
	CheckGLError("glXCopyContext");
}
typedef void (APIENTRYP PFNGLXCOPYIMAGESUBDATANV)(Display *dpy, GLXContext srcCtx, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLXContext dstCtx, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth);
PFNGLXCOPYIMAGESUBDATANV gglXCopyImageSubDataNV;
static PFNGLXCOPYIMAGESUBDATANV _glXCopyImageSubDataNV;
static void APIENTRY d_glXCopyImageSubDataNV(Display *dpy, GLXContext srcCtx, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLXContext dstCtx, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth) {
	_glXCopyImageSubDataNV(dpy, srcCtx, srcName, srcTarget, srcLevel, srcX, srcY, srcZ, dstCtx, dstName, dstTarget, dstLevel, dstX, dstY, dstZ, width, height, depth);
	CheckGLError("glXCopyImageSubDataNV");
}
typedef void (APIENTRYP PFNGLXCOPYSUBBUFFERMESA)(Display *dpy, GLXDrawable drawable, int x, int y, int width, int height);
PFNGLXCOPYSUBBUFFERMESA gglXCopySubBufferMESA;
static PFNGLXCOPYSUBBUFFERMESA _glXCopySubBufferMESA;
static void APIENTRY d_glXCopySubBufferMESA(Display *dpy, GLXDrawable drawable, int x, int y, int width, int height) {
	_glXCopySubBufferMESA(dpy, drawable, x, y, width, height);
	CheckGLError("glXCopySubBufferMESA");
}
typedef GLXContext (APIENTRYP PFNGLXCREATEASSOCIATEDCONTEXTAMD)(unsigned int id, GLXContext share_list);
PFNGLXCREATEASSOCIATEDCONTEXTAMD gglXCreateAssociatedContextAMD;
static PFNGLXCREATEASSOCIATEDCONTEXTAMD _glXCreateAssociatedContextAMD;
static GLXContext APIENTRY d_glXCreateAssociatedContextAMD(unsigned int id, GLXContext share_list) {
	GLXContext ret = _glXCreateAssociatedContextAMD(id, share_list);
	CheckGLError("glXCreateAssociatedContextAMD");
	return ret;
}
typedef GLXContext (APIENTRYP PFNGLXCREATEASSOCIATEDCONTEXTATTRIBSAMD)(unsigned int id, GLXContext share_context, const int *attribList);
PFNGLXCREATEASSOCIATEDCONTEXTATTRIBSAMD gglXCreateAssociatedContextAttribsAMD;
static PFNGLXCREATEASSOCIATEDCONTEXTATTRIBSAMD _glXCreateAssociatedContextAttribsAMD;
static GLXContext APIENTRY d_glXCreateAssociatedContextAttribsAMD(unsigned int id, GLXContext share_context, const int *attribList) {
	GLXContext ret = _glXCreateAssociatedContextAttribsAMD(id, share_context, attribList);
	CheckGLError("glXCreateAssociatedContextAttribsAMD");
	return ret;
}
typedef GLXContext (APIENTRYP PFNGLXCREATECONTEXTATTRIBSARB)(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list);
PFNGLXCREATECONTEXTATTRIBSARB gglXCreateContextAttribsARB;
static PFNGLXCREATECONTEXTATTRIBSARB _glXCreateContextAttribsARB;
static GLXContext APIENTRY d_glXCreateContextAttribsARB(Display *dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list) {
	GLXContext ret = _glXCreateContextAttribsARB(dpy, config, share_context, direct, attrib_list);
	CheckGLError("glXCreateContextAttribsARB");
	return ret;
}
typedef GLXContext (APIENTRYP PFNGLXCREATECONTEXT)(Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
PFNGLXCREATECONTEXT gglXCreateContext;
static PFNGLXCREATECONTEXT _glXCreateContext;
static GLXContext APIENTRY d_glXCreateContext(Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct) {
	GLXContext ret = _glXCreateContext(dpy, vis, shareList, direct);
	CheckGLError("glXCreateContext");
	return ret;
}
typedef GLXContext (APIENTRYP PFNGLXCREATECONTEXTWITHCONFIGSGIX)(Display *dpy, GLXFBConfigSGIX config, int render_type, GLXContext share_list, Bool direct);
PFNGLXCREATECONTEXTWITHCONFIGSGIX gglXCreateContextWithConfigSGIX;
static PFNGLXCREATECONTEXTWITHCONFIGSGIX _glXCreateContextWithConfigSGIX;
static GLXContext APIENTRY d_glXCreateContextWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, int render_type, GLXContext share_list, Bool direct) {
	GLXContext ret = _glXCreateContextWithConfigSGIX(dpy, config, render_type, share_list, direct);
	CheckGLError("glXCreateContextWithConfigSGIX");
	return ret;
}
typedef GLXPbufferSGIX (APIENTRYP PFNGLXCREATEGLXPBUFFERSGIX)(Display *dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height, int *attrib_list);
PFNGLXCREATEGLXPBUFFERSGIX gglXCreateGLXPbufferSGIX;
static PFNGLXCREATEGLXPBUFFERSGIX _glXCreateGLXPbufferSGIX;
static GLXPbufferSGIX APIENTRY d_glXCreateGLXPbufferSGIX(Display *dpy, GLXFBConfigSGIX config, unsigned int width, unsigned int height, int *attrib_list) {
	GLXPbufferSGIX ret = _glXCreateGLXPbufferSGIX(dpy, config, width, height, attrib_list);
	CheckGLError("glXCreateGLXPbufferSGIX");
	return ret;
}
typedef GLXPixmap (APIENTRYP PFNGLXCREATEGLXPIXMAP)(Display *dpy, XVisualInfo *visual, Pixmap pixmap);
PFNGLXCREATEGLXPIXMAP gglXCreateGLXPixmap;
static PFNGLXCREATEGLXPIXMAP _glXCreateGLXPixmap;
static GLXPixmap APIENTRY d_glXCreateGLXPixmap(Display *dpy, XVisualInfo *visual, Pixmap pixmap) {
	GLXPixmap ret = _glXCreateGLXPixmap(dpy, visual, pixmap);
	CheckGLError("glXCreateGLXPixmap");
	return ret;
}
typedef GLXPixmap (APIENTRYP PFNGLXCREATEGLXPIXMAPMESA)(Display *dpy, XVisualInfo *visual, Pixmap pixmap, Colormap cmap);
PFNGLXCREATEGLXPIXMAPMESA gglXCreateGLXPixmapMESA;
static PFNGLXCREATEGLXPIXMAPMESA _glXCreateGLXPixmapMESA;
static GLXPixmap APIENTRY d_glXCreateGLXPixmapMESA(Display *dpy, XVisualInfo *visual, Pixmap pixmap, Colormap cmap) {
	GLXPixmap ret = _glXCreateGLXPixmapMESA(dpy, visual, pixmap, cmap);
	CheckGLError("glXCreateGLXPixmapMESA");
	return ret;
}
typedef GLXPixmap (APIENTRYP PFNGLXCREATEGLXPIXMAPWITHCONFIGSGIX)(Display *dpy, GLXFBConfigSGIX config, Pixmap pixmap);
PFNGLXCREATEGLXPIXMAPWITHCONFIGSGIX gglXCreateGLXPixmapWithConfigSGIX;
static PFNGLXCREATEGLXPIXMAPWITHCONFIGSGIX _glXCreateGLXPixmapWithConfigSGIX;
static GLXPixmap APIENTRY d_glXCreateGLXPixmapWithConfigSGIX(Display *dpy, GLXFBConfigSGIX config, Pixmap pixmap) {
	GLXPixmap ret = _glXCreateGLXPixmapWithConfigSGIX(dpy, config, pixmap);
	CheckGLError("glXCreateGLXPixmapWithConfigSGIX");
	return ret;
}
typedef GLXVideoSourceSGIX (APIENTRYP PFNGLXCREATEGLXVIDEOSOURCESGIX)(Display *display, int screen, VLServer server, VLPath path, int nodeClass, VLNode drainNode);
PFNGLXCREATEGLXVIDEOSOURCESGIX gglXCreateGLXVideoSourceSGIX;
static PFNGLXCREATEGLXVIDEOSOURCESGIX _glXCreateGLXVideoSourceSGIX;
static GLXVideoSourceSGIX APIENTRY d_glXCreateGLXVideoSourceSGIX(Display *display, int screen, VLServer server, VLPath path, int nodeClass, VLNode drainNode) {
	GLXVideoSourceSGIX ret = _glXCreateGLXVideoSourceSGIX(display, screen, server, path, nodeClass, drainNode);
	CheckGLError("glXCreateGLXVideoSourceSGIX");
	return ret;
}
typedef GLXContext (APIENTRYP PFNGLXCREATENEWCONTEXT)(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct);
PFNGLXCREATENEWCONTEXT gglXCreateNewContext;
static PFNGLXCREATENEWCONTEXT _glXCreateNewContext;
static GLXContext APIENTRY d_glXCreateNewContext(Display *dpy, GLXFBConfig config, int render_type, GLXContext share_list, Bool direct) {
	GLXContext ret = _glXCreateNewContext(dpy, config, render_type, share_list, direct);
	CheckGLError("glXCreateNewContext");
	return ret;
}
typedef GLXPbuffer (APIENTRYP PFNGLXCREATEPBUFFER)(Display *dpy, GLXFBConfig config, const int *attrib_list);
PFNGLXCREATEPBUFFER gglXCreatePbuffer;
static PFNGLXCREATEPBUFFER _glXCreatePbuffer;
static GLXPbuffer APIENTRY d_glXCreatePbuffer(Display *dpy, GLXFBConfig config, const int *attrib_list) {
	GLXPbuffer ret = _glXCreatePbuffer(dpy, config, attrib_list);
	CheckGLError("glXCreatePbuffer");
	return ret;
}
typedef GLXPixmap (APIENTRYP PFNGLXCREATEPIXMAP)(Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attrib_list);
PFNGLXCREATEPIXMAP gglXCreatePixmap;
static PFNGLXCREATEPIXMAP _glXCreatePixmap;
static GLXPixmap APIENTRY d_glXCreatePixmap(Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attrib_list) {
	GLXPixmap ret = _glXCreatePixmap(dpy, config, pixmap, attrib_list);
	CheckGLError("glXCreatePixmap");
	return ret;
}
typedef GLXWindow (APIENTRYP PFNGLXCREATEWINDOW)(Display *dpy, GLXFBConfig config, Window win, const int *attrib_list);
PFNGLXCREATEWINDOW gglXCreateWindow;
static PFNGLXCREATEWINDOW _glXCreateWindow;
static GLXWindow APIENTRY d_glXCreateWindow(Display *dpy, GLXFBConfig config, Window win, const int *attrib_list) {
	GLXWindow ret = _glXCreateWindow(dpy, config, win, attrib_list);
	CheckGLError("glXCreateWindow");
	return ret;
}
typedef void (APIENTRYP PFNGLXCUSHIONSGI)(Display *dpy, Window window, float cushion);
PFNGLXCUSHIONSGI gglXCushionSGI;
static PFNGLXCUSHIONSGI _glXCushionSGI;
static void APIENTRY d_glXCushionSGI(Display *dpy, Window window, float cushion) {
	_glXCushionSGI(dpy, window, cushion);
	CheckGLError("glXCushionSGI");
}
typedef Bool (APIENTRYP PFNGLXDELAYBEFORESWAPNV)(Display *dpy, GLXDrawable drawable, GLfloat seconds);
PFNGLXDELAYBEFORESWAPNV gglXDelayBeforeSwapNV;
static PFNGLXDELAYBEFORESWAPNV _glXDelayBeforeSwapNV;
static Bool APIENTRY d_glXDelayBeforeSwapNV(Display *dpy, GLXDrawable drawable, GLfloat seconds) {
	Bool ret = _glXDelayBeforeSwapNV(dpy, drawable, seconds);
	CheckGLError("glXDelayBeforeSwapNV");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXDELETEASSOCIATEDCONTEXTAMD)(GLXContext ctx);
PFNGLXDELETEASSOCIATEDCONTEXTAMD gglXDeleteAssociatedContextAMD;
static PFNGLXDELETEASSOCIATEDCONTEXTAMD _glXDeleteAssociatedContextAMD;
static Bool APIENTRY d_glXDeleteAssociatedContextAMD(GLXContext ctx) {
	Bool ret = _glXDeleteAssociatedContextAMD(ctx);
	CheckGLError("glXDeleteAssociatedContextAMD");
	return ret;
}
typedef void (APIENTRYP PFNGLXDESTROYCONTEXT)(Display *dpy, GLXContext ctx);
PFNGLXDESTROYCONTEXT gglXDestroyContext;
static PFNGLXDESTROYCONTEXT _glXDestroyContext;
static void APIENTRY d_glXDestroyContext(Display *dpy, GLXContext ctx) {
	_glXDestroyContext(dpy, ctx);
	CheckGLError("glXDestroyContext");
}
typedef void (APIENTRYP PFNGLXDESTROYGLXPBUFFERSGIX)(Display *dpy, GLXPbufferSGIX pbuf);
PFNGLXDESTROYGLXPBUFFERSGIX gglXDestroyGLXPbufferSGIX;
static PFNGLXDESTROYGLXPBUFFERSGIX _glXDestroyGLXPbufferSGIX;
static void APIENTRY d_glXDestroyGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuf) {
	_glXDestroyGLXPbufferSGIX(dpy, pbuf);
	CheckGLError("glXDestroyGLXPbufferSGIX");
}
typedef void (APIENTRYP PFNGLXDESTROYGLXPIXMAP)(Display *dpy, GLXPixmap pixmap);
PFNGLXDESTROYGLXPIXMAP gglXDestroyGLXPixmap;
static PFNGLXDESTROYGLXPIXMAP _glXDestroyGLXPixmap;
static void APIENTRY d_glXDestroyGLXPixmap(Display *dpy, GLXPixmap pixmap) {
	_glXDestroyGLXPixmap(dpy, pixmap);
	CheckGLError("glXDestroyGLXPixmap");
}
typedef void (APIENTRYP PFNGLXDESTROYGLXVIDEOSOURCESGIX)(Display *dpy, GLXVideoSourceSGIX glxvideosource);
PFNGLXDESTROYGLXVIDEOSOURCESGIX gglXDestroyGLXVideoSourceSGIX;
static PFNGLXDESTROYGLXVIDEOSOURCESGIX _glXDestroyGLXVideoSourceSGIX;
static void APIENTRY d_glXDestroyGLXVideoSourceSGIX(Display *dpy, GLXVideoSourceSGIX glxvideosource) {
	_glXDestroyGLXVideoSourceSGIX(dpy, glxvideosource);
	CheckGLError("glXDestroyGLXVideoSourceSGIX");
}
typedef int (APIENTRYP PFNGLXDESTROYHYPERPIPECONFIGSGIX)(Display *dpy, int hpId);
PFNGLXDESTROYHYPERPIPECONFIGSGIX gglXDestroyHyperpipeConfigSGIX;
static PFNGLXDESTROYHYPERPIPECONFIGSGIX _glXDestroyHyperpipeConfigSGIX;
static int APIENTRY d_glXDestroyHyperpipeConfigSGIX(Display *dpy, int hpId) {
	int ret = _glXDestroyHyperpipeConfigSGIX(dpy, hpId);
	CheckGLError("glXDestroyHyperpipeConfigSGIX");
	return ret;
}
typedef void (APIENTRYP PFNGLXDESTROYPBUFFER)(Display *dpy, GLXPbuffer pbuf);
PFNGLXDESTROYPBUFFER gglXDestroyPbuffer;
static PFNGLXDESTROYPBUFFER _glXDestroyPbuffer;
static void APIENTRY d_glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf) {
	_glXDestroyPbuffer(dpy, pbuf);
	CheckGLError("glXDestroyPbuffer");
}
typedef void (APIENTRYP PFNGLXDESTROYPIXMAP)(Display *dpy, GLXPixmap pixmap);
PFNGLXDESTROYPIXMAP gglXDestroyPixmap;
static PFNGLXDESTROYPIXMAP _glXDestroyPixmap;
static void APIENTRY d_glXDestroyPixmap(Display *dpy, GLXPixmap pixmap) {
	_glXDestroyPixmap(dpy, pixmap);
	CheckGLError("glXDestroyPixmap");
}
typedef void (APIENTRYP PFNGLXDESTROYWINDOW)(Display *dpy, GLXWindow win);
PFNGLXDESTROYWINDOW gglXDestroyWindow;
static PFNGLXDESTROYWINDOW _glXDestroyWindow;
static void APIENTRY d_glXDestroyWindow(Display *dpy, GLXWindow win) {
	_glXDestroyWindow(dpy, win);
	CheckGLError("glXDestroyWindow");
}
typedef GLXVideoCaptureDeviceNV * (APIENTRYP PFNGLXENUMERATEVIDEOCAPTUREDEVICESNV)(Display *dpy, int screen, int *nelements);
PFNGLXENUMERATEVIDEOCAPTUREDEVICESNV gglXEnumerateVideoCaptureDevicesNV;
static PFNGLXENUMERATEVIDEOCAPTUREDEVICESNV _glXEnumerateVideoCaptureDevicesNV;
static GLXVideoCaptureDeviceNV * APIENTRY d_glXEnumerateVideoCaptureDevicesNV(Display *dpy, int screen, int *nelements) {
	GLXVideoCaptureDeviceNV * ret = _glXEnumerateVideoCaptureDevicesNV(dpy, screen, nelements);
	CheckGLError("glXEnumerateVideoCaptureDevicesNV");
	return ret;
}
typedef unsigned int * (APIENTRYP PFNGLXENUMERATEVIDEODEVICESNV)(Display *dpy, int screen, int *nelements);
PFNGLXENUMERATEVIDEODEVICESNV gglXEnumerateVideoDevicesNV;
static PFNGLXENUMERATEVIDEODEVICESNV _glXEnumerateVideoDevicesNV;
static unsigned int * APIENTRY d_glXEnumerateVideoDevicesNV(Display *dpy, int screen, int *nelements) {
	unsigned int * ret = _glXEnumerateVideoDevicesNV(dpy, screen, nelements);
	CheckGLError("glXEnumerateVideoDevicesNV");
	return ret;
}
typedef void (APIENTRYP PFNGLXFREECONTEXTEXT)(Display *dpy, GLXContext context);
PFNGLXFREECONTEXTEXT gglXFreeContextEXT;
static PFNGLXFREECONTEXTEXT _glXFreeContextEXT;
static void APIENTRY d_glXFreeContextEXT(Display *dpy, GLXContext context) {
	_glXFreeContextEXT(dpy, context);
	CheckGLError("glXFreeContextEXT");
}
typedef unsigned int (APIENTRYP PFNGLXGETAGPOFFSETMESA)(const void *pointer);
PFNGLXGETAGPOFFSETMESA gglXGetAGPOffsetMESA;
static PFNGLXGETAGPOFFSETMESA _glXGetAGPOffsetMESA;
static unsigned int APIENTRY d_glXGetAGPOffsetMESA(const void *pointer) {
	unsigned int ret = _glXGetAGPOffsetMESA(pointer);
	CheckGLError("glXGetAGPOffsetMESA");
	return ret;
}
typedef const char * (APIENTRYP PFNGLXGETCLIENTSTRING)(Display *dpy, int name);
PFNGLXGETCLIENTSTRING gglXGetClientString;
static PFNGLXGETCLIENTSTRING _glXGetClientString;
static const char * APIENTRY d_glXGetClientString(Display *dpy, int name) {
	const char * ret = _glXGetClientString(dpy, name);
	CheckGLError("glXGetClientString");
	return ret;
}
typedef int (APIENTRYP PFNGLXGETCONFIG)(Display *dpy, XVisualInfo *visual, int attrib, int *value);
PFNGLXGETCONFIG gglXGetConfig;
static PFNGLXGETCONFIG _glXGetConfig;
static int APIENTRY d_glXGetConfig(Display *dpy, XVisualInfo *visual, int attrib, int *value) {
	int ret = _glXGetConfig(dpy, visual, attrib, value);
	CheckGLError("glXGetConfig");
	return ret;
}
typedef unsigned int (APIENTRYP PFNGLXGETCONTEXTGPUIDAMD)(GLXContext ctx);
PFNGLXGETCONTEXTGPUIDAMD gglXGetContextGPUIDAMD;
static PFNGLXGETCONTEXTGPUIDAMD _glXGetContextGPUIDAMD;
static unsigned int APIENTRY d_glXGetContextGPUIDAMD(GLXContext ctx) {
	unsigned int ret = _glXGetContextGPUIDAMD(ctx);
	CheckGLError("glXGetContextGPUIDAMD");
	return ret;
}
typedef GLXContextID (APIENTRYP PFNGLXGETCONTEXTIDEXT)(const GLXContext context);
PFNGLXGETCONTEXTIDEXT gglXGetContextIDEXT;
static PFNGLXGETCONTEXTIDEXT _glXGetContextIDEXT;
static GLXContextID APIENTRY d_glXGetContextIDEXT(const GLXContext context) {
	GLXContextID ret = _glXGetContextIDEXT(context);
	CheckGLError("glXGetContextIDEXT");
	return ret;
}
typedef GLXContext (APIENTRYP PFNGLXGETCURRENTASSOCIATEDCONTEXTAMD)();
PFNGLXGETCURRENTASSOCIATEDCONTEXTAMD gglXGetCurrentAssociatedContextAMD;
static PFNGLXGETCURRENTASSOCIATEDCONTEXTAMD _glXGetCurrentAssociatedContextAMD;
static GLXContext APIENTRY d_glXGetCurrentAssociatedContextAMD() {
	GLXContext ret = _glXGetCurrentAssociatedContextAMD();
	CheckGLError("glXGetCurrentAssociatedContextAMD");
	return ret;
}
typedef GLXContext (APIENTRYP PFNGLXGETCURRENTCONTEXT)();
PFNGLXGETCURRENTCONTEXT gglXGetCurrentContext;
static PFNGLXGETCURRENTCONTEXT _glXGetCurrentContext;
static GLXContext APIENTRY d_glXGetCurrentContext() {
	GLXContext ret = _glXGetCurrentContext();
	CheckGLError("glXGetCurrentContext");
	return ret;
}
typedef Display * (APIENTRYP PFNGLXGETCURRENTDISPLAYEXT)();
PFNGLXGETCURRENTDISPLAYEXT gglXGetCurrentDisplayEXT;
static PFNGLXGETCURRENTDISPLAYEXT _glXGetCurrentDisplayEXT;
static Display * APIENTRY d_glXGetCurrentDisplayEXT() {
	Display * ret = _glXGetCurrentDisplayEXT();
	CheckGLError("glXGetCurrentDisplayEXT");
	return ret;
}
typedef Display * (APIENTRYP PFNGLXGETCURRENTDISPLAY)();
PFNGLXGETCURRENTDISPLAY gglXGetCurrentDisplay;
static PFNGLXGETCURRENTDISPLAY _glXGetCurrentDisplay;
static Display * APIENTRY d_glXGetCurrentDisplay() {
	Display * ret = _glXGetCurrentDisplay();
	CheckGLError("glXGetCurrentDisplay");
	return ret;
}
typedef GLXDrawable (APIENTRYP PFNGLXGETCURRENTDRAWABLE)();
PFNGLXGETCURRENTDRAWABLE gglXGetCurrentDrawable;
static PFNGLXGETCURRENTDRAWABLE _glXGetCurrentDrawable;
static GLXDrawable APIENTRY d_glXGetCurrentDrawable() {
	GLXDrawable ret = _glXGetCurrentDrawable();
	CheckGLError("glXGetCurrentDrawable");
	return ret;
}
typedef GLXDrawable (APIENTRYP PFNGLXGETCURRENTREADDRAWABLESGI)();
PFNGLXGETCURRENTREADDRAWABLESGI gglXGetCurrentReadDrawableSGI;
static PFNGLXGETCURRENTREADDRAWABLESGI _glXGetCurrentReadDrawableSGI;
static GLXDrawable APIENTRY d_glXGetCurrentReadDrawableSGI() {
	GLXDrawable ret = _glXGetCurrentReadDrawableSGI();
	CheckGLError("glXGetCurrentReadDrawableSGI");
	return ret;
}
typedef GLXDrawable (APIENTRYP PFNGLXGETCURRENTREADDRAWABLE)();
PFNGLXGETCURRENTREADDRAWABLE gglXGetCurrentReadDrawable;
static PFNGLXGETCURRENTREADDRAWABLE _glXGetCurrentReadDrawable;
static GLXDrawable APIENTRY d_glXGetCurrentReadDrawable() {
	GLXDrawable ret = _glXGetCurrentReadDrawable();
	CheckGLError("glXGetCurrentReadDrawable");
	return ret;
}
typedef int (APIENTRYP PFNGLXGETFBCONFIGATTRIB)(Display *dpy, GLXFBConfig config, int attribute, int *value);
PFNGLXGETFBCONFIGATTRIB gglXGetFBConfigAttrib;
static PFNGLXGETFBCONFIGATTRIB _glXGetFBConfigAttrib;
static int APIENTRY d_glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config, int attribute, int *value) {
	int ret = _glXGetFBConfigAttrib(dpy, config, attribute, value);
	CheckGLError("glXGetFBConfigAttrib");
	return ret;
}
typedef int (APIENTRYP PFNGLXGETFBCONFIGATTRIBSGIX)(Display *dpy, GLXFBConfigSGIX config, int attribute, int *value);
PFNGLXGETFBCONFIGATTRIBSGIX gglXGetFBConfigAttribSGIX;
static PFNGLXGETFBCONFIGATTRIBSGIX _glXGetFBConfigAttribSGIX;
static int APIENTRY d_glXGetFBConfigAttribSGIX(Display *dpy, GLXFBConfigSGIX config, int attribute, int *value) {
	int ret = _glXGetFBConfigAttribSGIX(dpy, config, attribute, value);
	CheckGLError("glXGetFBConfigAttribSGIX");
	return ret;
}
typedef GLXFBConfigSGIX (APIENTRYP PFNGLXGETFBCONFIGFROMVISUALSGIX)(Display *dpy, XVisualInfo *vis);
PFNGLXGETFBCONFIGFROMVISUALSGIX gglXGetFBConfigFromVisualSGIX;
static PFNGLXGETFBCONFIGFROMVISUALSGIX _glXGetFBConfigFromVisualSGIX;
static GLXFBConfigSGIX APIENTRY d_glXGetFBConfigFromVisualSGIX(Display *dpy, XVisualInfo *vis) {
	GLXFBConfigSGIX ret = _glXGetFBConfigFromVisualSGIX(dpy, vis);
	CheckGLError("glXGetFBConfigFromVisualSGIX");
	return ret;
}
typedef GLXFBConfig * (APIENTRYP PFNGLXGETFBCONFIGS)(Display *dpy, int screen, int *nelements);
PFNGLXGETFBCONFIGS gglXGetFBConfigs;
static PFNGLXGETFBCONFIGS _glXGetFBConfigs;
static GLXFBConfig * APIENTRY d_glXGetFBConfigs(Display *dpy, int screen, int *nelements) {
	GLXFBConfig * ret = _glXGetFBConfigs(dpy, screen, nelements);
	CheckGLError("glXGetFBConfigs");
	return ret;
}
typedef unsigned int (APIENTRYP PFNGLXGETGPUIDSAMD)(unsigned int maxCount, unsigned int *ids);
PFNGLXGETGPUIDSAMD gglXGetGPUIDsAMD;
static PFNGLXGETGPUIDSAMD _glXGetGPUIDsAMD;
static unsigned int APIENTRY d_glXGetGPUIDsAMD(unsigned int maxCount, unsigned int *ids) {
	unsigned int ret = _glXGetGPUIDsAMD(maxCount, ids);
	CheckGLError("glXGetGPUIDsAMD");
	return ret;
}
typedef int (APIENTRYP PFNGLXGETGPUINFOAMD)(unsigned int id, int property, GLenum dataType, unsigned int size, void *data);
PFNGLXGETGPUINFOAMD gglXGetGPUInfoAMD;
static PFNGLXGETGPUINFOAMD _glXGetGPUInfoAMD;
static int APIENTRY d_glXGetGPUInfoAMD(unsigned int id, int property, GLenum dataType, unsigned int size, void *data) {
	int ret = _glXGetGPUInfoAMD(id, property, dataType, size, data);
	CheckGLError("glXGetGPUInfoAMD");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXGETMSCRATEOML)(Display *dpy, GLXDrawable drawable, int32_t *numerator, int32_t *denominator);
PFNGLXGETMSCRATEOML gglXGetMscRateOML;
static PFNGLXGETMSCRATEOML _glXGetMscRateOML;
static Bool APIENTRY d_glXGetMscRateOML(Display *dpy, GLXDrawable drawable, int32_t *numerator, int32_t *denominator) {
	Bool ret = _glXGetMscRateOML(dpy, drawable, numerator, denominator);
	CheckGLError("glXGetMscRateOML");
	return ret;
}
typedef __GLXextFuncPtr (APIENTRYP PFNGLXGETPROCADDRESSARB)(const GLubyte *procName);
PFNGLXGETPROCADDRESSARB gglXGetProcAddressARB;
static PFNGLXGETPROCADDRESSARB _glXGetProcAddressARB;
static __GLXextFuncPtr APIENTRY d_glXGetProcAddressARB(const GLubyte *procName) {
	__GLXextFuncPtr ret = _glXGetProcAddressARB(procName);
	CheckGLError("glXGetProcAddressARB");
	return ret;
}
typedef __GLXextFuncPtr (APIENTRYP PFNGLXGETPROCADDRESS)(const GLubyte *procName);
PFNGLXGETPROCADDRESS gglXGetProcAddress;
static PFNGLXGETPROCADDRESS _glXGetProcAddress;
static __GLXextFuncPtr APIENTRY d_glXGetProcAddress(const GLubyte *procName) {
	__GLXextFuncPtr ret = _glXGetProcAddress(procName);
	CheckGLError("glXGetProcAddress");
	return ret;
}
typedef void (APIENTRYP PFNGLXGETSELECTEDEVENT)(Display *dpy, GLXDrawable draw, unsigned long *event_mask);
PFNGLXGETSELECTEDEVENT gglXGetSelectedEvent;
static PFNGLXGETSELECTEDEVENT _glXGetSelectedEvent;
static void APIENTRY d_glXGetSelectedEvent(Display *dpy, GLXDrawable draw, unsigned long *event_mask) {
	_glXGetSelectedEvent(dpy, draw, event_mask);
	CheckGLError("glXGetSelectedEvent");
}
typedef void (APIENTRYP PFNGLXGETSELECTEDEVENTSGIX)(Display *dpy, GLXDrawable drawable, unsigned long *mask);
PFNGLXGETSELECTEDEVENTSGIX gglXGetSelectedEventSGIX;
static PFNGLXGETSELECTEDEVENTSGIX _glXGetSelectedEventSGIX;
static void APIENTRY d_glXGetSelectedEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long *mask) {
	_glXGetSelectedEventSGIX(dpy, drawable, mask);
	CheckGLError("glXGetSelectedEventSGIX");
}
typedef Bool (APIENTRYP PFNGLXGETSYNCVALUESOML)(Display *dpy, GLXDrawable drawable, int64_t *ust, int64_t *msc, int64_t *sbc);
PFNGLXGETSYNCVALUESOML gglXGetSyncValuesOML;
static PFNGLXGETSYNCVALUESOML _glXGetSyncValuesOML;
static Bool APIENTRY d_glXGetSyncValuesOML(Display *dpy, GLXDrawable drawable, int64_t *ust, int64_t *msc, int64_t *sbc) {
	Bool ret = _glXGetSyncValuesOML(dpy, drawable, ust, msc, sbc);
	CheckGLError("glXGetSyncValuesOML");
	return ret;
}
typedef Status (APIENTRYP PFNGLXGETTRANSPARENTINDEXSUN)(Display *dpy, Window overlay, Window underlay, long *pTransparentIndex);
PFNGLXGETTRANSPARENTINDEXSUN gglXGetTransparentIndexSUN;
static PFNGLXGETTRANSPARENTINDEXSUN _glXGetTransparentIndexSUN;
static Status APIENTRY d_glXGetTransparentIndexSUN(Display *dpy, Window overlay, Window underlay, long *pTransparentIndex) {
	Status ret = _glXGetTransparentIndexSUN(dpy, overlay, underlay, pTransparentIndex);
	CheckGLError("glXGetTransparentIndexSUN");
	return ret;
}
typedef int (APIENTRYP PFNGLXGETVIDEODEVICENV)(Display *dpy, int screen, int numVideoDevices, GLXVideoDeviceNV *pVideoDevice);
PFNGLXGETVIDEODEVICENV gglXGetVideoDeviceNV;
static PFNGLXGETVIDEODEVICENV _glXGetVideoDeviceNV;
static int APIENTRY d_glXGetVideoDeviceNV(Display *dpy, int screen, int numVideoDevices, GLXVideoDeviceNV *pVideoDevice) {
	int ret = _glXGetVideoDeviceNV(dpy, screen, numVideoDevices, pVideoDevice);
	CheckGLError("glXGetVideoDeviceNV");
	return ret;
}
typedef int (APIENTRYP PFNGLXGETVIDEOINFONV)(Display *dpy, int screen, GLXVideoDeviceNV VideoDevice, unsigned long *pulCounterOutputPbuffer, unsigned long *pulCounterOutputVideo);
PFNGLXGETVIDEOINFONV gglXGetVideoInfoNV;
static PFNGLXGETVIDEOINFONV _glXGetVideoInfoNV;
static int APIENTRY d_glXGetVideoInfoNV(Display *dpy, int screen, GLXVideoDeviceNV VideoDevice, unsigned long *pulCounterOutputPbuffer, unsigned long *pulCounterOutputVideo) {
	int ret = _glXGetVideoInfoNV(dpy, screen, VideoDevice, pulCounterOutputPbuffer, pulCounterOutputVideo);
	CheckGLError("glXGetVideoInfoNV");
	return ret;
}
typedef int (APIENTRYP PFNGLXGETVIDEOSYNCSGI)(unsigned int *count);
PFNGLXGETVIDEOSYNCSGI gglXGetVideoSyncSGI;
static PFNGLXGETVIDEOSYNCSGI _glXGetVideoSyncSGI;
static int APIENTRY d_glXGetVideoSyncSGI(unsigned int *count) {
	int ret = _glXGetVideoSyncSGI(count);
	CheckGLError("glXGetVideoSyncSGI");
	return ret;
}
typedef XVisualInfo * (APIENTRYP PFNGLXGETVISUALFROMFBCONFIG)(Display *dpy, GLXFBConfig config);
PFNGLXGETVISUALFROMFBCONFIG gglXGetVisualFromFBConfig;
static PFNGLXGETVISUALFROMFBCONFIG _glXGetVisualFromFBConfig;
static XVisualInfo * APIENTRY d_glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config) {
	XVisualInfo * ret = _glXGetVisualFromFBConfig(dpy, config);
	CheckGLError("glXGetVisualFromFBConfig");
	return ret;
}
typedef XVisualInfo * (APIENTRYP PFNGLXGETVISUALFROMFBCONFIGSGIX)(Display *dpy, GLXFBConfigSGIX config);
PFNGLXGETVISUALFROMFBCONFIGSGIX gglXGetVisualFromFBConfigSGIX;
static PFNGLXGETVISUALFROMFBCONFIGSGIX _glXGetVisualFromFBConfigSGIX;
static XVisualInfo * APIENTRY d_glXGetVisualFromFBConfigSGIX(Display *dpy, GLXFBConfigSGIX config) {
	XVisualInfo * ret = _glXGetVisualFromFBConfigSGIX(dpy, config);
	CheckGLError("glXGetVisualFromFBConfigSGIX");
	return ret;
}
typedef int (APIENTRYP PFNGLXHYPERPIPEATTRIBSGIX)(Display *dpy, int timeSlice, int attrib, int size, void *attribList);
PFNGLXHYPERPIPEATTRIBSGIX gglXHyperpipeAttribSGIX;
static PFNGLXHYPERPIPEATTRIBSGIX _glXHyperpipeAttribSGIX;
static int APIENTRY d_glXHyperpipeAttribSGIX(Display *dpy, int timeSlice, int attrib, int size, void *attribList) {
	int ret = _glXHyperpipeAttribSGIX(dpy, timeSlice, attrib, size, attribList);
	CheckGLError("glXHyperpipeAttribSGIX");
	return ret;
}
typedef int (APIENTRYP PFNGLXHYPERPIPECONFIGSGIX)(Display *dpy, int networkId, int npipes, GLXHyperpipeConfigSGIX *cfg, int *hpId);
PFNGLXHYPERPIPECONFIGSGIX gglXHyperpipeConfigSGIX;
static PFNGLXHYPERPIPECONFIGSGIX _glXHyperpipeConfigSGIX;
static int APIENTRY d_glXHyperpipeConfigSGIX(Display *dpy, int networkId, int npipes, GLXHyperpipeConfigSGIX *cfg, int *hpId) {
	int ret = _glXHyperpipeConfigSGIX(dpy, networkId, npipes, cfg, hpId);
	CheckGLError("glXHyperpipeConfigSGIX");
	return ret;
}
typedef GLXContext (APIENTRYP PFNGLXIMPORTCONTEXTEXT)(Display *dpy, GLXContextID contextID);
PFNGLXIMPORTCONTEXTEXT gglXImportContextEXT;
static PFNGLXIMPORTCONTEXTEXT _glXImportContextEXT;
static GLXContext APIENTRY d_glXImportContextEXT(Display *dpy, GLXContextID contextID) {
	GLXContext ret = _glXImportContextEXT(dpy, contextID);
	CheckGLError("glXImportContextEXT");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXISDIRECT)(Display *dpy, GLXContext ctx);
PFNGLXISDIRECT gglXIsDirect;
static PFNGLXISDIRECT _glXIsDirect;
static Bool APIENTRY d_glXIsDirect(Display *dpy, GLXContext ctx) {
	Bool ret = _glXIsDirect(dpy, ctx);
	CheckGLError("glXIsDirect");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXJOINSWAPGROUPNV)(Display *dpy, GLXDrawable drawable, GLuint group);
PFNGLXJOINSWAPGROUPNV gglXJoinSwapGroupNV;
static PFNGLXJOINSWAPGROUPNV _glXJoinSwapGroupNV;
static Bool APIENTRY d_glXJoinSwapGroupNV(Display *dpy, GLXDrawable drawable, GLuint group) {
	Bool ret = _glXJoinSwapGroupNV(dpy, drawable, group);
	CheckGLError("glXJoinSwapGroupNV");
	return ret;
}
typedef void (APIENTRYP PFNGLXJOINSWAPGROUPSGIX)(Display *dpy, GLXDrawable drawable, GLXDrawable member);
PFNGLXJOINSWAPGROUPSGIX gglXJoinSwapGroupSGIX;
static PFNGLXJOINSWAPGROUPSGIX _glXJoinSwapGroupSGIX;
static void APIENTRY d_glXJoinSwapGroupSGIX(Display *dpy, GLXDrawable drawable, GLXDrawable member) {
	_glXJoinSwapGroupSGIX(dpy, drawable, member);
	CheckGLError("glXJoinSwapGroupSGIX");
}
typedef void (APIENTRYP PFNGLXLOCKVIDEOCAPTUREDEVICENV)(Display *dpy, GLXVideoCaptureDeviceNV device);
PFNGLXLOCKVIDEOCAPTUREDEVICENV gglXLockVideoCaptureDeviceNV;
static PFNGLXLOCKVIDEOCAPTUREDEVICENV _glXLockVideoCaptureDeviceNV;
static void APIENTRY d_glXLockVideoCaptureDeviceNV(Display *dpy, GLXVideoCaptureDeviceNV device) {
	_glXLockVideoCaptureDeviceNV(dpy, device);
	CheckGLError("glXLockVideoCaptureDeviceNV");
}
typedef Bool (APIENTRYP PFNGLXMAKEASSOCIATEDCONTEXTCURRENTAMD)(GLXContext ctx);
PFNGLXMAKEASSOCIATEDCONTEXTCURRENTAMD gglXMakeAssociatedContextCurrentAMD;
static PFNGLXMAKEASSOCIATEDCONTEXTCURRENTAMD _glXMakeAssociatedContextCurrentAMD;
static Bool APIENTRY d_glXMakeAssociatedContextCurrentAMD(GLXContext ctx) {
	Bool ret = _glXMakeAssociatedContextCurrentAMD(ctx);
	CheckGLError("glXMakeAssociatedContextCurrentAMD");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXMAKECONTEXTCURRENT)(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
PFNGLXMAKECONTEXTCURRENT gglXMakeContextCurrent;
static PFNGLXMAKECONTEXTCURRENT _glXMakeContextCurrent;
static Bool APIENTRY d_glXMakeContextCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx) {
	Bool ret = _glXMakeContextCurrent(dpy, draw, read, ctx);
	CheckGLError("glXMakeContextCurrent");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXMAKECURRENT)(Display *dpy, GLXDrawable drawable, GLXContext ctx);
PFNGLXMAKECURRENT gglXMakeCurrent;
static PFNGLXMAKECURRENT _glXMakeCurrent;
static Bool APIENTRY d_glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx) {
	Bool ret = _glXMakeCurrent(dpy, drawable, ctx);
	CheckGLError("glXMakeCurrent");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXMAKECURRENTREADSGI)(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
PFNGLXMAKECURRENTREADSGI gglXMakeCurrentReadSGI;
static PFNGLXMAKECURRENTREADSGI _glXMakeCurrentReadSGI;
static Bool APIENTRY d_glXMakeCurrentReadSGI(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx) {
	Bool ret = _glXMakeCurrentReadSGI(dpy, draw, read, ctx);
	CheckGLError("glXMakeCurrentReadSGI");
	return ret;
}
typedef int (APIENTRYP PFNGLXQUERYCHANNELDELTASSGIX)(Display *display, int screen, int channel, int *x, int *y, int *w, int *h);
PFNGLXQUERYCHANNELDELTASSGIX gglXQueryChannelDeltasSGIX;
static PFNGLXQUERYCHANNELDELTASSGIX _glXQueryChannelDeltasSGIX;
static int APIENTRY d_glXQueryChannelDeltasSGIX(Display *display, int screen, int channel, int *x, int *y, int *w, int *h) {
	int ret = _glXQueryChannelDeltasSGIX(display, screen, channel, x, y, w, h);
	CheckGLError("glXQueryChannelDeltasSGIX");
	return ret;
}
typedef int (APIENTRYP PFNGLXQUERYCHANNELRECTSGIX)(Display *display, int screen, int channel, int *dx, int *dy, int *dw, int *dh);
PFNGLXQUERYCHANNELRECTSGIX gglXQueryChannelRectSGIX;
static PFNGLXQUERYCHANNELRECTSGIX _glXQueryChannelRectSGIX;
static int APIENTRY d_glXQueryChannelRectSGIX(Display *display, int screen, int channel, int *dx, int *dy, int *dw, int *dh) {
	int ret = _glXQueryChannelRectSGIX(display, screen, channel, dx, dy, dw, dh);
	CheckGLError("glXQueryChannelRectSGIX");
	return ret;
}
typedef int (APIENTRYP PFNGLXQUERYCONTEXT)(Display *dpy, GLXContext ctx, int attribute, int *value);
PFNGLXQUERYCONTEXT gglXQueryContext;
static PFNGLXQUERYCONTEXT _glXQueryContext;
static int APIENTRY d_glXQueryContext(Display *dpy, GLXContext ctx, int attribute, int *value) {
	int ret = _glXQueryContext(dpy, ctx, attribute, value);
	CheckGLError("glXQueryContext");
	return ret;
}
typedef int (APIENTRYP PFNGLXQUERYCONTEXTINFOEXT)(Display *dpy, GLXContext context, int attribute, int *value);
PFNGLXQUERYCONTEXTINFOEXT gglXQueryContextInfoEXT;
static PFNGLXQUERYCONTEXTINFOEXT _glXQueryContextInfoEXT;
static int APIENTRY d_glXQueryContextInfoEXT(Display *dpy, GLXContext context, int attribute, int *value) {
	int ret = _glXQueryContextInfoEXT(dpy, context, attribute, value);
	CheckGLError("glXQueryContextInfoEXT");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXQUERYCURRENTRENDERERINTEGERMESA)(int attribute, unsigned int *value);
PFNGLXQUERYCURRENTRENDERERINTEGERMESA gglXQueryCurrentRendererIntegerMESA;
static PFNGLXQUERYCURRENTRENDERERINTEGERMESA _glXQueryCurrentRendererIntegerMESA;
static Bool APIENTRY d_glXQueryCurrentRendererIntegerMESA(int attribute, unsigned int *value) {
	Bool ret = _glXQueryCurrentRendererIntegerMESA(attribute, value);
	CheckGLError("glXQueryCurrentRendererIntegerMESA");
	return ret;
}
typedef const char * (APIENTRYP PFNGLXQUERYCURRENTRENDERERSTRINGMESA)(int attribute);
PFNGLXQUERYCURRENTRENDERERSTRINGMESA gglXQueryCurrentRendererStringMESA;
static PFNGLXQUERYCURRENTRENDERERSTRINGMESA _glXQueryCurrentRendererStringMESA;
static const char * APIENTRY d_glXQueryCurrentRendererStringMESA(int attribute) {
	const char * ret = _glXQueryCurrentRendererStringMESA(attribute);
	CheckGLError("glXQueryCurrentRendererStringMESA");
	return ret;
}
typedef void (APIENTRYP PFNGLXQUERYDRAWABLE)(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value);
PFNGLXQUERYDRAWABLE gglXQueryDrawable;
static PFNGLXQUERYDRAWABLE _glXQueryDrawable;
static void APIENTRY d_glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value) {
	_glXQueryDrawable(dpy, draw, attribute, value);
	CheckGLError("glXQueryDrawable");
}
typedef Bool (APIENTRYP PFNGLXQUERYEXTENSION)(Display *dpy, int *errorb, int *event);
PFNGLXQUERYEXTENSION gglXQueryExtension;
static PFNGLXQUERYEXTENSION _glXQueryExtension;
static Bool APIENTRY d_glXQueryExtension(Display *dpy, int *errorb, int *event) {
	Bool ret = _glXQueryExtension(dpy, errorb, event);
	CheckGLError("glXQueryExtension");
	return ret;
}
typedef const char * (APIENTRYP PFNGLXQUERYEXTENSIONSSTRING)(Display *dpy, int screen);
PFNGLXQUERYEXTENSIONSSTRING gglXQueryExtensionsString;
static PFNGLXQUERYEXTENSIONSSTRING _glXQueryExtensionsString;
static const char * APIENTRY d_glXQueryExtensionsString(Display *dpy, int screen) {
	const char * ret = _glXQueryExtensionsString(dpy, screen);
	CheckGLError("glXQueryExtensionsString");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXQUERYFRAMECOUNTNV)(Display *dpy, int screen, GLuint *count);
PFNGLXQUERYFRAMECOUNTNV gglXQueryFrameCountNV;
static PFNGLXQUERYFRAMECOUNTNV _glXQueryFrameCountNV;
static Bool APIENTRY d_glXQueryFrameCountNV(Display *dpy, int screen, GLuint *count) {
	Bool ret = _glXQueryFrameCountNV(dpy, screen, count);
	CheckGLError("glXQueryFrameCountNV");
	return ret;
}
typedef int (APIENTRYP PFNGLXQUERYGLXPBUFFERSGIX)(Display *dpy, GLXPbufferSGIX pbuf, int attribute, unsigned int *value);
PFNGLXQUERYGLXPBUFFERSGIX gglXQueryGLXPbufferSGIX;
static PFNGLXQUERYGLXPBUFFERSGIX _glXQueryGLXPbufferSGIX;
static int APIENTRY d_glXQueryGLXPbufferSGIX(Display *dpy, GLXPbufferSGIX pbuf, int attribute, unsigned int *value) {
	int ret = _glXQueryGLXPbufferSGIX(dpy, pbuf, attribute, value);
	CheckGLError("glXQueryGLXPbufferSGIX");
	return ret;
}
typedef int (APIENTRYP PFNGLXQUERYHYPERPIPEATTRIBSGIX)(Display *dpy, int timeSlice, int attrib, int size, void *returnAttribList);
PFNGLXQUERYHYPERPIPEATTRIBSGIX gglXQueryHyperpipeAttribSGIX;
static PFNGLXQUERYHYPERPIPEATTRIBSGIX _glXQueryHyperpipeAttribSGIX;
static int APIENTRY d_glXQueryHyperpipeAttribSGIX(Display *dpy, int timeSlice, int attrib, int size, void *returnAttribList) {
	int ret = _glXQueryHyperpipeAttribSGIX(dpy, timeSlice, attrib, size, returnAttribList);
	CheckGLError("glXQueryHyperpipeAttribSGIX");
	return ret;
}
typedef int (APIENTRYP PFNGLXQUERYHYPERPIPEBESTATTRIBSGIX)(Display *dpy, int timeSlice, int attrib, int size, void *attribList, void *returnAttribList);
PFNGLXQUERYHYPERPIPEBESTATTRIBSGIX gglXQueryHyperpipeBestAttribSGIX;
static PFNGLXQUERYHYPERPIPEBESTATTRIBSGIX _glXQueryHyperpipeBestAttribSGIX;
static int APIENTRY d_glXQueryHyperpipeBestAttribSGIX(Display *dpy, int timeSlice, int attrib, int size, void *attribList, void *returnAttribList) {
	int ret = _glXQueryHyperpipeBestAttribSGIX(dpy, timeSlice, attrib, size, attribList, returnAttribList);
	CheckGLError("glXQueryHyperpipeBestAttribSGIX");
	return ret;
}
typedef GLXHyperpipeConfigSGIX * (APIENTRYP PFNGLXQUERYHYPERPIPECONFIGSGIX)(Display *dpy, int hpId, int *npipes);
PFNGLXQUERYHYPERPIPECONFIGSGIX gglXQueryHyperpipeConfigSGIX;
static PFNGLXQUERYHYPERPIPECONFIGSGIX _glXQueryHyperpipeConfigSGIX;
static GLXHyperpipeConfigSGIX * APIENTRY d_glXQueryHyperpipeConfigSGIX(Display *dpy, int hpId, int *npipes) {
	GLXHyperpipeConfigSGIX * ret = _glXQueryHyperpipeConfigSGIX(dpy, hpId, npipes);
	CheckGLError("glXQueryHyperpipeConfigSGIX");
	return ret;
}
typedef GLXHyperpipeNetworkSGIX * (APIENTRYP PFNGLXQUERYHYPERPIPENETWORKSGIX)(Display *dpy, int *npipes);
PFNGLXQUERYHYPERPIPENETWORKSGIX gglXQueryHyperpipeNetworkSGIX;
static PFNGLXQUERYHYPERPIPENETWORKSGIX _glXQueryHyperpipeNetworkSGIX;
static GLXHyperpipeNetworkSGIX * APIENTRY d_glXQueryHyperpipeNetworkSGIX(Display *dpy, int *npipes) {
	GLXHyperpipeNetworkSGIX * ret = _glXQueryHyperpipeNetworkSGIX(dpy, npipes);
	CheckGLError("glXQueryHyperpipeNetworkSGIX");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXQUERYMAXSWAPBARRIERSSGIX)(Display *dpy, int screen, int *max);
PFNGLXQUERYMAXSWAPBARRIERSSGIX gglXQueryMaxSwapBarriersSGIX;
static PFNGLXQUERYMAXSWAPBARRIERSSGIX _glXQueryMaxSwapBarriersSGIX;
static Bool APIENTRY d_glXQueryMaxSwapBarriersSGIX(Display *dpy, int screen, int *max) {
	Bool ret = _glXQueryMaxSwapBarriersSGIX(dpy, screen, max);
	CheckGLError("glXQueryMaxSwapBarriersSGIX");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXQUERYMAXSWAPGROUPSNV)(Display *dpy, int screen, GLuint *maxGroups, GLuint *maxBarriers);
PFNGLXQUERYMAXSWAPGROUPSNV gglXQueryMaxSwapGroupsNV;
static PFNGLXQUERYMAXSWAPGROUPSNV _glXQueryMaxSwapGroupsNV;
static Bool APIENTRY d_glXQueryMaxSwapGroupsNV(Display *dpy, int screen, GLuint *maxGroups, GLuint *maxBarriers) {
	Bool ret = _glXQueryMaxSwapGroupsNV(dpy, screen, maxGroups, maxBarriers);
	CheckGLError("glXQueryMaxSwapGroupsNV");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXQUERYRENDERERINTEGERMESA)(Display *dpy, int screen, int renderer, int attribute, unsigned int *value);
PFNGLXQUERYRENDERERINTEGERMESA gglXQueryRendererIntegerMESA;
static PFNGLXQUERYRENDERERINTEGERMESA _glXQueryRendererIntegerMESA;
static Bool APIENTRY d_glXQueryRendererIntegerMESA(Display *dpy, int screen, int renderer, int attribute, unsigned int *value) {
	Bool ret = _glXQueryRendererIntegerMESA(dpy, screen, renderer, attribute, value);
	CheckGLError("glXQueryRendererIntegerMESA");
	return ret;
}
typedef const char * (APIENTRYP PFNGLXQUERYRENDERERSTRINGMESA)(Display *dpy, int screen, int renderer, int attribute);
PFNGLXQUERYRENDERERSTRINGMESA gglXQueryRendererStringMESA;
static PFNGLXQUERYRENDERERSTRINGMESA _glXQueryRendererStringMESA;
static const char * APIENTRY d_glXQueryRendererStringMESA(Display *dpy, int screen, int renderer, int attribute) {
	const char * ret = _glXQueryRendererStringMESA(dpy, screen, renderer, attribute);
	CheckGLError("glXQueryRendererStringMESA");
	return ret;
}
typedef const char * (APIENTRYP PFNGLXQUERYSERVERSTRING)(Display *dpy, int screen, int name);
PFNGLXQUERYSERVERSTRING gglXQueryServerString;
static PFNGLXQUERYSERVERSTRING _glXQueryServerString;
static const char * APIENTRY d_glXQueryServerString(Display *dpy, int screen, int name) {
	const char * ret = _glXQueryServerString(dpy, screen, name);
	CheckGLError("glXQueryServerString");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXQUERYSWAPGROUPNV)(Display *dpy, GLXDrawable drawable, GLuint *group, GLuint *barrier);
PFNGLXQUERYSWAPGROUPNV gglXQuerySwapGroupNV;
static PFNGLXQUERYSWAPGROUPNV _glXQuerySwapGroupNV;
static Bool APIENTRY d_glXQuerySwapGroupNV(Display *dpy, GLXDrawable drawable, GLuint *group, GLuint *barrier) {
	Bool ret = _glXQuerySwapGroupNV(dpy, drawable, group, barrier);
	CheckGLError("glXQuerySwapGroupNV");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXQUERYVERSION)(Display *dpy, int *maj, int *min);
PFNGLXQUERYVERSION gglXQueryVersion;
static PFNGLXQUERYVERSION _glXQueryVersion;
static Bool APIENTRY d_glXQueryVersion(Display *dpy, int *maj, int *min) {
	Bool ret = _glXQueryVersion(dpy, maj, min);
	CheckGLError("glXQueryVersion");
	return ret;
}
typedef int (APIENTRYP PFNGLXQUERYVIDEOCAPTUREDEVICENV)(Display *dpy, GLXVideoCaptureDeviceNV device, int attribute, int *value);
PFNGLXQUERYVIDEOCAPTUREDEVICENV gglXQueryVideoCaptureDeviceNV;
static PFNGLXQUERYVIDEOCAPTUREDEVICENV _glXQueryVideoCaptureDeviceNV;
static int APIENTRY d_glXQueryVideoCaptureDeviceNV(Display *dpy, GLXVideoCaptureDeviceNV device, int attribute, int *value) {
	int ret = _glXQueryVideoCaptureDeviceNV(dpy, device, attribute, value);
	CheckGLError("glXQueryVideoCaptureDeviceNV");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXRELEASEBUFFERSMESA)(Display *dpy, GLXDrawable drawable);
PFNGLXRELEASEBUFFERSMESA gglXReleaseBuffersMESA;
static PFNGLXRELEASEBUFFERSMESA _glXReleaseBuffersMESA;
static Bool APIENTRY d_glXReleaseBuffersMESA(Display *dpy, GLXDrawable drawable) {
	Bool ret = _glXReleaseBuffersMESA(dpy, drawable);
	CheckGLError("glXReleaseBuffersMESA");
	return ret;
}
typedef void (APIENTRYP PFNGLXRELEASETEXIMAGEEXT)(Display *dpy, GLXDrawable drawable, int buffer);
PFNGLXRELEASETEXIMAGEEXT gglXReleaseTexImageEXT;
static PFNGLXRELEASETEXIMAGEEXT _glXReleaseTexImageEXT;
static void APIENTRY d_glXReleaseTexImageEXT(Display *dpy, GLXDrawable drawable, int buffer) {
	_glXReleaseTexImageEXT(dpy, drawable, buffer);
	CheckGLError("glXReleaseTexImageEXT");
}
typedef void (APIENTRYP PFNGLXRELEASEVIDEOCAPTUREDEVICENV)(Display *dpy, GLXVideoCaptureDeviceNV device);
PFNGLXRELEASEVIDEOCAPTUREDEVICENV gglXReleaseVideoCaptureDeviceNV;
static PFNGLXRELEASEVIDEOCAPTUREDEVICENV _glXReleaseVideoCaptureDeviceNV;
static void APIENTRY d_glXReleaseVideoCaptureDeviceNV(Display *dpy, GLXVideoCaptureDeviceNV device) {
	_glXReleaseVideoCaptureDeviceNV(dpy, device);
	CheckGLError("glXReleaseVideoCaptureDeviceNV");
}
typedef int (APIENTRYP PFNGLXRELEASEVIDEODEVICENV)(Display *dpy, int screen, GLXVideoDeviceNV VideoDevice);
PFNGLXRELEASEVIDEODEVICENV gglXReleaseVideoDeviceNV;
static PFNGLXRELEASEVIDEODEVICENV _glXReleaseVideoDeviceNV;
static int APIENTRY d_glXReleaseVideoDeviceNV(Display *dpy, int screen, GLXVideoDeviceNV VideoDevice) {
	int ret = _glXReleaseVideoDeviceNV(dpy, screen, VideoDevice);
	CheckGLError("glXReleaseVideoDeviceNV");
	return ret;
}
typedef int (APIENTRYP PFNGLXRELEASEVIDEOIMAGENV)(Display *dpy, GLXPbuffer pbuf);
PFNGLXRELEASEVIDEOIMAGENV gglXReleaseVideoImageNV;
static PFNGLXRELEASEVIDEOIMAGENV _glXReleaseVideoImageNV;
static int APIENTRY d_glXReleaseVideoImageNV(Display *dpy, GLXPbuffer pbuf) {
	int ret = _glXReleaseVideoImageNV(dpy, pbuf);
	CheckGLError("glXReleaseVideoImageNV");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXRESETFRAMECOUNTNV)(Display *dpy, int screen);
PFNGLXRESETFRAMECOUNTNV gglXResetFrameCountNV;
static PFNGLXRESETFRAMECOUNTNV _glXResetFrameCountNV;
static Bool APIENTRY d_glXResetFrameCountNV(Display *dpy, int screen) {
	Bool ret = _glXResetFrameCountNV(dpy, screen);
	CheckGLError("glXResetFrameCountNV");
	return ret;
}
typedef void (APIENTRYP PFNGLXSELECTEVENT)(Display *dpy, GLXDrawable draw, unsigned long event_mask);
PFNGLXSELECTEVENT gglXSelectEvent;
static PFNGLXSELECTEVENT _glXSelectEvent;
static void APIENTRY d_glXSelectEvent(Display *dpy, GLXDrawable draw, unsigned long event_mask) {
	_glXSelectEvent(dpy, draw, event_mask);
	CheckGLError("glXSelectEvent");
}
typedef void (APIENTRYP PFNGLXSELECTEVENTSGIX)(Display *dpy, GLXDrawable drawable, unsigned long mask);
PFNGLXSELECTEVENTSGIX gglXSelectEventSGIX;
static PFNGLXSELECTEVENTSGIX _glXSelectEventSGIX;
static void APIENTRY d_glXSelectEventSGIX(Display *dpy, GLXDrawable drawable, unsigned long mask) {
	_glXSelectEventSGIX(dpy, drawable, mask);
	CheckGLError("glXSelectEventSGIX");
}
typedef int (APIENTRYP PFNGLXSENDPBUFFERTOVIDEONV)(Display *dpy, GLXPbuffer pbuf, int iBufferType, unsigned long *pulCounterPbuffer, GLboolean bBlock);
PFNGLXSENDPBUFFERTOVIDEONV gglXSendPbufferToVideoNV;
static PFNGLXSENDPBUFFERTOVIDEONV _glXSendPbufferToVideoNV;
static int APIENTRY d_glXSendPbufferToVideoNV(Display *dpy, GLXPbuffer pbuf, int iBufferType, unsigned long *pulCounterPbuffer, GLboolean bBlock) {
	int ret = _glXSendPbufferToVideoNV(dpy, pbuf, iBufferType, pulCounterPbuffer, bBlock);
	CheckGLError("glXSendPbufferToVideoNV");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXSET3DFXMODEMESA)(int mode);
PFNGLXSET3DFXMODEMESA gglXSet3DfxModeMESA;
static PFNGLXSET3DFXMODEMESA _glXSet3DfxModeMESA;
static Bool APIENTRY d_glXSet3DfxModeMESA(int mode) {
	Bool ret = _glXSet3DfxModeMESA(mode);
	CheckGLError("glXSet3DfxModeMESA");
	return ret;
}
typedef void (APIENTRYP PFNGLXSWAPBUFFERS)(Display *dpy, GLXDrawable drawable);
PFNGLXSWAPBUFFERS gglXSwapBuffers;
static PFNGLXSWAPBUFFERS _glXSwapBuffers;
static void APIENTRY d_glXSwapBuffers(Display *dpy, GLXDrawable drawable) {
	_glXSwapBuffers(dpy, drawable);
	CheckGLError("glXSwapBuffers");
}
typedef int64_t (APIENTRYP PFNGLXSWAPBUFFERSMSCOML)(Display *dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor, int64_t remainder);
PFNGLXSWAPBUFFERSMSCOML gglXSwapBuffersMscOML;
static PFNGLXSWAPBUFFERSMSCOML _glXSwapBuffersMscOML;
static int64_t APIENTRY d_glXSwapBuffersMscOML(Display *dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor, int64_t remainder) {
	int64_t ret = _glXSwapBuffersMscOML(dpy, drawable, target_msc, divisor, remainder);
	CheckGLError("glXSwapBuffersMscOML");
	return ret;
}
typedef void (APIENTRYP PFNGLXSWAPINTERVALEXT)(Display *dpy, GLXDrawable drawable, int interval);
PFNGLXSWAPINTERVALEXT gglXSwapIntervalEXT;
static PFNGLXSWAPINTERVALEXT _glXSwapIntervalEXT;
static void APIENTRY d_glXSwapIntervalEXT(Display *dpy, GLXDrawable drawable, int interval) {
	_glXSwapIntervalEXT(dpy, drawable, interval);
	CheckGLError("glXSwapIntervalEXT");
}
typedef int (APIENTRYP PFNGLXSWAPINTERVALSGI)(int interval);
PFNGLXSWAPINTERVALSGI gglXSwapIntervalSGI;
static PFNGLXSWAPINTERVALSGI _glXSwapIntervalSGI;
static int APIENTRY d_glXSwapIntervalSGI(int interval) {
	int ret = _glXSwapIntervalSGI(interval);
	CheckGLError("glXSwapIntervalSGI");
	return ret;
}
typedef void (APIENTRYP PFNGLXUSEXFONT)(Font font, int first, int count, int list);
PFNGLXUSEXFONT gglXUseXFont;
static PFNGLXUSEXFONT _glXUseXFont;
static void APIENTRY d_glXUseXFont(Font font, int first, int count, int list) {
	_glXUseXFont(font, first, count, list);
	CheckGLError("glXUseXFont");
}
typedef Bool (APIENTRYP PFNGLXWAITFORMSCOML)(Display *dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor, int64_t remainder, int64_t *ust, int64_t *msc, int64_t *sbc);
PFNGLXWAITFORMSCOML gglXWaitForMscOML;
static PFNGLXWAITFORMSCOML _glXWaitForMscOML;
static Bool APIENTRY d_glXWaitForMscOML(Display *dpy, GLXDrawable drawable, int64_t target_msc, int64_t divisor, int64_t remainder, int64_t *ust, int64_t *msc, int64_t *sbc) {
	Bool ret = _glXWaitForMscOML(dpy, drawable, target_msc, divisor, remainder, ust, msc, sbc);
	CheckGLError("glXWaitForMscOML");
	return ret;
}
typedef Bool (APIENTRYP PFNGLXWAITFORSBCOML)(Display *dpy, GLXDrawable drawable, int64_t target_sbc, int64_t *ust, int64_t *msc, int64_t *sbc);
PFNGLXWAITFORSBCOML gglXWaitForSbcOML;
static PFNGLXWAITFORSBCOML _glXWaitForSbcOML;
static Bool APIENTRY d_glXWaitForSbcOML(Display *dpy, GLXDrawable drawable, int64_t target_sbc, int64_t *ust, int64_t *msc, int64_t *sbc) {
	Bool ret = _glXWaitForSbcOML(dpy, drawable, target_sbc, ust, msc, sbc);
	CheckGLError("glXWaitForSbcOML");
	return ret;
}
typedef void (APIENTRYP PFNGLXWAITGL)();
PFNGLXWAITGL gglXWaitGL;
static PFNGLXWAITGL _glXWaitGL;
static void APIENTRY d_glXWaitGL() {
	_glXWaitGL();
	CheckGLError("glXWaitGL");
}
typedef int (APIENTRYP PFNGLXWAITVIDEOSYNCSGI)(int divisor, int remainder, unsigned int *count);
PFNGLXWAITVIDEOSYNCSGI gglXWaitVideoSyncSGI;
static PFNGLXWAITVIDEOSYNCSGI _glXWaitVideoSyncSGI;
static int APIENTRY d_glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count) {
	int ret = _glXWaitVideoSyncSGI(divisor, remainder, count);
	CheckGLError("glXWaitVideoSyncSGI");
	return ret;
}
typedef void (APIENTRYP PFNGLXWAITX)();
PFNGLXWAITX gglXWaitX;
static PFNGLXWAITX _glXWaitX;
static void APIENTRY d_glXWaitX() {
	_glXWaitX();
	CheckGLError("glXWaitX");
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

gglxext_t gglxext;
static const char *gglxext_str = NULL;

static int gglx_check_extension(const char *ext) {
	return strstr(gglxext_str, ext) ? 1 : 0;
}

void gglx_init(int enableDebug) {
	/* GLX_VERSION_1_0 */
	_glXChooseVisual = (PFNGLXCHOOSEVISUAL)GPA(glXChooseVisual);
	_glXCreateContext = (PFNGLXCREATECONTEXT)GPA(glXCreateContext);
	_glXDestroyContext = (PFNGLXDESTROYCONTEXT)GPA(glXDestroyContext);
	_glXMakeCurrent = (PFNGLXMAKECURRENT)GPA(glXMakeCurrent);
	_glXCopyContext = (PFNGLXCOPYCONTEXT)GPA(glXCopyContext);
	_glXSwapBuffers = (PFNGLXSWAPBUFFERS)GPA(glXSwapBuffers);
	_glXCreateGLXPixmap = (PFNGLXCREATEGLXPIXMAP)GPA(glXCreateGLXPixmap);
	_glXDestroyGLXPixmap = (PFNGLXDESTROYGLXPIXMAP)GPA(glXDestroyGLXPixmap);
	_glXQueryExtension = (PFNGLXQUERYEXTENSION)GPA(glXQueryExtension);
	_glXQueryVersion = (PFNGLXQUERYVERSION)GPA(glXQueryVersion);
	_glXIsDirect = (PFNGLXISDIRECT)GPA(glXIsDirect);
	_glXGetConfig = (PFNGLXGETCONFIG)GPA(glXGetConfig);
	_glXGetCurrentContext = (PFNGLXGETCURRENTCONTEXT)GPA(glXGetCurrentContext);
	_glXGetCurrentDrawable = (PFNGLXGETCURRENTDRAWABLE)GPA(glXGetCurrentDrawable);
	_glXWaitGL = (PFNGLXWAITGL)GPA(glXWaitGL);
	_glXWaitX = (PFNGLXWAITX)GPA(glXWaitX);
	_glXUseXFont = (PFNGLXUSEXFONT)GPA(glXUseXFont);

	/* GLX_VERSION_1_1 */
	_glXQueryExtensionsString = (PFNGLXQUERYEXTENSIONSSTRING)GPA(glXQueryExtensionsString);
	_glXQueryServerString = (PFNGLXQUERYSERVERSTRING)GPA(glXQueryServerString);
	_glXGetClientString = (PFNGLXGETCLIENTSTRING)GPA(glXGetClientString);

	/* GLX_VERSION_1_2 */
	_glXGetCurrentDisplay = (PFNGLXGETCURRENTDISPLAY)GPA(glXGetCurrentDisplay);

	/* GLX_VERSION_1_3 */
	_glXGetFBConfigs = (PFNGLXGETFBCONFIGS)GPA(glXGetFBConfigs);
	_glXChooseFBConfig = (PFNGLXCHOOSEFBCONFIG)GPA(glXChooseFBConfig);
	_glXGetFBConfigAttrib = (PFNGLXGETFBCONFIGATTRIB)GPA(glXGetFBConfigAttrib);
	_glXGetVisualFromFBConfig = (PFNGLXGETVISUALFROMFBCONFIG)GPA(glXGetVisualFromFBConfig);
	_glXCreateWindow = (PFNGLXCREATEWINDOW)GPA(glXCreateWindow);
	_glXDestroyWindow = (PFNGLXDESTROYWINDOW)GPA(glXDestroyWindow);
	_glXCreatePixmap = (PFNGLXCREATEPIXMAP)GPA(glXCreatePixmap);
	_glXDestroyPixmap = (PFNGLXDESTROYPIXMAP)GPA(glXDestroyPixmap);
	_glXCreatePbuffer = (PFNGLXCREATEPBUFFER)GPA(glXCreatePbuffer);
	_glXDestroyPbuffer = (PFNGLXDESTROYPBUFFER)GPA(glXDestroyPbuffer);
	_glXQueryDrawable = (PFNGLXQUERYDRAWABLE)GPA(glXQueryDrawable);
	_glXCreateNewContext = (PFNGLXCREATENEWCONTEXT)GPA(glXCreateNewContext);
	_glXMakeContextCurrent = (PFNGLXMAKECONTEXTCURRENT)GPA(glXMakeContextCurrent);
	_glXGetCurrentReadDrawable = (PFNGLXGETCURRENTREADDRAWABLE)GPA(glXGetCurrentReadDrawable);
	_glXQueryContext = (PFNGLXQUERYCONTEXT)GPA(glXQueryContext);
	_glXSelectEvent = (PFNGLXSELECTEVENT)GPA(glXSelectEvent);
	_glXGetSelectedEvent = (PFNGLXGETSELECTEDEVENT)GPA(glXGetSelectedEvent);

	/* GLX_VERSION_1_4 */
	_glXGetProcAddress = (PFNGLXGETPROCADDRESS)GPA(glXGetProcAddress);

	/* GLX_AMD_gpu_association */
	_glXGetGPUIDsAMD = (PFNGLXGETGPUIDSAMD)GPA(glXGetGPUIDsAMD);
	_glXGetGPUInfoAMD = (PFNGLXGETGPUINFOAMD)GPA(glXGetGPUInfoAMD);
	_glXGetContextGPUIDAMD = (PFNGLXGETCONTEXTGPUIDAMD)GPA(glXGetContextGPUIDAMD);
	_glXCreateAssociatedContextAMD = (PFNGLXCREATEASSOCIATEDCONTEXTAMD)GPA(glXCreateAssociatedContextAMD);
	_glXCreateAssociatedContextAttribsAMD = (PFNGLXCREATEASSOCIATEDCONTEXTATTRIBSAMD)GPA(glXCreateAssociatedContextAttribsAMD);
	_glXDeleteAssociatedContextAMD = (PFNGLXDELETEASSOCIATEDCONTEXTAMD)GPA(glXDeleteAssociatedContextAMD);
	_glXMakeAssociatedContextCurrentAMD = (PFNGLXMAKEASSOCIATEDCONTEXTCURRENTAMD)GPA(glXMakeAssociatedContextCurrentAMD);
	_glXGetCurrentAssociatedContextAMD = (PFNGLXGETCURRENTASSOCIATEDCONTEXTAMD)GPA(glXGetCurrentAssociatedContextAMD);
	_glXBlitContextFramebufferAMD = (PFNGLXBLITCONTEXTFRAMEBUFFERAMD)GPA(glXBlitContextFramebufferAMD);

	/* GLX_ARB_create_context */
	_glXCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARB)GPA(glXCreateContextAttribsARB);

	/* GLX_ARB_get_proc_address */
	_glXGetProcAddressARB = (PFNGLXGETPROCADDRESSARB)GPA(glXGetProcAddressARB);

	/* GLX_EXT_import_context */
	_glXGetCurrentDisplayEXT = (PFNGLXGETCURRENTDISPLAYEXT)GPA(glXGetCurrentDisplayEXT);
	_glXQueryContextInfoEXT = (PFNGLXQUERYCONTEXTINFOEXT)GPA(glXQueryContextInfoEXT);
	_glXGetContextIDEXT = (PFNGLXGETCONTEXTIDEXT)GPA(glXGetContextIDEXT);
	_glXImportContextEXT = (PFNGLXIMPORTCONTEXTEXT)GPA(glXImportContextEXT);
	_glXFreeContextEXT = (PFNGLXFREECONTEXTEXT)GPA(glXFreeContextEXT);

	/* GLX_EXT_swap_control */
	_glXSwapIntervalEXT = (PFNGLXSWAPINTERVALEXT)GPA(glXSwapIntervalEXT);

	/* GLX_EXT_texture_from_pixmap */
	_glXBindTexImageEXT = (PFNGLXBINDTEXIMAGEEXT)GPA(glXBindTexImageEXT);
	_glXReleaseTexImageEXT = (PFNGLXRELEASETEXIMAGEEXT)GPA(glXReleaseTexImageEXT);

	/* GLX_MESA_agp_offset */
	_glXGetAGPOffsetMESA = (PFNGLXGETAGPOFFSETMESA)GPA(glXGetAGPOffsetMESA);

	/* GLX_MESA_copy_sub_buffer */
	_glXCopySubBufferMESA = (PFNGLXCOPYSUBBUFFERMESA)GPA(glXCopySubBufferMESA);

	/* GLX_MESA_pixmap_colormap */
	_glXCreateGLXPixmapMESA = (PFNGLXCREATEGLXPIXMAPMESA)GPA(glXCreateGLXPixmapMESA);

	/* GLX_MESA_query_renderer */
	_glXQueryCurrentRendererIntegerMESA = (PFNGLXQUERYCURRENTRENDERERINTEGERMESA)GPA(glXQueryCurrentRendererIntegerMESA);
	_glXQueryCurrentRendererStringMESA = (PFNGLXQUERYCURRENTRENDERERSTRINGMESA)GPA(glXQueryCurrentRendererStringMESA);
	_glXQueryRendererIntegerMESA = (PFNGLXQUERYRENDERERINTEGERMESA)GPA(glXQueryRendererIntegerMESA);
	_glXQueryRendererStringMESA = (PFNGLXQUERYRENDERERSTRINGMESA)GPA(glXQueryRendererStringMESA);

	/* GLX_MESA_release_buffers */
	_glXReleaseBuffersMESA = (PFNGLXRELEASEBUFFERSMESA)GPA(glXReleaseBuffersMESA);

	/* GLX_MESA_set_3dfx_mode */
	_glXSet3DfxModeMESA = (PFNGLXSET3DFXMODEMESA)GPA(glXSet3DfxModeMESA);

	/* GLX_NV_copy_buffer */
	_glXCopyBufferSubDataNV = (PFNGLXCOPYBUFFERSUBDATANV)GPA(glXCopyBufferSubDataNV);
	_glXNamedCopyBufferSubDataNV = (PFNGLXNAMEDCOPYBUFFERSUBDATANV)GPA(glXNamedCopyBufferSubDataNV);

	/* GLX_NV_copy_image */
	_glXCopyImageSubDataNV = (PFNGLXCOPYIMAGESUBDATANV)GPA(glXCopyImageSubDataNV);

	/* GLX_NV_delay_before_swap */
	_glXDelayBeforeSwapNV = (PFNGLXDELAYBEFORESWAPNV)GPA(glXDelayBeforeSwapNV);

	/* GLX_NV_present_video */
	_glXEnumerateVideoDevicesNV = (PFNGLXENUMERATEVIDEODEVICESNV)GPA(glXEnumerateVideoDevicesNV);
	_glXBindVideoDeviceNV = (PFNGLXBINDVIDEODEVICENV)GPA(glXBindVideoDeviceNV);

	/* GLX_NV_swap_group */
	_glXJoinSwapGroupNV = (PFNGLXJOINSWAPGROUPNV)GPA(glXJoinSwapGroupNV);
	_glXBindSwapBarrierNV = (PFNGLXBINDSWAPBARRIERNV)GPA(glXBindSwapBarrierNV);
	_glXQuerySwapGroupNV = (PFNGLXQUERYSWAPGROUPNV)GPA(glXQuerySwapGroupNV);
	_glXQueryMaxSwapGroupsNV = (PFNGLXQUERYMAXSWAPGROUPSNV)GPA(glXQueryMaxSwapGroupsNV);
	_glXQueryFrameCountNV = (PFNGLXQUERYFRAMECOUNTNV)GPA(glXQueryFrameCountNV);
	_glXResetFrameCountNV = (PFNGLXRESETFRAMECOUNTNV)GPA(glXResetFrameCountNV);

	/* GLX_NV_video_capture */
	_glXBindVideoCaptureDeviceNV = (PFNGLXBINDVIDEOCAPTUREDEVICENV)GPA(glXBindVideoCaptureDeviceNV);
	_glXEnumerateVideoCaptureDevicesNV = (PFNGLXENUMERATEVIDEOCAPTUREDEVICESNV)GPA(glXEnumerateVideoCaptureDevicesNV);
	_glXLockVideoCaptureDeviceNV = (PFNGLXLOCKVIDEOCAPTUREDEVICENV)GPA(glXLockVideoCaptureDeviceNV);
	_glXQueryVideoCaptureDeviceNV = (PFNGLXQUERYVIDEOCAPTUREDEVICENV)GPA(glXQueryVideoCaptureDeviceNV);
	_glXReleaseVideoCaptureDeviceNV = (PFNGLXRELEASEVIDEOCAPTUREDEVICENV)GPA(glXReleaseVideoCaptureDeviceNV);

	/* GLX_NV_video_out */
	_glXGetVideoDeviceNV = (PFNGLXGETVIDEODEVICENV)GPA(glXGetVideoDeviceNV);
	_glXReleaseVideoDeviceNV = (PFNGLXRELEASEVIDEODEVICENV)GPA(glXReleaseVideoDeviceNV);
	_glXBindVideoImageNV = (PFNGLXBINDVIDEOIMAGENV)GPA(glXBindVideoImageNV);
	_glXReleaseVideoImageNV = (PFNGLXRELEASEVIDEOIMAGENV)GPA(glXReleaseVideoImageNV);
	_glXSendPbufferToVideoNV = (PFNGLXSENDPBUFFERTOVIDEONV)GPA(glXSendPbufferToVideoNV);
	_glXGetVideoInfoNV = (PFNGLXGETVIDEOINFONV)GPA(glXGetVideoInfoNV);

	/* GLX_OML_sync_control */
	_glXGetSyncValuesOML = (PFNGLXGETSYNCVALUESOML)GPA(glXGetSyncValuesOML);
	_glXGetMscRateOML = (PFNGLXGETMSCRATEOML)GPA(glXGetMscRateOML);
	_glXSwapBuffersMscOML = (PFNGLXSWAPBUFFERSMSCOML)GPA(glXSwapBuffersMscOML);
	_glXWaitForMscOML = (PFNGLXWAITFORMSCOML)GPA(glXWaitForMscOML);
	_glXWaitForSbcOML = (PFNGLXWAITFORSBCOML)GPA(glXWaitForSbcOML);

	/* GLX_SGI_cushion */
	_glXCushionSGI = (PFNGLXCUSHIONSGI)GPA(glXCushionSGI);

	/* GLX_SGI_make_current_read */
	_glXMakeCurrentReadSGI = (PFNGLXMAKECURRENTREADSGI)GPA(glXMakeCurrentReadSGI);
	_glXGetCurrentReadDrawableSGI = (PFNGLXGETCURRENTREADDRAWABLESGI)GPA(glXGetCurrentReadDrawableSGI);

	/* GLX_SGI_swap_control */
	_glXSwapIntervalSGI = (PFNGLXSWAPINTERVALSGI)GPA(glXSwapIntervalSGI);

	/* GLX_SGI_video_sync */
	_glXGetVideoSyncSGI = (PFNGLXGETVIDEOSYNCSGI)GPA(glXGetVideoSyncSGI);
	_glXWaitVideoSyncSGI = (PFNGLXWAITVIDEOSYNCSGI)GPA(glXWaitVideoSyncSGI);

	/* GLX_SGIX_dmbuffer */
	_glXAssociateDMPbufferSGIX = (PFNGLXASSOCIATEDMPBUFFERSGIX)GPA(glXAssociateDMPbufferSGIX);

	/* GLX_SGIX_fbconfig */
	_glXGetFBConfigAttribSGIX = (PFNGLXGETFBCONFIGATTRIBSGIX)GPA(glXGetFBConfigAttribSGIX);
	_glXChooseFBConfigSGIX = (PFNGLXCHOOSEFBCONFIGSGIX)GPA(glXChooseFBConfigSGIX);
	_glXCreateGLXPixmapWithConfigSGIX = (PFNGLXCREATEGLXPIXMAPWITHCONFIGSGIX)GPA(glXCreateGLXPixmapWithConfigSGIX);
	_glXCreateContextWithConfigSGIX = (PFNGLXCREATECONTEXTWITHCONFIGSGIX)GPA(glXCreateContextWithConfigSGIX);
	_glXGetVisualFromFBConfigSGIX = (PFNGLXGETVISUALFROMFBCONFIGSGIX)GPA(glXGetVisualFromFBConfigSGIX);
	_glXGetFBConfigFromVisualSGIX = (PFNGLXGETFBCONFIGFROMVISUALSGIX)GPA(glXGetFBConfigFromVisualSGIX);

	/* GLX_SGIX_hyperpipe */
	_glXQueryHyperpipeNetworkSGIX = (PFNGLXQUERYHYPERPIPENETWORKSGIX)GPA(glXQueryHyperpipeNetworkSGIX);
	_glXHyperpipeConfigSGIX = (PFNGLXHYPERPIPECONFIGSGIX)GPA(glXHyperpipeConfigSGIX);
	_glXQueryHyperpipeConfigSGIX = (PFNGLXQUERYHYPERPIPECONFIGSGIX)GPA(glXQueryHyperpipeConfigSGIX);
	_glXDestroyHyperpipeConfigSGIX = (PFNGLXDESTROYHYPERPIPECONFIGSGIX)GPA(glXDestroyHyperpipeConfigSGIX);
	_glXBindHyperpipeSGIX = (PFNGLXBINDHYPERPIPESGIX)GPA(glXBindHyperpipeSGIX);
	_glXQueryHyperpipeBestAttribSGIX = (PFNGLXQUERYHYPERPIPEBESTATTRIBSGIX)GPA(glXQueryHyperpipeBestAttribSGIX);
	_glXHyperpipeAttribSGIX = (PFNGLXHYPERPIPEATTRIBSGIX)GPA(glXHyperpipeAttribSGIX);
	_glXQueryHyperpipeAttribSGIX = (PFNGLXQUERYHYPERPIPEATTRIBSGIX)GPA(glXQueryHyperpipeAttribSGIX);

	/* GLX_SGIX_pbuffer */
	_glXCreateGLXPbufferSGIX = (PFNGLXCREATEGLXPBUFFERSGIX)GPA(glXCreateGLXPbufferSGIX);
	_glXDestroyGLXPbufferSGIX = (PFNGLXDESTROYGLXPBUFFERSGIX)GPA(glXDestroyGLXPbufferSGIX);
	_glXQueryGLXPbufferSGIX = (PFNGLXQUERYGLXPBUFFERSGIX)GPA(glXQueryGLXPbufferSGIX);
	_glXSelectEventSGIX = (PFNGLXSELECTEVENTSGIX)GPA(glXSelectEventSGIX);
	_glXGetSelectedEventSGIX = (PFNGLXGETSELECTEDEVENTSGIX)GPA(glXGetSelectedEventSGIX);

	/* GLX_SGIX_swap_barrier */
	_glXBindSwapBarrierSGIX = (PFNGLXBINDSWAPBARRIERSGIX)GPA(glXBindSwapBarrierSGIX);
	_glXQueryMaxSwapBarriersSGIX = (PFNGLXQUERYMAXSWAPBARRIERSSGIX)GPA(glXQueryMaxSwapBarriersSGIX);

	/* GLX_SGIX_swap_group */
	_glXJoinSwapGroupSGIX = (PFNGLXJOINSWAPGROUPSGIX)GPA(glXJoinSwapGroupSGIX);

	/* GLX_SGIX_video_resize */
	_glXBindChannelToWindowSGIX = (PFNGLXBINDCHANNELTOWINDOWSGIX)GPA(glXBindChannelToWindowSGIX);
	_glXChannelRectSGIX = (PFNGLXCHANNELRECTSGIX)GPA(glXChannelRectSGIX);
	_glXQueryChannelRectSGIX = (PFNGLXQUERYCHANNELRECTSGIX)GPA(glXQueryChannelRectSGIX);
	_glXQueryChannelDeltasSGIX = (PFNGLXQUERYCHANNELDELTASSGIX)GPA(glXQueryChannelDeltasSGIX);
	_glXChannelRectSyncSGIX = (PFNGLXCHANNELRECTSYNCSGIX)GPA(glXChannelRectSyncSGIX);

	/* GLX_SGIX_video_source */
	_glXCreateGLXVideoSourceSGIX = (PFNGLXCREATEGLXVIDEOSOURCESGIX)GPA(glXCreateGLXVideoSourceSGIX);
	_glXDestroyGLXVideoSourceSGIX = (PFNGLXDESTROYGLXVIDEOSOURCESGIX)GPA(glXDestroyGLXVideoSourceSGIX);

	/* GLX_SUN_get_transparent_index */
	_glXGetTransparentIndexSUN = (PFNGLXGETTRANSPARENTINDEXSUN)GPA(glXGetTransparentIndexSUN);

	gglx_rebind(enableDebug);

	gglxext_str = (const char *)_glGetString(GL_EXTENSIONS);
	memset(&gglxext, 0, sizeof(gglxext));
	if (gglx_check_extension("GLX_3DFX_multisample")) gglxext._GLX_3DFX_multisample = 1;
	if (gglx_check_extension("GLX_AMD_gpu_association")) gglxext._GLX_AMD_gpu_association = 1;
	if (gglx_check_extension("GLX_ARB_context_flush_control")) gglxext._GLX_ARB_context_flush_control = 1;
	if (gglx_check_extension("GLX_ARB_create_context")) gglxext._GLX_ARB_create_context = 1;
	if (gglx_check_extension("GLX_ARB_create_context_profile")) gglxext._GLX_ARB_create_context_profile = 1;
	if (gglx_check_extension("GLX_ARB_create_context_robustness")) gglxext._GLX_ARB_create_context_robustness = 1;
	if (gglx_check_extension("GLX_ARB_fbconfig_float")) gglxext._GLX_ARB_fbconfig_float = 1;
	if (gglx_check_extension("GLX_ARB_framebuffer_sRGB")) gglxext._GLX_ARB_framebuffer_sRGB = 1;
	if (gglx_check_extension("GLX_ARB_get_proc_address")) gglxext._GLX_ARB_get_proc_address = 1;
	if (gglx_check_extension("GLX_ARB_multisample")) gglxext._GLX_ARB_multisample = 1;
	if (gglx_check_extension("GLX_ARB_robustness_application_isolation")) gglxext._GLX_ARB_robustness_application_isolation = 1;
	if (gglx_check_extension("GLX_ARB_robustness_share_group_isolation")) gglxext._GLX_ARB_robustness_share_group_isolation = 1;
	if (gglx_check_extension("GLX_ARB_vertex_buffer_object")) gglxext._GLX_ARB_vertex_buffer_object = 1;
	if (gglx_check_extension("GLX_EXT_buffer_age")) gglxext._GLX_EXT_buffer_age = 1;
	if (gglx_check_extension("GLX_EXT_create_context_es_profile")) gglxext._GLX_EXT_create_context_es_profile = 1;
	if (gglx_check_extension("GLX_EXT_create_context_es2_profile")) gglxext._GLX_EXT_create_context_es2_profile = 1;
	if (gglx_check_extension("GLX_EXT_fbconfig_packed_float")) gglxext._GLX_EXT_fbconfig_packed_float = 1;
	if (gglx_check_extension("GLX_EXT_framebuffer_sRGB")) gglxext._GLX_EXT_framebuffer_sRGB = 1;
	if (gglx_check_extension("GLX_EXT_import_context")) gglxext._GLX_EXT_import_context = 1;
	if (gglx_check_extension("GLX_EXT_libglvnd")) gglxext._GLX_EXT_libglvnd = 1;
	if (gglx_check_extension("GLX_EXT_stereo_tree")) gglxext._GLX_EXT_stereo_tree = 1;
	if (gglx_check_extension("GLX_EXT_swap_control")) gglxext._GLX_EXT_swap_control = 1;
	if (gglx_check_extension("GLX_EXT_swap_control_tear")) gglxext._GLX_EXT_swap_control_tear = 1;
	if (gglx_check_extension("GLX_EXT_texture_from_pixmap")) gglxext._GLX_EXT_texture_from_pixmap = 1;
	if (gglx_check_extension("GLX_EXT_visual_info")) gglxext._GLX_EXT_visual_info = 1;
	if (gglx_check_extension("GLX_EXT_visual_rating")) gglxext._GLX_EXT_visual_rating = 1;
	if (gglx_check_extension("GLX_INTEL_swap_event")) gglxext._GLX_INTEL_swap_event = 1;
	if (gglx_check_extension("GLX_MESA_agp_offset")) gglxext._GLX_MESA_agp_offset = 1;
	if (gglx_check_extension("GLX_MESA_copy_sub_buffer")) gglxext._GLX_MESA_copy_sub_buffer = 1;
	if (gglx_check_extension("GLX_MESA_pixmap_colormap")) gglxext._GLX_MESA_pixmap_colormap = 1;
	if (gglx_check_extension("GLX_MESA_query_renderer")) gglxext._GLX_MESA_query_renderer = 1;
	if (gglx_check_extension("GLX_MESA_release_buffers")) gglxext._GLX_MESA_release_buffers = 1;
	if (gglx_check_extension("GLX_MESA_set_3dfx_mode")) gglxext._GLX_MESA_set_3dfx_mode = 1;
	if (gglx_check_extension("GLX_NV_copy_buffer")) gglxext._GLX_NV_copy_buffer = 1;
	if (gglx_check_extension("GLX_NV_copy_image")) gglxext._GLX_NV_copy_image = 1;
	if (gglx_check_extension("GLX_NV_delay_before_swap")) gglxext._GLX_NV_delay_before_swap = 1;
	if (gglx_check_extension("GLX_NV_float_buffer")) gglxext._GLX_NV_float_buffer = 1;
	if (gglx_check_extension("GLX_NV_multisample_coverage")) gglxext._GLX_NV_multisample_coverage = 1;
	if (gglx_check_extension("GLX_NV_present_video")) gglxext._GLX_NV_present_video = 1;
	if (gglx_check_extension("GLX_NV_robustness_video_memory_purge")) gglxext._GLX_NV_robustness_video_memory_purge = 1;
	if (gglx_check_extension("GLX_NV_swap_group")) gglxext._GLX_NV_swap_group = 1;
	if (gglx_check_extension("GLX_NV_video_capture")) gglxext._GLX_NV_video_capture = 1;
	if (gglx_check_extension("GLX_NV_video_out")) gglxext._GLX_NV_video_out = 1;
	if (gglx_check_extension("GLX_OML_swap_method")) gglxext._GLX_OML_swap_method = 1;
	if (gglx_check_extension("GLX_OML_sync_control")) gglxext._GLX_OML_sync_control = 1;
	if (gglx_check_extension("GLX_SGI_cushion")) gglxext._GLX_SGI_cushion = 1;
	if (gglx_check_extension("GLX_SGI_make_current_read")) gglxext._GLX_SGI_make_current_read = 1;
	if (gglx_check_extension("GLX_SGI_swap_control")) gglxext._GLX_SGI_swap_control = 1;
	if (gglx_check_extension("GLX_SGI_video_sync")) gglxext._GLX_SGI_video_sync = 1;
	if (gglx_check_extension("GLX_SGIS_blended_overlay")) gglxext._GLX_SGIS_blended_overlay = 1;
	if (gglx_check_extension("GLX_SGIS_multisample")) gglxext._GLX_SGIS_multisample = 1;
	if (gglx_check_extension("GLX_SGIS_shared_multisample")) gglxext._GLX_SGIS_shared_multisample = 1;
	if (gglx_check_extension("GLX_SGIX_dmbuffer")) gglxext._GLX_SGIX_dmbuffer = 1;
	if (gglx_check_extension("GLX_SGIX_fbconfig")) gglxext._GLX_SGIX_fbconfig = 1;
	if (gglx_check_extension("GLX_SGIX_hyperpipe")) gglxext._GLX_SGIX_hyperpipe = 1;
	if (gglx_check_extension("GLX_SGIX_pbuffer")) gglxext._GLX_SGIX_pbuffer = 1;
	if (gglx_check_extension("GLX_SGIX_swap_barrier")) gglxext._GLX_SGIX_swap_barrier = 1;
	if (gglx_check_extension("GLX_SGIX_swap_group")) gglxext._GLX_SGIX_swap_group = 1;
	if (gglx_check_extension("GLX_SGIX_video_resize")) gglxext._GLX_SGIX_video_resize = 1;
	if (gglx_check_extension("GLX_SGIX_video_source")) gglxext._GLX_SGIX_video_source = 1;
	if (gglx_check_extension("GLX_SGIX_visual_select_group")) gglxext._GLX_SGIX_visual_select_group = 1;
	if (gglx_check_extension("GLX_SUN_get_transparent_index")) gglxext._GLX_SUN_get_transparent_index = 1;
}

void gglx_rebind(int enableDebug) {
	if (!enableDebug) {
		gglXAssociateDMPbufferSGIX = _glXAssociateDMPbufferSGIX;
		gglXBindChannelToWindowSGIX = _glXBindChannelToWindowSGIX;
		gglXBindHyperpipeSGIX = _glXBindHyperpipeSGIX;
		gglXBindSwapBarrierNV = _glXBindSwapBarrierNV;
		gglXBindSwapBarrierSGIX = _glXBindSwapBarrierSGIX;
		gglXBindTexImageEXT = _glXBindTexImageEXT;
		gglXBindVideoCaptureDeviceNV = _glXBindVideoCaptureDeviceNV;
		gglXBindVideoDeviceNV = _glXBindVideoDeviceNV;
		gglXBindVideoImageNV = _glXBindVideoImageNV;
		gglXBlitContextFramebufferAMD = _glXBlitContextFramebufferAMD;
		gglXChannelRectSGIX = _glXChannelRectSGIX;
		gglXChannelRectSyncSGIX = _glXChannelRectSyncSGIX;
		gglXChooseFBConfig = _glXChooseFBConfig;
		gglXChooseFBConfigSGIX = _glXChooseFBConfigSGIX;
		gglXChooseVisual = _glXChooseVisual;
		gglXCopyBufferSubDataNV = _glXCopyBufferSubDataNV;
		gglXNamedCopyBufferSubDataNV = _glXNamedCopyBufferSubDataNV;
		gglXCopyContext = _glXCopyContext;
		gglXCopyImageSubDataNV = _glXCopyImageSubDataNV;
		gglXCopySubBufferMESA = _glXCopySubBufferMESA;
		gglXCreateAssociatedContextAMD = _glXCreateAssociatedContextAMD;
		gglXCreateAssociatedContextAttribsAMD = _glXCreateAssociatedContextAttribsAMD;
		gglXCreateContextAttribsARB = _glXCreateContextAttribsARB;
		gglXCreateContext = _glXCreateContext;
		gglXCreateContextWithConfigSGIX = _glXCreateContextWithConfigSGIX;
		gglXCreateGLXPbufferSGIX = _glXCreateGLXPbufferSGIX;
		gglXCreateGLXPixmap = _glXCreateGLXPixmap;
		gglXCreateGLXPixmapMESA = _glXCreateGLXPixmapMESA;
		gglXCreateGLXPixmapWithConfigSGIX = _glXCreateGLXPixmapWithConfigSGIX;
		gglXCreateGLXVideoSourceSGIX = _glXCreateGLXVideoSourceSGIX;
		gglXCreateNewContext = _glXCreateNewContext;
		gglXCreatePbuffer = _glXCreatePbuffer;
		gglXCreatePixmap = _glXCreatePixmap;
		gglXCreateWindow = _glXCreateWindow;
		gglXCushionSGI = _glXCushionSGI;
		gglXDelayBeforeSwapNV = _glXDelayBeforeSwapNV;
		gglXDeleteAssociatedContextAMD = _glXDeleteAssociatedContextAMD;
		gglXDestroyContext = _glXDestroyContext;
		gglXDestroyGLXPbufferSGIX = _glXDestroyGLXPbufferSGIX;
		gglXDestroyGLXPixmap = _glXDestroyGLXPixmap;
		gglXDestroyGLXVideoSourceSGIX = _glXDestroyGLXVideoSourceSGIX;
		gglXDestroyHyperpipeConfigSGIX = _glXDestroyHyperpipeConfigSGIX;
		gglXDestroyPbuffer = _glXDestroyPbuffer;
		gglXDestroyPixmap = _glXDestroyPixmap;
		gglXDestroyWindow = _glXDestroyWindow;
		gglXEnumerateVideoCaptureDevicesNV = _glXEnumerateVideoCaptureDevicesNV;
		gglXEnumerateVideoDevicesNV = _glXEnumerateVideoDevicesNV;
		gglXFreeContextEXT = _glXFreeContextEXT;
		gglXGetAGPOffsetMESA = _glXGetAGPOffsetMESA;
		gglXGetClientString = _glXGetClientString;
		gglXGetConfig = _glXGetConfig;
		gglXGetContextGPUIDAMD = _glXGetContextGPUIDAMD;
		gglXGetContextIDEXT = _glXGetContextIDEXT;
		gglXGetCurrentAssociatedContextAMD = _glXGetCurrentAssociatedContextAMD;
		gglXGetCurrentContext = _glXGetCurrentContext;
		gglXGetCurrentDisplayEXT = _glXGetCurrentDisplayEXT;
		gglXGetCurrentDisplay = _glXGetCurrentDisplay;
		gglXGetCurrentDrawable = _glXGetCurrentDrawable;
		gglXGetCurrentReadDrawableSGI = _glXGetCurrentReadDrawableSGI;
		gglXGetCurrentReadDrawable = _glXGetCurrentReadDrawable;
		gglXGetFBConfigAttrib = _glXGetFBConfigAttrib;
		gglXGetFBConfigAttribSGIX = _glXGetFBConfigAttribSGIX;
		gglXGetFBConfigFromVisualSGIX = _glXGetFBConfigFromVisualSGIX;
		gglXGetFBConfigs = _glXGetFBConfigs;
		gglXGetGPUIDsAMD = _glXGetGPUIDsAMD;
		gglXGetGPUInfoAMD = _glXGetGPUInfoAMD;
		gglXGetMscRateOML = _glXGetMscRateOML;
		gglXGetProcAddressARB = _glXGetProcAddressARB;
		gglXGetProcAddress = _glXGetProcAddress;
		gglXGetSelectedEvent = _glXGetSelectedEvent;
		gglXGetSelectedEventSGIX = _glXGetSelectedEventSGIX;
		gglXGetSyncValuesOML = _glXGetSyncValuesOML;
		gglXGetTransparentIndexSUN = _glXGetTransparentIndexSUN;
		gglXGetVideoDeviceNV = _glXGetVideoDeviceNV;
		gglXGetVideoInfoNV = _glXGetVideoInfoNV;
		gglXGetVideoSyncSGI = _glXGetVideoSyncSGI;
		gglXGetVisualFromFBConfig = _glXGetVisualFromFBConfig;
		gglXGetVisualFromFBConfigSGIX = _glXGetVisualFromFBConfigSGIX;
		gglXHyperpipeAttribSGIX = _glXHyperpipeAttribSGIX;
		gglXHyperpipeConfigSGIX = _glXHyperpipeConfigSGIX;
		gglXImportContextEXT = _glXImportContextEXT;
		gglXIsDirect = _glXIsDirect;
		gglXJoinSwapGroupNV = _glXJoinSwapGroupNV;
		gglXJoinSwapGroupSGIX = _glXJoinSwapGroupSGIX;
		gglXLockVideoCaptureDeviceNV = _glXLockVideoCaptureDeviceNV;
		gglXMakeAssociatedContextCurrentAMD = _glXMakeAssociatedContextCurrentAMD;
		gglXMakeContextCurrent = _glXMakeContextCurrent;
		gglXMakeCurrent = _glXMakeCurrent;
		gglXMakeCurrentReadSGI = _glXMakeCurrentReadSGI;
		gglXQueryChannelDeltasSGIX = _glXQueryChannelDeltasSGIX;
		gglXQueryChannelRectSGIX = _glXQueryChannelRectSGIX;
		gglXQueryContext = _glXQueryContext;
		gglXQueryContextInfoEXT = _glXQueryContextInfoEXT;
		gglXQueryCurrentRendererIntegerMESA = _glXQueryCurrentRendererIntegerMESA;
		gglXQueryCurrentRendererStringMESA = _glXQueryCurrentRendererStringMESA;
		gglXQueryDrawable = _glXQueryDrawable;
		gglXQueryExtension = _glXQueryExtension;
		gglXQueryExtensionsString = _glXQueryExtensionsString;
		gglXQueryFrameCountNV = _glXQueryFrameCountNV;
		gglXQueryGLXPbufferSGIX = _glXQueryGLXPbufferSGIX;
		gglXQueryHyperpipeAttribSGIX = _glXQueryHyperpipeAttribSGIX;
		gglXQueryHyperpipeBestAttribSGIX = _glXQueryHyperpipeBestAttribSGIX;
		gglXQueryHyperpipeConfigSGIX = _glXQueryHyperpipeConfigSGIX;
		gglXQueryHyperpipeNetworkSGIX = _glXQueryHyperpipeNetworkSGIX;
		gglXQueryMaxSwapBarriersSGIX = _glXQueryMaxSwapBarriersSGIX;
		gglXQueryMaxSwapGroupsNV = _glXQueryMaxSwapGroupsNV;
		gglXQueryRendererIntegerMESA = _glXQueryRendererIntegerMESA;
		gglXQueryRendererStringMESA = _glXQueryRendererStringMESA;
		gglXQueryServerString = _glXQueryServerString;
		gglXQuerySwapGroupNV = _glXQuerySwapGroupNV;
		gglXQueryVersion = _glXQueryVersion;
		gglXQueryVideoCaptureDeviceNV = _glXQueryVideoCaptureDeviceNV;
		gglXReleaseBuffersMESA = _glXReleaseBuffersMESA;
		gglXReleaseTexImageEXT = _glXReleaseTexImageEXT;
		gglXReleaseVideoCaptureDeviceNV = _glXReleaseVideoCaptureDeviceNV;
		gglXReleaseVideoDeviceNV = _glXReleaseVideoDeviceNV;
		gglXReleaseVideoImageNV = _glXReleaseVideoImageNV;
		gglXResetFrameCountNV = _glXResetFrameCountNV;
		gglXSelectEvent = _glXSelectEvent;
		gglXSelectEventSGIX = _glXSelectEventSGIX;
		gglXSendPbufferToVideoNV = _glXSendPbufferToVideoNV;
		gglXSet3DfxModeMESA = _glXSet3DfxModeMESA;
		gglXSwapBuffers = _glXSwapBuffers;
		gglXSwapBuffersMscOML = _glXSwapBuffersMscOML;
		gglXSwapIntervalEXT = _glXSwapIntervalEXT;
		gglXSwapIntervalSGI = _glXSwapIntervalSGI;
		gglXUseXFont = _glXUseXFont;
		gglXWaitForMscOML = _glXWaitForMscOML;
		gglXWaitForSbcOML = _glXWaitForSbcOML;
		gglXWaitGL = _glXWaitGL;
		gglXWaitVideoSyncSGI = _glXWaitVideoSyncSGI;
		gglXWaitX = _glXWaitX;
	} else {
		gglXAssociateDMPbufferSGIX = d_glXAssociateDMPbufferSGIX;
		gglXBindChannelToWindowSGIX = d_glXBindChannelToWindowSGIX;
		gglXBindHyperpipeSGIX = d_glXBindHyperpipeSGIX;
		gglXBindSwapBarrierNV = d_glXBindSwapBarrierNV;
		gglXBindSwapBarrierSGIX = d_glXBindSwapBarrierSGIX;
		gglXBindTexImageEXT = d_glXBindTexImageEXT;
		gglXBindVideoCaptureDeviceNV = d_glXBindVideoCaptureDeviceNV;
		gglXBindVideoDeviceNV = d_glXBindVideoDeviceNV;
		gglXBindVideoImageNV = d_glXBindVideoImageNV;
		gglXBlitContextFramebufferAMD = d_glXBlitContextFramebufferAMD;
		gglXChannelRectSGIX = d_glXChannelRectSGIX;
		gglXChannelRectSyncSGIX = d_glXChannelRectSyncSGIX;
		gglXChooseFBConfig = d_glXChooseFBConfig;
		gglXChooseFBConfigSGIX = d_glXChooseFBConfigSGIX;
		gglXChooseVisual = d_glXChooseVisual;
		gglXCopyBufferSubDataNV = d_glXCopyBufferSubDataNV;
		gglXNamedCopyBufferSubDataNV = d_glXNamedCopyBufferSubDataNV;
		gglXCopyContext = d_glXCopyContext;
		gglXCopyImageSubDataNV = d_glXCopyImageSubDataNV;
		gglXCopySubBufferMESA = d_glXCopySubBufferMESA;
		gglXCreateAssociatedContextAMD = d_glXCreateAssociatedContextAMD;
		gglXCreateAssociatedContextAttribsAMD = d_glXCreateAssociatedContextAttribsAMD;
		gglXCreateContextAttribsARB = d_glXCreateContextAttribsARB;
		gglXCreateContext = d_glXCreateContext;
		gglXCreateContextWithConfigSGIX = d_glXCreateContextWithConfigSGIX;
		gglXCreateGLXPbufferSGIX = d_glXCreateGLXPbufferSGIX;
		gglXCreateGLXPixmap = d_glXCreateGLXPixmap;
		gglXCreateGLXPixmapMESA = d_glXCreateGLXPixmapMESA;
		gglXCreateGLXPixmapWithConfigSGIX = d_glXCreateGLXPixmapWithConfigSGIX;
		gglXCreateGLXVideoSourceSGIX = d_glXCreateGLXVideoSourceSGIX;
		gglXCreateNewContext = d_glXCreateNewContext;
		gglXCreatePbuffer = d_glXCreatePbuffer;
		gglXCreatePixmap = d_glXCreatePixmap;
		gglXCreateWindow = d_glXCreateWindow;
		gglXCushionSGI = d_glXCushionSGI;
		gglXDelayBeforeSwapNV = d_glXDelayBeforeSwapNV;
		gglXDeleteAssociatedContextAMD = d_glXDeleteAssociatedContextAMD;
		gglXDestroyContext = d_glXDestroyContext;
		gglXDestroyGLXPbufferSGIX = d_glXDestroyGLXPbufferSGIX;
		gglXDestroyGLXPixmap = d_glXDestroyGLXPixmap;
		gglXDestroyGLXVideoSourceSGIX = d_glXDestroyGLXVideoSourceSGIX;
		gglXDestroyHyperpipeConfigSGIX = d_glXDestroyHyperpipeConfigSGIX;
		gglXDestroyPbuffer = d_glXDestroyPbuffer;
		gglXDestroyPixmap = d_glXDestroyPixmap;
		gglXDestroyWindow = d_glXDestroyWindow;
		gglXEnumerateVideoCaptureDevicesNV = d_glXEnumerateVideoCaptureDevicesNV;
		gglXEnumerateVideoDevicesNV = d_glXEnumerateVideoDevicesNV;
		gglXFreeContextEXT = d_glXFreeContextEXT;
		gglXGetAGPOffsetMESA = d_glXGetAGPOffsetMESA;
		gglXGetClientString = d_glXGetClientString;
		gglXGetConfig = d_glXGetConfig;
		gglXGetContextGPUIDAMD = d_glXGetContextGPUIDAMD;
		gglXGetContextIDEXT = d_glXGetContextIDEXT;
		gglXGetCurrentAssociatedContextAMD = d_glXGetCurrentAssociatedContextAMD;
		gglXGetCurrentContext = d_glXGetCurrentContext;
		gglXGetCurrentDisplayEXT = d_glXGetCurrentDisplayEXT;
		gglXGetCurrentDisplay = d_glXGetCurrentDisplay;
		gglXGetCurrentDrawable = d_glXGetCurrentDrawable;
		gglXGetCurrentReadDrawableSGI = d_glXGetCurrentReadDrawableSGI;
		gglXGetCurrentReadDrawable = d_glXGetCurrentReadDrawable;
		gglXGetFBConfigAttrib = d_glXGetFBConfigAttrib;
		gglXGetFBConfigAttribSGIX = d_glXGetFBConfigAttribSGIX;
		gglXGetFBConfigFromVisualSGIX = d_glXGetFBConfigFromVisualSGIX;
		gglXGetFBConfigs = d_glXGetFBConfigs;
		gglXGetGPUIDsAMD = d_glXGetGPUIDsAMD;
		gglXGetGPUInfoAMD = d_glXGetGPUInfoAMD;
		gglXGetMscRateOML = d_glXGetMscRateOML;
		gglXGetProcAddressARB = d_glXGetProcAddressARB;
		gglXGetProcAddress = d_glXGetProcAddress;
		gglXGetSelectedEvent = d_glXGetSelectedEvent;
		gglXGetSelectedEventSGIX = d_glXGetSelectedEventSGIX;
		gglXGetSyncValuesOML = d_glXGetSyncValuesOML;
		gglXGetTransparentIndexSUN = d_glXGetTransparentIndexSUN;
		gglXGetVideoDeviceNV = d_glXGetVideoDeviceNV;
		gglXGetVideoInfoNV = d_glXGetVideoInfoNV;
		gglXGetVideoSyncSGI = d_glXGetVideoSyncSGI;
		gglXGetVisualFromFBConfig = d_glXGetVisualFromFBConfig;
		gglXGetVisualFromFBConfigSGIX = d_glXGetVisualFromFBConfigSGIX;
		gglXHyperpipeAttribSGIX = d_glXHyperpipeAttribSGIX;
		gglXHyperpipeConfigSGIX = d_glXHyperpipeConfigSGIX;
		gglXImportContextEXT = d_glXImportContextEXT;
		gglXIsDirect = d_glXIsDirect;
		gglXJoinSwapGroupNV = d_glXJoinSwapGroupNV;
		gglXJoinSwapGroupSGIX = d_glXJoinSwapGroupSGIX;
		gglXLockVideoCaptureDeviceNV = d_glXLockVideoCaptureDeviceNV;
		gglXMakeAssociatedContextCurrentAMD = d_glXMakeAssociatedContextCurrentAMD;
		gglXMakeContextCurrent = d_glXMakeContextCurrent;
		gglXMakeCurrent = d_glXMakeCurrent;
		gglXMakeCurrentReadSGI = d_glXMakeCurrentReadSGI;
		gglXQueryChannelDeltasSGIX = d_glXQueryChannelDeltasSGIX;
		gglXQueryChannelRectSGIX = d_glXQueryChannelRectSGIX;
		gglXQueryContext = d_glXQueryContext;
		gglXQueryContextInfoEXT = d_glXQueryContextInfoEXT;
		gglXQueryCurrentRendererIntegerMESA = d_glXQueryCurrentRendererIntegerMESA;
		gglXQueryCurrentRendererStringMESA = d_glXQueryCurrentRendererStringMESA;
		gglXQueryDrawable = d_glXQueryDrawable;
		gglXQueryExtension = d_glXQueryExtension;
		gglXQueryExtensionsString = d_glXQueryExtensionsString;
		gglXQueryFrameCountNV = d_glXQueryFrameCountNV;
		gglXQueryGLXPbufferSGIX = d_glXQueryGLXPbufferSGIX;
		gglXQueryHyperpipeAttribSGIX = d_glXQueryHyperpipeAttribSGIX;
		gglXQueryHyperpipeBestAttribSGIX = d_glXQueryHyperpipeBestAttribSGIX;
		gglXQueryHyperpipeConfigSGIX = d_glXQueryHyperpipeConfigSGIX;
		gglXQueryHyperpipeNetworkSGIX = d_glXQueryHyperpipeNetworkSGIX;
		gglXQueryMaxSwapBarriersSGIX = d_glXQueryMaxSwapBarriersSGIX;
		gglXQueryMaxSwapGroupsNV = d_glXQueryMaxSwapGroupsNV;
		gglXQueryRendererIntegerMESA = d_glXQueryRendererIntegerMESA;
		gglXQueryRendererStringMESA = d_glXQueryRendererStringMESA;
		gglXQueryServerString = d_glXQueryServerString;
		gglXQuerySwapGroupNV = d_glXQuerySwapGroupNV;
		gglXQueryVersion = d_glXQueryVersion;
		gglXQueryVideoCaptureDeviceNV = d_glXQueryVideoCaptureDeviceNV;
		gglXReleaseBuffersMESA = d_glXReleaseBuffersMESA;
		gglXReleaseTexImageEXT = d_glXReleaseTexImageEXT;
		gglXReleaseVideoCaptureDeviceNV = d_glXReleaseVideoCaptureDeviceNV;
		gglXReleaseVideoDeviceNV = d_glXReleaseVideoDeviceNV;
		gglXReleaseVideoImageNV = d_glXReleaseVideoImageNV;
		gglXResetFrameCountNV = d_glXResetFrameCountNV;
		gglXSelectEvent = d_glXSelectEvent;
		gglXSelectEventSGIX = d_glXSelectEventSGIX;
		gglXSendPbufferToVideoNV = d_glXSendPbufferToVideoNV;
		gglXSet3DfxModeMESA = d_glXSet3DfxModeMESA;
		gglXSwapBuffers = d_glXSwapBuffers;
		gglXSwapBuffersMscOML = d_glXSwapBuffersMscOML;
		gglXSwapIntervalEXT = d_glXSwapIntervalEXT;
		gglXSwapIntervalSGI = d_glXSwapIntervalSGI;
		gglXUseXFont = d_glXUseXFont;
		gglXWaitForMscOML = d_glXWaitForMscOML;
		gglXWaitForSbcOML = d_glXWaitForSbcOML;
		gglXWaitGL = d_glXWaitGL;
		gglXWaitVideoSyncSGI = d_glXWaitVideoSyncSGI;
		gglXWaitX = d_glXWaitX;
	}
}
