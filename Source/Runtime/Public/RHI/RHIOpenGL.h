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

/*
===============================================================================

    OpenGL Rendering Hardware Interface

===============================================================================
*/

#include "Containers/Array.h"
#include "Image/Image.h"
#include "RHI.h"

BE_NAMESPACE_BEGIN

class Rect;
class Vec2;
class Vec4;
class Vec4;
class Color4;
class Mat2;
class Mat3;
class Mat4;

struct GLContext;
struct GLStencilState;
struct GLBuffer;
struct GLTexture;
struct GLShader;
struct GLVertexFormat;
struct GLRenderTarget;
struct GLQuery;
struct GLSync;

class OpenGLRHI : public RHI {
public:
    OpenGLRHI();

    void                    Init(WindowHandle windowHandle, const Settings *settings);
    void                    Shutdown();

    bool                    IsInitialized() const { return initialized; }

    bool                    SupportsPolygonMode() const;
    bool                    SupportsPackedFloat() const;
    bool                    SupportsDepthBufferFloat() const;
    bool                    SupportsPixelBufferObject() const;
    bool                    SupportsTextureRectangle() const;
    bool                    SupportsTextureArray() const;
    bool                    SupportsTextureBufferObject() const;
    bool                    SupportsTextureCompressionS3TC() const;
    bool                    SupportsTextureCompressionLATC() const;
    bool                    SupportsTextureCompressionETC2() const;
    bool                    SupportsInstancedArrays() const;
    bool                    SupportsBufferStorage() const;
    bool                    SupportsMultiDrawIndirect() const;
    bool                    SupportsDebugLabel() const;

    Handle                  CreateContext(WindowHandle windowHandle, bool useSharedContext);
    void                    DestroyContext(Handle ctxHandle);
    void                    ActivateSurface(Handle ctxHandle, WindowHandle windowHandle);
    void                    DeactivateSurface(Handle ctxHandle);
    void                    SetContext(Handle ctxHandle);
    void                    SetContextDisplayFunc(Handle ctxHandle, DisplayContextFunc displayFunc, void *dataPtr, bool onDemandDrawing);
    void                    DisplayContext(Handle ctxHandle);
    WindowHandle            GetWindowHandleFromContext(Handle ctxHandle);
    void                    GetDisplayMetrics(Handle ctxHandle, DisplayMetrics *displayMetrics) const;

    bool                    IsFullscreen() const;
    bool                    SetFullscreen(Handle windowHandle, int width, int height);
    void                    ResetFullscreen(Handle windowHandle);

    void                    GetGammaRamp(unsigned short ramp[768]) const;
    void                    SetGammaRamp(unsigned short ramp[768]) const;

    bool                    SwapBuffers();
    void                    SwapInterval(int interval) const;

    void                    Clear(int clearBits, const Color4 &color, float depth, unsigned int stencil);
    void                    ReadPixels(int x, int y, int width, int height, Image::Format imageFormat, byte *data);

    unsigned int            GetStateBits() const;
    const Rect &            GetViewport() const;
    int                     GetCullFace() const;
    const Rect &            GetScissor() const;

    void                    SetDefaultState();
    void                    SetStateBits(unsigned int state);
    void                    SetCullFace(int cull);
    void                    SetDepthBias(float slopeScaleBias, float constantBias);
    void                    SetDepthRange(float znear, float zfar);
    void                    SetDepthClamp(bool enable);
    void                    SetDepthBounds(float zmin, float zmax);
    void                    SetViewport(const Rect &viewportRect);
    void                    SetScissor(const Rect &scissorRect);
    void                    SetSRGBWrite(bool enable);

    void                    EnableLineSmooth(bool enable);
    float                   GetLineWidth() const;
    void                    SetLineWidth(float width);

    Handle                  CreateStencilState(int readMask, int writeMask, StencilFunc funcBack, int failBack, int zfailBack, int zpassBack, StencilFunc funcFront, int failFront, int zfailFront, int zpassFront);
    void                    DestroyStencilState(Handle stencilStateHandle);
    void                    SetStencilState(Handle stencilStateHandle, int ref);

