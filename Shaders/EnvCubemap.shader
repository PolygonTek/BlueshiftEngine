shader "EnvCubemap" {
    properties {
        _EXPLICIT_MIPMAPS("Explicit Mipmaps") : enum "No Mipmaps;Use Mipmaps" = "0" (shaderDefine)
        mipLevel("Mip Level") : float range 0 1.0 0.001 = "0"
        envCubeMap("Env Cube Map") : texture = "_defaultCubeTexture"
    }

    glsl_vp {
        $include "EnvCubemap.vp"
    }

    glsl_fp {
        $include "EnvCubemap.fp"
    }
}
