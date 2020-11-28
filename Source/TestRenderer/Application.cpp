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
#include "Application.h"

struct Vertex2D {
    BE1::Vec2       position;
    BE1::Vec2       texCoord;
};

struct Vertex3D {
    BE1::Vec3       position;
    BE1::Vec2       texCoord;
    uint32_t        color;  // bgra
};

Application         app;

static CVAR(r_colorBits, "32", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "number of color bits");
static CVAR(r_depthBits, "24", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "number of depth bits");
static CVAR(r_stencilBits, "8", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "number of stencil bits");
static CVAR(r_multiSamples, "0", BE1::CVar::Flag::Integer | BE1::CVar::Flag::Archive, "number of antialiasing samples");

void Application::InitVertexFormats() {
    const BE1::RHI::VertexElement vertex2DElements[] = {
        { 0, 0, BE1::RHI::VertexElement::Usage::Position, 2, BE1::RHI::VertexElement::Type::Float, false },
        { 0, 8, BE1::RHI::VertexElement::Usage::TexCoord, 2, BE1::RHI::VertexElement::Type::Float, false },
    };

    vertex2DFormat = BE1::rhi.CreateVertexFormat(COUNT_OF(vertex2DElements), vertex2DElements);
    
    const BE1::RHI::VertexElement vertex3DElements[] = {
        { 0, 0,  BE1::RHI::VertexElement::Usage::Position, 3, BE1::RHI::VertexElement::Type::Float, false },
        { 0, 12, BE1::RHI::VertexElement::Usage::TexCoord, 2, BE1::RHI::VertexElement::Type::Float, false },
        { 0, 20, BE1::RHI::VertexElement::Usage::Color,    4, BE1::RHI::VertexElement::Type::UByte, true }
    };

    vertex3DFormat = BE1::rhi.CreateVertexFormat(COUNT_OF(vertex3DElements), vertex3DElements);
}

void Application::InitShaders() {
    const char *defaultVSText = R"(
in vec4 in_position : POSITION;
in vec2 in_texCoord : TEXCOORD;
in vec4 in_color : COLOR;
out vec4 v2f_color;
out vec2 v2f_texCoord;
uniform mat4 modelViewProjMatrix;
void main() {
    v2f_color = in_color;
    v2f_texCoord = in_texCoord;
    gl_Position = modelViewProjMatrix * in_position;
})";

    const char *defaultFSText = R"(
//precision highp float;
in vec4 v2f_color;
in vec2 v2f_texCoord;
out vec4 o_fragColor : FRAG_COLOR;
uniform sampler2D baseMap;
void main() {
    o_fragColor = tex2D(baseMap, v2f_texCoord) * v2f_color;
})";

    defaultShader = BE1::rhi.CreateShader("default_shader", defaultVSText, defaultFSText);

    const char *clipRectVSText = R"(
in vec4 in_position : POSITION;
in vec2 in_texCoord : TEXCOORD;
out vec2 v2f_texCoord;
void main() {
    v2f_texCoord = in_texCoord;
    gl_Position = in_position;
})";

    const char *clipRectFSText = R"(
//precision highp float;
in vec2 v2f_texCoord;
out vec4 o_fragColor : FRAG_COLOR;
uniform sampler2D baseMap;
void main() {
    o_fragColor = tex2D(baseMap, v2f_texCoord);
})";
    
    clipRectShader = BE1::rhi.CreateShader("cliprect_shader", clipRectVSText, clipRectFSText);
}

void Application::Init(BE1::RHI::WindowHandle windowHandle) {
    BE1::RHI::Settings settings;
    settings.colorBits      = 24;
    settings.alphaBits      = settings.colorBits == 32 ? 8 : 0;
    settings.depthBits      = 24;
    settings.stencilBits    = 0;
    settings.multiSamples   = 0;

    BE1::rhi.Init(windowHandle, &settings);
}

void Application::Shutdown() {
    BE1::rhi.Shutdown();
}

