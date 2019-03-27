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

#include "Precompiled.h"
#include "Platform/PlatformFile.h"
#include "RHI/RHIOpenGL.h"
#include "RGLInternal.h"

BE_NAMESPACE_BEGIN

const GLenum toGLTopology[] = {
    GL_TRIANGLES,
    GL_TRIANGLE_FAN,
    GL_TRIANGLE_STRIP,
    GL_LINES,
    GL_LINE_STRIP,
    GL_LINE_LOOP,
    GL_POINTS,
};

OpenGLRHI       rhi;

Str             GLShader::programCacheDir;

CVar            gl_sRGB("gl_sRGB", "1", CVar::Flag::Bool | CVar::Flag::Archive, "enable sRGB color calibration");

OpenGLRHI::OpenGLRHI() {
    initialized = false;
    currentContext = nullptr;
    mainContext = nullptr;
}

void OpenGLRHI::Init(WindowHandle windowHandle, const Settings *settings) {
    BE_LOG("Initializing OpenGL Renderer...\n");

    InitHandles();

    InitMainContext(windowHandle, settings);

    currentContext = mainContext;

    InitGL();

    SetDefaultState();

    if (OpenGL::SupportsProgramBinary()) {
        GLShader::programCacheDir = "Cache/ProgramBinaryCache";

        if (!PlatformFile::DirectoryExists(GLShader::programCacheDir)) {
            PlatformFile::CreateDirectoryTree(GLShader::programCacheDir);
        }
    }

    initialized = true;
}

void OpenGLRHI::Shutdown() {
    BE_LOG("Shutting down OpenGL Renderer...\n");

    initialized = false;

    gglFinish();

    FreeMainContext();

    currentContext = nullptr;

    FreeHandles();
}

void OpenGLRHI::InitHandles() {
    contextList.SetGranularity(16);
    GLContext *zeroContext = new GLContext;
    memset(zeroContext, 0, sizeof(*zeroContext));
    contextList.Append(zeroContext);

    stencilStateList.SetGranularity(32);
    GLStencilState *zeroStencilState = new GLStencilState;
    memset(zeroStencilState, 0, sizeof(*zeroStencilState));
    stencilStateList.Append(zeroStencilState);

    bufferList.SetGranularity(1024);
    GLBuffer *zeroBuffer = new GLBuffer;
    memset(zeroBuffer, 0, sizeof(*zeroBuffer));
    bufferList.Append(zeroBuffer);

    syncList.SetGranularity(8);
    GLSync *zeroSync = new GLSync;
    memset(zeroSync, 0, sizeof(*zeroSync));
    syncList.Append(zeroSync);

    textureList.SetGranularity(1024);
    GLTexture *zeroTexture = new GLTexture;
    memset(zeroTexture, 0, sizeof(*zeroTexture));
    textureList.Append(zeroTexture);

    shaderList.SetGranularity(1024);
    GLShader *zeroShader = new GLShader;
    memset(zeroShader, 0, sizeof(*zeroShader));
    shaderList.Append(zeroShader);

    vertexFormatList.SetGranularity(64);
    GLVertexFormat *zeroVertexFormat = new GLVertexFormat;
    memset(zeroVertexFormat, 0, sizeof(*zeroVertexFormat));
    vertexFormatList.Append(zeroVertexFormat);

    renderTargetList.SetGranularity(64);
    GLRenderTarget *zeroRenderTarget = new GLRenderTarget;
    memset(zeroRenderTarget, 0, sizeof(*zeroRenderTarget));
    renderTargetList.Append(zeroRenderTarget);

    queryList.SetGranularity(32);
    GLQuery *zeroQuery = new GLQuery;
    memset(zeroQuery, 0, sizeof(*zeroQuery));
    queryList.Append(zeroQuery);
}

void OpenGLRHI::FreeHandles() {
    contextList.DeleteContents(true);
    stencilStateList.DeleteContents(true);
    bufferList.DeleteContents(true);
    syncList.DeleteContents(true);
    textureList.DeleteContents(true);
    shaderList.DeleteContents(true);
    vertexFormatList.DeleteContents(true);
    renderTargetList.DeleteContents(true);
    queryList.DeleteContents(true);
}

