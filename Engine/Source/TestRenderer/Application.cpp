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

static CVAR(r_colorBits, L"32", BE1::CVar::Integer | BE1::CVar::Archive, L"number of color bits");
static CVAR(r_depthBits, L"24", BE1::CVar::Integer | BE1::CVar::Archive, L"number of depth bits");
static CVAR(r_stencilBits, L"8", BE1::CVar::Integer | BE1::CVar::Archive, L"number of stencil bits");
static CVAR(r_multiSamples, L"0", BE1::CVar::Integer | BE1::CVar::Archive, L"number of antialiasing samples");

void Application::InitVertexFormats() {
    const BE1::Renderer::VertexElement vertex2DElements[] = {
        { 0, 0, BE1::Renderer::VertexElement::Position, 2, BE1::Renderer::VertexElement::FloatType, false },
        { 0, 8, BE1::Renderer::VertexElement::TexCoord, 2, BE1::Renderer::VertexElement::FloatType, false },
    };

    vertex2DFormat = BE1::glr.CreateVertexFormat(COUNT_OF(vertex2DElements), vertex2DElements);
    
    const BE1::Renderer::VertexElement vertex3DElements[] = {
        { 0, 0,  BE1::Renderer::VertexElement::Position, 3, BE1::Renderer::VertexElement::FloatType, false },
        { 0, 12, BE1::Renderer::VertexElement::TexCoord, 2, BE1::Renderer::VertexElement::FloatType, false },
        { 0, 20, BE1::Renderer::VertexElement::Color,    4, BE1::Renderer::VertexElement::UByteType, true }
    };

    vertex3DFormat = BE1::glr.CreateVertexFormat(COUNT_OF(vertex3DElements), vertex3DElements);
}

void Application::InitShaders() {
    const char *defaultVSText =
        "in vec4 in_position : POSITION;\n"
        "in vec2 in_texCoord : TEXCOORD;\n"
        "in vec4 in_color : COLOR;\n"
        "out vec4 v2f_color;\n"
        "out vec2 v2f_texCoord;\n"
        "uniform mat4 modelViewProjMatrix;\n"
        "void main() {\n"
        "   v2f_color = in_color;\n"
        "   v2f_texCoord = in_texCoord;\n"
        "   gl_Position = modelViewProjMatrix * in_position;\n"
        "}";

    const char *defaultFSText =
        //"precision highp float;\n"
        "in vec4 v2f_color;\n"
        "in vec2 v2f_texCoord;\n"
        "out vec4 o_fragColor : FRAG_COLOR;\n"
        "uniform sampler2D baseMap;\n"
        "void main() {\n"
        "   o_fragColor = tex2D(baseMap, v2f_texCoord) * v2f_color;\n"
        "}";

    defaultShader = BE1::glr.CreateShader("default_shader", defaultVSText, defaultFSText);

    const char *clipRectVSText = 
        "in vec4 in_position : POSITION;\n"
        "in vec2 in_texCoord : TEXCOORD;\n"
        "out vec2 v2f_texCoord;\n"
        "void main() {\n"
        "   v2f_texCoord = in_texCoord;\n"
        "   gl_Position = in_position;\n"
        "}";

    const char *clipRectFSText =
        //"precision highp float;\n"
        "in vec2 v2f_texCoord;\n"
        "out vec4 o_fragColor : FRAG_COLOR;\n"
        "uniform sampler2D baseMap;\n"
        "void main() {\n"
        "   o_fragColor = tex2D(baseMap, v2f_texCoord);\n"
        "}";
    
    clipRectShader = BE1::glr.CreateShader("cliprect_shader", clipRectVSText, clipRectFSText);
}

void Application::Init() {
    BE1::Renderer::Settings settings;
    settings.colorBits      = r_colorBits.GetInteger();
    settings.alphaBits      = settings.colorBits == 32 ? 8 : 0;
    settings.depthBits      = r_depthBits.GetInteger();
    settings.stencilBits    = r_stencilBits.GetInteger();
    settings.multiSamples   = r_multiSamples.GetInteger();

    BE1::glr.Init(&settings);
}

void Application::Shutdown() {
    BE1::glr.Shutdown();
}

