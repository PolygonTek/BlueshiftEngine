// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Application.h"

#ifdef ENABLE_IMGUI
#include "imgui/imgui.h"
#endif

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
    const BE1::Graphics::VertexElement vertex2DElements[] = {
        { 0, 0, BE1::Graphics::VertexElement::Usage::Position, 2, BE1::Graphics::VertexElement::Type::Float, false },
        { 0, 8, BE1::Graphics::VertexElement::Usage::TexCoord, 2, BE1::Graphics::VertexElement::Type::Float, false },
    };

    vertex2DFormat = BE1::graphics.CreateVertexFormat(COUNT_OF(vertex2DElements), vertex2DElements);
    
    const BE1::Graphics::VertexElement vertex3DElements[] = {
        { 0, 0,  BE1::Graphics::VertexElement::Usage::Position, 3, BE1::Graphics::VertexElement::Type::Float, false },
        { 0, 12, BE1::Graphics::VertexElement::Usage::TexCoord, 2, BE1::Graphics::VertexElement::Type::Float, false },
        { 0, 20, BE1::Graphics::VertexElement::Usage::Color,    4, BE1::Graphics::VertexElement::Type::UByte, true }
    };

    vertex3DFormat = BE1::graphics.CreateVertexFormat(COUNT_OF(vertex3DElements), vertex3DElements);
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

    defaultShader = BE1::graphics.CreateShader("default_shader", defaultVSText, defaultFSText);

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
    
    clipRectShader = BE1::graphics.CreateShader("cliprect_shader", clipRectVSText, clipRectFSText);
}

void Application::Init(BE1::Graphics::WindowHandle windowHandle) {
    BE1::Graphics::Settings settings;
    settings.colorBits      = 24;
    settings.alphaBits      = settings.colorBits == 32 ? 8 : 0;
    settings.depthBits      = 24;
    settings.stencilBits    = 0;
    settings.multiSamples   = 0;

    BE1::graphics.Init(windowHandle, &settings);
}

void Application::Shutdown() {
    BE1::graphics.Shutdown();
}

void Application::LoadResources() {
    InitVertexFormats();

    streamBuffer = BE1::graphics.CreateBuffer(BE1::Graphics::BufferType::Vertex, BE1::Graphics::BufferUsage::Stream, 0);

    const Vertex3D verts[] = {
        { BE1::Vec3(-200,  200, 0), BE1::Vec2(0, 0), 0xFFFFFFFF },
        { BE1::Vec3(-200, -200, 0), BE1::Vec2(0, 1), 0xFFFFFFFF },
        { BE1::Vec3( 200, -200, 0), BE1::Vec2(1, 1), 0xFFFFFFFF },
        { BE1::Vec3( 200, -200, 0), BE1::Vec2(1, 1), 0xFFFFFFFF },
        { BE1::Vec3( 200,  200, 0), BE1::Vec2(1, 0), 0xFFFFFFFF },
        { BE1::Vec3(-200,  200, 0), BE1::Vec2(0, 0), 0xFFFFFFFF }
    };

    defaultVertexBuffer = BE1::graphics.CreateBuffer(BE1::Graphics::BufferType::Vertex, BE1::Graphics::BufferUsage::Static, sizeof(Vertex3D) * COUNT_OF(verts), 0, verts);

    BE1::Image *image = BE1::Image::NewImageFromFile("Data/EngineTextures/checker.dds");
    if (!image->IsEmpty()) {
        defaultTexture = BE1::graphics.CreateTexture(BE1::Graphics::TextureType::Texture2D);
        BE1::graphics.BindTexture(defaultTexture);
        BE1::graphics.SetTextureImage(BE1::Graphics::TextureType::Texture2D, image, image->GetFormat(), true, true);
        BE1::graphics.SetTextureAddressMode(BE1::Graphics::AddressMode::Clamp);
        BE1::graphics.SetTextureFilter(BE1::Graphics::TextureFilter::LinearMipmapLinear);
        BE1::graphics.BindTexture(BE1::Graphics::NullTexture);
        delete image;
    }

    InitShaders();

    renderTargetTexture = BE1::graphics.CreateTexture(BE1::Graphics::TextureType::Texture2D);
    BE1::Image rtImage;
    rtImage.InitFromMemory(200, 200, 1, 1, 1, BE1::Image::Format::R8G8B8A8, BE1::Image::GammaSpace::sRGB, nullptr, BE1::Image::Flag::None);

    BE1::graphics.BindTexture(renderTargetTexture);
    BE1::graphics.SetTextureImage(BE1::Graphics::TextureType::Texture2D, &rtImage, BE1::Image::Format::R8G8B8A8, false, true);
    BE1::graphics.SetTextureAddressMode(BE1::Graphics::AddressMode::Clamp);
    BE1::graphics.SetTextureFilter(BE1::Graphics::TextureFilter::Linear);
}
    
