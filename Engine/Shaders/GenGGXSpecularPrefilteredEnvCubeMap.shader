shader "GenGGXSpecularPrefilteredEnvCubeMap" {
    glsl_vp {
        in vec4 in_position : POSITION;
        in vec2 in_st : TEXCOORD0;

        out vec2 v2f_texCoord;

        void main() {
            gl_Position = in_position;
            v2f_texCoord = in_st;
        }
    }

    glsl_fp {
        $include "fragment_common.glsl"
        $include "IBL.glsl"

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

            vec3 N = faceToGLCubeMapCoords(targetCubeMapFace, targetFaceX, targetFaceY, targetCubeMapSize).xyz;
            vec3 V = N;

            vec3 color = vec3(0.0);

            float totalWeights = 0.0;

            for (float y = 0.0; y < 1.0; y += 0.01) {
                for (float x = 0.0; x < 1.0; x += 0.01) {
                    vec3 H = importanceSampleGGX(vec2(x, y), roughness, N);
                    vec3 L = 2.0 * dot(V, H) * H - V;

                    float NdotL = max(dot(N, L), 0.0);
                    float NdotH = max(dot(N, H), 0.0);

                    if (NdotL > 0.0) {
                        color += texCUBE(radianceCubeMap, L).rgb * NdotL;
                        totalWeights += NdotL;
                    }
                }
            }

            o_fragColor = vec4(color / totalWeights, 1.0);
        }
    }
}