void Application::LoadResources() {
    InitVertexFormats();

    streamBuffer = BE1::glr.CreateBuffer(BE1::Renderer::VertexBuffer, BE1::Renderer::Stream, 0);

    const Vertex3D verts[] = {
        { BE1::Vec3(0.0f,  288.7f, 0), BE1::Vec2(0.5,    1.0), 0x00FFFFFF },
        { BE1::Vec3(-250.0f, -144.3f, 0), BE1::Vec2(0.067,  0.25), 0x00FFFFFF },
        { BE1::Vec3(250.0f, -144.3f, 0), BE1::Vec2(0.933,  0.25), 0x00FFFFFF }
    };

    defaultVertexBuffer = BE1::glr.CreateBuffer(BE1::Renderer::VertexBuffer, BE1::Renderer::Static, sizeof(Vertex3D) * COUNT_OF(verts), 0, verts);

    BE1::Image *image = BE1::Image::NewImageFromFile("Data/EngineTextures/defaultUVs.png");
    if (!image->IsEmpty()) {
        defaultTexture = BE1::glr.CreateTexture(BE1::Renderer::Texture2D);
        BE1::glr.BindTexture(defaultTexture);
        BE1::glr.SetTextureImage(BE1::Renderer::Texture2D, image, image->GetFormat(), false, false);
        BE1::glr.SetTextureAddressMode(BE1::Renderer::Repeat);
        BE1::glr.SetTextureFilter(BE1::Renderer::Linear);
        BE1::glr.BindTexture(BE1::Renderer::NullTexture);
        delete image;
    }

    InitShaders();

    renderTargetTexture = BE1::glr.CreateTexture(BE1::Renderer::Texture2D);
    BE1::Image rtImage;
    rtImage.InitFromMemory(200, 200, 1, 1, 1, BE1::Image::RGBA_8_8_8_8, NULL, 0);

    BE1::glr.BindTexture(renderTargetTexture);
    BE1::glr.SetTextureImage(BE1::Renderer::Texture2D, &rtImage, BE1::Image::RGBA_8_8_8_8, false, false);
    BE1::glr.SetTextureAddressMode(BE1::Renderer::Clamp);
    BE1::glr.SetTextureFilter(BE1::Renderer::Linear);
}
    
void Application::FreeResources() {
    BE1::glr.DeleteTexture(defaultTexture);
    BE1::glr.DeleteShader(defaultShader);
    BE1::glr.DeleteBuffer(defaultVertexBuffer);
    BE1::glr.DeleteVertexFormat(vertex2DFormat);
    BE1::glr.DeleteVertexFormat(vertex3DFormat);
    BE1::glr.DeleteBuffer(streamBuffer);
}

BE1::Renderer::Handle Application::CreateRenderTarget(const BE1::Renderer::Handle contextHandle) {
    BE1::glr.SetContext(contextHandle);

    return BE1::glr.CreateRenderTarget(BE1::Renderer::RenderTarget2D, 200, 200, 1, &renderTargetTexture, BE1::Renderer::NullTexture, true, BE1::Renderer::HasDepthBuffer);
}

void Application::DrawClipRect(float s, float t, float s2, float t2) {
    const struct Pic2D {
        BE1::Vec2 position;
        BE1::Vec2 st;
    } verts[] = { 
        { BE1::Vec2(-1, -1), BE1::Vec2(s, t2) },
        { BE1::Vec2(+1, -1), BE1::Vec2(s2, t2) },
        { BE1::Vec2(+1, +1), BE1::Vec2(s2, t) },
        { BE1::Vec2(-1, +1), BE1::Vec2(s, t) }
    };
        
    BE1::glr.BindBuffer(BE1::Renderer::VertexBuffer, streamBuffer);
    BE1::glr.BufferDiscardWrite(streamBuffer, 4 * sizeof(verts[0]), verts);

    BE1::glr.SetVertexFormat(vertex2DFormat);
    BE1::glr.SetStreamSource(0, streamBuffer, 0, sizeof(verts[0]));
    BE1::glr.DrawArrays(BE1::Renderer::TriangleFanPrim, 0, 4);
}