    Handle                  CreateTexture(TextureType type);
    void                    DestroyTexture(Handle textureHandle);
    void                    SelectTextureUnit(unsigned int unit);
    void                    BindTexture(Handle textureHandle);

    void                    AdjustTextureSize(TextureType type, bool useNPOT, int inWidth, int inHeight, int inDepth, int *outWidth, int *outHeight, int *outDepth);
    void                    AdjustTextureFormat(TextureType type, bool useCompression, bool useNormalMap, Image::Format inFormat, Image::Format *outFormat);

    void                    SetTextureFilter(TextureFilter filter);
    void                    SetTextureAddressMode(AddressMode addressMode);
    void                    SetTextureAnisotropy(int aniso);
    void                    SetTextureBorderColor(const Color4 &rgba);
    void                    SetTextureShadowFunc(bool set);
    void                    SetTextureLODBias(float bias);
    void                    SetTextureLevel(int baseLevel, int maxLevel = 1000);
    void                    GenerateMipmap();

    void                    SetTextureImage(TextureType textureType, const Image *srcImage, Image::Format dstFormat, bool useMipmaps, bool useSRGB);
    void                    SetTextureImageBuffer(Image::Format dstFormat, bool sRGB, int bufferHandle);

    void                    SetTextureSubImage2D(int level, int xoffset, int yoffset, int width, int height, Image::Format srcFormat, const void *pixels);
    void                    SetTextureSubImage3D(int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, Image::Format srcFormat, const void *pixels);
    void                    SetTextureSubImage2DArray(int level, int xoffset, int yoffset, int zoffset, int width, int height, int arrays, Image::Format srcFormat, const void *pixels);
    void                    SetTextureSubImageCube(CubeMapFace face, int level, int xoffset, int yoffset, int width, int height, Image::Format srcFormat, const void *pixels);
    void                    SetTextureSubImageRect(int xoffset, int yoffset, int width, int height, Image::Format srcFormat, const void *pixels);

    void                    CopyTextureSubImage2D(int xoffset, int yoffset, int x, int y, int width, int height);

    void                    GetTextureImage2D(int level, Image::Format format, void *pixels);
    void                    GetTextureImage3D(int level, Image::Format format, void *pixels);
    void                    GetTextureImageCube(CubeMapFace face, int level, Image::Format format, void *pixels);
    void                    GetTextureImageRect(Image::Format format, void *pixels);

    Handle                  CreateRenderTarget(RenderTargetType type, int width, int height, int numColorTextures, Handle *colorTextureHandles, Handle depthTextureHandle, bool sRGB, int flags);
    void                    DestroyRenderTarget(Handle renderTargetHandle);
    void                    BeginRenderTarget(Handle renderTargetHandle, int level = 0, int sliceIndex = 0, unsigned int mrtBitMask = 0);
    void                    EndRenderTarget();
    void                    BlitRenderTarget(Handle srcRenderTargetHandle, const Rect &srcRect, Handle dstRenderTargetHandle, const Rect &dstRect, int mask, int filter) const;

    Handle                  CreateShader(const char *name, const char *vsText, const char *fsText);
    void                    DestroyShader(Handle shaderHandle);
    void                    BindShader(Handle shaderHandle);

    int                     GetSamplerUnit(Handle shaderHandle, const char *name) const;
    void                    SetTexture(int unit, Handle textureHandle);

                            /// Returns the index of shader constant with the given name.
    int                     GetShaderConstantIndex(int shaderHandle, const char *name) const;

                            /// Returns the block index of shader constant with the given name.
    int                     GetShaderConstantBlockIndex(int shaderHandle, const char *name) const;

                            /// Sets the value of integer constant variable for the current bound shader.
    void                    SetShaderConstant1i(int index, const int constant) const;
    void                    SetShaderConstant2i(int index, const int *constant) const;
    void                    SetShaderConstant3i(int index, const int *constant) const;
    void                    SetShaderConstant4i(int index, const int *constant) const;

