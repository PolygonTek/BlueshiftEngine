/*********************************************************************************************
 *
 * gegl.c
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

#include "gegl.h"
#include <string.h>

extern void CheckGLError(const char *msg);

typedef EGLBoolean (APIENTRYP PFNEGLBINDAPI)(EGLenum api);
PFNEGLBINDAPI geglBindAPI;
static PFNEGLBINDAPI _eglBindAPI;
static EGLBoolean APIENTRY d_eglBindAPI(EGLenum api) {
	EGLBoolean ret = _eglBindAPI(api);
	CheckGLError("eglBindAPI");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLBINDTEXIMAGE)(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
PFNEGLBINDTEXIMAGE geglBindTexImage;
static PFNEGLBINDTEXIMAGE _eglBindTexImage;
static EGLBoolean APIENTRY d_eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer) {
	EGLBoolean ret = _eglBindTexImage(dpy, surface, buffer);
	CheckGLError("eglBindTexImage");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLCHOOSECONFIG)(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
PFNEGLCHOOSECONFIG geglChooseConfig;
static PFNEGLCHOOSECONFIG _eglChooseConfig;
static EGLBoolean APIENTRY d_eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
	EGLBoolean ret = _eglChooseConfig(dpy, attrib_list, configs, config_size, num_config);
	CheckGLError("eglChooseConfig");
	return ret;
}
typedef EGLint (APIENTRYP PFNEGLCLIENTWAITSYNC)(EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout);
PFNEGLCLIENTWAITSYNC geglClientWaitSync;
static PFNEGLCLIENTWAITSYNC _eglClientWaitSync;
static EGLint APIENTRY d_eglClientWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout) {
	EGLint ret = _eglClientWaitSync(dpy, sync, flags, timeout);
	CheckGLError("eglClientWaitSync");
	return ret;
}
typedef EGLint (APIENTRYP PFNEGLCLIENTWAITSYNCKHR)(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout);
PFNEGLCLIENTWAITSYNCKHR geglClientWaitSyncKHR;
static PFNEGLCLIENTWAITSYNCKHR _eglClientWaitSyncKHR;
static EGLint APIENTRY d_eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout) {
	EGLint ret = _eglClientWaitSyncKHR(dpy, sync, flags, timeout);
	CheckGLError("eglClientWaitSyncKHR");
	return ret;
}
typedef EGLint (APIENTRYP PFNEGLCLIENTWAITSYNCNV)(EGLSyncNV sync, EGLint flags, EGLTimeNV timeout);
PFNEGLCLIENTWAITSYNCNV geglClientWaitSyncNV;
static PFNEGLCLIENTWAITSYNCNV _eglClientWaitSyncNV;
static EGLint APIENTRY d_eglClientWaitSyncNV(EGLSyncNV sync, EGLint flags, EGLTimeNV timeout) {
	EGLint ret = _eglClientWaitSyncNV(sync, flags, timeout);
	CheckGLError("eglClientWaitSyncNV");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLCOPYBUFFERS)(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target);
PFNEGLCOPYBUFFERS geglCopyBuffers;
static PFNEGLCOPYBUFFERS _eglCopyBuffers;
static EGLBoolean APIENTRY d_eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target) {
	EGLBoolean ret = _eglCopyBuffers(dpy, surface, target);
	CheckGLError("eglCopyBuffers");
	return ret;
}
typedef EGLContext (APIENTRYP PFNEGLCREATECONTEXT)(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
PFNEGLCREATECONTEXT geglCreateContext;
static PFNEGLCREATECONTEXT _eglCreateContext;
static EGLContext APIENTRY d_eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list) {
	EGLContext ret = _eglCreateContext(dpy, config, share_context, attrib_list);
	CheckGLError("eglCreateContext");
	return ret;
}
typedef EGLImageKHR (APIENTRYP PFNEGLCREATEDRMIMAGEMESA)(EGLDisplay dpy, const EGLint *attrib_list);
PFNEGLCREATEDRMIMAGEMESA geglCreateDRMImageMESA;
static PFNEGLCREATEDRMIMAGEMESA _eglCreateDRMImageMESA;
static EGLImageKHR APIENTRY d_eglCreateDRMImageMESA(EGLDisplay dpy, const EGLint *attrib_list) {
	EGLImageKHR ret = _eglCreateDRMImageMESA(dpy, attrib_list);
	CheckGLError("eglCreateDRMImageMESA");
	return ret;
}
typedef EGLSyncNV (APIENTRYP PFNEGLCREATEFENCESYNCNV)(EGLDisplay dpy, EGLenum condition, const EGLint *attrib_list);
PFNEGLCREATEFENCESYNCNV geglCreateFenceSyncNV;
static PFNEGLCREATEFENCESYNCNV _eglCreateFenceSyncNV;
static EGLSyncNV APIENTRY d_eglCreateFenceSyncNV(EGLDisplay dpy, EGLenum condition, const EGLint *attrib_list) {
	EGLSyncNV ret = _eglCreateFenceSyncNV(dpy, condition, attrib_list);
	CheckGLError("eglCreateFenceSyncNV");
	return ret;
}
typedef EGLImage (APIENTRYP PFNEGLCREATEIMAGE)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLAttrib *attrib_list);
PFNEGLCREATEIMAGE geglCreateImage;
static PFNEGLCREATEIMAGE _eglCreateImage;
static EGLImage APIENTRY d_eglCreateImage(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLAttrib *attrib_list) {
	EGLImage ret = _eglCreateImage(dpy, ctx, target, buffer, attrib_list);
	CheckGLError("eglCreateImage");
	return ret;
}
typedef EGLImageKHR (APIENTRYP PFNEGLCREATEIMAGEKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
PFNEGLCREATEIMAGEKHR geglCreateImageKHR;
static PFNEGLCREATEIMAGEKHR _eglCreateImageKHR;
static EGLImageKHR APIENTRY d_eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list) {
	EGLImageKHR ret = _eglCreateImageKHR(dpy, ctx, target, buffer, attrib_list);
	CheckGLError("eglCreateImageKHR");
	return ret;
}
typedef EGLClientBuffer (APIENTRYP PFNEGLCREATENATIVECLIENTBUFFERANDROID)(const EGLint *attrib_list);
PFNEGLCREATENATIVECLIENTBUFFERANDROID geglCreateNativeClientBufferANDROID;
static PFNEGLCREATENATIVECLIENTBUFFERANDROID _eglCreateNativeClientBufferANDROID;
static EGLClientBuffer APIENTRY d_eglCreateNativeClientBufferANDROID(const EGLint *attrib_list) {
	EGLClientBuffer ret = _eglCreateNativeClientBufferANDROID(attrib_list);
	CheckGLError("eglCreateNativeClientBufferANDROID");
	return ret;
}
typedef EGLSurface (APIENTRYP PFNEGLCREATEPBUFFERFROMCLIENTBUFFER)(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list);
PFNEGLCREATEPBUFFERFROMCLIENTBUFFER geglCreatePbufferFromClientBuffer;
static PFNEGLCREATEPBUFFERFROMCLIENTBUFFER _eglCreatePbufferFromClientBuffer;
static EGLSurface APIENTRY d_eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list) {
	EGLSurface ret = _eglCreatePbufferFromClientBuffer(dpy, buftype, buffer, config, attrib_list);
	CheckGLError("eglCreatePbufferFromClientBuffer");
	return ret;
}
typedef EGLSurface (APIENTRYP PFNEGLCREATEPBUFFERSURFACE)(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
PFNEGLCREATEPBUFFERSURFACE geglCreatePbufferSurface;
static PFNEGLCREATEPBUFFERSURFACE _eglCreatePbufferSurface;
static EGLSurface APIENTRY d_eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list) {
	EGLSurface ret = _eglCreatePbufferSurface(dpy, config, attrib_list);
	CheckGLError("eglCreatePbufferSurface");
	return ret;
}
typedef EGLSurface (APIENTRYP PFNEGLCREATEPIXMAPSURFACE)(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list);
PFNEGLCREATEPIXMAPSURFACE geglCreatePixmapSurface;
static PFNEGLCREATEPIXMAPSURFACE _eglCreatePixmapSurface;
static EGLSurface APIENTRY d_eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list) {
	EGLSurface ret = _eglCreatePixmapSurface(dpy, config, pixmap, attrib_list);
	CheckGLError("eglCreatePixmapSurface");
	return ret;
}
typedef EGLSurface (APIENTRYP PFNEGLCREATEPIXMAPSURFACEHI)(EGLDisplay dpy, EGLConfig config, struct EGLClientPixmapHI *pixmap);
PFNEGLCREATEPIXMAPSURFACEHI geglCreatePixmapSurfaceHI;
static PFNEGLCREATEPIXMAPSURFACEHI _eglCreatePixmapSurfaceHI;
static EGLSurface APIENTRY d_eglCreatePixmapSurfaceHI(EGLDisplay dpy, EGLConfig config, struct EGLClientPixmapHI *pixmap) {
	EGLSurface ret = _eglCreatePixmapSurfaceHI(dpy, config, pixmap);
	CheckGLError("eglCreatePixmapSurfaceHI");
	return ret;
}
typedef EGLSurface (APIENTRYP PFNEGLCREATEPLATFORMPIXMAPSURFACE)(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list);
PFNEGLCREATEPLATFORMPIXMAPSURFACE geglCreatePlatformPixmapSurface;
static PFNEGLCREATEPLATFORMPIXMAPSURFACE _eglCreatePlatformPixmapSurface;
static EGLSurface APIENTRY d_eglCreatePlatformPixmapSurface(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list) {
	EGLSurface ret = _eglCreatePlatformPixmapSurface(dpy, config, native_pixmap, attrib_list);
	CheckGLError("eglCreatePlatformPixmapSurface");
	return ret;
}
typedef EGLSurface (APIENTRYP PFNEGLCREATEPLATFORMPIXMAPSURFACEEXT)(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list);
PFNEGLCREATEPLATFORMPIXMAPSURFACEEXT geglCreatePlatformPixmapSurfaceEXT;
static PFNEGLCREATEPLATFORMPIXMAPSURFACEEXT _eglCreatePlatformPixmapSurfaceEXT;
static EGLSurface APIENTRY d_eglCreatePlatformPixmapSurfaceEXT(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLint *attrib_list) {
	EGLSurface ret = _eglCreatePlatformPixmapSurfaceEXT(dpy, config, native_pixmap, attrib_list);
	CheckGLError("eglCreatePlatformPixmapSurfaceEXT");
	return ret;
}
typedef EGLSurface (APIENTRYP PFNEGLCREATEPLATFORMWINDOWSURFACE)(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLAttrib *attrib_list);
PFNEGLCREATEPLATFORMWINDOWSURFACE geglCreatePlatformWindowSurface;
static PFNEGLCREATEPLATFORMWINDOWSURFACE _eglCreatePlatformWindowSurface;
static EGLSurface APIENTRY d_eglCreatePlatformWindowSurface(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLAttrib *attrib_list) {
	EGLSurface ret = _eglCreatePlatformWindowSurface(dpy, config, native_window, attrib_list);
	CheckGLError("eglCreatePlatformWindowSurface");
	return ret;
}
typedef EGLSurface (APIENTRYP PFNEGLCREATEPLATFORMWINDOWSURFACEEXT)(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list);
PFNEGLCREATEPLATFORMWINDOWSURFACEEXT geglCreatePlatformWindowSurfaceEXT;
static PFNEGLCREATEPLATFORMWINDOWSURFACEEXT _eglCreatePlatformWindowSurfaceEXT;
static EGLSurface APIENTRY d_eglCreatePlatformWindowSurfaceEXT(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLint *attrib_list) {
	EGLSurface ret = _eglCreatePlatformWindowSurfaceEXT(dpy, config, native_window, attrib_list);
	CheckGLError("eglCreatePlatformWindowSurfaceEXT");
	return ret;
}
typedef EGLStreamKHR (APIENTRYP PFNEGLCREATESTREAMFROMFILEDESCRIPTORKHR)(EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor);
PFNEGLCREATESTREAMFROMFILEDESCRIPTORKHR geglCreateStreamFromFileDescriptorKHR;
static PFNEGLCREATESTREAMFROMFILEDESCRIPTORKHR _eglCreateStreamFromFileDescriptorKHR;
static EGLStreamKHR APIENTRY d_eglCreateStreamFromFileDescriptorKHR(EGLDisplay dpy, EGLNativeFileDescriptorKHR file_descriptor) {
	EGLStreamKHR ret = _eglCreateStreamFromFileDescriptorKHR(dpy, file_descriptor);
	CheckGLError("eglCreateStreamFromFileDescriptorKHR");
	return ret;
}
typedef EGLStreamKHR (APIENTRYP PFNEGLCREATESTREAMKHR)(EGLDisplay dpy, const EGLint *attrib_list);
PFNEGLCREATESTREAMKHR geglCreateStreamKHR;
static PFNEGLCREATESTREAMKHR _eglCreateStreamKHR;
static EGLStreamKHR APIENTRY d_eglCreateStreamKHR(EGLDisplay dpy, const EGLint *attrib_list) {
	EGLStreamKHR ret = _eglCreateStreamKHR(dpy, attrib_list);
	CheckGLError("eglCreateStreamKHR");
	return ret;
}
typedef EGLStreamKHR (APIENTRYP PFNEGLCREATESTREAMATTRIBKHR)(EGLDisplay dpy, const EGLAttrib *attrib_list);
PFNEGLCREATESTREAMATTRIBKHR geglCreateStreamAttribKHR;
static PFNEGLCREATESTREAMATTRIBKHR _eglCreateStreamAttribKHR;
static EGLStreamKHR APIENTRY d_eglCreateStreamAttribKHR(EGLDisplay dpy, const EGLAttrib *attrib_list) {
	EGLStreamKHR ret = _eglCreateStreamAttribKHR(dpy, attrib_list);
	CheckGLError("eglCreateStreamAttribKHR");
	return ret;
}
typedef EGLSurface (APIENTRYP PFNEGLCREATESTREAMPRODUCERSURFACEKHR)(EGLDisplay dpy, EGLConfig config, EGLStreamKHR stream, const EGLint *attrib_list);
PFNEGLCREATESTREAMPRODUCERSURFACEKHR geglCreateStreamProducerSurfaceKHR;
static PFNEGLCREATESTREAMPRODUCERSURFACEKHR _eglCreateStreamProducerSurfaceKHR;
static EGLSurface APIENTRY d_eglCreateStreamProducerSurfaceKHR(EGLDisplay dpy, EGLConfig config, EGLStreamKHR stream, const EGLint *attrib_list) {
	EGLSurface ret = _eglCreateStreamProducerSurfaceKHR(dpy, config, stream, attrib_list);
	CheckGLError("eglCreateStreamProducerSurfaceKHR");
	return ret;
}
typedef EGLSyncKHR (APIENTRYP PFNEGLCREATESTREAMSYNCNV)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum type, const EGLint *attrib_list);
PFNEGLCREATESTREAMSYNCNV geglCreateStreamSyncNV;
static PFNEGLCREATESTREAMSYNCNV _eglCreateStreamSyncNV;
static EGLSyncKHR APIENTRY d_eglCreateStreamSyncNV(EGLDisplay dpy, EGLStreamKHR stream, EGLenum type, const EGLint *attrib_list) {
	EGLSyncKHR ret = _eglCreateStreamSyncNV(dpy, stream, type, attrib_list);
	CheckGLError("eglCreateStreamSyncNV");
	return ret;
}
typedef EGLSync (APIENTRYP PFNEGLCREATESYNC)(EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list);
PFNEGLCREATESYNC geglCreateSync;
static PFNEGLCREATESYNC _eglCreateSync;
static EGLSync APIENTRY d_eglCreateSync(EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list) {
	EGLSync ret = _eglCreateSync(dpy, type, attrib_list);
	CheckGLError("eglCreateSync");
	return ret;
}
typedef EGLSyncKHR (APIENTRYP PFNEGLCREATESYNCKHR)(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list);
PFNEGLCREATESYNCKHR geglCreateSyncKHR;
static PFNEGLCREATESYNCKHR _eglCreateSyncKHR;
static EGLSyncKHR APIENTRY d_eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list) {
	EGLSyncKHR ret = _eglCreateSyncKHR(dpy, type, attrib_list);
	CheckGLError("eglCreateSyncKHR");
	return ret;
}
typedef EGLSyncKHR (APIENTRYP PFNEGLCREATESYNC64KHR)(EGLDisplay dpy, EGLenum type, const EGLAttribKHR *attrib_list);
PFNEGLCREATESYNC64KHR geglCreateSync64KHR;
static PFNEGLCREATESYNC64KHR _eglCreateSync64KHR;
static EGLSyncKHR APIENTRY d_eglCreateSync64KHR(EGLDisplay dpy, EGLenum type, const EGLAttribKHR *attrib_list) {
	EGLSyncKHR ret = _eglCreateSync64KHR(dpy, type, attrib_list);
	CheckGLError("eglCreateSync64KHR");
	return ret;
}
typedef EGLSurface (APIENTRYP PFNEGLCREATEWINDOWSURFACE)(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list);
PFNEGLCREATEWINDOWSURFACE geglCreateWindowSurface;
static PFNEGLCREATEWINDOWSURFACE _eglCreateWindowSurface;
static EGLSurface APIENTRY d_eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list) {
	EGLSurface ret = _eglCreateWindowSurface(dpy, config, win, attrib_list);
	CheckGLError("eglCreateWindowSurface");
	return ret;
}
typedef EGLint (APIENTRYP PFNEGLDEBUGMESSAGECONTROLKHR)(EGLDEBUGPROCKHR callback, const EGLAttrib *attrib_list);
PFNEGLDEBUGMESSAGECONTROLKHR geglDebugMessageControlKHR;
static PFNEGLDEBUGMESSAGECONTROLKHR _eglDebugMessageControlKHR;
static EGLint APIENTRY d_eglDebugMessageControlKHR(EGLDEBUGPROCKHR callback, const EGLAttrib *attrib_list) {
	EGLint ret = _eglDebugMessageControlKHR(callback, attrib_list);
	CheckGLError("eglDebugMessageControlKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLDESTROYCONTEXT)(EGLDisplay dpy, EGLContext ctx);
PFNEGLDESTROYCONTEXT geglDestroyContext;
static PFNEGLDESTROYCONTEXT _eglDestroyContext;
static EGLBoolean APIENTRY d_eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
	EGLBoolean ret = _eglDestroyContext(dpy, ctx);
	CheckGLError("eglDestroyContext");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLDESTROYIMAGE)(EGLDisplay dpy, EGLImage image);
PFNEGLDESTROYIMAGE geglDestroyImage;
static PFNEGLDESTROYIMAGE _eglDestroyImage;
static EGLBoolean APIENTRY d_eglDestroyImage(EGLDisplay dpy, EGLImage image) {
	EGLBoolean ret = _eglDestroyImage(dpy, image);
	CheckGLError("eglDestroyImage");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLDESTROYIMAGEKHR)(EGLDisplay dpy, EGLImageKHR image);
PFNEGLDESTROYIMAGEKHR geglDestroyImageKHR;
static PFNEGLDESTROYIMAGEKHR _eglDestroyImageKHR;
static EGLBoolean APIENTRY d_eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image) {
	EGLBoolean ret = _eglDestroyImageKHR(dpy, image);
	CheckGLError("eglDestroyImageKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLDESTROYSTREAMKHR)(EGLDisplay dpy, EGLStreamKHR stream);
PFNEGLDESTROYSTREAMKHR geglDestroyStreamKHR;
static PFNEGLDESTROYSTREAMKHR _eglDestroyStreamKHR;
static EGLBoolean APIENTRY d_eglDestroyStreamKHR(EGLDisplay dpy, EGLStreamKHR stream) {
	EGLBoolean ret = _eglDestroyStreamKHR(dpy, stream);
	CheckGLError("eglDestroyStreamKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLDESTROYSURFACE)(EGLDisplay dpy, EGLSurface surface);
PFNEGLDESTROYSURFACE geglDestroySurface;
static PFNEGLDESTROYSURFACE _eglDestroySurface;
static EGLBoolean APIENTRY d_eglDestroySurface(EGLDisplay dpy, EGLSurface surface) {
	EGLBoolean ret = _eglDestroySurface(dpy, surface);
	CheckGLError("eglDestroySurface");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLDESTROYSYNC)(EGLDisplay dpy, EGLSync sync);
PFNEGLDESTROYSYNC geglDestroySync;
static PFNEGLDESTROYSYNC _eglDestroySync;
static EGLBoolean APIENTRY d_eglDestroySync(EGLDisplay dpy, EGLSync sync) {
	EGLBoolean ret = _eglDestroySync(dpy, sync);
	CheckGLError("eglDestroySync");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLDESTROYSYNCKHR)(EGLDisplay dpy, EGLSyncKHR sync);
PFNEGLDESTROYSYNCKHR geglDestroySyncKHR;
static PFNEGLDESTROYSYNCKHR _eglDestroySyncKHR;
static EGLBoolean APIENTRY d_eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync) {
	EGLBoolean ret = _eglDestroySyncKHR(dpy, sync);
	CheckGLError("eglDestroySyncKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLDESTROYSYNCNV)(EGLSyncNV sync);
PFNEGLDESTROYSYNCNV geglDestroySyncNV;
static PFNEGLDESTROYSYNCNV _eglDestroySyncNV;
static EGLBoolean APIENTRY d_eglDestroySyncNV(EGLSyncNV sync) {
	EGLBoolean ret = _eglDestroySyncNV(sync);
	CheckGLError("eglDestroySyncNV");
	return ret;
}
typedef EGLint (APIENTRYP PFNEGLDUPNATIVEFENCEFDANDROID)(EGLDisplay dpy, EGLSyncKHR sync);
PFNEGLDUPNATIVEFENCEFDANDROID geglDupNativeFenceFDANDROID;
static PFNEGLDUPNATIVEFENCEFDANDROID _eglDupNativeFenceFDANDROID;
static EGLint APIENTRY d_eglDupNativeFenceFDANDROID(EGLDisplay dpy, EGLSyncKHR sync) {
	EGLint ret = _eglDupNativeFenceFDANDROID(dpy, sync);
	CheckGLError("eglDupNativeFenceFDANDROID");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLEXPORTDMABUFIMAGEMESA)(EGLDisplay dpy, EGLImageKHR image, int *fds, EGLint *strides, EGLint *offsets);
PFNEGLEXPORTDMABUFIMAGEMESA geglExportDMABUFImageMESA;
static PFNEGLEXPORTDMABUFIMAGEMESA _eglExportDMABUFImageMESA;
static EGLBoolean APIENTRY d_eglExportDMABUFImageMESA(EGLDisplay dpy, EGLImageKHR image, int *fds, EGLint *strides, EGLint *offsets) {
	EGLBoolean ret = _eglExportDMABUFImageMESA(dpy, image, fds, strides, offsets);
	CheckGLError("eglExportDMABUFImageMESA");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLEXPORTDMABUFIMAGEQUERYMESA)(EGLDisplay dpy, EGLImageKHR image, int *fourcc, int *num_planes, EGLuint64KHR *modifiers);
PFNEGLEXPORTDMABUFIMAGEQUERYMESA geglExportDMABUFImageQueryMESA;
static PFNEGLEXPORTDMABUFIMAGEQUERYMESA _eglExportDMABUFImageQueryMESA;
static EGLBoolean APIENTRY d_eglExportDMABUFImageQueryMESA(EGLDisplay dpy, EGLImageKHR image, int *fourcc, int *num_planes, EGLuint64KHR *modifiers) {
	EGLBoolean ret = _eglExportDMABUFImageQueryMESA(dpy, image, fourcc, num_planes, modifiers);
	CheckGLError("eglExportDMABUFImageQueryMESA");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLEXPORTDRMIMAGEMESA)(EGLDisplay dpy, EGLImageKHR image, EGLint *name, EGLint *handle, EGLint *stride);
PFNEGLEXPORTDRMIMAGEMESA geglExportDRMImageMESA;
static PFNEGLEXPORTDRMIMAGEMESA _eglExportDRMImageMESA;
static EGLBoolean APIENTRY d_eglExportDRMImageMESA(EGLDisplay dpy, EGLImageKHR image, EGLint *name, EGLint *handle, EGLint *stride) {
	EGLBoolean ret = _eglExportDRMImageMESA(dpy, image, name, handle, stride);
	CheckGLError("eglExportDRMImageMESA");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLFENCENV)(EGLSyncNV sync);
PFNEGLFENCENV geglFenceNV;
static PFNEGLFENCENV _eglFenceNV;
static EGLBoolean APIENTRY d_eglFenceNV(EGLSyncNV sync) {
	EGLBoolean ret = _eglFenceNV(sync);
	CheckGLError("eglFenceNV");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLGETCONFIGATTRIB)(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
PFNEGLGETCONFIGATTRIB geglGetConfigAttrib;
static PFNEGLGETCONFIGATTRIB _eglGetConfigAttrib;
static EGLBoolean APIENTRY d_eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value) {
	EGLBoolean ret = _eglGetConfigAttrib(dpy, config, attribute, value);
	CheckGLError("eglGetConfigAttrib");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLGETCONFIGS)(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
PFNEGLGETCONFIGS geglGetConfigs;
static PFNEGLGETCONFIGS _eglGetConfigs;
static EGLBoolean APIENTRY d_eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config) {
	EGLBoolean ret = _eglGetConfigs(dpy, configs, config_size, num_config);
	CheckGLError("eglGetConfigs");
	return ret;
}
typedef EGLContext (APIENTRYP PFNEGLGETCURRENTCONTEXT)();
PFNEGLGETCURRENTCONTEXT geglGetCurrentContext;
static PFNEGLGETCURRENTCONTEXT _eglGetCurrentContext;
static EGLContext APIENTRY d_eglGetCurrentContext() {
	EGLContext ret = _eglGetCurrentContext();
	CheckGLError("eglGetCurrentContext");
	return ret;
}
typedef EGLDisplay (APIENTRYP PFNEGLGETCURRENTDISPLAY)();
PFNEGLGETCURRENTDISPLAY geglGetCurrentDisplay;
static PFNEGLGETCURRENTDISPLAY _eglGetCurrentDisplay;
static EGLDisplay APIENTRY d_eglGetCurrentDisplay() {
	EGLDisplay ret = _eglGetCurrentDisplay();
	CheckGLError("eglGetCurrentDisplay");
	return ret;
}
typedef EGLSurface (APIENTRYP PFNEGLGETCURRENTSURFACE)(EGLint readdraw);
PFNEGLGETCURRENTSURFACE geglGetCurrentSurface;
static PFNEGLGETCURRENTSURFACE _eglGetCurrentSurface;
static EGLSurface APIENTRY d_eglGetCurrentSurface(EGLint readdraw) {
	EGLSurface ret = _eglGetCurrentSurface(readdraw);
	CheckGLError("eglGetCurrentSurface");
	return ret;
}
typedef EGLDisplay (APIENTRYP PFNEGLGETDISPLAY)(EGLNativeDisplayType display_id);
PFNEGLGETDISPLAY geglGetDisplay;
static PFNEGLGETDISPLAY _eglGetDisplay;
static EGLDisplay APIENTRY d_eglGetDisplay(EGLNativeDisplayType display_id) {
	EGLDisplay ret = _eglGetDisplay(display_id);
	CheckGLError("eglGetDisplay");
	return ret;
}
typedef EGLint (APIENTRYP PFNEGLGETERROR)();
PFNEGLGETERROR geglGetError;
static PFNEGLGETERROR _eglGetError;
static EGLint APIENTRY d_eglGetError() {
	EGLint ret = _eglGetError();
	CheckGLError("eglGetError");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLGETOUTPUTLAYERSEXT)(EGLDisplay dpy, const EGLAttrib *attrib_list, EGLOutputLayerEXT *layers, EGLint max_layers, EGLint *num_layers);
PFNEGLGETOUTPUTLAYERSEXT geglGetOutputLayersEXT;
static PFNEGLGETOUTPUTLAYERSEXT _eglGetOutputLayersEXT;
static EGLBoolean APIENTRY d_eglGetOutputLayersEXT(EGLDisplay dpy, const EGLAttrib *attrib_list, EGLOutputLayerEXT *layers, EGLint max_layers, EGLint *num_layers) {
	EGLBoolean ret = _eglGetOutputLayersEXT(dpy, attrib_list, layers, max_layers, num_layers);
	CheckGLError("eglGetOutputLayersEXT");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLGETOUTPUTPORTSEXT)(EGLDisplay dpy, const EGLAttrib *attrib_list, EGLOutputPortEXT *ports, EGLint max_ports, EGLint *num_ports);
PFNEGLGETOUTPUTPORTSEXT geglGetOutputPortsEXT;
static PFNEGLGETOUTPUTPORTSEXT _eglGetOutputPortsEXT;
static EGLBoolean APIENTRY d_eglGetOutputPortsEXT(EGLDisplay dpy, const EGLAttrib *attrib_list, EGLOutputPortEXT *ports, EGLint max_ports, EGLint *num_ports) {
	EGLBoolean ret = _eglGetOutputPortsEXT(dpy, attrib_list, ports, max_ports, num_ports);
	CheckGLError("eglGetOutputPortsEXT");
	return ret;
}
typedef EGLDisplay (APIENTRYP PFNEGLGETPLATFORMDISPLAY)(EGLenum platform, void *native_display, const EGLAttrib *attrib_list);
PFNEGLGETPLATFORMDISPLAY geglGetPlatformDisplay;
static PFNEGLGETPLATFORMDISPLAY _eglGetPlatformDisplay;
static EGLDisplay APIENTRY d_eglGetPlatformDisplay(EGLenum platform, void *native_display, const EGLAttrib *attrib_list) {
	EGLDisplay ret = _eglGetPlatformDisplay(platform, native_display, attrib_list);
	CheckGLError("eglGetPlatformDisplay");
	return ret;
}
typedef EGLDisplay (APIENTRYP PFNEGLGETPLATFORMDISPLAYEXT)(EGLenum platform, void *native_display, const EGLint *attrib_list);
PFNEGLGETPLATFORMDISPLAYEXT geglGetPlatformDisplayEXT;
static PFNEGLGETPLATFORMDISPLAYEXT _eglGetPlatformDisplayEXT;
static EGLDisplay APIENTRY d_eglGetPlatformDisplayEXT(EGLenum platform, void *native_display, const EGLint *attrib_list) {
	EGLDisplay ret = _eglGetPlatformDisplayEXT(platform, native_display, attrib_list);
	CheckGLError("eglGetPlatformDisplayEXT");
	return ret;
}
typedef __eglMustCastToProperFunctionPointerType (APIENTRYP PFNEGLGETPROCADDRESS)(const char *procname);
PFNEGLGETPROCADDRESS geglGetProcAddress;
static PFNEGLGETPROCADDRESS _eglGetProcAddress;
static __eglMustCastToProperFunctionPointerType APIENTRY d_eglGetProcAddress(const char *procname) {
	__eglMustCastToProperFunctionPointerType ret = _eglGetProcAddress(procname);
	CheckGLError("eglGetProcAddress");
	return ret;
}
typedef EGLNativeFileDescriptorKHR (APIENTRYP PFNEGLGETSTREAMFILEDESCRIPTORKHR)(EGLDisplay dpy, EGLStreamKHR stream);
PFNEGLGETSTREAMFILEDESCRIPTORKHR geglGetStreamFileDescriptorKHR;
static PFNEGLGETSTREAMFILEDESCRIPTORKHR _eglGetStreamFileDescriptorKHR;
static EGLNativeFileDescriptorKHR APIENTRY d_eglGetStreamFileDescriptorKHR(EGLDisplay dpy, EGLStreamKHR stream) {
	EGLNativeFileDescriptorKHR ret = _eglGetStreamFileDescriptorKHR(dpy, stream);
	CheckGLError("eglGetStreamFileDescriptorKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLGETSYNCATTRIB)(EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib *value);
PFNEGLGETSYNCATTRIB geglGetSyncAttrib;
static PFNEGLGETSYNCATTRIB _eglGetSyncAttrib;
static EGLBoolean APIENTRY d_eglGetSyncAttrib(EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib *value) {
	EGLBoolean ret = _eglGetSyncAttrib(dpy, sync, attribute, value);
	CheckGLError("eglGetSyncAttrib");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLGETSYNCATTRIBKHR)(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint *value);
PFNEGLGETSYNCATTRIBKHR geglGetSyncAttribKHR;
static PFNEGLGETSYNCATTRIBKHR _eglGetSyncAttribKHR;
static EGLBoolean APIENTRY d_eglGetSyncAttribKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint *value) {
	EGLBoolean ret = _eglGetSyncAttribKHR(dpy, sync, attribute, value);
	CheckGLError("eglGetSyncAttribKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLGETSYNCATTRIBNV)(EGLSyncNV sync, EGLint attribute, EGLint *value);
PFNEGLGETSYNCATTRIBNV geglGetSyncAttribNV;
static PFNEGLGETSYNCATTRIBNV _eglGetSyncAttribNV;
static EGLBoolean APIENTRY d_eglGetSyncAttribNV(EGLSyncNV sync, EGLint attribute, EGLint *value) {
	EGLBoolean ret = _eglGetSyncAttribNV(sync, attribute, value);
	CheckGLError("eglGetSyncAttribNV");
	return ret;
}
typedef EGLuint64NV (APIENTRYP PFNEGLGETSYSTEMTIMEFREQUENCYNV)();
PFNEGLGETSYSTEMTIMEFREQUENCYNV geglGetSystemTimeFrequencyNV;
static PFNEGLGETSYSTEMTIMEFREQUENCYNV _eglGetSystemTimeFrequencyNV;
static EGLuint64NV APIENTRY d_eglGetSystemTimeFrequencyNV() {
	EGLuint64NV ret = _eglGetSystemTimeFrequencyNV();
	CheckGLError("eglGetSystemTimeFrequencyNV");
	return ret;
}
typedef EGLuint64NV (APIENTRYP PFNEGLGETSYSTEMTIMENV)();
PFNEGLGETSYSTEMTIMENV geglGetSystemTimeNV;
static PFNEGLGETSYSTEMTIMENV _eglGetSystemTimeNV;
static EGLuint64NV APIENTRY d_eglGetSystemTimeNV() {
	EGLuint64NV ret = _eglGetSystemTimeNV();
	CheckGLError("eglGetSystemTimeNV");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLINITIALIZE)(EGLDisplay dpy, EGLint *major, EGLint *minor);
PFNEGLINITIALIZE geglInitialize;
static PFNEGLINITIALIZE _eglInitialize;
static EGLBoolean APIENTRY d_eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor) {
	EGLBoolean ret = _eglInitialize(dpy, major, minor);
	CheckGLError("eglInitialize");
	return ret;
}
typedef EGLint (APIENTRYP PFNEGLLABELOBJECTKHR)(EGLDisplay display, EGLenum objectType, EGLObjectKHR object, EGLLabelKHR label);
PFNEGLLABELOBJECTKHR geglLabelObjectKHR;
static PFNEGLLABELOBJECTKHR _eglLabelObjectKHR;
static EGLint APIENTRY d_eglLabelObjectKHR(EGLDisplay display, EGLenum objectType, EGLObjectKHR object, EGLLabelKHR label) {
	EGLint ret = _eglLabelObjectKHR(display, objectType, object, label);
	CheckGLError("eglLabelObjectKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLLOCKSURFACEKHR)(EGLDisplay dpy, EGLSurface surface, const EGLint *attrib_list);
PFNEGLLOCKSURFACEKHR geglLockSurfaceKHR;
static PFNEGLLOCKSURFACEKHR _eglLockSurfaceKHR;
static EGLBoolean APIENTRY d_eglLockSurfaceKHR(EGLDisplay dpy, EGLSurface surface, const EGLint *attrib_list) {
	EGLBoolean ret = _eglLockSurfaceKHR(dpy, surface, attrib_list);
	CheckGLError("eglLockSurfaceKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLMAKECURRENT)(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
PFNEGLMAKECURRENT geglMakeCurrent;
static PFNEGLMAKECURRENT _eglMakeCurrent;
static EGLBoolean APIENTRY d_eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
	EGLBoolean ret = _eglMakeCurrent(dpy, draw, read, ctx);
	CheckGLError("eglMakeCurrent");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLOUTPUTLAYERATTRIBEXT)(EGLDisplay dpy, EGLOutputLayerEXT layer, EGLint attribute, EGLAttrib value);
PFNEGLOUTPUTLAYERATTRIBEXT geglOutputLayerAttribEXT;
static PFNEGLOUTPUTLAYERATTRIBEXT _eglOutputLayerAttribEXT;
static EGLBoolean APIENTRY d_eglOutputLayerAttribEXT(EGLDisplay dpy, EGLOutputLayerEXT layer, EGLint attribute, EGLAttrib value) {
	EGLBoolean ret = _eglOutputLayerAttribEXT(dpy, layer, attribute, value);
	CheckGLError("eglOutputLayerAttribEXT");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLOUTPUTPORTATTRIBEXT)(EGLDisplay dpy, EGLOutputPortEXT port, EGLint attribute, EGLAttrib value);
PFNEGLOUTPUTPORTATTRIBEXT geglOutputPortAttribEXT;
static PFNEGLOUTPUTPORTATTRIBEXT _eglOutputPortAttribEXT;
static EGLBoolean APIENTRY d_eglOutputPortAttribEXT(EGLDisplay dpy, EGLOutputPortEXT port, EGLint attribute, EGLAttrib value) {
	EGLBoolean ret = _eglOutputPortAttribEXT(dpy, port, attribute, value);
	CheckGLError("eglOutputPortAttribEXT");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLPOSTSUBBUFFERNV)(EGLDisplay dpy, EGLSurface surface, EGLint x, EGLint y, EGLint width, EGLint height);
PFNEGLPOSTSUBBUFFERNV geglPostSubBufferNV;
static PFNEGLPOSTSUBBUFFERNV _eglPostSubBufferNV;
static EGLBoolean APIENTRY d_eglPostSubBufferNV(EGLDisplay dpy, EGLSurface surface, EGLint x, EGLint y, EGLint width, EGLint height) {
	EGLBoolean ret = _eglPostSubBufferNV(dpy, surface, x, y, width, height);
	CheckGLError("eglPostSubBufferNV");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLPRESENTATIONTIMEANDROID)(EGLDisplay dpy, EGLSurface surface, EGLnsecsANDROID time);
PFNEGLPRESENTATIONTIMEANDROID geglPresentationTimeANDROID;
static PFNEGLPRESENTATIONTIMEANDROID _eglPresentationTimeANDROID;
static EGLBoolean APIENTRY d_eglPresentationTimeANDROID(EGLDisplay dpy, EGLSurface surface, EGLnsecsANDROID time) {
	EGLBoolean ret = _eglPresentationTimeANDROID(dpy, surface, time);
	CheckGLError("eglPresentationTimeANDROID");
	return ret;
}
typedef EGLenum (APIENTRYP PFNEGLQUERYAPI)();
PFNEGLQUERYAPI geglQueryAPI;
static PFNEGLQUERYAPI _eglQueryAPI;
static EGLenum APIENTRY d_eglQueryAPI() {
	EGLenum ret = _eglQueryAPI();
	CheckGLError("eglQueryAPI");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYCONTEXT)(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value);
PFNEGLQUERYCONTEXT geglQueryContext;
static PFNEGLQUERYCONTEXT _eglQueryContext;
static EGLBoolean APIENTRY d_eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value) {
	EGLBoolean ret = _eglQueryContext(dpy, ctx, attribute, value);
	CheckGLError("eglQueryContext");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYDEBUGKHR)(EGLint attribute, EGLAttrib *value);
PFNEGLQUERYDEBUGKHR geglQueryDebugKHR;
static PFNEGLQUERYDEBUGKHR _eglQueryDebugKHR;
static EGLBoolean APIENTRY d_eglQueryDebugKHR(EGLint attribute, EGLAttrib *value) {
	EGLBoolean ret = _eglQueryDebugKHR(attribute, value);
	CheckGLError("eglQueryDebugKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYDEVICEATTRIBEXT)(EGLDeviceEXT device, EGLint attribute, EGLAttrib *value);
PFNEGLQUERYDEVICEATTRIBEXT geglQueryDeviceAttribEXT;
static PFNEGLQUERYDEVICEATTRIBEXT _eglQueryDeviceAttribEXT;
static EGLBoolean APIENTRY d_eglQueryDeviceAttribEXT(EGLDeviceEXT device, EGLint attribute, EGLAttrib *value) {
	EGLBoolean ret = _eglQueryDeviceAttribEXT(device, attribute, value);
	CheckGLError("eglQueryDeviceAttribEXT");
	return ret;
}
typedef const char * (APIENTRYP PFNEGLQUERYDEVICESTRINGEXT)(EGLDeviceEXT device, EGLint name);
PFNEGLQUERYDEVICESTRINGEXT geglQueryDeviceStringEXT;
static PFNEGLQUERYDEVICESTRINGEXT _eglQueryDeviceStringEXT;
static const char * APIENTRY d_eglQueryDeviceStringEXT(EGLDeviceEXT device, EGLint name) {
	const char * ret = _eglQueryDeviceStringEXT(device, name);
	CheckGLError("eglQueryDeviceStringEXT");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYDEVICESEXT)(EGLint max_devices, EGLDeviceEXT *devices, EGLint *num_devices);
PFNEGLQUERYDEVICESEXT geglQueryDevicesEXT;
static PFNEGLQUERYDEVICESEXT _eglQueryDevicesEXT;
static EGLBoolean APIENTRY d_eglQueryDevicesEXT(EGLint max_devices, EGLDeviceEXT *devices, EGLint *num_devices) {
	EGLBoolean ret = _eglQueryDevicesEXT(max_devices, devices, num_devices);
	CheckGLError("eglQueryDevicesEXT");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYDISPLAYATTRIBEXT)(EGLDisplay dpy, EGLint attribute, EGLAttrib *value);
PFNEGLQUERYDISPLAYATTRIBEXT geglQueryDisplayAttribEXT;
static PFNEGLQUERYDISPLAYATTRIBEXT _eglQueryDisplayAttribEXT;
static EGLBoolean APIENTRY d_eglQueryDisplayAttribEXT(EGLDisplay dpy, EGLint attribute, EGLAttrib *value) {
	EGLBoolean ret = _eglQueryDisplayAttribEXT(dpy, attribute, value);
	CheckGLError("eglQueryDisplayAttribEXT");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYDISPLAYATTRIBNV)(EGLDisplay dpy, EGLint attribute, EGLAttrib *value);
PFNEGLQUERYDISPLAYATTRIBNV geglQueryDisplayAttribNV;
static PFNEGLQUERYDISPLAYATTRIBNV _eglQueryDisplayAttribNV;
static EGLBoolean APIENTRY d_eglQueryDisplayAttribNV(EGLDisplay dpy, EGLint attribute, EGLAttrib *value) {
	EGLBoolean ret = _eglQueryDisplayAttribNV(dpy, attribute, value);
	CheckGLError("eglQueryDisplayAttribNV");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYDMABUFFORMATSEXT)(EGLDisplay dpy, EGLint max_formats, EGLint *formats, EGLint *num_formats);
PFNEGLQUERYDMABUFFORMATSEXT geglQueryDmaBufFormatsEXT;
static PFNEGLQUERYDMABUFFORMATSEXT _eglQueryDmaBufFormatsEXT;
static EGLBoolean APIENTRY d_eglQueryDmaBufFormatsEXT(EGLDisplay dpy, EGLint max_formats, EGLint *formats, EGLint *num_formats) {
	EGLBoolean ret = _eglQueryDmaBufFormatsEXT(dpy, max_formats, formats, num_formats);
	CheckGLError("eglQueryDmaBufFormatsEXT");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYDMABUFMODIFIERSEXT)(EGLDisplay dpy, EGLint format, EGLint max_modifiers, EGLuint64KHR *modifiers, EGLBoolean *external_only, EGLint *num_modifiers);
PFNEGLQUERYDMABUFMODIFIERSEXT geglQueryDmaBufModifiersEXT;
static PFNEGLQUERYDMABUFMODIFIERSEXT _eglQueryDmaBufModifiersEXT;
static EGLBoolean APIENTRY d_eglQueryDmaBufModifiersEXT(EGLDisplay dpy, EGLint format, EGLint max_modifiers, EGLuint64KHR *modifiers, EGLBoolean *external_only, EGLint *num_modifiers) {
	EGLBoolean ret = _eglQueryDmaBufModifiersEXT(dpy, format, max_modifiers, modifiers, external_only, num_modifiers);
	CheckGLError("eglQueryDmaBufModifiersEXT");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYNATIVEDISPLAYNV)(EGLDisplay dpy, EGLNativeDisplayType *display_id);
PFNEGLQUERYNATIVEDISPLAYNV geglQueryNativeDisplayNV;
static PFNEGLQUERYNATIVEDISPLAYNV _eglQueryNativeDisplayNV;
static EGLBoolean APIENTRY d_eglQueryNativeDisplayNV(EGLDisplay dpy, EGLNativeDisplayType *display_id) {
	EGLBoolean ret = _eglQueryNativeDisplayNV(dpy, display_id);
	CheckGLError("eglQueryNativeDisplayNV");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYNATIVEPIXMAPNV)(EGLDisplay dpy, EGLSurface surf, EGLNativePixmapType *pixmap);
PFNEGLQUERYNATIVEPIXMAPNV geglQueryNativePixmapNV;
static PFNEGLQUERYNATIVEPIXMAPNV _eglQueryNativePixmapNV;
static EGLBoolean APIENTRY d_eglQueryNativePixmapNV(EGLDisplay dpy, EGLSurface surf, EGLNativePixmapType *pixmap) {
	EGLBoolean ret = _eglQueryNativePixmapNV(dpy, surf, pixmap);
	CheckGLError("eglQueryNativePixmapNV");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYNATIVEWINDOWNV)(EGLDisplay dpy, EGLSurface surf, EGLNativeWindowType *window);
PFNEGLQUERYNATIVEWINDOWNV geglQueryNativeWindowNV;
static PFNEGLQUERYNATIVEWINDOWNV _eglQueryNativeWindowNV;
static EGLBoolean APIENTRY d_eglQueryNativeWindowNV(EGLDisplay dpy, EGLSurface surf, EGLNativeWindowType *window) {
	EGLBoolean ret = _eglQueryNativeWindowNV(dpy, surf, window);
	CheckGLError("eglQueryNativeWindowNV");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYOUTPUTLAYERATTRIBEXT)(EGLDisplay dpy, EGLOutputLayerEXT layer, EGLint attribute, EGLAttrib *value);
PFNEGLQUERYOUTPUTLAYERATTRIBEXT geglQueryOutputLayerAttribEXT;
static PFNEGLQUERYOUTPUTLAYERATTRIBEXT _eglQueryOutputLayerAttribEXT;
static EGLBoolean APIENTRY d_eglQueryOutputLayerAttribEXT(EGLDisplay dpy, EGLOutputLayerEXT layer, EGLint attribute, EGLAttrib *value) {
	EGLBoolean ret = _eglQueryOutputLayerAttribEXT(dpy, layer, attribute, value);
	CheckGLError("eglQueryOutputLayerAttribEXT");
	return ret;
}
typedef const char * (APIENTRYP PFNEGLQUERYOUTPUTLAYERSTRINGEXT)(EGLDisplay dpy, EGLOutputLayerEXT layer, EGLint name);
PFNEGLQUERYOUTPUTLAYERSTRINGEXT geglQueryOutputLayerStringEXT;
static PFNEGLQUERYOUTPUTLAYERSTRINGEXT _eglQueryOutputLayerStringEXT;
static const char * APIENTRY d_eglQueryOutputLayerStringEXT(EGLDisplay dpy, EGLOutputLayerEXT layer, EGLint name) {
	const char * ret = _eglQueryOutputLayerStringEXT(dpy, layer, name);
	CheckGLError("eglQueryOutputLayerStringEXT");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYOUTPUTPORTATTRIBEXT)(EGLDisplay dpy, EGLOutputPortEXT port, EGLint attribute, EGLAttrib *value);
PFNEGLQUERYOUTPUTPORTATTRIBEXT geglQueryOutputPortAttribEXT;
static PFNEGLQUERYOUTPUTPORTATTRIBEXT _eglQueryOutputPortAttribEXT;
static EGLBoolean APIENTRY d_eglQueryOutputPortAttribEXT(EGLDisplay dpy, EGLOutputPortEXT port, EGLint attribute, EGLAttrib *value) {
	EGLBoolean ret = _eglQueryOutputPortAttribEXT(dpy, port, attribute, value);
	CheckGLError("eglQueryOutputPortAttribEXT");
	return ret;
}
typedef const char * (APIENTRYP PFNEGLQUERYOUTPUTPORTSTRINGEXT)(EGLDisplay dpy, EGLOutputPortEXT port, EGLint name);
PFNEGLQUERYOUTPUTPORTSTRINGEXT geglQueryOutputPortStringEXT;
static PFNEGLQUERYOUTPUTPORTSTRINGEXT _eglQueryOutputPortStringEXT;
static const char * APIENTRY d_eglQueryOutputPortStringEXT(EGLDisplay dpy, EGLOutputPortEXT port, EGLint name) {
	const char * ret = _eglQueryOutputPortStringEXT(dpy, port, name);
	CheckGLError("eglQueryOutputPortStringEXT");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYSTREAMKHR)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint *value);
PFNEGLQUERYSTREAMKHR geglQueryStreamKHR;
static PFNEGLQUERYSTREAMKHR _eglQueryStreamKHR;
static EGLBoolean APIENTRY d_eglQueryStreamKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint *value) {
	EGLBoolean ret = _eglQueryStreamKHR(dpy, stream, attribute, value);
	CheckGLError("eglQueryStreamKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYSTREAMATTRIBKHR)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLAttrib *value);
PFNEGLQUERYSTREAMATTRIBKHR geglQueryStreamAttribKHR;
static PFNEGLQUERYSTREAMATTRIBKHR _eglQueryStreamAttribKHR;
static EGLBoolean APIENTRY d_eglQueryStreamAttribKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLAttrib *value) {
	EGLBoolean ret = _eglQueryStreamAttribKHR(dpy, stream, attribute, value);
	CheckGLError("eglQueryStreamAttribKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYSTREAMMETADATANV)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum name, EGLint n, EGLint offset, EGLint size, void *data);
PFNEGLQUERYSTREAMMETADATANV geglQueryStreamMetadataNV;
static PFNEGLQUERYSTREAMMETADATANV _eglQueryStreamMetadataNV;
static EGLBoolean APIENTRY d_eglQueryStreamMetadataNV(EGLDisplay dpy, EGLStreamKHR stream, EGLenum name, EGLint n, EGLint offset, EGLint size, void *data) {
	EGLBoolean ret = _eglQueryStreamMetadataNV(dpy, stream, name, n, offset, size, data);
	CheckGLError("eglQueryStreamMetadataNV");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYSTREAMTIMEKHR)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLTimeKHR *value);
PFNEGLQUERYSTREAMTIMEKHR geglQueryStreamTimeKHR;
static PFNEGLQUERYSTREAMTIMEKHR _eglQueryStreamTimeKHR;
static EGLBoolean APIENTRY d_eglQueryStreamTimeKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLTimeKHR *value) {
	EGLBoolean ret = _eglQueryStreamTimeKHR(dpy, stream, attribute, value);
	CheckGLError("eglQueryStreamTimeKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYSTREAMU64KHR)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLuint64KHR *value);
PFNEGLQUERYSTREAMU64KHR geglQueryStreamu64KHR;
static PFNEGLQUERYSTREAMU64KHR _eglQueryStreamu64KHR;
static EGLBoolean APIENTRY d_eglQueryStreamu64KHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLuint64KHR *value) {
	EGLBoolean ret = _eglQueryStreamu64KHR(dpy, stream, attribute, value);
	CheckGLError("eglQueryStreamu64KHR");
	return ret;
}
typedef const char * (APIENTRYP PFNEGLQUERYSTRING)(EGLDisplay dpy, EGLint name);
PFNEGLQUERYSTRING geglQueryString;
static PFNEGLQUERYSTRING _eglQueryString;
static const char * APIENTRY d_eglQueryString(EGLDisplay dpy, EGLint name) {
	const char * ret = _eglQueryString(dpy, name);
	CheckGLError("eglQueryString");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYSURFACE)(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);
PFNEGLQUERYSURFACE geglQuerySurface;
static PFNEGLQUERYSURFACE _eglQuerySurface;
static EGLBoolean APIENTRY d_eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value) {
	EGLBoolean ret = _eglQuerySurface(dpy, surface, attribute, value);
	CheckGLError("eglQuerySurface");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYSURFACE64KHR)(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLAttribKHR *value);
PFNEGLQUERYSURFACE64KHR geglQuerySurface64KHR;
static PFNEGLQUERYSURFACE64KHR _eglQuerySurface64KHR;
static EGLBoolean APIENTRY d_eglQuerySurface64KHR(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLAttribKHR *value) {
	EGLBoolean ret = _eglQuerySurface64KHR(dpy, surface, attribute, value);
	CheckGLError("eglQuerySurface64KHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLQUERYSURFACEPOINTERANGLE)(EGLDisplay dpy, EGLSurface surface, EGLint attribute, void **value);
PFNEGLQUERYSURFACEPOINTERANGLE geglQuerySurfacePointerANGLE;
static PFNEGLQUERYSURFACEPOINTERANGLE _eglQuerySurfacePointerANGLE;
static EGLBoolean APIENTRY d_eglQuerySurfacePointerANGLE(EGLDisplay dpy, EGLSurface surface, EGLint attribute, void **value) {
	EGLBoolean ret = _eglQuerySurfacePointerANGLE(dpy, surface, attribute, value);
	CheckGLError("eglQuerySurfacePointerANGLE");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLRELEASETEXIMAGE)(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
PFNEGLRELEASETEXIMAGE geglReleaseTexImage;
static PFNEGLRELEASETEXIMAGE _eglReleaseTexImage;
static EGLBoolean APIENTRY d_eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer) {
	EGLBoolean ret = _eglReleaseTexImage(dpy, surface, buffer);
	CheckGLError("eglReleaseTexImage");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLRELEASETHREAD)();
PFNEGLRELEASETHREAD geglReleaseThread;
static PFNEGLRELEASETHREAD _eglReleaseThread;
static EGLBoolean APIENTRY d_eglReleaseThread() {
	EGLBoolean ret = _eglReleaseThread();
	CheckGLError("eglReleaseThread");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLRESETSTREAMNV)(EGLDisplay dpy, EGLStreamKHR stream);
PFNEGLRESETSTREAMNV geglResetStreamNV;
static PFNEGLRESETSTREAMNV _eglResetStreamNV;
static EGLBoolean APIENTRY d_eglResetStreamNV(EGLDisplay dpy, EGLStreamKHR stream) {
	EGLBoolean ret = _eglResetStreamNV(dpy, stream);
	CheckGLError("eglResetStreamNV");
	return ret;
}
typedef void (APIENTRYP PFNEGLSETBLOBCACHEFUNCSANDROID)(EGLDisplay dpy, EGLSetBlobFuncANDROID set, EGLGetBlobFuncANDROID get);
PFNEGLSETBLOBCACHEFUNCSANDROID geglSetBlobCacheFuncsANDROID;
static PFNEGLSETBLOBCACHEFUNCSANDROID _eglSetBlobCacheFuncsANDROID;
static void APIENTRY d_eglSetBlobCacheFuncsANDROID(EGLDisplay dpy, EGLSetBlobFuncANDROID set, EGLGetBlobFuncANDROID get) {
	_eglSetBlobCacheFuncsANDROID(dpy, set, get);
	CheckGLError("eglSetBlobCacheFuncsANDROID");
}
typedef EGLBoolean (APIENTRYP PFNEGLSETDAMAGEREGIONKHR)(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects);
PFNEGLSETDAMAGEREGIONKHR geglSetDamageRegionKHR;
static PFNEGLSETDAMAGEREGIONKHR _eglSetDamageRegionKHR;
static EGLBoolean APIENTRY d_eglSetDamageRegionKHR(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects) {
	EGLBoolean ret = _eglSetDamageRegionKHR(dpy, surface, rects, n_rects);
	CheckGLError("eglSetDamageRegionKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSETSTREAMATTRIBKHR)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLAttrib value);
PFNEGLSETSTREAMATTRIBKHR geglSetStreamAttribKHR;
static PFNEGLSETSTREAMATTRIBKHR _eglSetStreamAttribKHR;
static EGLBoolean APIENTRY d_eglSetStreamAttribKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLAttrib value) {
	EGLBoolean ret = _eglSetStreamAttribKHR(dpy, stream, attribute, value);
	CheckGLError("eglSetStreamAttribKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSETSTREAMMETADATANV)(EGLDisplay dpy, EGLStreamKHR stream, EGLint n, EGLint offset, EGLint size, const void *data);
PFNEGLSETSTREAMMETADATANV geglSetStreamMetadataNV;
static PFNEGLSETSTREAMMETADATANV _eglSetStreamMetadataNV;
static EGLBoolean APIENTRY d_eglSetStreamMetadataNV(EGLDisplay dpy, EGLStreamKHR stream, EGLint n, EGLint offset, EGLint size, const void *data) {
	EGLBoolean ret = _eglSetStreamMetadataNV(dpy, stream, n, offset, size, data);
	CheckGLError("eglSetStreamMetadataNV");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSIGNALSYNCKHR)(EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode);
PFNEGLSIGNALSYNCKHR geglSignalSyncKHR;
static PFNEGLSIGNALSYNCKHR _eglSignalSyncKHR;
static EGLBoolean APIENTRY d_eglSignalSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLenum mode) {
	EGLBoolean ret = _eglSignalSyncKHR(dpy, sync, mode);
	CheckGLError("eglSignalSyncKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSIGNALSYNCNV)(EGLSyncNV sync, EGLenum mode);
PFNEGLSIGNALSYNCNV geglSignalSyncNV;
static PFNEGLSIGNALSYNCNV _eglSignalSyncNV;
static EGLBoolean APIENTRY d_eglSignalSyncNV(EGLSyncNV sync, EGLenum mode) {
	EGLBoolean ret = _eglSignalSyncNV(sync, mode);
	CheckGLError("eglSignalSyncNV");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSTREAMATTRIBKHR)(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint value);
PFNEGLSTREAMATTRIBKHR geglStreamAttribKHR;
static PFNEGLSTREAMATTRIBKHR _eglStreamAttribKHR;
static EGLBoolean APIENTRY d_eglStreamAttribKHR(EGLDisplay dpy, EGLStreamKHR stream, EGLenum attribute, EGLint value) {
	EGLBoolean ret = _eglStreamAttribKHR(dpy, stream, attribute, value);
	CheckGLError("eglStreamAttribKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSTREAMCONSUMERACQUIREKHR)(EGLDisplay dpy, EGLStreamKHR stream);
PFNEGLSTREAMCONSUMERACQUIREKHR geglStreamConsumerAcquireKHR;
static PFNEGLSTREAMCONSUMERACQUIREKHR _eglStreamConsumerAcquireKHR;
static EGLBoolean APIENTRY d_eglStreamConsumerAcquireKHR(EGLDisplay dpy, EGLStreamKHR stream) {
	EGLBoolean ret = _eglStreamConsumerAcquireKHR(dpy, stream);
	CheckGLError("eglStreamConsumerAcquireKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSTREAMCONSUMERACQUIREATTRIBKHR)(EGLDisplay dpy, EGLStreamKHR stream, const EGLAttrib *attrib_list);
PFNEGLSTREAMCONSUMERACQUIREATTRIBKHR geglStreamConsumerAcquireAttribKHR;
static PFNEGLSTREAMCONSUMERACQUIREATTRIBKHR _eglStreamConsumerAcquireAttribKHR;
static EGLBoolean APIENTRY d_eglStreamConsumerAcquireAttribKHR(EGLDisplay dpy, EGLStreamKHR stream, const EGLAttrib *attrib_list) {
	EGLBoolean ret = _eglStreamConsumerAcquireAttribKHR(dpy, stream, attrib_list);
	CheckGLError("eglStreamConsumerAcquireAttribKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALKHR)(EGLDisplay dpy, EGLStreamKHR stream);
PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALKHR geglStreamConsumerGLTextureExternalKHR;
static PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALKHR _eglStreamConsumerGLTextureExternalKHR;
static EGLBoolean APIENTRY d_eglStreamConsumerGLTextureExternalKHR(EGLDisplay dpy, EGLStreamKHR stream) {
	EGLBoolean ret = _eglStreamConsumerGLTextureExternalKHR(dpy, stream);
	CheckGLError("eglStreamConsumerGLTextureExternalKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALATTRIBSNV)(EGLDisplay dpy, EGLStreamKHR stream, EGLAttrib *attrib_list);
PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALATTRIBSNV geglStreamConsumerGLTextureExternalAttribsNV;
static PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALATTRIBSNV _eglStreamConsumerGLTextureExternalAttribsNV;
static EGLBoolean APIENTRY d_eglStreamConsumerGLTextureExternalAttribsNV(EGLDisplay dpy, EGLStreamKHR stream, EGLAttrib *attrib_list) {
	EGLBoolean ret = _eglStreamConsumerGLTextureExternalAttribsNV(dpy, stream, attrib_list);
	CheckGLError("eglStreamConsumerGLTextureExternalAttribsNV");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSTREAMCONSUMEROUTPUTEXT)(EGLDisplay dpy, EGLStreamKHR stream, EGLOutputLayerEXT layer);
PFNEGLSTREAMCONSUMEROUTPUTEXT geglStreamConsumerOutputEXT;
static PFNEGLSTREAMCONSUMEROUTPUTEXT _eglStreamConsumerOutputEXT;
static EGLBoolean APIENTRY d_eglStreamConsumerOutputEXT(EGLDisplay dpy, EGLStreamKHR stream, EGLOutputLayerEXT layer) {
	EGLBoolean ret = _eglStreamConsumerOutputEXT(dpy, stream, layer);
	CheckGLError("eglStreamConsumerOutputEXT");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSTREAMCONSUMERRELEASEKHR)(EGLDisplay dpy, EGLStreamKHR stream);
PFNEGLSTREAMCONSUMERRELEASEKHR geglStreamConsumerReleaseKHR;
static PFNEGLSTREAMCONSUMERRELEASEKHR _eglStreamConsumerReleaseKHR;
static EGLBoolean APIENTRY d_eglStreamConsumerReleaseKHR(EGLDisplay dpy, EGLStreamKHR stream) {
	EGLBoolean ret = _eglStreamConsumerReleaseKHR(dpy, stream);
	CheckGLError("eglStreamConsumerReleaseKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSTREAMCONSUMERRELEASEATTRIBKHR)(EGLDisplay dpy, EGLStreamKHR stream, const EGLAttrib *attrib_list);
PFNEGLSTREAMCONSUMERRELEASEATTRIBKHR geglStreamConsumerReleaseAttribKHR;
static PFNEGLSTREAMCONSUMERRELEASEATTRIBKHR _eglStreamConsumerReleaseAttribKHR;
static EGLBoolean APIENTRY d_eglStreamConsumerReleaseAttribKHR(EGLDisplay dpy, EGLStreamKHR stream, const EGLAttrib *attrib_list) {
	EGLBoolean ret = _eglStreamConsumerReleaseAttribKHR(dpy, stream, attrib_list);
	CheckGLError("eglStreamConsumerReleaseAttribKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSURFACEATTRIB)(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
PFNEGLSURFACEATTRIB geglSurfaceAttrib;
static PFNEGLSURFACEATTRIB _eglSurfaceAttrib;
static EGLBoolean APIENTRY d_eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value) {
	EGLBoolean ret = _eglSurfaceAttrib(dpy, surface, attribute, value);
	CheckGLError("eglSurfaceAttrib");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSWAPBUFFERS)(EGLDisplay dpy, EGLSurface surface);
PFNEGLSWAPBUFFERS geglSwapBuffers;
static PFNEGLSWAPBUFFERS _eglSwapBuffers;
static EGLBoolean APIENTRY d_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
	EGLBoolean ret = _eglSwapBuffers(dpy, surface);
	CheckGLError("eglSwapBuffers");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSWAPBUFFERSWITHDAMAGEEXT)(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects);
PFNEGLSWAPBUFFERSWITHDAMAGEEXT geglSwapBuffersWithDamageEXT;
static PFNEGLSWAPBUFFERSWITHDAMAGEEXT _eglSwapBuffersWithDamageEXT;
static EGLBoolean APIENTRY d_eglSwapBuffersWithDamageEXT(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects) {
	EGLBoolean ret = _eglSwapBuffersWithDamageEXT(dpy, surface, rects, n_rects);
	CheckGLError("eglSwapBuffersWithDamageEXT");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSWAPBUFFERSWITHDAMAGEKHR)(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects);
PFNEGLSWAPBUFFERSWITHDAMAGEKHR geglSwapBuffersWithDamageKHR;
static PFNEGLSWAPBUFFERSWITHDAMAGEKHR _eglSwapBuffersWithDamageKHR;
static EGLBoolean APIENTRY d_eglSwapBuffersWithDamageKHR(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects) {
	EGLBoolean ret = _eglSwapBuffersWithDamageKHR(dpy, surface, rects, n_rects);
	CheckGLError("eglSwapBuffersWithDamageKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSWAPBUFFERSREGIONNOK)(EGLDisplay dpy, EGLSurface surface, EGLint numRects, const EGLint *rects);
PFNEGLSWAPBUFFERSREGIONNOK geglSwapBuffersRegionNOK;
static PFNEGLSWAPBUFFERSREGIONNOK _eglSwapBuffersRegionNOK;
static EGLBoolean APIENTRY d_eglSwapBuffersRegionNOK(EGLDisplay dpy, EGLSurface surface, EGLint numRects, const EGLint *rects) {
	EGLBoolean ret = _eglSwapBuffersRegionNOK(dpy, surface, numRects, rects);
	CheckGLError("eglSwapBuffersRegionNOK");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSWAPBUFFERSREGION2NOK)(EGLDisplay dpy, EGLSurface surface, EGLint numRects, const EGLint *rects);
PFNEGLSWAPBUFFERSREGION2NOK geglSwapBuffersRegion2NOK;
static PFNEGLSWAPBUFFERSREGION2NOK _eglSwapBuffersRegion2NOK;
static EGLBoolean APIENTRY d_eglSwapBuffersRegion2NOK(EGLDisplay dpy, EGLSurface surface, EGLint numRects, const EGLint *rects) {
	EGLBoolean ret = _eglSwapBuffersRegion2NOK(dpy, surface, numRects, rects);
	CheckGLError("eglSwapBuffersRegion2NOK");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLSWAPINTERVAL)(EGLDisplay dpy, EGLint interval);
PFNEGLSWAPINTERVAL geglSwapInterval;
static PFNEGLSWAPINTERVAL _eglSwapInterval;
static EGLBoolean APIENTRY d_eglSwapInterval(EGLDisplay dpy, EGLint interval) {
	EGLBoolean ret = _eglSwapInterval(dpy, interval);
	CheckGLError("eglSwapInterval");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLTERMINATE)(EGLDisplay dpy);
PFNEGLTERMINATE geglTerminate;
static PFNEGLTERMINATE _eglTerminate;
static EGLBoolean APIENTRY d_eglTerminate(EGLDisplay dpy) {
	EGLBoolean ret = _eglTerminate(dpy);
	CheckGLError("eglTerminate");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLUNLOCKSURFACEKHR)(EGLDisplay dpy, EGLSurface surface);
PFNEGLUNLOCKSURFACEKHR geglUnlockSurfaceKHR;
static PFNEGLUNLOCKSURFACEKHR _eglUnlockSurfaceKHR;
static EGLBoolean APIENTRY d_eglUnlockSurfaceKHR(EGLDisplay dpy, EGLSurface surface) {
	EGLBoolean ret = _eglUnlockSurfaceKHR(dpy, surface);
	CheckGLError("eglUnlockSurfaceKHR");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLWAITCLIENT)();
PFNEGLWAITCLIENT geglWaitClient;
static PFNEGLWAITCLIENT _eglWaitClient;
static EGLBoolean APIENTRY d_eglWaitClient() {
	EGLBoolean ret = _eglWaitClient();
	CheckGLError("eglWaitClient");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLWAITGL)();
PFNEGLWAITGL geglWaitGL;
static PFNEGLWAITGL _eglWaitGL;
static EGLBoolean APIENTRY d_eglWaitGL() {
	EGLBoolean ret = _eglWaitGL();
	CheckGLError("eglWaitGL");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLWAITNATIVE)(EGLint engine);
PFNEGLWAITNATIVE geglWaitNative;
static PFNEGLWAITNATIVE _eglWaitNative;
static EGLBoolean APIENTRY d_eglWaitNative(EGLint engine) {
	EGLBoolean ret = _eglWaitNative(engine);
	CheckGLError("eglWaitNative");
	return ret;
}
typedef EGLBoolean (APIENTRYP PFNEGLWAITSYNC)(EGLDisplay dpy, EGLSync sync, EGLint flags);
PFNEGLWAITSYNC geglWaitSync;
static PFNEGLWAITSYNC _eglWaitSync;
static EGLBoolean APIENTRY d_eglWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags) {
	EGLBoolean ret = _eglWaitSync(dpy, sync, flags);
	CheckGLError("eglWaitSync");
	return ret;
}
typedef EGLint (APIENTRYP PFNEGLWAITSYNCKHR)(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags);
PFNEGLWAITSYNCKHR geglWaitSyncKHR;
static PFNEGLWAITSYNCKHR _eglWaitSyncKHR;
static EGLint APIENTRY d_eglWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags) {
	EGLint ret = _eglWaitSyncKHR(dpy, sync, flags);
	CheckGLError("eglWaitSyncKHR");
	return ret;
}

#include <EGL/egl.h>
#define GPA(a) eglGetProcAddress(#a)

geglext_t geglext;
static const char *geglext_str = NULL;

static int gegl_check_extension(const char *ext) {
	return strstr(geglext_str, ext) ? 1 : 0;
}

void gegl_init(EGLDisplay display, int enableDebug) {
	/* EGL_VERSION_1_0 */
	_eglChooseConfig = (PFNEGLCHOOSECONFIG)GPA(eglChooseConfig);
	_eglCopyBuffers = (PFNEGLCOPYBUFFERS)GPA(eglCopyBuffers);
	_eglCreateContext = (PFNEGLCREATECONTEXT)GPA(eglCreateContext);
	_eglCreatePbufferSurface = (PFNEGLCREATEPBUFFERSURFACE)GPA(eglCreatePbufferSurface);
	_eglCreatePixmapSurface = (PFNEGLCREATEPIXMAPSURFACE)GPA(eglCreatePixmapSurface);
	_eglCreateWindowSurface = (PFNEGLCREATEWINDOWSURFACE)GPA(eglCreateWindowSurface);
	_eglDestroyContext = (PFNEGLDESTROYCONTEXT)GPA(eglDestroyContext);
	_eglDestroySurface = (PFNEGLDESTROYSURFACE)GPA(eglDestroySurface);
	_eglGetConfigAttrib = (PFNEGLGETCONFIGATTRIB)GPA(eglGetConfigAttrib);
	_eglGetConfigs = (PFNEGLGETCONFIGS)GPA(eglGetConfigs);
	_eglGetCurrentDisplay = (PFNEGLGETCURRENTDISPLAY)GPA(eglGetCurrentDisplay);
	_eglGetCurrentSurface = (PFNEGLGETCURRENTSURFACE)GPA(eglGetCurrentSurface);
	_eglGetDisplay = (PFNEGLGETDISPLAY)GPA(eglGetDisplay);
	_eglGetError = (PFNEGLGETERROR)GPA(eglGetError);
	_eglGetProcAddress = (PFNEGLGETPROCADDRESS)GPA(eglGetProcAddress);
	_eglInitialize = (PFNEGLINITIALIZE)GPA(eglInitialize);
	_eglMakeCurrent = (PFNEGLMAKECURRENT)GPA(eglMakeCurrent);
	_eglQueryContext = (PFNEGLQUERYCONTEXT)GPA(eglQueryContext);
	_eglQueryString = (PFNEGLQUERYSTRING)GPA(eglQueryString);
	_eglQuerySurface = (PFNEGLQUERYSURFACE)GPA(eglQuerySurface);
	_eglSwapBuffers = (PFNEGLSWAPBUFFERS)GPA(eglSwapBuffers);
	_eglTerminate = (PFNEGLTERMINATE)GPA(eglTerminate);
	_eglWaitGL = (PFNEGLWAITGL)GPA(eglWaitGL);
	_eglWaitNative = (PFNEGLWAITNATIVE)GPA(eglWaitNative);

	/* EGL_VERSION_1_1 */
	_eglBindTexImage = (PFNEGLBINDTEXIMAGE)GPA(eglBindTexImage);
	_eglReleaseTexImage = (PFNEGLRELEASETEXIMAGE)GPA(eglReleaseTexImage);
	_eglSurfaceAttrib = (PFNEGLSURFACEATTRIB)GPA(eglSurfaceAttrib);
	_eglSwapInterval = (PFNEGLSWAPINTERVAL)GPA(eglSwapInterval);

	/* EGL_VERSION_1_2 */
	_eglBindAPI = (PFNEGLBINDAPI)GPA(eglBindAPI);
	_eglQueryAPI = (PFNEGLQUERYAPI)GPA(eglQueryAPI);
	_eglCreatePbufferFromClientBuffer = (PFNEGLCREATEPBUFFERFROMCLIENTBUFFER)GPA(eglCreatePbufferFromClientBuffer);
	_eglReleaseThread = (PFNEGLRELEASETHREAD)GPA(eglReleaseThread);
	_eglWaitClient = (PFNEGLWAITCLIENT)GPA(eglWaitClient);

	/* EGL_VERSION_1_4 */
	_eglGetCurrentContext = (PFNEGLGETCURRENTCONTEXT)GPA(eglGetCurrentContext);

	/* EGL_VERSION_1_5 */
	_eglCreateSync = (PFNEGLCREATESYNC)GPA(eglCreateSync);
	_eglDestroySync = (PFNEGLDESTROYSYNC)GPA(eglDestroySync);
	_eglClientWaitSync = (PFNEGLCLIENTWAITSYNC)GPA(eglClientWaitSync);
	_eglGetSyncAttrib = (PFNEGLGETSYNCATTRIB)GPA(eglGetSyncAttrib);
	_eglCreateImage = (PFNEGLCREATEIMAGE)GPA(eglCreateImage);
	_eglDestroyImage = (PFNEGLDESTROYIMAGE)GPA(eglDestroyImage);
	_eglGetPlatformDisplay = (PFNEGLGETPLATFORMDISPLAY)GPA(eglGetPlatformDisplay);
	_eglCreatePlatformWindowSurface = (PFNEGLCREATEPLATFORMWINDOWSURFACE)GPA(eglCreatePlatformWindowSurface);
	_eglCreatePlatformPixmapSurface = (PFNEGLCREATEPLATFORMPIXMAPSURFACE)GPA(eglCreatePlatformPixmapSurface);
	_eglWaitSync = (PFNEGLWAITSYNC)GPA(eglWaitSync);

