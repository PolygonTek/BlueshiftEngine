#include "Precompiled.h"
#include "CoreCommon.h"
#include "Render/Render.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

ShadowMapManager    shadowMapManager;

ShadowMapManager::ShadowMapManager() {
    indirectionCubeMapTexture = NULL;

    shadowRenderTexture = NULL;
    vscmTexture = NULL;

    shadowMapRT = NULL;
    vscmRT = NULL;

    vscmCleared[0] = false;
    vscmCleared[1] = false;
    vscmCleared[2] = false;
    vscmCleared[3] = false;
    vscmCleared[4] = false;
    vscmCleared[5] = false;
}

void ShadowMapManager::Shutdown() {
    if (shadowMapRT) {
        if (shadowRenderTexture) {
            textureManager.ReleaseAndDeleteTexture(shadowRenderTexture);
        }

        if (shadowMapRT) {
            RenderTarget::Delete(shadowMapRT);
            shadowMapRT = NULL;
        }
    }

    if (vscmRT) {
        if (vscmTexture) {
            textureManager.ReleaseAndDeleteTexture(vscmTexture);
        }

        if (vscmRT) {
            RenderTarget::Delete(vscmRT);
            vscmRT = NULL;
        }
    }
    
    if (indirectionCubeMapTexture) {
        textureManager.ReleaseAndDeleteTexture(indirectionCubeMapTexture);
        indirectionCubeMapTexture = NULL;
    }
}

void ShadowMapManager::Init() {
    Shutdown();

    if (r_shadows.GetInteger() == 0) {
        return;
    }

    Image::Format shadowImageFormat = (r_shadowMapFloat.GetBool() && glr.SupportsDepthBufferFloat()) ? Image::Depth_32F : Image::Depth_24;

    Renderer::TextureType textureType = Renderer::Texture2D;

    int csmCount = r_CSM_count.GetInteger();
    if (csmCount > 1) {
        textureType = Renderer::Texture2DArray;
    }

    // Cascaded shadow map
    shadowRenderTexture = textureManager.AllocTexture("_shadowRender");
    shadowRenderTexture->CreateEmpty(textureType, r_shadowMapSize.GetInteger(), r_shadowMapSize.GetInteger(), 1, csmCount, shadowImageFormat,
        Texture::Shadow | Texture::Clamp | Texture::NoMipmaps | Texture::HighQuality | Texture::HighPriority);

    shadowMapRT = RenderTarget::Create(NULL, shadowRenderTexture, 0);

    // Virtual shadow cube map
    vscmTexture = textureManager.AllocTexture(va("_vscmRender"));
    vscmTexture->CreateEmpty(Renderer::Texture2D, r_shadowCubeMapSize.GetInteger(), r_shadowCubeMapSize.GetInteger(), 1, 1, shadowImageFormat,
        Texture::Shadow | Texture::Clamp | Texture::NoMipmaps | Texture::NonPowerOfTwo | Texture::HighQuality | Texture::HighPriority);
    
    vscmRT = RenderTarget::Create(NULL, vscmTexture, 0);
    vscmRT->Clear(vec4_zero, 1.0f, 0);

    vscmCleared[0] = false;
    vscmCleared[1] = false;
    vscmCleared[2] = false;
    vscmCleared[3] = false;
    vscmCleared[4] = false;
    vscmCleared[5] = false;

    if (!indirectionCubeMapTexture) {
        indirectionCubeMapTexture = textureManager.AllocTexture("_indirectionCubeMap");
        indirectionCubeMapTexture->CreateIndirectionCubemap(256, vscmRT->GetWidth(), vscmRT->GetHeight());
    }
    vscmBiasedFov = DEG2RAD(90.0f + 0.8f);
    vscmBiasedScale = 1.0f / Math::Tan(vscmBiasedFov * 0.5f);
}

BE_NAMESPACE_END
