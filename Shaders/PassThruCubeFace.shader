shader "PassThruCubeFace" {
    glsl_vp {
        $include "clipQuad.vp"
    }

    glsl_fp {
        $include "FragmentCommon.glsl"

        in vec2 v2f_texCoord;

        out vec4 o_fragColor : FRAG_COLOR;

        uniform samplerCube cubemap;
        uniform int targetCubeMapSize;
        uniform int targetCubeMapFace;

        void main() {
            int targetFaceX = int(min(floor(v2f_texCoord.x * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));
            int targetFaceY = int(min(floor(v2f_texCoord.y * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));

            vec3 cubicCoords = FaceToGLCubeMapCoords(targetCubeMapFace, targetFaceX, targetFaceY, targetCubeMapSize).xyz;

            o_fragColor = texCUBE(cubemap, cubicCoords);
        }
    }
}