#ifdef EGL_ANDROID_blob_cache
	_eglSetBlobCacheFuncsANDROID = (PFNEGLSETBLOBCACHEFUNCSANDROID)GPA(eglSetBlobCacheFuncsANDROID);
#endif

#ifdef EGL_ANDROID_create_native_client_buffer
	_eglCreateNativeClientBufferANDROID = (PFNEGLCREATENATIVECLIENTBUFFERANDROID)GPA(eglCreateNativeClientBufferANDROID);
#endif

#ifdef EGL_ANDROID_native_fence_sync
	_eglDupNativeFenceFDANDROID = (PFNEGLDUPNATIVEFENCEFDANDROID)GPA(eglDupNativeFenceFDANDROID);
#endif

#ifdef EGL_ANDROID_presentation_time
	_eglPresentationTimeANDROID = (PFNEGLPRESENTATIONTIMEANDROID)GPA(eglPresentationTimeANDROID);
#endif

#ifdef EGL_ANGLE_query_surface_pointer
	_eglQuerySurfacePointerANGLE = (PFNEGLQUERYSURFACEPOINTERANGLE)GPA(eglQuerySurfacePointerANGLE);
#endif

#ifdef EGL_EXT_device_base
	_eglQueryDeviceAttribEXT = (PFNEGLQUERYDEVICEATTRIBEXT)GPA(eglQueryDeviceAttribEXT);
	_eglQueryDeviceStringEXT = (PFNEGLQUERYDEVICESTRINGEXT)GPA(eglQueryDeviceStringEXT);
	_eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXT)GPA(eglQueryDevicesEXT);
	_eglQueryDisplayAttribEXT = (PFNEGLQUERYDISPLAYATTRIBEXT)GPA(eglQueryDisplayAttribEXT);