void OpenGLRHI::InitGL() {
    OpenGL::Init();

    vendorString = (const char *)gglGetString(GL_VENDOR);
    BE_LOG("GL vendor: %s\n", vendorString);

    rendererString = (const char *)gglGetString(GL_RENDERER);
    BE_LOG("GL renderer: %s\n", rendererString);

    versionString = (const char *)gglGetString(GL_VERSION);
    version = atof(versionString);
    BE_LOG("GL version: %s\n", versionString);

    BE_LOG("GL extensions:");
    GLint numExtensions = 0;
    gglGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    for (int i = 0; i < numExtensions; i++) {
        const char *extension = (const char *)gglGetStringi(GL_EXTENSIONS, i);
        BE_LOG(" %s", extension);
    }
    BE_LOG("\n");

#if defined(__WIN32__)
    const char *winExtensions = (const char *)gwglGetExtensionsStringARB(mainContext->hdc);
    BE_LOG("WGL extensions: %s\n", winExtensions);
#endif

#if defined(__ANDROID__)
    const char *eglExtensions = (const char *)geglQueryString(mainContext->eglDisplay, EGL_EXTENSIONS);
    BE_LOG("EGL extensions: %s\n", eglExtensions);
#endif

    glslVersionString = (const char *)gglGetString(GL_SHADING_LANGUAGE_VERSION);
    if (gglGetError() == GL_INVALID_ENUM) {
        glslVersionString = "1.051";
    }
    glslVersion = atof(glslVersionString);

    BE_LOG("GLSL version: %s\n", glslVersionString);

    /*int red, green, blue, alpha;
    gglGetIntegerv(GL_RED_BITS, &red);
    gglGetIntegerv(GL_GREEN_BITS, &green);
    gglGetIntegerv(GL_BLUE_BITS, &blue);
    gglGetIntegerv(GL_ALPHA_BITS, &alpha);
    gglGetIntegerv(GL_DEPTH_BITS, &depthBits);
    gglGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    colorBits = red + green + blue + alpha;
    BE_LOG("Pixel format: color(%i-bits) depth(%i-bits) stencil(%i-bits)\n", colorBits, depthBits, stencilBits);*/

    memset(&hwLimit, 0, sizeof(hwLimit));

    // texture limits
    gglGetIntegerv(GL_MAX_TEXTURE_SIZE, &hwLimit.maxTextureSize);
    BE_LOG("Maximum texture size: %i\n", hwLimit.maxTextureSize);

    gglGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &hwLimit.max3dTextureSize);
    BE_LOG("Maximum 3d texture size: %i\n", hwLimit.max3dTextureSize);

    gglGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &hwLimit.maxCubeMapTextureSize);
    BE_LOG("Maximum cube map texture size: %i\n", hwLimit.maxCubeMapTextureSize);

#ifdef GL_ARB_texture_rectangle
    gglGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, &hwLimit.maxRectangleTextureSize);
    BE_LOG("Maximum rectangle texture size: %i\n", hwLimit.maxRectangleTextureSize);