void Application::LoadResources() {
    InitVertexFormats();

    streamBuffer = BE1::rhi.CreateBuffer(BE1::RHI::BufferType::Vertex, BE1::RHI::BufferUsage::Stream, 0);

    const Vertex3D verts[] = {
        { BE1::Vec3(-200,  200, 0), BE1::Vec2(0, 0), 0xFFFFFFFF },
        { BE1::Vec3(-200, -200, 0), BE1::Vec2(0, 1), 0xFFFFFFFF },
        { BE1::Vec3( 200, -200, 0), BE1::Vec2(1, 1), 0xFFFFFFFF },
        { BE1::Vec3( 200, -200, 0), BE1::Vec2(1, 1), 0xFFFFFFFF },
        { BE1::Vec3( 200,  200, 0), BE1::Vec2(1, 0), 0xFFFFFFFF },
        { BE1::Vec3(-200,  200, 0), BE1::Vec2(0, 0), 0xFFFFFFFF }
    };

    defaultVertexBuffer = BE1::rhi.CreateBuffer(BE1::RHI::BufferType::Vertex, BE1::RHI::BufferUsage::Static, sizeof(Vertex3D) * COUNT_OF(verts), 0, verts);

    BE1::Image *image = BE1::Image::NewImageFromFile("Data/EngineTextures/checker.dds");
    if (!image->IsEmpty()) {
        defaultTexture = BE1::rhi.CreateTexture(BE1::RHI::TextureType::Texture2D);
        BE1::rhi.BindTexture(defaultTexture);
        BE1::rhi.SetTextureImage(BE1::RHI::TextureType::Texture2D, image, image->GetFormat(), true, true);
        BE1::rhi.SetTextureAddressMode(BE1::RHI::AddressMode::Clamp);
        BE1::rhi.SetTextureFilter(BE1::RHI::TextureFilter::LinearMipmapLinear);
        BE1::rhi.BindTexture(BE1::RHI::NullTexture);
        delete image;
    }

    InitShaders();

    renderTargetTexture = BE1::rhi.CreateTexture(BE1::RHI::TextureType::Texture2D);
    BE1::Image rtImage;
    rtImage.InitFromMemory(200, 200, 1, 1, 1, BE1::Image::Format::RGBA_8_8_8_8, BE1::Image::GammaSpace::sRGB, nullptr, 0);

    BE1::rhi.BindTexture(renderTargetTexture);
    BE1::rhi.SetTextureImage(BE1::RHI::TextureType::Texture2D, &rtImage, BE1::Image::Format::RGBA_8_8_8_8, false, true);
    BE1::rhi.SetTextureAddressMode(BE1::RHI::AddressMode::Clamp);
    BE1::rhi.SetTextureFilter(BE1::RHI::TextureFilter::Linear);
}
    
void Application::FreeResources() {
    BE1::rhi.DestroyTexture(defaultTexture);
    BE1::rhi.DestroyShader(defaultShader);
    BE1::rhi.DestroyBuffer(defaultVertexBuffer);
    BE1::rhi.DestroyVertexFormat(vertex2DFormat);
    BE1::rhi.DestroyVertexFormat(vertex3DFormat);
    BE1::rhi.DestroyBuffer(streamBuffer);
}

BE1::RHI::Handle Application::CreateRenderTarget(const BE1::RHI::Handle contextHandle) {
    BE1::rhi.SetContext(contextHandle);

    return BE1::rhi.CreateRenderTarget(BE1::RHI::RenderTargetType::RT2D, 400, 400, 1, &renderTargetTexture, BE1::RHI::NullTexture, BE1::RHI::RenderTargetFlag::HasDepthBuffer | BE1::RHI::RenderTargetFlag::SRGBWrite);
}

void Application::DrawClipRect(float s1, float t1, float s2, float t2) {
    const struct Pic2D {
        BE1::Vec2 position;
        BE1::Vec2 st;
    } verts[] = { 
        { BE1::Vec2(-1, -1), BE1::Vec2(s1, t1) },
        { BE1::Vec2(+1, -1), BE1::Vec2(s2, t1) },
        { BE1::Vec2(+1, +1), BE1::Vec2(s2, t2) },
        { BE1::Vec2(-1, +1), BE1::Vec2(s1, t2) }
    };
        
    BE1::rhi.BindBuffer(BE1::RHI::BufferType::Vertex, streamBuffer);
    BE1::rhi.BufferDiscardWrite(streamBuffer, 4 * sizeof(verts[0]), verts);

    BE1::rhi.SetVertexFormat(vertex2DFormat);
    BE1::rhi.SetStreamSource(0, streamBuffer, 0, sizeof(verts[0]));
    BE1::rhi.DrawArrays(BE1::RHI::Topology::TriangleFan, 0, 4);
}