#endif

#ifdef EGL_EXT_device_enumeration
	_eglQueryDevicesEXT = (PFNEGLQUERYDEVICESEXT)GPA(eglQueryDevicesEXT);
#endif

#ifdef EGL_EXT_device_query
	_eglQueryDeviceAttribEXT = (PFNEGLQUERYDEVICEATTRIBEXT)GPA(eglQueryDeviceAttribEXT);
	_eglQueryDeviceStringEXT = (PFNEGLQUERYDEVICESTRINGEXT)GPA(eglQueryDeviceStringEXT);
	_eglQueryDisplayAttribEXT = (PFNEGLQUERYDISPLAYATTRIBEXT)GPA(eglQueryDisplayAttribEXT);
#endif

#ifdef EGL_EXT_image_dma_buf_import_modifiers
	_eglQueryDmaBufFormatsEXT = (PFNEGLQUERYDMABUFFORMATSEXT)GPA(eglQueryDmaBufFormatsEXT);
	_eglQueryDmaBufModifiersEXT = (PFNEGLQUERYDMABUFMODIFIERSEXT)GPA(eglQueryDmaBufModifiersEXT);
#endif

#ifdef EGL_EXT_output_base
	_eglGetOutputLayersEXT = (PFNEGLGETOUTPUTLAYERSEXT)GPA(eglGetOutputLayersEXT);
	_eglGetOutputPortsEXT = (PFNEGLGETOUTPUTPORTSEXT)GPA(eglGetOutputPortsEXT);
	_eglOutputLayerAttribEXT = (PFNEGLOUTPUTLAYERATTRIBEXT)GPA(eglOutputLayerAttribEXT);
	_eglQueryOutputLayerAttribEXT = (PFNEGLQUERYOUTPUTLAYERATTRIBEXT)GPA(eglQueryOutputLayerAttribEXT);
	_eglQueryOutputLayerStringEXT = (PFNEGLQUERYOUTPUTLAYERSTRINGEXT)GPA(eglQueryOutputLayerStringEXT);
	_eglOutputPortAttribEXT = (PFNEGLOUTPUTPORTATTRIBEXT)GPA(eglOutputPortAttribEXT);
	_eglQueryOutputPortAttribEXT = (PFNEGLQUERYOUTPUTPORTATTRIBEXT)GPA(eglQueryOutputPortAttribEXT);
	_eglQueryOutputPortStringEXT = (PFNEGLQUERYOUTPUTPORTSTRINGEXT)GPA(eglQueryOutputPortStringEXT);
