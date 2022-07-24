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

    Str                     GetGPUString() const;

    const HWLimit &         HWLimit() const { return hwLimit; }

    bool                    SupportsPolygonMode() const;
    bool                    SupportsPackedFloat() const;
    bool                    SupportsDepthBufferFloat() const;
    bool                    SupportsPixelBuffer() const;
    bool                    SupportsTextureRectangle() const;
    bool                    SupportsTextureArray() const;
    bool                    SupportsTextureBuffer() const;
    bool                    SupportsTextureCompressionS3TC() const;
    bool                    SupportsTextureCompressionLATC() const;
    bool                    SupportsTextureCompressionRGTC() const;
    bool                    SupportsTextureCompressionETC2() const;
    bool                    SupportsInstancedArrays() const;
    bool                    SupportsBufferStorage() const;
    bool                    SupportsMultiDrawIndirect() const;
    bool                    SupportsDebugLabel() const;
    bool                    SupportsTimestampQueries() const;
    bool                    SupportsCopyImage() const;

    bool                    IsFullscreen() const;
    bool                    SetFullscreen(Handle windowHandle, int width, int height);
    void                    ResetFullscreen(Handle windowHandle);

    void                    GetGammaRamp(unsigned short ramp[768]) const;
    void                    SetGammaRamp(unsigned short ramp[768]) const;

    bool                    SwapBuffers();
    void                    SwapInterval(int interval) const;

    void                    Clear(int clearBits, const Color4 &color, float depth, unsigned int stencil);

    void                    ReadPixels(int x, int y, int width, int height, Image::Format::Enum imageFormat, byte *data);

    void                    CheckError(const char *fmt, ...) const;

    //---------------------------------------------------------------------------------------------
    // Context
    //---------------------------------------------------------------------------------------------

    Handle                  CreateContext(WindowHandle windowHandle, bool useSharedContext);
    void                    DestroyContext(Handle ctxHandle);
    void                    ActivateSurface(Handle ctxHandle, WindowHandle windowHandle);
    void                    DeactivateSurface(Handle ctxHandle);
    void                    SetContext(Handle ctxHandle);
    void                    SetContextDisplayFunc(Handle ctxHandle, DisplayContextFunc displayFunc, void *dataPtr, bool onDemandDrawing);
    void                    DisplayContext(Handle ctxHandle);
    WindowHandle            GetWindowHandleFromContext(Handle ctxHandle);
    void                    GetDisplayMetrics(Handle ctxHandle, DisplayMetrics *displayMetrics) const;

    //---------------------------------------------------------------------------------------------
    // Render State
    //---------------------------------------------------------------------------------------------

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
    bool                    IsSRGBWriteEnabled() const;

    void                    EnableLineSmooth(bool enable);
    float                   GetLineWidth() const;
    void                    SetLineWidth(float width);

    //---------------------------------------------------------------------------------------------
    // Depth Stencil
    //---------------------------------------------------------------------------------------------

    Handle                  CreateStencilState(int readMask, int writeMask, 
                                StencilFunc::Enum funcBack, int failBack, int zfailBack, int zpassBack, 
                                StencilFunc::Enum funcFront, int failFront, int zfailFront, int zpassFront);
    void                    DestroyStencilState(Handle stencilStateHandle);
    void                    SetStencilState(Handle stencilStateHandle, int ref);

    //---------------------------------------------------------------------------------------------
    // Texture
    //---------------------------------------------------------------------------------------------

    Handle                  CreateTexture(TextureType::Enum type);
    void                    DestroyTexture(Handle textureHandle);
    void                    SelectTextureUnit(unsigned int unit);
    void                    BindTexture(Handle textureHandle);
    void                    SetTexture(int textureUnit, Handle textureHandle);

    void                    AdjustTextureSize(TextureType::Enum type, bool useNPOT, int inWidth, int inHeight, int inDepth, int *outWidth, int *outHeight, int *outDepth);
    void                    AdjustTextureFormat(TextureType::Enum type, bool useCompression, bool useNormalMap, Image::Format::Enum inFormat, Image::Format::Enum *outFormat);

    void                    SetTextureFilter(TextureFilter::Enum filter);
    void                    SetTextureAddressMode(AddressMode::Enum addressMode);
    void                    SetTextureAnisotropy(int aniso);
    void                    SetTextureBorderColor(const Color4 &rgba);
    void                    SetTextureShadowFunc(bool set);
    void                    SetTextureLODBias(float bias);
    void                    SetTextureLevel(int baseLevel, int maxLevel = 1000);
    void                    GenerateMipmap();

    void                    SetTextureImage(TextureType::Enum textureType, const Image *srcImage, Image::Format::Enum dstFormat, bool useMipmaps, bool isSRGB);
    void                    SetTextureImageBuffer(Image::Format::Enum dstFormat, bool sRGB, int bufferHandle);

    void                    SetTextureSubImage2D(int level, int xoffset, int yoffset, int width, int height, Image::Format::Enum srcFormat, const void *pixels);
    void                    SetTextureSubImage3D(int level, int xoffset, int yoffset, int zoffset, int width, int height, int depth, Image::Format::Enum srcFormat, const void *pixels);
    void                    SetTextureSubImage2DArray(int level, int xoffset, int yoffset, int zoffset, int width, int height, int arrays, Image::Format::Enum srcFormat, const void *pixels);
    void                    SetTextureSubImageCube(CubeMapFace::Enum face, int level, int xoffset, int yoffset, int width, int height, Image::Format::Enum srcFormat, const void *pixels);
    void                    SetTextureSubImageRect(int xoffset, int yoffset, int width, int height, Image::Format::Enum srcFormat, const void *pixels);

    void                    CopyTextureSubImage2D(int xoffset, int yoffset, int x, int y, int width, int height);
    void                    CopyImageSubData(Handle srcTextureHandle, int srcLevel, int srcX, int srcY, int srcZ, Handle dstTextureHandle, int dstLevel, int dstX, int dstY, int dstZ, int width, int height, int depth);

    void                    GetTextureImage2D(int level, Image::Format::Enum format, void *pixels);
    void                    GetTextureImage3D(int level, Image::Format::Enum format, void *pixels);
    void                    GetTextureImageCube(CubeMapFace::Enum face, int level, Image::Format::Enum format, void *pixels);
    void                    GetTextureImageRect(Image::Format::Enum format, void *pixels);

    //---------------------------------------------------------------------------------------------
    // Render Target
    //---------------------------------------------------------------------------------------------

    Handle                  CreateRenderTarget(RenderTargetType::Enum type, int width, int height, int numColorTextures, Handle *colorTextureHandles, Handle depthTextureHandle, int flags);
    void                    DestroyRenderTarget(Handle renderTargetHandle);
    void                    BeginRenderTarget(Handle renderTargetHandle, int level = 0, int sliceIndex = 0);
    void                    EndRenderTarget();
    void                    DiscardRenderTarget(bool depth, bool stencil, uint32_t colorBitMask);
    void                    BlitRenderTarget(Handle srcRenderTargetHandle, const Rect &srcRect, Handle dstRenderTargetHandle, const Rect &dstRect, int mask, BlitFilter::Enum filter) const;
    void                    SetDrawBuffersMask(unsigned int mrtBitMask);

    //---------------------------------------------------------------------------------------------
    // Shader
    //---------------------------------------------------------------------------------------------

    Handle                  CreateShader(const char *name, const char *vsText, const char *fsText);
    void                    DestroyShader(Handle shaderHandle);
    void                    BindShader(Handle shaderHandle);

                            /// Returns texture image unit with the given name.
    int                     GetShaderTextureUnit(Handle shaderHandle, const char *name) const;

                            /// Returns the index of shader constant with the given name.
    int                     GetShaderConstantIndex(Handle shaderHandle, const char *name) const;

                            /// Returns the block index of shader constant with the given name.
    int                     GetShaderConstantBlockIndex(Handle shaderHandle, const char *name) const;

                            /// Sets the value of integer constant variable for the current bound shader.
    void                    SetShaderConstant1i(int constantIndex, const int constant) const;
    void                    SetShaderConstant2i(int constantIndex, const int *constant) const;
    void                    SetShaderConstant3i(int constantIndex, const int *constant) const;
    void                    SetShaderConstant4i(int constantIndex, const int *constant) const;

                            /// Sets the value of unsigned integer constant variable for the current bound shader.
    void                    SetShaderConstant1ui(int constantIndex, const unsigned int constant) const;
    void                    SetShaderConstant2ui(int constantIndex, const unsigned int *constant) const;
    void                    SetShaderConstant3ui(int constantIndex, const unsigned int *constant) const;
    void                    SetShaderConstant4ui(int constantIndex, const unsigned int *constant) const;

                            /// Sets the value of float constant variable for the current bound shader.
    void                    SetShaderConstant1f(int constantIndex, const float constant) const;
    void                    SetShaderConstant2f(int constantIndex, const float *constant) const;
    void                    SetShaderConstant3f(int constantIndex, const float *constant) const;
    void                    SetShaderConstant4f(int constantIndex, const float *constant) const;
    void                    SetShaderConstant2f(int constantIndex, const Vec2 &constant) const;
    void                    SetShaderConstant3f(int constantIndex, const Vec3 &constant) const;
    void                    SetShaderConstant4f(int constantIndex, const Vec4 &constant) const;

                            /// Sets the value of float matrix constant variable for the current bound shader.
    void                    SetShaderConstant2x2f(int constantIndex, bool rowMajor, const Mat2 &constant) const;
    void                    SetShaderConstant3x3f(int constantIndex, bool rowMajor, const Mat3 &constant) const;
    void                    SetShaderConstant4x4f(int constantIndex, bool rowMajor, const Mat4 &constant) const;
    void                    SetShaderConstant4x3f(int constantIndex, bool rowMajor, const Mat3x4 &constant) const;

                            /// Sets the value of integer constant array variable for the current bound shader.
    void                    SetShaderConstantArray1i(int constantIndex, int count, const int *constant) const;
    void                    SetShaderConstantArray2i(int constantIndex, int count, const int *constant) const;
    void                    SetShaderConstantArray3i(int constantIndex, int count, const int *constant) const;
    void                    SetShaderConstantArray4i(int constantIndex, int count, const int *constant) const;

                            /// Sets the value of float array constant variable for the current bound shader.
    void                    SetShaderConstantArray1f(int constantIndex, int count, const float *constant) const;
    void                    SetShaderConstantArray2f(int constantIndex, int count, const float *constant) const;
    void                    SetShaderConstantArray3f(int constantIndex, int count, const float *constant) const;
    void                    SetShaderConstantArray4f(int constantIndex, int count, const float *constant) const;
    void                    SetShaderConstantArray2f(int constantIndex, int count, const Vec2 *constant) const;
    void                    SetShaderConstantArray3f(int constantIndex, int count, const Vec3 *constant) const;
    void                    SetShaderConstantArray4f(int constantIndex, int count, const Vec4 *constant) const;

                            /// Sets the value of float matrix constant array variable for the current bound shader.
    void                    SetShaderConstantArray2x2f(int constantIndex, bool rowMajor, int count, const Mat2 *constant) const;
    void                    SetShaderConstantArray3x3f(int constantIndex, bool rowMajor, int count, const Mat3 *constant) const;
    void                    SetShaderConstantArray4x4f(int constantIndex, bool rowMajor, int count, const Mat4 *constant) const;
    void                    SetShaderConstantArray4x3f(int constantIndex, bool rowMajor, int count, const Mat3x4 *constant) const;

    void                    SetShaderConstantBlock(int constantIndex, int bindingIndex);

    //---------------------------------------------------------------------------------------------
    // Buffer
    //---------------------------------------------------------------------------------------------

    Handle                  CreateBuffer(BufferType::Enum type, BufferUsage::Enum usage, int size, int pitch = 0, const void *data = nullptr);
    void                    DestroyBuffer(Handle bufferHandle);
    void                    BindBuffer(BufferType::Enum type, Handle bufferHandle);

    void                    BindIndexedBuffer(BufferType::Enum type, int bindingIndex, Handle bufferHandle);
    void                    BindIndexedBufferRange(BufferType::Enum type, int bindingIndex, Handle bufferHandle, int offset, int size);

    void *                  MapBuffer(Handle bufferHandle, BufferLockMode::Enum lockMode) { return MapBufferRange(bufferHandle, lockMode, 0, -1); }
    void *                  MapBufferRange(Handle bufferHandle, BufferLockMode::Enum lockMode, int offset, int size);
    bool                    UnmapBuffer(Handle bufferHandle);
    void                    FlushMappedBufferRange(Handle bufferHandle, int offset, int size);
    void                    WriteBuffer(byte *dst, const byte *src, int numBytes);

                            /// Discards current buffer and write data to new buffer. 
                            /// Returns written start offset. (Always 0)
    int                     BufferDiscardWrite(Handle bufferHandle, int size, const void *data);

                            /// Write data asyncronous while maintaining the existing buffer.
                            /// Returns written start offset.
                            /// If overflow occurs, -1 is returned. If data == nullptr, write is not performed, so only overflow can be checked.
    int                     BufferWrite(Handle bufferHandle, int alignSize, int size, const void *data);

                            /// Quickly copy buffers created with CopyReadBuffer on GPU without burdening CPU memory copy.
    int                     BufferCopy(Handle readBufferHandle, Handle writeBufferHandle, int alignSize, int size);

                            /// Sets write offset to 0.
    void                    BufferRewind(Handle bufferHandle);

    //---------------------------------------------------------------------------------------------
    // GPU Synchronization
    //---------------------------------------------------------------------------------------------

    Handle                  CreateSync();
    void                    DestroySync(Handle syncHandle);
    bool                    IsSync(Handle syncHandle) const;
    void                    FenceSync(Handle syncHandle);
    void                    DeleteSync(Handle syncHandle);
    void                    WaitSync(Handle syncHandle);

    //---------------------------------------------------------------------------------------------
    // Vertex Format
    //---------------------------------------------------------------------------------------------

    Handle                  CreateVertexFormat(int numElements, const VertexElement *elements);
    void                    DestroyVertexFormat(Handle vertexFormatHandle);
    void                    SetVertexFormat(Handle vertexFormatHandle);

    //---------------------------------------------------------------------------------------------
    // Drawing
    //---------------------------------------------------------------------------------------------
    
                            // Similar with SetStreamSource in D3D. Must be called after SetVertexFormat()
    void                    SetStreamSource(int stream, Handle vertexBufferHandle, int base, int stride);

    void                    DrawArrays(Topology::Enum topology, int startVertex, int numVerts) const;
    void                    DrawArraysInstanced(Topology::Enum topology, int startVertex, int numVerts, int instanceCount) const;
    void                    DrawElements(Topology::Enum topology, int startIndex, int numIndices, int indexSize, const void *ptr) const;
    void                    DrawElementsInstanced(Topology::Enum topology, int startIndex, int numIndices, int indexSize, const void *ptr, int instanceCount) const;
    void                    DrawElementsBaseVertex(Topology::Enum topology, int startIndex, int numIndices, int indexSize, const void *ptr, int baseVertexIndex) const;
    void                    DrawElementsInstancedBaseVertex(Topology::Enum topology, int startIndex, int numIndices, int indexSize, const void *ptr, int instanceCount, int baseVertexIndex) const;
    void                    DrawElementsIndirect(Topology::Enum topology, int indexSize, int indirectBufferOffset) const;
    void                    MultiDrawElementsIndirect(Topology::Enum topology, int indexSize, int indirectBufferOffset, int drawCount, int stride) const;

    //---------------------------------------------------------------------------------------------
    // Query (Occlusion, Timestamp)
    //---------------------------------------------------------------------------------------------

    Handle                  CreateQuery(QueryType::Enum queryType);
    void                    DestroyQuery(Handle queryHandle);
    void                    BeginQuery(Handle queryHandle);
    void                    EndQuery(Handle queryHandle);
    void                    QueryTimestamp(Handle queryHandle);
    bool                    QueryResultAvailable(Handle queryHandle) const;
    uint64_t                QueryResult(Handle queryHandle) const;

    //---------------------------------------------------------------------------------------------
    // ImGui
    //---------------------------------------------------------------------------------------------
    void                    ImGuiBeginFrame(Handle ctxHandle);
    void                    ImGuiRender();
    void                    ImGuiEndFrame();

protected:
    void                    InitMainContext(WindowHandle windowHandle, const Settings *settings);
    void                    FreeMainContext();
    void                    InitGL();

    void                    InitHandles();
    void                    FreeHandles();

    void                    ImGuiCreateContext(GLContext *ctx);
    void                    ImGuiDestroyContext(GLContext *ctx);

    int                     GetTypeSize(const VertexElement::Type::Enum type) const;
    void                    SetShaderConstantGeneric(int index, bool rowMajor, int count, const void *data) const;
    void                    BeginUnpackAlignment(int pitch);
    void                    EndUnpackAlignment();

    bool                    initialized;

    const char *            vendorString;
    const char *            rendererString;
    const char *            versionString;
    float                   version;
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
