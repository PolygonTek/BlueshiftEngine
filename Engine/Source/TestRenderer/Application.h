#pragma once

class Application {
public:
    void                    Init();
    void                    Shutdown();

    void                    LoadResources();
    void                    FreeResources();

    BE1::Renderer::Handle   CreateRenderTarget(const BE1::Renderer::Handle contextHandle);

    void                    Draw(const BE1::Renderer::Handle contextHandle, const BE1::Renderer::Handle renderTargetHandle, float t);

    void                    RunFrame();

private:
    void                    DrawClipRect(float s, float t, float s2, float t2);
    void                    DrawToRenderTarget(const BE1::Renderer::Handle renderTargetHandle, float t);

    void                    InitVertexFormats();
    void                    InitShaders();
    
    int                     screenWidth;
    int                     screenHeight;

    BE1::Renderer::Handle   streamBuffer;
    BE1::Renderer::Handle   defaultVertexBuffer;
    BE1::Renderer::Handle   vertex2DFormat;
    BE1::Renderer::Handle   vertex3DFormat;
    BE1::Renderer::Handle   defaultShader;
    BE1::Renderer::Handle   clipRectShader;
    BE1::Renderer::Handle   defaultTexture;
    
    BE1::Renderer::Handle   renderTargetTexture;

    BE1::Mat4               modelViewProjMatrix;
};

extern Application          app;