#endif

#ifdef EGL_EXT_platform_base
	_eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXT)GPA(eglGetPlatformDisplayEXT);
	_eglCreatePlatformWindowSurfaceEXT = (PFNEGLCREATEPLATFORMWINDOWSURFACEEXT)GPA(eglCreatePlatformWindowSurfaceEXT);
	_eglCreatePlatformPixmapSurfaceEXT = (PFNEGLCREATEPLATFORMPIXMAPSURFACEEXT)GPA(eglCreatePlatformPixmapSurfaceEXT);
#endif

#ifdef EGL_EXT_stream_consumer_egloutput
	_eglStreamConsumerOutputEXT = (PFNEGLSTREAMCONSUMEROUTPUTEXT)GPA(eglStreamConsumerOutputEXT);
#endif

#ifdef EGL_EXT_swap_buffers_with_damage
	_eglSwapBuffersWithDamageEXT = (PFNEGLSWAPBUFFERSWITHDAMAGEEXT)GPA(eglSwapBuffersWithDamageEXT);
#endif

#ifdef EGL_HI_clientpixmap
	_eglCreatePixmapSurfaceHI = (PFNEGLCREATEPIXMAPSURFACEHI)GPA(eglCreatePixmapSurfaceHI);
#endif

#ifdef EGL_KHR_cl_event2
	_eglCreateSync64KHR = (PFNEGLCREATESYNC64KHR)GPA(eglCreateSync64KHR);