#endif
    
    if (OpenGL::SupportsTextureBufferObject()) {
        gglGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &hwLimit.maxTextureBufferSize);
        BE_LOG("Maximum texture buffer size: %i\n", hwLimit.maxTextureBufferSize);
    }
    
    if (OpenGL::SupportsTextureFilterAnisotropic()) {
        gglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &hwLimit.maxTextureAnisotropy);
        BE_LOG("Maximum texture anisotropy: %i\n", hwLimit.maxTextureAnisotropy);
    }
    
    // the maximum supported texture image units that can be used to access texture maps from the fragment shader.
    gglGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &hwLimit.maxTextureImageUnits);
    BE_LOG("Maximum texture image units: %i\n", hwLimit.maxTextureImageUnits);

    // the maximum number of 4 component generic vertex attributes accessible to a vertex shader.
    gglGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &hwLimit.maxVertexAttribs);
    BE_LOG("Maximum vertex attribs: %i\n", hwLimit.maxVertexAttribs);

    // the maximum number of individual floating point, integer, or boolean values that can be held in uniform variable storage for a vertex shader.
    gglGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &hwLimit.maxVertexUniformComponents);
    BE_LOG("Maximum vertex uniform components: %i\n", hwLimit.maxVertexUniformComponents);

    // the maximum number of vector floating point, integer, or boolean values that can be held in uniform variable storage for a vertex shader.
    gglGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &hwLimit.maxVertexUniformVectors);
    BE_LOG("Maximum vertex uniform vectors: %i\n", hwLimit.maxVertexUniformVectors);

    // the maximum supported texture image units that can be used to access texture maps from the vertex shader.
    gglGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &hwLimit.maxVertexTextureImageUnits);
    BE_LOG("Maximum vertex texture image units: %i\n", hwLimit.maxVertexTextureImageUnits);

    // the maximum number of individual floating point, integer, or boolean values that can be held in uniform variable storage for a fragment shader.
    gglGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &hwLimit.maxFragmentUniformComponents);
    BE_LOG("Maximum fragment uniform components: %i\n", hwLimit.maxFragmentUniformComponents);

    // the maximum number of vector floating point, integer, or boolean values that can be held in uniform variable storage for a fragment shader.
    gglGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &hwLimit.maxFragmentUniformVectors);
    BE_LOG("Maximum fragment uniform vectors: %i\n", hwLimit.maxFragmentUniformVectors);

    // the maximum number of components of the inputs read by the fragment shader.
    gglGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &hwLimit.maxFragmentInputComponents);
    BE_LOG("Maximum fragment input components: %i\n", hwLimit.maxFragmentInputComponents);

    if (OpenGL::SupportsGeometryShader()) {
        // the maximum supported texture image units that can be used to access texture maps from the geometry shader.
        gglGetIntegerv(GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS, &hwLimit.maxGeometryTextureImageUnits);
        BE_LOG("Maximum geometry texture image units: %i\n", hwLimit.maxGeometryTextureImageUnits);
        
        gglGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &hwLimit.maxGeometryOutputVertices);
        BE_LOG("Maximum geometry output vertices: %i\n", hwLimit.maxGeometryOutputVertices);
    }

    // the maximum number of uniform buffer binding points on the context.
    gglGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &hwLimit.maxUniformBufferBindings);
    BE_LOG("Maximum uniform buffer bindings: %i\n", hwLimit.maxUniformBufferBindings);

    // the maximum size in basic machine units of a uniform block.
    gglGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &hwLimit.maxUniformBlockSize);
    BE_LOG("Maximum uniform block size: %i\n", hwLimit.maxUniformBlockSize);

    // the minimum required alignment for uniform buffer sizes and offset.
    gglGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &hwLimit.uniformBufferOffsetAlignment);
    BE_LOG("Maximum uniform buffer offset alignment: %i\n", hwLimit.uniformBufferOffsetAlignment);

    // GL_ARB_framebuffer_object (3.0)
    gglGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &hwLimit.maxRenderBufferSize);
    gglGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &hwLimit.maxColorAttachments);
    BE_LOG("Maximum render buffer size: %i\n", hwLimit.maxRenderBufferSize);

    // GL_ARB_draw_buffers (2.0)
    gglGetIntegerv(GL_MAX_DRAW_BUFFERS, &hwLimit.maxDrawBuffers);
    BE_LOG("Maximum MRT: %i\n", hwLimit.maxDrawBuffers);

    // Checking NVDIA vertex program version
#ifdef GL_NV_vertex_program4
    if (gglext._GL_NV_vertex_program4) {
        hwLimit.nvVertexProgramVersion = 4;
    } else if (gglext._GL_NV_vertex_program3) {
        hwLimit.nvVertexProgramVersion = 3;
    } else if (gglext._GL_NV_vertex_program2 && gglext._GL_NV_vertex_program2_option) {
        hwLimit.nvVertexProgramVersion = 2;
    } else if (gglext._GL_NV_vertex_program && gglext._GL_NV_vertex_program1_1) {
        hwLimit.nvVertexProgramVersion = 1;
    }
#endif

    // Checking NVDIA fragment program version
#ifdef GL_NV_fragment_program4
    if (gglext._GL_NV_fragment_program4) {
        hwLimit.nvFragmentProgramVersion = 4;
    } else if (gglext._GL_NV_fragment_program2 && gglext._GL_NV_fragment_program_option) {
        hwLimit.nvFragmentProgramVersion = 2;
    } else if (gglext._GL_NV_fragment_program) {
        hwLimit.nvFragmentProgramVersion = 1;
    }
#endif
    
    // GL_ARB_fragment_shader
    gglHint(GL_FRAGMENT_SHADER_DERIVATIVE_HINT, GL_NICEST);

#ifdef GL_ARB_texture_compression // 1.3
    gglHint(GL_TEXTURE_COMPRESSION_HINT, GL_NICEST);
#endif

#ifdef GL_SGIS_generate_mipmap // 1.4 - deprecated in OpenGL3
    //gglHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
#endif

#ifdef GL_NV_multisample_filter_hint
    if (gglext._GL_NV_multisample_filter_hint) {
        gglHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
    }
#endif

    GLint encoding;
    gglBindFramebuffer(GL_FRAMEBUFFER, mainContext->defaultFramebuffer);
#ifdef GL_ES_VERSION_3_0
    GLenum attachment = GL_BACK;
#else
    GLenum attachment = GL_FRONT_LEFT;
