#pragma once

/*
-------------------------------------------------------------------------------

    Render Target

-------------------------------------------------------------------------------
*/

BE_NAMESPACE_BEGIN

class Texture;

class RenderTarget {
    friend class RenderBackEnd;

public:
    enum {
        MaxMultipleColorTextures = 8
    };

    int                     GetWidth() const;
    int                     GetHeight() const;

    const Texture *         ColorTexture(int index = 0) const { return colorTextures[index]; }
    const Texture *         DepthStencilTexture() const { return depthStencilTexture; }

    void                    Begin(int level = 0, int sliceIndex = 0, unsigned int mrtBitMask = 0) const;
    void                    End() const;

    void                    Clear(const Color4 &clearColor, float clearDepth, int clearStencil) const;

    static RenderTarget *   Create(int numColorTextures, const Texture **colorTextures, const Texture *depthStencilTexture, int hasDepthStencilBuffer);
    static RenderTarget *   Create(const Texture *colorTexture, const Texture *depthStencilTexture, int flags);
    static void             Delete(RenderTarget *renderTarget);

private:
    friend void             RB_DrawRenderTargetTexture();

    Renderer::Handle        rtHandle;
    const Texture *         colorTextures[MaxMultipleColorTextures];
    const Texture *         depthStencilTexture;
    int                     flags;
    static Array<RenderTarget *> rts;
};

BE_NAMESPACE_END