#endif

#ifdef EGL_KHR_debug
	_eglDebugMessageControlKHR = (PFNEGLDEBUGMESSAGECONTROLKHR)GPA(eglDebugMessageControlKHR);
	_eglQueryDebugKHR = (PFNEGLQUERYDEBUGKHR)GPA(eglQueryDebugKHR);
	_eglLabelObjectKHR = (PFNEGLLABELOBJECTKHR)GPA(eglLabelObjectKHR);
#endif

#ifdef EGL_KHR_fence_sync
	_eglCreateSyncKHR = (PFNEGLCREATESYNCKHR)GPA(eglCreateSyncKHR);
	_eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHR)GPA(eglDestroySyncKHR);
	_eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHR)GPA(eglClientWaitSyncKHR);
	_eglGetSyncAttribKHR = (PFNEGLGETSYNCATTRIBKHR)GPA(eglGetSyncAttribKHR);
#endif

#ifdef EGL_KHR_image
	_eglCreateImageKHR = (PFNEGLCREATEIMAGEKHR)GPA(eglCreateImageKHR);
	_eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHR)GPA(eglDestroyImageKHR);
#endif

#ifdef EGL_KHR_image_base
	_eglCreateImageKHR = (PFNEGLCREATEIMAGEKHR)GPA(eglCreateImageKHR);
	_eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHR)GPA(eglDestroyImageKHR);
