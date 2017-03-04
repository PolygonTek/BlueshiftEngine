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
#include "Render/Render.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

Array<RenderTarget *>   RenderTarget::rts;

int RenderTarget::GetWidth() const {
    if (colorTextures[0]) {
        return colorTextures[0]->GetWidth();
    }
    if (depthStencilTexture) {
        return depthStencilTexture->GetWidth();
    }
    assert(0);
    return 0;
}

int RenderTarget::GetHeight() const {
    if (colorTextures[0]) {
        return colorTextures[0]->GetHeight();
    }
    if (depthStencilTexture) {
        return depthStencilTexture->GetHeight();
    }
    assert(0);
    return 0;
}

void RenderTarget::Begin(int level, int sliceIndex, unsigned int mrtBitMask) const {
    glr.BeginRenderTarget(rtHandle, level, sliceIndex, mrtBitMask);
}

void RenderTarget::End() const {
    glr.EndRenderTarget();
}

void RenderTarget::Clear(const Color4 &clearColor, float clearDepth, int clearStencil) const {
    Begin();

    glr.SetViewport(Rect(0, 0, GetWidth(), GetHeight()));

    int writeBits = 0;
    int clearBits = 0;

    if (colorTextures[0]) {
        writeBits |= Renderer::ColorWrite | Renderer::AlphaWrite;
        clearBits |= Renderer::ColorBit;
    }

    if (depthStencilTexture || flags & Renderer::HasDepthBuffer) {
        writeBits |= Renderer::DepthWrite;
        clearBits |= Renderer::DepthBit;
    }

    if ((depthStencilTexture && Image::IsDepthStencilFormat(depthStencilTexture->GetFormat())) || flags & Renderer::HasStencilBuffer) {		
        clearBits |= Renderer::StencilBit;
    }

    glr.SetStateBits(writeBits);
    glr.Clear(clearBits, clearColor, clearDepth, clearStencil);

    End();
}

RenderTarget *RenderTarget::Create(const Texture *colorTexture, const Texture *depthStencilTexture, int flags) {
    if (colorTexture) {
        return Create(1, (const Texture **)&colorTexture, depthStencilTexture, flags);
    }

    return Create(0, nullptr, depthStencilTexture, flags);
}

RenderTarget *RenderTarget::Create(int numColorTextures, const Texture **colorTextures, const Texture *depthStencilTexture, int flags) {
    Renderer::Handle            colorTextureHandles[MaxMultipleColorTextures] = { Renderer::NullTexture, };
    Renderer::Handle            depthStencilTextureHandle = Renderer::NullTexture;
    Renderer::TextureType       textureType;
    int                         width;
    int                         height;
    bool                        sRGB = true;
    bool                        colorMipmaps = false;

    if (numColorTextures > 0 && colorTextures[0]) {
        for (int i = 0; i < numColorTextures; i++) {
            colorTextureHandles[i] = colorTextures[i]->textureHandle;
        }

        textureType     = colorTextures[0]->type;
        width           = colorTextures[0]->width;
        height          = colorTextures[0]->height;
        sRGB            = !Image::IsFloatFormat(colorTextures[0]->format) && !Image::IsHalfFormat(colorTextures[0]->format) && (colorTextures[0]->flags & Texture::SRGB);
    }

    if (depthStencilTexture) {
        if (numColorTextures > 0) {
            if ((textureType != depthStencilTexture->type) || (width != depthStencilTexture->width) || (height != depthStencilTexture->height)) {
                BE_ERRLOG(L"RenderTarget::Create: color textures and depth texture must have same type and size");
                return nullptr;
            }
        } else {
            textureType = depthStencilTexture->type;
            width       = depthStencilTexture->width;
            height      = depthStencilTexture->height;
        }

        depthStencilTextureHandle = depthStencilTexture->textureHandle;
    }

    Renderer::RenderTargetType rtType;
    switch (textureType) {
    case Renderer::Texture2D:
        rtType = Renderer::RenderTarget2D;
        break;
    case Renderer::TextureCubeMap:
        rtType = Renderer::RenderTargetCubeMap;
        break;
    case Renderer::Texture2DArray:
        rtType = Renderer::RenderTarget2DArray;
        break;
    default:
        rtType = Renderer::RenderTarget2D; // to suppress a warning
        BE_FATALERROR(L"RenderTarget::Create: invalid texture for render target");
        break;
    }
    
    RenderTarget *rt = new RenderTarget;
    rt->rtHandle = glr.CreateRenderTarget(rtType, width, height, numColorTextures, colorTextureHandles, depthStencilTextureHandle, sRGB, flags);

    int i = 0;
    for (; i < numColorTextures; i++) {
        rt->colorTextures[i] = colorTextures[i];
    }
    for (; i < MaxMultipleColorTextures; i++) {
        rt->colorTextures[i] = nullptr;
    }

    rt->depthStencilTexture = depthStencilTexture;
    rt->flags = flags;

    rts.Append(rt);

    return rt;
}

void RenderTarget::Delete(RenderTarget *renderTarget) {
    glr.DeleteRenderTarget(renderTarget->rtHandle);
    rts.RemoveIndex(rts.FindIndex(renderTarget));
    delete renderTarget;
}

BE_NAMESPACE_END