void Application::FreeResources() {
    BE1::graphics.DestroyTexture(defaultTexture);
    BE1::graphics.DestroyShader(defaultShader);
    BE1::graphics.DestroyShader(clipRectShader);
    BE1::graphics.DestroyBuffer(defaultVertexBuffer);
    BE1::graphics.DestroyVertexFormat(vertex2DFormat);
    BE1::graphics.DestroyVertexFormat(vertex3DFormat);
    BE1::graphics.DestroyBuffer(streamBuffer);
}

BE1::Graphics::Handle Application::CreateRenderTarget(const BE1::Graphics::Handle contextHandle) {
    BE1::graphics.SetContext(contextHandle);

    return BE1::graphics.CreateRenderTarget(BE1::Graphics::RenderTargetType::RT2D, 400, 400, 1, &renderTargetTexture, BE1::Graphics::NullTexture, BE1::Graphics::RenderTargetFlag::HasDepthBuffer | BE1::Graphics::RenderTargetFlag::SRGBWrite);
}

void Application::DrawClipRect(float s1, float t1, float s2, float t2) {
    const struct Pic2D {
        BE1::Vec2 position;
        BE1::Vec2 st;
    } verts[] = { 
        { BE1::Vec2(-1, -1), BE1::Vec2(s1, t1) },
        { BE1::Vec2(+1, -1), BE1::Vec2(s2, t1) },
        { BE1::Vec2(-1, +1), BE1::Vec2(s1, t2) },
        { BE1::Vec2(+1, +1), BE1::Vec2(s2, t2) }
    };
        
    BE1::graphics.BindBuffer(BE1::Graphics::BufferType::Vertex, streamBuffer);
    BE1::graphics.BufferDiscardWrite(streamBuffer, 4 * sizeof(verts[0]), verts);

    BE1::graphics.SetVertexFormat(vertex2DFormat);
    BE1::graphics.SetStreamSource(0, streamBuffer, 0, sizeof(verts[0]));
    BE1::graphics.DrawArrays(BE1::Graphics::Topology::TriangleStrip, 0, 4);
}

void Application::DrawToRenderTarget(BE1::Graphics::Handle renderTargetHandle, float t) {
    BE1::graphics.BeginRenderTarget(renderTargetHandle);
    
    BE1::Rect rect = BE1::Rect(0, 0, 200, 200);
    BE1::graphics.SetViewport(rect);

    // Set projection matrix to flip vertically
    BE1::Mat4 projMatrix;
    projMatrix.SetOrthoRH(-400, +400, +400, -400, -1, +1, true);
    
    BE1::Mat4 modelMatrix = BE1::Rotation(BE1::Vec3(0, 0, 0), BE1::Vec3(0, 0, 1), -t * 80.0f).ToMat4();
    
    modelViewProjMatrix = projMatrix * modelMatrix;
    
    BE1::graphics.SetStateBits(BE1::Graphics::ColorWrite | BE1::Graphics::AlphaWrite);
    BE1::graphics.Clear(BE1::Graphics::ClearBit::Color | BE1::Graphics::ClearBit::Depth, BE1::Color4(0.0f, 0.0f, 0.5f, 0), 0, 0);
    BE1::graphics.SetCullFace(BE1::Graphics::CullType::None);
    
    BE1::graphics.BindShader(defaultShader);
    BE1::graphics.SetShaderConstant4x4f(BE1::graphics.GetShaderConstantIndex(defaultShader, "modelViewProjMatrix"), true, modelViewProjMatrix);
    BE1::graphics.SetTexture(BE1::graphics.GetShaderTextureUnit(defaultShader, "baseMap"), defaultTexture);
    
    BE1::graphics.BindBuffer(BE1::Graphics::BufferType::Vertex, defaultVertexBuffer);
    BE1::graphics.SetVertexFormat(vertex3DFormat);
    BE1::graphics.SetStreamSource(0, defaultVertexBuffer, 0, sizeof(Vertex3D));
    BE1::graphics.DrawArrays(BE1::Graphics::Topology::TriangleList, 0, 6);

    BE1::graphics.EndRenderTarget();
}