#endif

#ifdef EGL_KHR_lock_surface
	_eglLockSurfaceKHR = (PFNEGLLOCKSURFACEKHR)GPA(eglLockSurfaceKHR);
	_eglUnlockSurfaceKHR = (PFNEGLUNLOCKSURFACEKHR)GPA(eglUnlockSurfaceKHR);
#endif

#ifdef EGL_KHR_lock_surface3
	_eglLockSurfaceKHR = (PFNEGLLOCKSURFACEKHR)GPA(eglLockSurfaceKHR);
	_eglUnlockSurfaceKHR = (PFNEGLUNLOCKSURFACEKHR)GPA(eglUnlockSurfaceKHR);
	_eglQuerySurface64KHR = (PFNEGLQUERYSURFACE64KHR)GPA(eglQuerySurface64KHR);
#endif

#ifdef EGL_KHR_partial_update
	_eglSetDamageRegionKHR = (PFNEGLSETDAMAGEREGIONKHR)GPA(eglSetDamageRegionKHR);
#endif

#ifdef EGL_KHR_reusable_sync
	_eglCreateSyncKHR = (PFNEGLCREATESYNCKHR)GPA(eglCreateSyncKHR);
	_eglDestroySyncKHR = (PFNEGLDESTROYSYNCKHR)GPA(eglDestroySyncKHR);
	_eglClientWaitSyncKHR = (PFNEGLCLIENTWAITSYNCKHR)GPA(eglClientWaitSyncKHR);
	_eglSignalSyncKHR = (PFNEGLSIGNALSYNCKHR)GPA(eglSignalSyncKHR);
	_eglGetSyncAttribKHR = (PFNEGLGETSYNCATTRIBKHR)GPA(eglGetSyncAttribKHR);
#endif

#ifdef EGL_KHR_stream
	_eglCreateStreamKHR = (PFNEGLCREATESTREAMKHR)GPA(eglCreateStreamKHR);
	_eglDestroyStreamKHR = (PFNEGLDESTROYSTREAMKHR)GPA(eglDestroyStreamKHR);
	_eglStreamAttribKHR = (PFNEGLSTREAMATTRIBKHR)GPA(eglStreamAttribKHR);
	_eglQueryStreamKHR = (PFNEGLQUERYSTREAMKHR)GPA(eglQueryStreamKHR);
	_eglQueryStreamu64KHR = (PFNEGLQUERYSTREAMU64KHR)GPA(eglQueryStreamu64KHR);
#endif

#ifdef EGL_KHR_stream_attrib
	_eglCreateStreamAttribKHR = (PFNEGLCREATESTREAMATTRIBKHR)GPA(eglCreateStreamAttribKHR);
	_eglSetStreamAttribKHR = (PFNEGLSETSTREAMATTRIBKHR)GPA(eglSetStreamAttribKHR);
	_eglQueryStreamAttribKHR = (PFNEGLQUERYSTREAMATTRIBKHR)GPA(eglQueryStreamAttribKHR);
	_eglStreamConsumerAcquireAttribKHR = (PFNEGLSTREAMCONSUMERACQUIREATTRIBKHR)GPA(eglStreamConsumerAcquireAttribKHR);
	_eglStreamConsumerReleaseAttribKHR = (PFNEGLSTREAMCONSUMERRELEASEATTRIBKHR)GPA(eglStreamConsumerReleaseAttribKHR);
#endif

#ifdef EGL_KHR_stream_consumer_gltexture
	_eglStreamConsumerGLTextureExternalKHR = (PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALKHR)GPA(eglStreamConsumerGLTextureExternalKHR);
	_eglStreamConsumerAcquireKHR = (PFNEGLSTREAMCONSUMERACQUIREKHR)GPA(eglStreamConsumerAcquireKHR);
	_eglStreamConsumerReleaseKHR = (PFNEGLSTREAMCONSUMERRELEASEKHR)GPA(eglStreamConsumerReleaseKHR);
#endif

#ifdef EGL_KHR_stream_cross_process_fd
	_eglGetStreamFileDescriptorKHR = (PFNEGLGETSTREAMFILEDESCRIPTORKHR)GPA(eglGetStreamFileDescriptorKHR);
	_eglCreateStreamFromFileDescriptorKHR = (PFNEGLCREATESTREAMFROMFILEDESCRIPTORKHR)GPA(eglCreateStreamFromFileDescriptorKHR);
#endif

#ifdef EGL_KHR_stream_fifo
	_eglQueryStreamTimeKHR = (PFNEGLQUERYSTREAMTIMEKHR)GPA(eglQueryStreamTimeKHR);
#endif

#ifdef EGL_KHR_stream_producer_eglsurface
	_eglCreateStreamProducerSurfaceKHR = (PFNEGLCREATESTREAMPRODUCERSURFACEKHR)GPA(eglCreateStreamProducerSurfaceKHR);
#endif

#ifdef EGL_KHR_swap_buffers_with_damage
	_eglSwapBuffersWithDamageKHR = (PFNEGLSWAPBUFFERSWITHDAMAGEKHR)GPA(eglSwapBuffersWithDamageKHR);
#endif

#ifdef EGL_KHR_wait_sync
	_eglWaitSyncKHR = (PFNEGLWAITSYNCKHR)GPA(eglWaitSyncKHR);
#endif

#ifdef EGL_MESA_drm_image
	_eglCreateDRMImageMESA = (PFNEGLCREATEDRMIMAGEMESA)GPA(eglCreateDRMImageMESA);
	_eglExportDRMImageMESA = (PFNEGLEXPORTDRMIMAGEMESA)GPA(eglExportDRMImageMESA);
#endif

#ifdef EGL_MESA_image_dma_buf_export
	_eglExportDMABUFImageQueryMESA = (PFNEGLEXPORTDMABUFIMAGEQUERYMESA)GPA(eglExportDMABUFImageQueryMESA);
	_eglExportDMABUFImageMESA = (PFNEGLEXPORTDMABUFIMAGEMESA)GPA(eglExportDMABUFImageMESA);
#endif

#ifdef EGL_NOK_swap_region
	_eglSwapBuffersRegionNOK = (PFNEGLSWAPBUFFERSREGIONNOK)GPA(eglSwapBuffersRegionNOK);
#endif

#ifdef EGL_NOK_swap_region2
	_eglSwapBuffersRegion2NOK = (PFNEGLSWAPBUFFERSREGION2NOK)GPA(eglSwapBuffersRegion2NOK);
#endif

#ifdef EGL_NV_native_query
	_eglQueryNativeDisplayNV = (PFNEGLQUERYNATIVEDISPLAYNV)GPA(eglQueryNativeDisplayNV);
	_eglQueryNativeWindowNV = (PFNEGLQUERYNATIVEWINDOWNV)GPA(eglQueryNativeWindowNV);
	_eglQueryNativePixmapNV = (PFNEGLQUERYNATIVEPIXMAPNV)GPA(eglQueryNativePixmapNV);
#endif

#ifdef EGL_NV_post_sub_buffer
	_eglPostSubBufferNV = (PFNEGLPOSTSUBBUFFERNV)GPA(eglPostSubBufferNV);
#endif

#ifdef EGL_NV_stream_consumer_gltexture_yuv
	_eglStreamConsumerGLTextureExternalAttribsNV = (PFNEGLSTREAMCONSUMERGLTEXTUREEXTERNALATTRIBSNV)GPA(eglStreamConsumerGLTextureExternalAttribsNV);
#endif

#ifdef EGL_NV_stream_metadata
	_eglQueryDisplayAttribNV = (PFNEGLQUERYDISPLAYATTRIBNV)GPA(eglQueryDisplayAttribNV);
	_eglSetStreamMetadataNV = (PFNEGLSETSTREAMMETADATANV)GPA(eglSetStreamMetadataNV);
	_eglQueryStreamMetadataNV = (PFNEGLQUERYSTREAMMETADATANV)GPA(eglQueryStreamMetadataNV);
#endif

#ifdef EGL_NV_stream_reset
	_eglResetStreamNV = (PFNEGLRESETSTREAMNV)GPA(eglResetStreamNV);
#endif

#ifdef EGL_NV_stream_sync
	_eglCreateStreamSyncNV = (PFNEGLCREATESTREAMSYNCNV)GPA(eglCreateStreamSyncNV);
#endif

#ifdef EGL_NV_sync
	_eglCreateFenceSyncNV = (PFNEGLCREATEFENCESYNCNV)GPA(eglCreateFenceSyncNV);
	_eglDestroySyncNV = (PFNEGLDESTROYSYNCNV)GPA(eglDestroySyncNV);
	_eglFenceNV = (PFNEGLFENCENV)GPA(eglFenceNV);
	_eglClientWaitSyncNV = (PFNEGLCLIENTWAITSYNCNV)GPA(eglClientWaitSyncNV);
	_eglSignalSyncNV = (PFNEGLSIGNALSYNCNV)GPA(eglSignalSyncNV);
	_eglGetSyncAttribNV = (PFNEGLGETSYNCATTRIBNV)GPA(eglGetSyncAttribNV);
#endif

#ifdef EGL_NV_system_time
	_eglGetSystemTimeFrequencyNV = (PFNEGLGETSYSTEMTIMEFREQUENCYNV)GPA(eglGetSystemTimeFrequencyNV);
	_eglGetSystemTimeNV = (PFNEGLGETSYSTEMTIMENV)GPA(eglGetSystemTimeNV);