                            /// Sets the value of float constant variable for the current bound shader.
    void                    SetShaderConstant1f(int index, const float constant) const;
    void                    SetShaderConstant2f(int index, const float *constant) const;
    void                    SetShaderConstant3f(int index, const float *constant) const;
    void                    SetShaderConstant4f(int index, const float *constant) const;
    void                    SetShaderConstant2f(int index, const Vec2 &constant) const;
    void                    SetShaderConstant3f(int index, const Vec3 &constant) const;
    void                    SetShaderConstant4f(int index, const Vec4 &constant) const;

                            /// Sets the value of float matrix constant variable for the current bound shader.
    void                    SetShaderConstant2x2f(int index, bool rowMajor, const Mat2 &constant) const;
    void                    SetShaderConstant3x3f(int index, bool rowMajor, const Mat3 &constant) const;
    void                    SetShaderConstant4x4f(int index, bool rowMajor, const Mat4 &constant) const;
    void                    SetShaderConstant4x3f(int index, bool rowMajor, const Mat3x4 &constant) const;

                            /// Sets the value of integer constant array variable for the current bound shader.
    void                    SetShaderConstantArray1i(int index, int count, const int *constant) const;
    void                    SetShaderConstantArray2i(int index, int count, const int *constant) const;
    void                    SetShaderConstantArray3i(int index, int count, const int *constant) const;
    void                    SetShaderConstantArray4i(int index, int count, const int *constant) const;

                            /// Sets the value of float array constant variable for the current bound shader.
    void                    SetShaderConstantArray1f(int index, int count, const float *constant) const;
    void                    SetShaderConstantArray2f(int index, int count, const float *constant) const;
    void                    SetShaderConstantArray3f(int index, int count, const float *constant) const;
    void                    SetShaderConstantArray4f(int index, int count, const float *constant) const;
    void                    SetShaderConstantArray2f(int index, int count, const Vec2 *constant) const;
    void                    SetShaderConstantArray3f(int index, int count, const Vec3 *constant) const;
    void                    SetShaderConstantArray4f(int index, int count, const Vec4 *constant) const;

                            /// Sets the value of float matrix constant array variable for the current bound shader.
    void                    SetShaderConstantArray2x2f(int index, bool rowMajor, int count, const Mat2 *constant) const;
    void                    SetShaderConstantArray3x3f(int index, bool rowMajor, int count, const Mat3 *constant) const;
    void                    SetShaderConstantArray4x4f(int index, bool rowMajor, int count, const Mat4 *constant) const;
    void                    SetShaderConstantArray4x3f(int index, bool rowMajor, int count, const Mat3x4 *constant) const;

    void                    SetShaderConstantBlock(int index, int bindingIndex);

    Handle                  CreateBuffer(BufferType type, BufferUsage usage, int size, int pitch = 0, const void *data = nullptr);
    void                    DestroyBuffer(Handle bufferHandle);
    void                    BindBuffer(BufferType type, Handle bufferHandle);

    void                    BindIndexedBuffer(BufferType type, int bindingIndex, Handle bufferHandle);
    void                    BindIndexedBufferRange(BufferType type, int bindingIndex, Handle bufferHandle, int offset, int size);

    void *                  MapBufferRange(Handle bufferHandle, BufferLockMode lockMode, int offset = 0, int size = -1);
    bool                    UnmapBuffer(Handle bufferHandle);
    void                    FlushMappedBufferRange(Handle bufferHandle, int offset = 0, int size = -1);

                            // 기존에 쓰던 buffer 를 버리고 새 버퍼에 data 를 write 한다. written start offset 을 리턴한다. (항상 0)
    int                     BufferDiscardWrite(Handle bufferHandle, int size, const void *data);
                            // 기존에 쓰던 buffer 를 유지하면서 data 를 asyncronous 하게 write 한다. written start offset 을 리턴한다.
                            // overflow 되면 -1 을 리턴, data == nullptr 이면 write 를 안하기 때문에 overflow 여부만 체크할 수 있다.
    int                     BufferWrite(Handle bufferHandle, int alignSize, int size, const void *data);
                            // CopyReadBuffer 로 생성한 버퍼를 CPU 메모리 카피 부담없이 GPU 상에서 빠르게 카피
    int                     BufferCopy(Handle readBufferHandle, Handle writeBufferHandle, int alignSize, int size);
                            // write offset 을 0 으로 만든다.
    void                    BufferRewind(Handle bufferHandle);