#endif
    gglGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &encoding);
    if (encoding == GL_LINEAR) {
        linearFrameBuffer = true;
    } else {
        linearFrameBuffer = false;
    }
    BE_LOG("default frame buffer encoding : %s\n", linearFrameBuffer ? "Linear" : "sRGB");

    if (gl_sRGB.GetBool()) {
        SetSRGBWrite(true);
    }
}

Str OpenGLRHI::GetGPUString() const {
    return Str(rendererString);
}

bool OpenGLRHI::SupportsPolygonMode() const {
    return OpenGL::SupportsPolygonMode();
}

bool OpenGLRHI::SupportsPackedFloat() const {
    return OpenGL::SupportsPackedFloat();
}

bool OpenGLRHI::SupportsDepthBufferFloat() const {
    return OpenGL::SupportsDepthBufferFloat();
}

bool OpenGLRHI::SupportsPixelBufferObject() const {
    return OpenGL::SupportsPixelBufferObject();
}

bool OpenGLRHI::SupportsTextureRectangle() const {
    return OpenGL::SupportsTextureRectangle();
}

bool OpenGLRHI::SupportsTextureArray() const {
    return OpenGL::SupportsTextureArray();
}

bool OpenGLRHI::SupportsTextureBufferObject() const {
    return OpenGL::SupportsTextureBufferObject();
}

bool OpenGLRHI::SupportsTextureCompressionS3TC() const {
    return OpenGL::SupportsTextureCompressionS3TC();
}

bool OpenGLRHI::SupportsTextureCompressionLATC() const {
    return OpenGL::SupportsTextureCompressionLATC();
}

bool OpenGLRHI::SupportsTextureCompressionETC2() const {
    return OpenGL::SupportsTextureCompressionETC2();
}

bool OpenGLRHI::SupportsInstancedArrays() const {
    return OpenGL::SupportsInstancedArrays();
}

bool OpenGLRHI::SupportsBufferStorage() const {
    return OpenGL::SupportsBufferStorage();
}

bool OpenGLRHI::SupportsMultiDrawIndirect() const {
    return OpenGL::SupportsMultiDrawIndirect();
}

bool OpenGLRHI::SupportsDebugLabel() const {
    return OpenGL::SupportsDebugLabel();
}

void OpenGLRHI::Clear(int clearBits, const Color4 &color, float depth, unsigned int stencil) {
#if 1
    if (clearBits & ClearBit::Color) {
        if (gl_sRGB.GetBool() && OpenGL::SupportsFrameBufferSRGB()) {
            gglClearBufferfv(GL_COLOR, 0, Color4(color.ToColor3().SRGBToLinear(), color[3]));
        } else {
            gglClearBufferfv(GL_COLOR, 0, color);
        }
    }

    if ((clearBits & (ClearBit::Depth | ClearBit::Stencil)) == (ClearBit::Depth | ClearBit::Stencil)) {
        gglStencilMask(~0);
        gglClearBufferfi(GL_DEPTH_STENCIL, 0, depth, stencil);
        gglStencilMask(0);
    } else if (clearBits & ClearBit::Depth) {
        gglClearBufferfv(GL_DEPTH, 0, &depth);
    } else if (clearBits & ClearBit::Stencil) {
        gglStencilMask(~0);
        gglClearBufferiv(GL_STENCIL, 0, (const GLint *)&stencil);
        gglStencilMask(0);
    }
#else
    GLbitfield bits = 0;

    if (clearBits & ColorBit) {
        bits |= GL_COLOR_BUFFER_BIT;
        if (gl_sRGB.GetBool()) {
            Vec3 linearColor = color.ToColor3().SRGBToLinear();
            gglClearColor(linearColor[0], linearColor[1], linearColor[2], color[3]);
        } else {
            gglClearColor(color[0], color[1], color[2], color[3]);
        }
    }
    
    if (clearBits & DepthBit) {
        bits |= GL_DEPTH_BUFFER_BIT;
        OpenGL::ClearDepth(depth);
    }
    
    if (clearBits & StencilBit) {
        bits |= GL_STENCIL_BUFFER_BIT;
        gglClearStencil(stencil);
        gglStencilMask(~0);
    }
    
    if (bits) {
        gglClear(bits);
    }
    
    if (clearBits & StencilBit) {
        gglStencilMask(0);
    }
#endif
}

void OpenGLRHI::ReadPixels(int x, int y, int width, int height, Image::Format::Enum imageFormat, byte *data) {
    GLenum  format;
    GLenum  type;
    
    if (!OpenGL::ImageFormatToGLFormat(imageFormat, false, &format, &type, nullptr)) {
        BE_WARNLOG("OpenGLRHI::ReadPixels: Unsupported image format %s\n", Image::FormatName(imageFormat));
        return;
    }
    
    GLint oldPackAlignment;
    gglGetIntegerv(GL_PACK_ALIGNMENT, &oldPackAlignment);
    gglPixelStorei(GL_PACK_ALIGNMENT, 1);

    gglFlush();
    gglReadPixels(x, y, width, height, format, type, data);

    gglPixelStorei(GL_PACK_ALIGNMENT, oldPackAlignment);
}