#endif

	gegl_rebind(enableDebug);

	geglext_str = (const char *)_eglQueryString(display, EGL_EXTENSIONS);
	memset(&geglext, 0, sizeof(geglext));
	if (gegl_check_extension("EGL_ANDROID_blob_cache")) geglext._EGL_ANDROID_blob_cache = 1;
	if (gegl_check_extension("EGL_ANDROID_create_native_client_buffer")) geglext._EGL_ANDROID_create_native_client_buffer = 1;
	if (gegl_check_extension("EGL_ANDROID_framebuffer_target")) geglext._EGL_ANDROID_framebuffer_target = 1;
	if (gegl_check_extension("EGL_ANDROID_front_buffer_auto_refresh")) geglext._EGL_ANDROID_front_buffer_auto_refresh = 1;
	if (gegl_check_extension("EGL_ANDROID_image_native_buffer")) geglext._EGL_ANDROID_image_native_buffer = 1;
	if (gegl_check_extension("EGL_ANDROID_native_fence_sync")) geglext._EGL_ANDROID_native_fence_sync = 1;
	if (gegl_check_extension("EGL_ANDROID_presentation_time")) geglext._EGL_ANDROID_presentation_time = 1;
	if (gegl_check_extension("EGL_ANDROID_recordable")) geglext._EGL_ANDROID_recordable = 1;
	if (gegl_check_extension("EGL_ANGLE_d3d_share_handle_client_buffer")) geglext._EGL_ANGLE_d3d_share_handle_client_buffer = 1;
	if (gegl_check_extension("EGL_ANGLE_device_d3d")) geglext._EGL_ANGLE_device_d3d = 1;
	if (gegl_check_extension("EGL_ANGLE_query_surface_pointer")) geglext._EGL_ANGLE_query_surface_pointer = 1;
	if (gegl_check_extension("EGL_ANGLE_surface_d3d_texture_2d_share_handle")) geglext._EGL_ANGLE_surface_d3d_texture_2d_share_handle = 1;
	if (gegl_check_extension("EGL_ANGLE_window_fixed_size")) geglext._EGL_ANGLE_window_fixed_size = 1;
	if (gegl_check_extension("EGL_ARM_implicit_external_sync")) geglext._EGL_ARM_implicit_external_sync = 1;
	if (gegl_check_extension("EGL_ARM_pixmap_multisample_discard")) geglext._EGL_ARM_pixmap_multisample_discard = 1;
	if (gegl_check_extension("EGL_EXT_buffer_age")) geglext._EGL_EXT_buffer_age = 1;
	if (gegl_check_extension("EGL_EXT_client_extensions")) geglext._EGL_EXT_client_extensions = 1;
	if (gegl_check_extension("EGL_EXT_create_context_robustness")) geglext._EGL_EXT_create_context_robustness = 1;
	if (gegl_check_extension("EGL_EXT_device_base")) geglext._EGL_EXT_device_base = 1;
	if (gegl_check_extension("EGL_EXT_device_drm")) geglext._EGL_EXT_device_drm = 1;
	if (gegl_check_extension("EGL_EXT_device_enumeration")) geglext._EGL_EXT_device_enumeration = 1;
	if (gegl_check_extension("EGL_EXT_device_openwf")) geglext._EGL_EXT_device_openwf = 1;
	if (gegl_check_extension("EGL_EXT_device_query")) geglext._EGL_EXT_device_query = 1;
	if (gegl_check_extension("EGL_EXT_gl_colorspace_bt2020_linear")) geglext._EGL_EXT_gl_colorspace_bt2020_linear = 1;
	if (gegl_check_extension("EGL_EXT_gl_colorspace_bt2020_pq")) geglext._EGL_EXT_gl_colorspace_bt2020_pq = 1;
	if (gegl_check_extension("EGL_EXT_gl_colorspace_scrgb_linear")) geglext._EGL_EXT_gl_colorspace_scrgb_linear = 1;
	if (gegl_check_extension("EGL_EXT_image_dma_buf_import")) geglext._EGL_EXT_image_dma_buf_import = 1;
	if (gegl_check_extension("EGL_EXT_image_dma_buf_import_modifiers")) geglext._EGL_EXT_image_dma_buf_import_modifiers = 1;
	if (gegl_check_extension("EGL_EXT_multiview_window")) geglext._EGL_EXT_multiview_window = 1;
	if (gegl_check_extension("EGL_EXT_output_base")) geglext._EGL_EXT_output_base = 1;
	if (gegl_check_extension("EGL_EXT_output_drm")) geglext._EGL_EXT_output_drm = 1;
	if (gegl_check_extension("EGL_EXT_output_openwf")) geglext._EGL_EXT_output_openwf = 1;
	if (gegl_check_extension("EGL_EXT_pixel_format_float")) geglext._EGL_EXT_pixel_format_float = 1;
	if (gegl_check_extension("EGL_EXT_platform_base")) geglext._EGL_EXT_platform_base = 1;
	if (gegl_check_extension("EGL_EXT_platform_device")) geglext._EGL_EXT_platform_device = 1;
	if (gegl_check_extension("EGL_EXT_platform_wayland")) geglext._EGL_EXT_platform_wayland = 1;
	if (gegl_check_extension("EGL_EXT_platform_x11")) geglext._EGL_EXT_platform_x11 = 1;
	if (gegl_check_extension("EGL_EXT_protected_content")) geglext._EGL_EXT_protected_content = 1;
	if (gegl_check_extension("EGL_EXT_protected_surface")) geglext._EGL_EXT_protected_surface = 1;
	if (gegl_check_extension("EGL_EXT_stream_consumer_egloutput")) geglext._EGL_EXT_stream_consumer_egloutput = 1;
	if (gegl_check_extension("EGL_EXT_surface_SMPTE2086_metadata")) geglext._EGL_EXT_surface_SMPTE2086_metadata = 1;
	if (gegl_check_extension("EGL_EXT_swap_buffers_with_damage")) geglext._EGL_EXT_swap_buffers_with_damage = 1;
	if (gegl_check_extension("EGL_EXT_yuv_surface")) geglext._EGL_EXT_yuv_surface = 1;
	if (gegl_check_extension("EGL_HI_clientpixmap")) geglext._EGL_HI_clientpixmap = 1;
	if (gegl_check_extension("EGL_HI_colorformats")) geglext._EGL_HI_colorformats = 1;
	if (gegl_check_extension("EGL_IMG_context_priority")) geglext._EGL_IMG_context_priority = 1;
	if (gegl_check_extension("EGL_IMG_image_plane_attribs")) geglext._EGL_IMG_image_plane_attribs = 1;
	if (gegl_check_extension("EGL_KHR_cl_event")) geglext._EGL_KHR_cl_event = 1;
	if (gegl_check_extension("EGL_KHR_cl_event2")) geglext._EGL_KHR_cl_event2 = 1;
	if (gegl_check_extension("EGL_KHR_config_attribs")) geglext._EGL_KHR_config_attribs = 1;
	if (gegl_check_extension("EGL_KHR_client_get_all_proc_addresses")) geglext._EGL_KHR_client_get_all_proc_addresses = 1;
	if (gegl_check_extension("EGL_KHR_context_flush_control")) geglext._EGL_KHR_context_flush_control = 1;
	if (gegl_check_extension("EGL_KHR_create_context")) geglext._EGL_KHR_create_context = 1;
	if (gegl_check_extension("EGL_KHR_create_context_no_error")) geglext._EGL_KHR_create_context_no_error = 1;
	if (gegl_check_extension("EGL_KHR_debug")) geglext._EGL_KHR_debug = 1;
	if (gegl_check_extension("EGL_KHR_fence_sync")) geglext._EGL_KHR_fence_sync = 1;
	if (gegl_check_extension("EGL_KHR_get_all_proc_addresses")) geglext._EGL_KHR_get_all_proc_addresses = 1;
	if (gegl_check_extension("EGL_KHR_gl_colorspace")) geglext._EGL_KHR_gl_colorspace = 1;
	if (gegl_check_extension("EGL_KHR_gl_renderbuffer_image")) geglext._EGL_KHR_gl_renderbuffer_image = 1;
	if (gegl_check_extension("EGL_KHR_gl_texture_2D_image")) geglext._EGL_KHR_gl_texture_2D_image = 1;
	if (gegl_check_extension("EGL_KHR_gl_texture_3D_image")) geglext._EGL_KHR_gl_texture_3D_image = 1;
	if (gegl_check_extension("EGL_KHR_gl_texture_cubemap_image")) geglext._EGL_KHR_gl_texture_cubemap_image = 1;
	if (gegl_check_extension("EGL_KHR_image")) geglext._EGL_KHR_image = 1;
	if (gegl_check_extension("EGL_KHR_image_base")) geglext._EGL_KHR_image_base = 1;
	if (gegl_check_extension("EGL_KHR_image_pixmap")) geglext._EGL_KHR_image_pixmap = 1;
	if (gegl_check_extension("EGL_KHR_lock_surface")) geglext._EGL_KHR_lock_surface = 1;
	if (gegl_check_extension("EGL_KHR_lock_surface2")) geglext._EGL_KHR_lock_surface2 = 1;
	if (gegl_check_extension("EGL_KHR_lock_surface3")) geglext._EGL_KHR_lock_surface3 = 1;
	if (gegl_check_extension("EGL_KHR_mutable_render_buffer")) geglext._EGL_KHR_mutable_render_buffer = 1;
	if (gegl_check_extension("EGL_KHR_no_config_context")) geglext._EGL_KHR_no_config_context = 1;
	if (gegl_check_extension("EGL_KHR_partial_update")) geglext._EGL_KHR_partial_update = 1;
	if (gegl_check_extension("EGL_KHR_platform_android")) geglext._EGL_KHR_platform_android = 1;
	if (gegl_check_extension("EGL_KHR_platform_gbm")) geglext._EGL_KHR_platform_gbm = 1;
	if (gegl_check_extension("EGL_KHR_platform_wayland")) geglext._EGL_KHR_platform_wayland = 1;
	if (gegl_check_extension("EGL_KHR_platform_x11")) geglext._EGL_KHR_platform_x11 = 1;
	if (gegl_check_extension("EGL_KHR_reusable_sync")) geglext._EGL_KHR_reusable_sync = 1;
	if (gegl_check_extension("EGL_KHR_stream")) geglext._EGL_KHR_stream = 1;
	if (gegl_check_extension("EGL_KHR_stream_attrib")) geglext._EGL_KHR_stream_attrib = 1;
	if (gegl_check_extension("EGL_KHR_stream_consumer_gltexture")) geglext._EGL_KHR_stream_consumer_gltexture = 1;
	if (gegl_check_extension("EGL_KHR_stream_cross_process_fd")) geglext._EGL_KHR_stream_cross_process_fd = 1;
	if (gegl_check_extension("EGL_KHR_stream_fifo")) geglext._EGL_KHR_stream_fifo = 1;
	if (gegl_check_extension("EGL_KHR_stream_producer_aldatalocator")) geglext._EGL_KHR_stream_producer_aldatalocator = 1;
	if (gegl_check_extension("EGL_KHR_stream_producer_eglsurface")) geglext._EGL_KHR_stream_producer_eglsurface = 1;
	if (gegl_check_extension("EGL_KHR_surfaceless_context")) geglext._EGL_KHR_surfaceless_context = 1;
	if (gegl_check_extension("EGL_KHR_swap_buffers_with_damage")) geglext._EGL_KHR_swap_buffers_with_damage = 1;
	if (gegl_check_extension("EGL_KHR_vg_parent_image")) geglext._EGL_KHR_vg_parent_image = 1;
	if (gegl_check_extension("EGL_KHR_wait_sync")) geglext._EGL_KHR_wait_sync = 1;
	if (gegl_check_extension("EGL_MESA_drm_image")) geglext._EGL_MESA_drm_image = 1;
	if (gegl_check_extension("EGL_MESA_image_dma_buf_export")) geglext._EGL_MESA_image_dma_buf_export = 1;
	if (gegl_check_extension("EGL_MESA_platform_gbm")) geglext._EGL_MESA_platform_gbm = 1;
	if (gegl_check_extension("EGL_MESA_platform_surfaceless")) geglext._EGL_MESA_platform_surfaceless = 1;
	if (gegl_check_extension("EGL_NOK_swap_region")) geglext._EGL_NOK_swap_region = 1;
	if (gegl_check_extension("EGL_NOK_swap_region2")) geglext._EGL_NOK_swap_region2 = 1;
	if (gegl_check_extension("EGL_NOK_texture_from_pixmap")) geglext._EGL_NOK_texture_from_pixmap = 1;
	if (gegl_check_extension("EGL_NV_3dvision_surface")) geglext._EGL_NV_3dvision_surface = 1;
	if (gegl_check_extension("EGL_NV_coverage_sample")) geglext._EGL_NV_coverage_sample = 1;
	if (gegl_check_extension("EGL_NV_coverage_sample_resolve")) geglext._EGL_NV_coverage_sample_resolve = 1;
	if (gegl_check_extension("EGL_NV_cuda_event")) geglext._EGL_NV_cuda_event = 1;
	if (gegl_check_extension("EGL_NV_depth_nonlinear")) geglext._EGL_NV_depth_nonlinear = 1;
	if (gegl_check_extension("EGL_NV_device_cuda")) geglext._EGL_NV_device_cuda = 1;
	if (gegl_check_extension("EGL_NV_native_query")) geglext._EGL_NV_native_query = 1;
	if (gegl_check_extension("EGL_NV_post_convert_rounding")) geglext._EGL_NV_post_convert_rounding = 1;
	if (gegl_check_extension("EGL_NV_post_sub_buffer")) geglext._EGL_NV_post_sub_buffer = 1;
	if (gegl_check_extension("EGL_NV_robustness_video_memory_purge")) geglext._EGL_NV_robustness_video_memory_purge = 1;
	if (gegl_check_extension("EGL_NV_stream_consumer_gltexture_yuv")) geglext._EGL_NV_stream_consumer_gltexture_yuv = 1;
	if (gegl_check_extension("EGL_NV_stream_cross_object")) geglext._EGL_NV_stream_cross_object = 1;
	if (gegl_check_extension("EGL_NV_stream_cross_display")) geglext._EGL_NV_stream_cross_display = 1;
	if (gegl_check_extension("EGL_NV_stream_cross_partition")) geglext._EGL_NV_stream_cross_partition = 1;
	if (gegl_check_extension("EGL_NV_stream_cross_process")) geglext._EGL_NV_stream_cross_process = 1;
	if (gegl_check_extension("EGL_NV_stream_cross_system")) geglext._EGL_NV_stream_cross_system = 1;
	if (gegl_check_extension("EGL_NV_stream_fifo_next")) geglext._EGL_NV_stream_fifo_next = 1;
	if (gegl_check_extension("EGL_NV_stream_fifo_synchronous")) geglext._EGL_NV_stream_fifo_synchronous = 1;
	if (gegl_check_extension("EGL_NV_stream_frame_limits")) geglext._EGL_NV_stream_frame_limits = 1;
	if (gegl_check_extension("EGL_NV_stream_metadata")) geglext._EGL_NV_stream_metadata = 1;
	if (gegl_check_extension("EGL_NV_stream_reset")) geglext._EGL_NV_stream_reset = 1;
	if (gegl_check_extension("EGL_NV_stream_remote")) geglext._EGL_NV_stream_remote = 1;
	if (gegl_check_extension("EGL_NV_stream_socket")) geglext._EGL_NV_stream_socket = 1;
	if (gegl_check_extension("EGL_NV_stream_socket_inet")) geglext._EGL_NV_stream_socket_inet = 1;
	if (gegl_check_extension("EGL_NV_stream_socket_unix")) geglext._EGL_NV_stream_socket_unix = 1;
	if (gegl_check_extension("EGL_NV_stream_sync")) geglext._EGL_NV_stream_sync = 1;
	if (gegl_check_extension("EGL_NV_sync")) geglext._EGL_NV_sync = 1;
	if (gegl_check_extension("EGL_NV_system_time")) geglext._EGL_NV_system_time = 1;
	if (gegl_check_extension("EGL_TIZEN_image_native_buffer")) geglext._EGL_TIZEN_image_native_buffer = 1;
	if (gegl_check_extension("EGL_TIZEN_image_native_surface")) geglext._EGL_TIZEN_image_native_surface = 1;
}

