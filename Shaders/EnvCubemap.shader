shader "EnvCubemap" {
    properties {
        mipLevel("Mip Level") : float range 0 11 1 = "0"
        envCubeMap("Env Cube Map") : texture "CUBE" = "_defaultCubeTexture"
    }

    glsl_vp {
        $include "EnvCubemap.vp"
    }

    glsl_fp {
        $include "EnvCubemap.fp"
    }
}