    Handle                  CreateSync();
    void                    DestroySync(Handle syncHandle);
    bool                    IsSync(Handle syncHandle) const;
    void                    FenceSync(Handle syncHandle);
    void                    DeleteSync(Handle syncHandle);
    void                    WaitSync(Handle syncHandle);

    Handle                  CreateVertexFormat(int numElements, const VertexElement *elements);
    void                    DestroyVertexFormat(Handle vertexFormatHandle);
    void                    SetVertexFormat(Handle vertexFormatHandle);
                            // D3D 의 SetStreamSource 와 유사. (SetVertexFormat 호출 후에 실행되어야 한다)
    void                    SetStreamSource(int stream, Handle vertexBufferHandle, int base, int stride);

    void                    DrawArrays(Primitive primitives, int startVertex, int numVerts) const;
    void                    DrawArraysInstanced(Primitive primitives, int startVertex, int numVerts, int instanceCount) const;
    void                    DrawElements(Primitive primitives, int startIndex, int numIndices, int indexSize, const void *ptr) const;
    void                    DrawElementsInstanced(Primitive primitives, int startIndex, int numIndices, int indexSize, const void *ptr, int instanceCount) const;
    void                    DrawElementsBaseVertex(Primitive primitives, int startIndex, int numIndices, int indexSize, const void *ptr, int baseVertexIndex) const;
    void                    DrawElementsInstancedBaseVertex(Primitive primitives, int startIndex, int numIndices, int indexSize, const void *ptr, int instanceCount, int baseVertexIndex) const;
    void                    DrawElementsIndirect(Primitive primitives, int indexSize, int indirectBufferOffset) const;
    void                    MultiDrawElementsIndirect(Primitive primitives, int indexSize, int indirectBufferOffset, int drawCount, int stride) const;

    Handle                  CreateQuery();
    void                    DestroyQuery(Handle queryHandle);
    void                    BeginQuery(Handle queryHandle);
    void                    EndQuery();
    bool                    QueryResultAvailable(Handle queryHandle) const;
    unsigned int            QueryResult(Handle queryHandle) const;

    void                    CheckError(const char *fmt, ...) const;

    Str                     GetGPUString() const;

    const HWLimit &         HWLimit() const { return hwLimit; }

protected:
    void                    InitMainContext(WindowHandle windowHandle, const Settings *settings);
    void                    FreeMainContext();
    void                    InitGL();

    void                    InitHandles();
    void                    FreeHandles();

    int                     GetTypeSize(const VertexElement::Type type) const;
    void                    SetShaderConstantGeneric(int index, bool rowMajor, int count, const void *data) const;
    void                    BeginUnpackAlignment(int pitch);
    void                    EndUnpackAlignment();

    bool                    initialized;

    const char *            vendorString;
    const char *            rendererString;
    const char *            versionString;
    float                   version;
    const char *            extensionsString;
    const char *            glslVersionString;
    float                   glslVersion;
    int                     colorBits;
    int                     depthBits;
    int                     stencilBits;
    int                     multiSamples;
    bool                    linearFrameBuffer;

    RHI::HWLimit            hwLimit;

    GLContext *             mainContext;
    Array<GLContext *>      contextList;
    GLContext *             currentContext;

    Array<GLStencilState *> stencilStateList;
    Array<GLBuffer *>       bufferList;
    Array<GLSync *>         syncList;
    Array<GLTexture *>      textureList;
    Array<GLShader *>       shaderList;
    Array<GLVertexFormat *> vertexFormatList;
    Array<GLRenderTarget *> renderTargetList;
    Array<GLQuery *>        queryList;
};

extern OpenGLRHI            rhi;

BE_NAMESPACE_END