void Application::DrawToRenderTarget(BE1::Renderer::Handle renderTargetHandle, float t) {
    BE1::glr.BeginRenderTarget(renderTargetHandle);
    
    BE1::Rect rect = BE1::Rect(0, 0, 200, 200);
    BE1::glr.SetViewport(rect);
    
    BE1::Mat4 projMatrix;
    projMatrix.SetOrtho(-400, 400, -400, 400, -1, 1);
    
    BE1::Mat4 modelMatrix = BE1::Rotation(BE1::Vec3(0, 0, 0), BE1::Vec3(0, 0, 1), -t * 80.0f).ToMat4();
    
    modelViewProjMatrix = projMatrix * modelMatrix;
    
    BE1::glr.SetStateBits(BE1::Renderer::ColorWrite | BE1::Renderer::AlphaWrite);
    BE1::glr.Clear(BE1::Renderer::ColorBit | BE1::Renderer::DepthBit, BE1::Color4(0.0f, 0.0f, 0.5f, 0), 0, 0);
    BE1::glr.SetCullFace(BE1::Renderer::NoCull);
    
    BE1::glr.BindShader(defaultShader);
    BE1::glr.SetShaderConstant4x4f(BE1::glr.GetShaderConstantLocation(defaultShader, "modelViewProjMatrix"), true, modelViewProjMatrix);
    BE1::glr.SetTexture(BE1::glr.GetSamplerUnit(defaultShader, "baseMap"), defaultTexture);
    
    BE1::glr.BindBuffer(BE1::Renderer::VertexBuffer, defaultVertexBuffer);
    BE1::glr.SetVertexFormat(vertex3DFormat);
    BE1::glr.SetStreamSource(0, defaultVertexBuffer, 0, sizeof(Vertex3D));
    BE1::glr.DrawArrays(BE1::Renderer::TrianglesPrim, 0, 3);

    BE1::glr.EndRenderTarget();
}

void Application::Draw(const BE1::Renderer::Handle contextHandle, const BE1::Renderer::Handle renderTargetHandle, float t) {
    BE1::glr.SetContext(contextHandle);
    
    DrawToRenderTarget(renderTargetHandle, t);

    BE1::glr.GetContextSize(contextHandle, NULL, NULL, &screenWidth, &screenHeight);
    
    BE1::Rect rect = BE1::Rect(0, 0, screenWidth, screenHeight);
    BE1::glr.SetViewport(rect);
    
    BE1::Mat4 modelMatrix = BE1::Rotation(BE1::Vec3(0, 0, 0), BE1::Vec3(0, 0, 1), t * 40.0f).ToMat4();

    BE1::Mat4 projMatrix;
    projMatrix.SetOrtho(-screenWidth * 0.5f, screenWidth * 0.5f, -screenHeight * 0.5f, screenHeight * 0.5f, -1, 1);
    
    modelViewProjMatrix = projMatrix * modelMatrix;

    BE1::glr.SetStateBits(BE1::Renderer::ColorWrite | BE1::Renderer::AlphaWrite);
    BE1::glr.Clear(BE1::Renderer::ColorBit | BE1::Renderer::DepthBit, BE1::Color4(0.5f, 0.5f, 0.5f, 0), 0, 0);
    BE1::glr.SetCullFace(BE1::Renderer::NoCull);
        
    BE1::glr.BindShader(defaultShader);
    BE1::glr.SetShaderConstant4x4f(BE1::glr.GetShaderConstantLocation(defaultShader, "modelViewProjMatrix"), true, modelViewProjMatrix);
    BE1::glr.SetTexture(BE1::glr.GetSamplerUnit(defaultShader, "baseMap"), renderTargetTexture);

    BE1::glr.BindBuffer(BE1::Renderer::VertexBuffer, defaultVertexBuffer);
    BE1::glr.SetVertexFormat(vertex3DFormat);
    BE1::glr.SetStreamSource(0, defaultVertexBuffer, 0, sizeof(Vertex3D));
    BE1::glr.DrawArrays(BE1::Renderer::TrianglesPrim, 0, 3);

    /*BE1::glr.BindShader(clipRectShader);
    BE1::glr.SetTexture(BE1::glr.GetSamplerUnit(clipRectShader, "baseMap"), defaultTexture);

    DrawClipRect(0.0f, 0.0f, 1.0f, 1.0f);*/

    BE1::glr.SwapBuffers();
}

void Application::RunFrame() {
   BE1::cmdSystem.ExecuteCommandBuffer();
}