void OpenGLRHI::DrawArrays(Topology::Enum topology, int startVertex, int numVerts) const {
    gglDrawArrays(toGLTopology[topology], startVertex, numVerts);
}

void OpenGLRHI::DrawArraysInstanced(Topology::Enum topology, int startVertex, int numVerts, int instanceCount) const {
    gglDrawArraysInstanced(toGLTopology[topology], startVertex, numVerts, instanceCount);
}

void OpenGLRHI::DrawElements(Topology::Enum topology, int startIndex, int numIndices, int indexSize, const void *ptr) const {
    GLenum indexType = indexSize == 1 ? GL_UNSIGNED_BYTE : (indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT);
    int indexBufferHandle = currentContext->state->bufferHandles[BufferType::Index];
    const GLvoid *indices = indexBufferHandle != 0 ? BUFFER_OFFSET(indexSize * startIndex) : (byte *)ptr + indexSize * startIndex;
    gglDrawElements(toGLTopology[topology], numIndices, indexType, indices);
}

void OpenGLRHI::DrawElementsInstanced(Topology::Enum topology, int startIndex, int numIndices, int indexSize, const void *ptr, int instanceCount) const {
    GLenum indexType = indexSize == 1 ? GL_UNSIGNED_BYTE : (indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT);
    int indexBufferHandle = currentContext->state->bufferHandles[BufferType::Index];
    const GLvoid *indices = indexBufferHandle != 0 ? BUFFER_OFFSET(indexSize * startIndex) : (byte *)ptr + indexSize * startIndex;
    gglDrawElementsInstanced(toGLTopology[topology], numIndices, indexType, indices, instanceCount);
}

void OpenGLRHI::DrawElementsBaseVertex(Topology::Enum topology, int startIndex, int numIndices, int indexSize, const void *ptr, int baseVertexIndex) const {
    GLenum indexType = indexSize == 1 ? GL_UNSIGNED_BYTE : (indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT);
    int indexBufferHandle = currentContext->state->bufferHandles[BufferType::Index];
    const GLvoid *indices = indexBufferHandle != 0 ? BUFFER_OFFSET(indexSize * startIndex) : (byte *)ptr + indexSize * startIndex;
    OpenGL::DrawElementsBaseVertex(toGLTopology[topology], numIndices, indexType, indices, baseVertexIndex);
}

void OpenGLRHI::DrawElementsInstancedBaseVertex(Topology::Enum topology, int startIndex, int numIndices, int indexSize, const void *ptr, int instanceCount, int baseVertexIndex) const {
    GLenum indexType = indexSize == 1 ? GL_UNSIGNED_BYTE : (indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT);
    int indexBufferHandle = currentContext->state->bufferHandles[BufferType::Index];
    const GLvoid *indices = indexBufferHandle != 0 ? BUFFER_OFFSET(indexSize * startIndex) : (byte *)ptr + indexSize * startIndex;
    OpenGL::DrawElementsInstancedBaseVertex(toGLTopology[topology], numIndices, indexType, indices, instanceCount, baseVertexIndex);
}

void OpenGLRHI::DrawElementsIndirect(Topology::Enum topology, int indexSize, int indirectBufferOffset) const {
    GLenum indexType = indexSize == 1 ? GL_UNSIGNED_BYTE : (indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT);
    OpenGL::DrawElementsIndirect(toGLTopology[topology], indexType, BUFFER_OFFSET(indirectBufferOffset));
}

void OpenGLRHI::MultiDrawElementsIndirect(Topology::Enum topology, int indexSize, int indirectBufferOffset, int drawCount, int stride) const {
    GLenum indexType = indexSize == 1 ? GL_UNSIGNED_BYTE : (indexSize == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT);
    OpenGL::MultiDrawElementsIndirect(toGLTopology[topology], indexType, BUFFER_OFFSET(indirectBufferOffset), drawCount, stride);
}

extern "C" void CheckGLError(const char *msg);

void OpenGLRHI::CheckError(const char *fmt, ...) const {
    char buffer[16384];
    va_list args;

    va_start(args, fmt);
    Str::vsnPrintf(buffer, COUNT_OF(buffer), fmt, args);
    va_end(args);

    CheckGLError(buffer);
}

BE_NAMESPACE_END
