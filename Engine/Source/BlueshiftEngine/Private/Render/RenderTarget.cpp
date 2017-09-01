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
    rhi.BeginRenderTarget(rtHandle, level, sliceIndex, mrtBitMask);
}

void RenderTarget::End() const {
    rhi.EndRenderTarget();
}

void RenderTarget::Clear(const Color4 &clearColor, float clearDepth, int clearStencil) const {
    Begin();

    rhi.SetViewport(Rect(0, 0, GetWidth(), GetHeight()));

    int writeBits = 0;
    int clearBits = 0;

    if (colorTextures[0]) {
        writeBits |= RHI::ColorWrite | RHI::AlphaWrite;
        clearBits |= RHI::ColorBit;
    }

    if (depthStencilTexture || flags & RHI::HasDepthBuffer) {
        writeBits |= RHI::DepthWrite;
        clearBits |= RHI::DepthBit;
    }

    if ((depthStencilTexture && Image::IsDepthStencilFormat(depthStencilTexture->GetFormat())) || flags & RHI::HasStencilBuffer) {		
        clearBits |= RHI::StencilBit;
    }

    rhi.SetStateBits(writeBits);
    rhi.Clear(clearBits, clearColor, clearDepth, clearStencil);

    End();
}

void RenderTarget::Blit(const Rect &srcRect, const Rect &dstRect, RenderTarget *target, int mask, int filter) const {
    rhi.BlitRenderTarget(rtHandle, srcRect, target->rtHandle, dstRect, mask, filter);
}

RenderTarget *RenderTarget::Create(const Texture *colorTexture, const Texture *depthStencilTexture, int flags) {
    if (colorTexture) {
        return Create(1, (const Texture **)&colorTexture, depthStencilTexture, flags);
    }

    return Create(0, nullptr, depthStencilTexture, flags);
}

RenderTarget *RenderTarget::Create(int numColorTextures, const Texture **colorTextures, const Texture *depthStencilTexture, int flags) {
    RHI::Handle         colorTextureHandles[MaxMultipleColorTextures] = { RHI::NullTexture, };
    RHI::Handle         depthStencilTextureHandle = RHI::NullTexture;
    RHI::TextureType    textureType;
    int                 width;
    int                 height;
    bool                sRGB = true;
    bool                colorMipmaps = false;

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

    RHI::RenderTargetType rtType;
    switch (textureType) {
    case RHI::Texture2D:
        rtType = RHI::RenderTarget2D;
        break;
    case RHI::TextureCubeMap:
        rtType = RHI::RenderTargetCubeMap;
        break;
    case RHI::Texture2DArray:
        rtType = RHI::RenderTarget2DArray;
        break;
    default:
        rtType = RHI::RenderTarget2D; // to suppress a warning
        BE_FATALERROR(L"RenderTarget::Create: invalid texture for render target");
        break;
    }
    
    RenderTarget *rt = new RenderTarget;
    rt->rtHandle = rhi.CreateRenderTarget(rtType, width, height, numColorTextures, colorTextureHandles, depthStencilTextureHandle, sRGB, flags);

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
    rhi.DeleteRenderTarget(renderTarget->rtHandle);
    rts.RemoveIndex(rts.FindIndex(renderTarget));
    delete renderTarget;
}

BE_NAMESPACE_END