void Application::DrawToRenderTarget(BE1::RHI::Handle renderTargetHandle, float t) {
    BE1::rhi.BeginRenderTarget(renderTargetHandle);
    
    BE1::Rect rect = BE1::Rect(0, 0, 200, 200);
    BE1::rhi.SetViewport(rect);

    // Set projection matrix to flip vertically
    BE1::Mat4 projMatrix;
    projMatrix.SetOrtho(-400, +400, +400, -400, -1, +1);
    
    BE1::Mat4 modelMatrix = BE1::Rotation(BE1::Vec3(0, 0, 0), BE1::Vec3(0, 0, 1), -t * 80.0f).ToMat4();
    
    modelViewProjMatrix = projMatrix * modelMatrix;
    
    BE1::rhi.SetStateBits(BE1::RHI::ColorWrite | BE1::RHI::AlphaWrite);
    BE1::rhi.Clear(BE1::RHI::ClearBit::Color | BE1::RHI::ClearBit::Depth, BE1::Color4(0.0f, 0.0f, 0.5f, 0), 0, 0);
    BE1::rhi.SetCullFace(BE1::RHI::CullType::None);
    
    BE1::rhi.BindShader(defaultShader);
    BE1::rhi.SetShaderConstant4x4f(BE1::rhi.GetShaderConstantIndex(defaultShader, "modelViewProjMatrix"), true, modelViewProjMatrix);
    BE1::rhi.SetTexture(BE1::rhi.GetSamplerUnit(defaultShader, "baseMap"), defaultTexture);
    
    BE1::rhi.BindBuffer(BE1::RHI::BufferType::Vertex, defaultVertexBuffer);
    BE1::rhi.SetVertexFormat(vertex3DFormat);
    BE1::rhi.SetStreamSource(0, defaultVertexBuffer, 0, sizeof(Vertex3D));
    BE1::rhi.DrawArrays(BE1::RHI::Topology::TriangleList, 0, 6);

    BE1::rhi.EndRenderTarget();
}

void Application::Draw(const BE1::RHI::Handle contextHandle, const BE1::RHI::Handle renderTargetHandle, float t) {
    BE1::rhi.SetContext(contextHandle);

#ifdef ENABLE_IMGUI
    BE1::rhi.ImGuiBeginFrame(contextHandle);
#endif

    DrawToRenderTarget(renderTargetHandle, t);

    BE1::RHI::DisplayMetrics displayMetrics;
    BE1::rhi.GetDisplayMetrics(contextHandle, &displayMetrics);
    
    BE1::Rect rect = BE1::Rect(0, 0, displayMetrics.backingWidth, displayMetrics.backingHeight);
    BE1::rhi.SetViewport(rect);

    BE1::Mat4 modelMatrix = BE1::Rotation(BE1::Vec3(0, 0, 0), BE1::Vec3(0, 0, 1), t * 40.0f).ToMat4();

    BE1::Mat4 projMatrix;
    projMatrix.SetOrtho(-displayMetrics.backingWidth * 0.5f, displayMetrics.backingWidth * 0.5f, -displayMetrics.backingHeight * 0.5f, displayMetrics.backingHeight * 0.5f, -1, 1);

    modelViewProjMatrix = projMatrix * modelMatrix;

    BE1::rhi.SetStateBits(BE1::RHI::ColorWrite | BE1::RHI::AlphaWrite);
    BE1::rhi.Clear(BE1::RHI::ClearBit::Color | BE1::RHI::ClearBit::Depth, BE1::Color4(0.5f, 0.5f, 0.5f, 0), 0, 0);
    BE1::rhi.SetCullFace(BE1::RHI::CullType::None);

#if 0
    BE1::rhi.BindShader(clipRectShader);
    BE1::rhi.SetTexture(BE1::rhi.GetSamplerUnit(clipRectShader, "baseMap"), renderTargetTexture);

    DrawClipRect(0.0f, 1.0f, 1.0f, 0.0f);
#else
    BE1::rhi.BindShader(defaultShader);
    BE1::rhi.SetShaderConstant4x4f(BE1::rhi.GetShaderConstantIndex(defaultShader, "modelViewProjMatrix"), true, modelViewProjMatrix);
    BE1::rhi.SetTexture(BE1::rhi.GetSamplerUnit(defaultShader, "baseMap"), renderTargetTexture);

    BE1::rhi.BindBuffer(BE1::RHI::BufferType::Vertex, defaultVertexBuffer);
    BE1::rhi.SetVertexFormat(vertex3DFormat);
    BE1::rhi.SetStreamSource(0, defaultVertexBuffer, 0, sizeof(Vertex3D));
    BE1::rhi.DrawArrays(BE1::RHI::Topology::TriangleList, 0, 6);
#endif

#ifdef ENABLE_IMGUI
    ImGui::Text("Hello, world !");
    //ImGui::ShowDemoWindow();
    
    BE1::rhi.ImGuiRender();
#endif

    BE1::rhi.SwapBuffers();
}

void Application::RunFrame() {
    BE1::cmdSystem.ExecuteCommandBuffer();
}