void gegl_rebind(int enableDebug) {
	if (!enableDebug) {
		geglBindAPI = _eglBindAPI;
		geglBindTexImage = _eglBindTexImage;
		geglChooseConfig = _eglChooseConfig;
		geglClientWaitSync = _eglClientWaitSync;
		geglClientWaitSyncKHR = _eglClientWaitSyncKHR;
		geglClientWaitSyncNV = _eglClientWaitSyncNV;
		geglCopyBuffers = _eglCopyBuffers;
		geglCreateContext = _eglCreateContext;
		geglCreateDRMImageMESA = _eglCreateDRMImageMESA;
		geglCreateFenceSyncNV = _eglCreateFenceSyncNV;
		geglCreateImage = _eglCreateImage;
		geglCreateImageKHR = _eglCreateImageKHR;
		geglCreateNativeClientBufferANDROID = _eglCreateNativeClientBufferANDROID;
		geglCreatePbufferFromClientBuffer = _eglCreatePbufferFromClientBuffer;
		geglCreatePbufferSurface = _eglCreatePbufferSurface;
		geglCreatePixmapSurface = _eglCreatePixmapSurface;
		geglCreatePixmapSurfaceHI = _eglCreatePixmapSurfaceHI;
		geglCreatePlatformPixmapSurface = _eglCreatePlatformPixmapSurface;
		geglCreatePlatformPixmapSurfaceEXT = _eglCreatePlatformPixmapSurfaceEXT;
		geglCreatePlatformWindowSurface = _eglCreatePlatformWindowSurface;
		geglCreatePlatformWindowSurfaceEXT = _eglCreatePlatformWindowSurfaceEXT;
		geglCreateStreamFromFileDescriptorKHR = _eglCreateStreamFromFileDescriptorKHR;
		geglCreateStreamKHR = _eglCreateStreamKHR;
		geglCreateStreamAttribKHR = _eglCreateStreamAttribKHR;
		geglCreateStreamProducerSurfaceKHR = _eglCreateStreamProducerSurfaceKHR;
		geglCreateStreamSyncNV = _eglCreateStreamSyncNV;
		geglCreateSync = _eglCreateSync;
		geglCreateSyncKHR = _eglCreateSyncKHR;
		geglCreateSync64KHR = _eglCreateSync64KHR;
		geglCreateWindowSurface = _eglCreateWindowSurface;
		geglDebugMessageControlKHR = _eglDebugMessageControlKHR;
		geglDestroyContext = _eglDestroyContext;
		geglDestroyImage = _eglDestroyImage;
		geglDestroyImageKHR = _eglDestroyImageKHR;
		geglDestroyStreamKHR = _eglDestroyStreamKHR;
		geglDestroySurface = _eglDestroySurface;
		geglDestroySync = _eglDestroySync;
		geglDestroySyncKHR = _eglDestroySyncKHR;
		geglDestroySyncNV = _eglDestroySyncNV;
		geglDupNativeFenceFDANDROID = _eglDupNativeFenceFDANDROID;
		geglExportDMABUFImageMESA = _eglExportDMABUFImageMESA;
		geglExportDMABUFImageQueryMESA = _eglExportDMABUFImageQueryMESA;
		geglExportDRMImageMESA = _eglExportDRMImageMESA;
		geglFenceNV = _eglFenceNV;
		geglGetConfigAttrib = _eglGetConfigAttrib;
		geglGetConfigs = _eglGetConfigs;
		geglGetCurrentContext = _eglGetCurrentContext;
		geglGetCurrentDisplay = _eglGetCurrentDisplay;
		geglGetCurrentSurface = _eglGetCurrentSurface;
		geglGetDisplay = _eglGetDisplay;
		geglGetError = _eglGetError;
		geglGetOutputLayersEXT = _eglGetOutputLayersEXT;
		geglGetOutputPortsEXT = _eglGetOutputPortsEXT;
		geglGetPlatformDisplay = _eglGetPlatformDisplay;
		geglGetPlatformDisplayEXT = _eglGetPlatformDisplayEXT;
		geglGetProcAddress = _eglGetProcAddress;
		geglGetStreamFileDescriptorKHR = _eglGetStreamFileDescriptorKHR;
		geglGetSyncAttrib = _eglGetSyncAttrib;
		geglGetSyncAttribKHR = _eglGetSyncAttribKHR;
		geglGetSyncAttribNV = _eglGetSyncAttribNV;
		geglGetSystemTimeFrequencyNV = _eglGetSystemTimeFrequencyNV;
		geglGetSystemTimeNV = _eglGetSystemTimeNV;
		geglInitialize = _eglInitialize;
		geglLabelObjectKHR = _eglLabelObjectKHR;
		geglLockSurfaceKHR = _eglLockSurfaceKHR;
		geglMakeCurrent = _eglMakeCurrent;
		geglOutputLayerAttribEXT = _eglOutputLayerAttribEXT;
		geglOutputPortAttribEXT = _eglOutputPortAttribEXT;
		geglPostSubBufferNV = _eglPostSubBufferNV;
		geglPresentationTimeANDROID = _eglPresentationTimeANDROID;
		geglQueryAPI = _eglQueryAPI;
		geglQueryContext = _eglQueryContext;
		geglQueryDebugKHR = _eglQueryDebugKHR;
		geglQueryDeviceAttribEXT = _eglQueryDeviceAttribEXT;
		geglQueryDeviceStringEXT = _eglQueryDeviceStringEXT;
		geglQueryDevicesEXT = _eglQueryDevicesEXT;
		geglQueryDisplayAttribEXT = _eglQueryDisplayAttribEXT;
		geglQueryDisplayAttribNV = _eglQueryDisplayAttribNV;
		geglQueryDmaBufFormatsEXT = _eglQueryDmaBufFormatsEXT;
		geglQueryDmaBufModifiersEXT = _eglQueryDmaBufModifiersEXT;
		geglQueryNativeDisplayNV = _eglQueryNativeDisplayNV;
		geglQueryNativePixmapNV = _eglQueryNativePixmapNV;
		geglQueryNativeWindowNV = _eglQueryNativeWindowNV;
		geglQueryOutputLayerAttribEXT = _eglQueryOutputLayerAttribEXT;
		geglQueryOutputLayerStringEXT = _eglQueryOutputLayerStringEXT;
		geglQueryOutputPortAttribEXT = _eglQueryOutputPortAttribEXT;
		geglQueryOutputPortStringEXT = _eglQueryOutputPortStringEXT;
		geglQueryStreamKHR = _eglQueryStreamKHR;
		geglQueryStreamAttribKHR = _eglQueryStreamAttribKHR;
		geglQueryStreamMetadataNV = _eglQueryStreamMetadataNV;
		geglQueryStreamTimeKHR = _eglQueryStreamTimeKHR;
		geglQueryStreamu64KHR = _eglQueryStreamu64KHR;
		geglQueryString = _eglQueryString;
		geglQuerySurface = _eglQuerySurface;
		geglQuerySurface64KHR = _eglQuerySurface64KHR;
		geglQuerySurfacePointerANGLE = _eglQuerySurfacePointerANGLE;
		geglReleaseTexImage = _eglReleaseTexImage;
		geglReleaseThread = _eglReleaseThread;
		geglResetStreamNV = _eglResetStreamNV;
		geglSetBlobCacheFuncsANDROID = _eglSetBlobCacheFuncsANDROID;
		geglSetDamageRegionKHR = _eglSetDamageRegionKHR;
		geglSetStreamAttribKHR = _eglSetStreamAttribKHR;
		geglSetStreamMetadataNV = _eglSetStreamMetadataNV;
		geglSignalSyncKHR = _eglSignalSyncKHR;
		geglSignalSyncNV = _eglSignalSyncNV;
		geglStreamAttribKHR = _eglStreamAttribKHR;
		geglStreamConsumerAcquireKHR = _eglStreamConsumerAcquireKHR;
		geglStreamConsumerAcquireAttribKHR = _eglStreamConsumerAcquireAttribKHR;
		geglStreamConsumerGLTextureExternalKHR = _eglStreamConsumerGLTextureExternalKHR;
		geglStreamConsumerGLTextureExternalAttribsNV = _eglStreamConsumerGLTextureExternalAttribsNV;
		geglStreamConsumerOutputEXT = _eglStreamConsumerOutputEXT;
		geglStreamConsumerReleaseKHR = _eglStreamConsumerReleaseKHR;
		geglStreamConsumerReleaseAttribKHR = _eglStreamConsumerReleaseAttribKHR;
		geglSurfaceAttrib = _eglSurfaceAttrib;
		geglSwapBuffers = _eglSwapBuffers;
		geglSwapBuffersWithDamageEXT = _eglSwapBuffersWithDamageEXT;
		geglSwapBuffersWithDamageKHR = _eglSwapBuffersWithDamageKHR;
		geglSwapBuffersRegionNOK = _eglSwapBuffersRegionNOK;
		geglSwapBuffersRegion2NOK = _eglSwapBuffersRegion2NOK;
		geglSwapInterval = _eglSwapInterval;
		geglTerminate = _eglTerminate;
		geglUnlockSurfaceKHR = _eglUnlockSurfaceKHR;
		geglWaitClient = _eglWaitClient;
		geglWaitGL = _eglWaitGL;
		geglWaitNative = _eglWaitNative;
		geglWaitSync = _eglWaitSync;
		geglWaitSyncKHR = _eglWaitSyncKHR;
	} else {
		geglBindAPI = d_eglBindAPI;
		geglBindTexImage = d_eglBindTexImage;
		geglChooseConfig = d_eglChooseConfig;
		geglClientWaitSync = d_eglClientWaitSync;
		geglClientWaitSyncKHR = d_eglClientWaitSyncKHR;
		geglClientWaitSyncNV = d_eglClientWaitSyncNV;
		geglCopyBuffers = d_eglCopyBuffers;
		geglCreateContext = d_eglCreateContext;
		geglCreateDRMImageMESA = d_eglCreateDRMImageMESA;
		geglCreateFenceSyncNV = d_eglCreateFenceSyncNV;
		geglCreateImage = d_eglCreateImage;
		geglCreateImageKHR = d_eglCreateImageKHR;
		geglCreateNativeClientBufferANDROID = d_eglCreateNativeClientBufferANDROID;
		geglCreatePbufferFromClientBuffer = d_eglCreatePbufferFromClientBuffer;
		geglCreatePbufferSurface = d_eglCreatePbufferSurface;
		geglCreatePixmapSurface = d_eglCreatePixmapSurface;
		geglCreatePixmapSurfaceHI = d_eglCreatePixmapSurfaceHI;
		geglCreatePlatformPixmapSurface = d_eglCreatePlatformPixmapSurface;
		geglCreatePlatformPixmapSurfaceEXT = d_eglCreatePlatformPixmapSurfaceEXT;
		geglCreatePlatformWindowSurface = d_eglCreatePlatformWindowSurface;
		geglCreatePlatformWindowSurfaceEXT = d_eglCreatePlatformWindowSurfaceEXT;
		geglCreateStreamFromFileDescriptorKHR = d_eglCreateStreamFromFileDescriptorKHR;
		geglCreateStreamKHR = d_eglCreateStreamKHR;
		geglCreateStreamAttribKHR = d_eglCreateStreamAttribKHR;
		geglCreateStreamProducerSurfaceKHR = d_eglCreateStreamProducerSurfaceKHR;
		geglCreateStreamSyncNV = d_eglCreateStreamSyncNV;
		geglCreateSync = d_eglCreateSync;
		geglCreateSyncKHR = d_eglCreateSyncKHR;
		geglCreateSync64KHR = d_eglCreateSync64KHR;
		geglCreateWindowSurface = d_eglCreateWindowSurface;
		geglDebugMessageControlKHR = d_eglDebugMessageControlKHR;
		geglDestroyContext = d_eglDestroyContext;
		geglDestroyImage = d_eglDestroyImage;
		geglDestroyImageKHR = d_eglDestroyImageKHR;
		geglDestroyStreamKHR = d_eglDestroyStreamKHR;
		geglDestroySurface = d_eglDestroySurface;
		geglDestroySync = d_eglDestroySync;
		geglDestroySyncKHR = d_eglDestroySyncKHR;
		geglDestroySyncNV = d_eglDestroySyncNV;
		geglDupNativeFenceFDANDROID = d_eglDupNativeFenceFDANDROID;
		geglExportDMABUFImageMESA = d_eglExportDMABUFImageMESA;
		geglExportDMABUFImageQueryMESA = d_eglExportDMABUFImageQueryMESA;
		geglExportDRMImageMESA = d_eglExportDRMImageMESA;
		geglFenceNV = d_eglFenceNV;
		geglGetConfigAttrib = d_eglGetConfigAttrib;
		geglGetConfigs = d_eglGetConfigs;
		geglGetCurrentContext = d_eglGetCurrentContext;
		geglGetCurrentDisplay = d_eglGetCurrentDisplay;
		geglGetCurrentSurface = d_eglGetCurrentSurface;
		geglGetDisplay = d_eglGetDisplay;
		geglGetError = d_eglGetError;
		geglGetOutputLayersEXT = d_eglGetOutputLayersEXT;
		geglGetOutputPortsEXT = d_eglGetOutputPortsEXT;
		geglGetPlatformDisplay = d_eglGetPlatformDisplay;
		geglGetPlatformDisplayEXT = d_eglGetPlatformDisplayEXT;
		geglGetProcAddress = d_eglGetProcAddress;
		geglGetStreamFileDescriptorKHR = d_eglGetStreamFileDescriptorKHR;
		geglGetSyncAttrib = d_eglGetSyncAttrib;
		geglGetSyncAttribKHR = d_eglGetSyncAttribKHR;
		geglGetSyncAttribNV = d_eglGetSyncAttribNV;
		geglGetSystemTimeFrequencyNV = d_eglGetSystemTimeFrequencyNV;
		geglGetSystemTimeNV = d_eglGetSystemTimeNV;
		geglInitialize = d_eglInitialize;
		geglLabelObjectKHR = d_eglLabelObjectKHR;
		geglLockSurfaceKHR = d_eglLockSurfaceKHR;
		geglMakeCurrent = d_eglMakeCurrent;
		geglOutputLayerAttribEXT = d_eglOutputLayerAttribEXT;
		geglOutputPortAttribEXT = d_eglOutputPortAttribEXT;
		geglPostSubBufferNV = d_eglPostSubBufferNV;
		geglPresentationTimeANDROID = d_eglPresentationTimeANDROID;
		geglQueryAPI = d_eglQueryAPI;
		geglQueryContext = d_eglQueryContext;
		geglQueryDebugKHR = d_eglQueryDebugKHR;
		geglQueryDeviceAttribEXT = d_eglQueryDeviceAttribEXT;
		geglQueryDeviceStringEXT = d_eglQueryDeviceStringEXT;
		geglQueryDevicesEXT = d_eglQueryDevicesEXT;
		geglQueryDisplayAttribEXT = d_eglQueryDisplayAttribEXT;
		geglQueryDisplayAttribNV = d_eglQueryDisplayAttribNV;
		geglQueryDmaBufFormatsEXT = d_eglQueryDmaBufFormatsEXT;
		geglQueryDmaBufModifiersEXT = d_eglQueryDmaBufModifiersEXT;
		geglQueryNativeDisplayNV = d_eglQueryNativeDisplayNV;
		geglQueryNativePixmapNV = d_eglQueryNativePixmapNV;
		geglQueryNativeWindowNV = d_eglQueryNativeWindowNV;
		geglQueryOutputLayerAttribEXT = d_eglQueryOutputLayerAttribEXT;
		geglQueryOutputLayerStringEXT = d_eglQueryOutputLayerStringEXT;
		geglQueryOutputPortAttribEXT = d_eglQueryOutputPortAttribEXT;
		geglQueryOutputPortStringEXT = d_eglQueryOutputPortStringEXT;
		geglQueryStreamKHR = d_eglQueryStreamKHR;
		geglQueryStreamAttribKHR = d_eglQueryStreamAttribKHR;
		geglQueryStreamMetadataNV = d_eglQueryStreamMetadataNV;
		geglQueryStreamTimeKHR = d_eglQueryStreamTimeKHR;
		geglQueryStreamu64KHR = d_eglQueryStreamu64KHR;
		geglQueryString = d_eglQueryString;
		geglQuerySurface = d_eglQuerySurface;
		geglQuerySurface64KHR = d_eglQuerySurface64KHR;
		geglQuerySurfacePointerANGLE = d_eglQuerySurfacePointerANGLE;
		geglReleaseTexImage = d_eglReleaseTexImage;
		geglReleaseThread = d_eglReleaseThread;
		geglResetStreamNV = d_eglResetStreamNV;
		geglSetBlobCacheFuncsANDROID = d_eglSetBlobCacheFuncsANDROID;
		geglSetDamageRegionKHR = d_eglSetDamageRegionKHR;
		geglSetStreamAttribKHR = d_eglSetStreamAttribKHR;
		geglSetStreamMetadataNV = d_eglSetStreamMetadataNV;
		geglSignalSyncKHR = d_eglSignalSyncKHR;
		geglSignalSyncNV = d_eglSignalSyncNV;
		geglStreamAttribKHR = d_eglStreamAttribKHR;
		geglStreamConsumerAcquireKHR = d_eglStreamConsumerAcquireKHR;
		geglStreamConsumerAcquireAttribKHR = d_eglStreamConsumerAcquireAttribKHR;
		geglStreamConsumerGLTextureExternalKHR = d_eglStreamConsumerGLTextureExternalKHR;
		geglStreamConsumerGLTextureExternalAttribsNV = d_eglStreamConsumerGLTextureExternalAttribsNV;
		geglStreamConsumerOutputEXT = d_eglStreamConsumerOutputEXT;
		geglStreamConsumerReleaseKHR = d_eglStreamConsumerReleaseKHR;
		geglStreamConsumerReleaseAttribKHR = d_eglStreamConsumerReleaseAttribKHR;
		geglSurfaceAttrib = d_eglSurfaceAttrib;
		geglSwapBuffers = d_eglSwapBuffers;
		geglSwapBuffersWithDamageEXT = d_eglSwapBuffersWithDamageEXT;
		geglSwapBuffersWithDamageKHR = d_eglSwapBuffersWithDamageKHR;
		geglSwapBuffersRegionNOK = d_eglSwapBuffersRegionNOK;
		geglSwapBuffersRegion2NOK = d_eglSwapBuffersRegion2NOK;
		geglSwapInterval = d_eglSwapInterval;
		geglTerminate = d_eglTerminate;
		geglUnlockSurfaceKHR = d_eglUnlockSurfaceKHR;
		geglWaitClient = d_eglWaitClient;
		geglWaitGL = d_eglWaitGL;
		geglWaitNative = d_eglWaitNative;
		geglWaitSync = d_eglWaitSync;
		geglWaitSyncKHR = d_eglWaitSyncKHR;
	}
}
