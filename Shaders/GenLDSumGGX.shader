shader "GenLDSumGGX" {
    glsl_vp {
        $include "clipQuad.vp"
    }

    glsl_fp {
        $include "FragmentCommon.glsl"
        $include "IBL.glsl"

        #define USE_MIPMAP_FILTERED_SAMPLING

        in vec2 v2f_texCoord;

        out vec4 o_fragColor : FRAG_COLOR;

        uniform samplerCube radianceCubeMap;
        uniform int radianceCubeMapSize;
        uniform int targetCubeMapSize;
        uniform int targetCubeMapFace;
        uniform float roughness;

        void main() {
            int targetFaceX = int(min(floor(v2f_texCoord.x * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));
            int targetFaceY = int(min(floor(v2f_texCoord.y * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));

            vec3 N = FaceToGLCubeMapCoords(targetCubeMapFace, targetFaceX, targetFaceY, targetCubeMapSize).xyz;

            float linearRoughness = roughness * roughness;

            vec3 integratedLD = IntegrateLD(N, linearRoughness, radianceCubeMap, float(radianceCubeMapSize));

            o_fragColor = vec4(integratedLD, 1.0);
        }
    }
}
