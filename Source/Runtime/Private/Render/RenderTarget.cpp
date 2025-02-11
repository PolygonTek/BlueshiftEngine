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

void RenderTarget::Begin(int level, int sliceIndex) const {
    graphics.BeginRenderTarget(rtHandle, level, sliceIndex);
}

void RenderTarget::End() const {
    graphics.EndRenderTarget();
}

void RenderTarget::SetMRTMask(unsigned int mrtBitMask) const {
    graphics.SetDrawBuffersMask(mrtBitMask);
}

void RenderTarget::Discard(bool depth, bool stencil, int colorBitMask) const {
    graphics.DiscardRenderTarget(depth, stencil, colorBitMask);
}

void RenderTarget::Clear(const Color4 &clearColor, float clearDepth, int clearStencil) const {
    Begin();

    graphics.SetViewport(Rect(0, 0, GetWidth(), GetHeight()));

    int writeBits = 0;
    int clearBits = 0;

    if (colorTextures[0]) {
        writeBits |= Graphics::ColorWrite | Graphics::AlphaWrite;
        clearBits |= Graphics::ClearBit::Color;
    }

    if (depthStencilTexture || flags & Graphics::RenderTargetFlag::HasDepthBuffer) {
        writeBits |= Graphics::DepthWrite;
        clearBits |= Graphics::ClearBit::Depth;
    }

    if ((depthStencilTexture && Image::IsDepthStencilFormat(depthStencilTexture->GetFormat())) || flags & Graphics::RenderTargetFlag::HasStencilBuffer) {
        clearBits |= Graphics::ClearBit::Stencil;
    }

    graphics.SetStateBits(writeBits);
    graphics.Clear(clearBits, clearColor, clearDepth, clearStencil);

    End();
}

void RenderTarget::Blit(const Rect &srcRect, const Rect &dstRect, RenderTarget *target, int mask, int filter) const {
    graphics.BlitRenderTarget(rtHandle, srcRect, target->rtHandle, dstRect, mask, (Graphics::BlitFilter::Enum)filter);
}

RenderTarget *RenderTarget::Create(const Texture *colorTexture, const Texture *depthStencilTexture, int flags) {
    if (colorTexture) {
        return Create(1, (const Texture **)&colorTexture, depthStencilTexture, flags);
    }

    return Create(0, nullptr, depthStencilTexture, flags);
}

RenderTarget *RenderTarget::Create(int numColorTextures, const Texture **colorTextures, const Texture *depthStencilTexture, int flags) {
    Graphics::Handle colorTextureHandles[MaxMultipleColorTextures] = { Graphics::NullTexture, };
    Graphics::Handle depthStencilTextureHandle = Graphics::NullTexture;
    Graphics::TextureType::Enum textureType;
    int width;
    int height;
    bool colorMipmaps = false;

    if (numColorTextures > 0 && colorTextures[0]) {
        for (int i = 0; i < numColorTextures; i++) {
            colorTextureHandles[i] = colorTextures[i]->textureHandle;
        }

        textureType     = colorTextures[0]->type;
        width           = colorTextures[0]->width;
        height          = colorTextures[0]->height;
    }

    if (depthStencilTexture) {
        if (numColorTextures > 0) {
            if ((textureType != depthStencilTexture->type) || (width != depthStencilTexture->width) || (height != depthStencilTexture->height)) {
                BE_ERRLOG("RenderTarget::Create: color textures and depth texture must have same type and size");
                return nullptr;
            }
        } else {
            textureType = depthStencilTexture->type;
            width       = depthStencilTexture->width;
            height      = depthStencilTexture->height;
        }

        depthStencilTextureHandle = depthStencilTexture->textureHandle;
    }

    Graphics::RenderTargetType::Enum rtType;
    switch (textureType) {
    case Graphics::TextureType::Texture2D:
        rtType = Graphics::RenderTargetType::RT2D;
        break;
    case Graphics::TextureType::TextureCubeMap:
        rtType = Graphics::RenderTargetType::RTCubeMap;
        break;
    case Graphics::TextureType::Texture2DArray:
        rtType = Graphics::RenderTargetType::RT2DArray;
        break;
    default:
        rtType = Graphics::RenderTargetType::RT2D; // to suppress a warning
        BE_FATALERROR("RenderTarget::Create: invalid texture for render target");
        break;
    }
    
    RenderTarget *rt = new RenderTarget;
    rt->rtHandle = graphics.CreateRenderTarget(rtType, width, height, numColorTextures, colorTextureHandles, depthStencilTextureHandle, flags);

    int i = 0;
    for (; i < numColorTextures; i++) {
        rt->colorTextures[i] = colorTextures[i];

        colorTextures[i]->renderTarget = rt;
    }
    for (; i < MaxMultipleColorTextures; i++) {
        rt->colorTextures[i] = nullptr;
    }

    rt->depthStencilTexture = depthStencilTexture;
    if (depthStencilTexture) {
        depthStencilTexture->renderTarget = rt;
    }

    rt->flags = flags;

    rts.Append(rt);

    return rt;
}

void RenderTarget::Delete(RenderTarget *renderTarget) {
    for (int i = 0; i < MaxMultipleColorTextures; i++) {
        if (renderTarget->colorTextures[i]) {
            renderTarget->colorTextures[i]->renderTarget = nullptr;
        }
    }

    if (renderTarget->depthStencilTexture) {
        renderTarget->depthStencilTexture->renderTarget = nullptr;
    }

    graphics.DestroyRenderTarget(renderTarget->rtHandle);
    rts.RemoveIndex(rts.FindIndex(renderTarget));
    delete renderTarget;
}

BE_NAMESPACE_END