void Application::Draw(const BE1::Graphics::Handle contextHandle, const BE1::Graphics::Handle renderTargetHandle, float t) {
    BE1::graphics.SetContext(contextHandle);

#ifdef ENABLE_IMGUI
    BE1::graphics.ImGuiBeginFrame(contextHandle);
#endif

    DrawToRenderTarget(renderTargetHandle, t);

    BE1::Graphics::DisplayMetrics displayMetrics;
    BE1::graphics.GetDisplayMetrics(contextHandle, &displayMetrics);
    
    BE1::Rect rect = BE1::Rect(0, 0, displayMetrics.backingWidth, displayMetrics.backingHeight);
    BE1::graphics.SetViewport(rect);

    BE1::Mat4 modelMatrix = BE1::Rotation(BE1::Vec3(0, 0, 0), BE1::Vec3(0, 0, 1), t * 40.0f).ToMat4();

    BE1::Mat4 projMatrix;
    projMatrix.SetOrthoRH(-displayMetrics.backingWidth * 0.5f, displayMetrics.backingWidth * 0.5f, -displayMetrics.backingHeight * 0.5f, displayMetrics.backingHeight * 0.5f, -1, 1, true);

    modelViewProjMatrix = projMatrix * modelMatrix;

    BE1::graphics.SetStateBits(BE1::Graphics::ColorWrite | BE1::Graphics::AlphaWrite);
    BE1::graphics.Clear(BE1::Graphics::ClearBit::Color | BE1::Graphics::ClearBit::Depth, BE1::Color4(0.5f, 0.5f, 0.5f, 0), 0, 0);
    BE1::graphics.SetCullFace(BE1::Graphics::CullType::None);

#if 0
    BE1::graphics.BindShader(clipRectShader);
    BE1::graphics.SetTexture(BE1::graphics.GetShaderTextureUnit(clipRectShader, "baseMap"), renderTargetTexture);

    DrawClipRect(0.0f, 1.0f, 1.0f, 0.0f);
#else
    BE1::graphics.BindShader(defaultShader);
    BE1::graphics.SetShaderConstant4x4f(BE1::graphics.GetShaderConstantIndex(defaultShader, "modelViewProjMatrix"), true, modelViewProjMatrix);
    BE1::graphics.SetTexture(BE1::graphics.GetShaderTextureUnit(defaultShader, "baseMap"), renderTargetTexture);

    BE1::graphics.BindBuffer(BE1::Graphics::BufferType::Vertex, defaultVertexBuffer);
    BE1::graphics.SetVertexFormat(vertex3DFormat);
    BE1::graphics.SetStreamSource(0, defaultVertexBuffer, 0, sizeof(Vertex3D));
    BE1::graphics.DrawArrays(BE1::Graphics::Topology::TriangleList, 0, 6);
#endif

#ifdef ENABLE_IMGUI
    ImGui::Text("Hello, world !");
    //ImGui::ShowDemoWindow();
    
    BE1::graphics.ImGuiRender();
#endif

    BE1::graphics.SwapBuffers();
}

void Application::RunFrame() {
    BE1::cmdSystem.ExecuteCommandBuffer();
}
