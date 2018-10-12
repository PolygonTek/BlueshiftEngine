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

    void                    Blit(const Rect &srcRect, const Rect &dstRect, RenderTarget *target, int mask, int filter) const;

    static RenderTarget *   Create(int numColorTextures, const Texture **colorTextures, const Texture *depthStencilTexture, int hasDepthStencilBuffer);
    static RenderTarget *   Create(const Texture *colorTexture, const Texture *depthStencilTexture, int flags);
    static void             Delete(RenderTarget *renderTarget);

private:
    friend void             RB_DrawRenderTargetTexture();

    RHI::Handle             rtHandle;
    const Texture *         colorTextures[MaxMultipleColorTextures];
    const Texture *         depthStencilTexture;
    int                     flags;
    static Array<RenderTarget *> rts;
};

BE_NAMESPACE_END
