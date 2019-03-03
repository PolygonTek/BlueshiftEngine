shader "GenIrradianceEnvCubeMap" {
    glsl_vp {
        $include "clipQuad.vp"
    }

    glsl_fp {
        $include "FragmentCommon.glsl"
        $include "IBL.glsl"

        in vec2 v2f_texCoord;

        out vec4 o_fragColor : FRAG_COLOR;

        uniform samplerCube radianceCubeMap;
        uniform int radianceCubeMapSize;
        uniform int targetCubeMapSize;
        uniform int targetCubeMapFace;
        
        void main() {
            int targetFaceX = int(min(floor(v2f_texCoord.x * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));
            int targetFaceY = int(min(floor(v2f_texCoord.y * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));

            vec3 N = FaceToGLCubeMapCoords(targetCubeMapFace, targetFaceX, targetFaceY, targetCubeMapSize).xyz;

            vec3 color = vec3(0.0);
#if 1 // Quasi Monte Carlo integration
            float numSamples = 0.0;

            const float inc = 1.0 / (log2(radianceCubeMapSize) * 5.0);

            mat3 tangentToWorld = GetLocalFrame(N);

            for (float y = 0.0; y < 1.0; y += inc) {
                for (float x = 0.0; x < 1.0; x += inc) {
                    vec3 L = tangentToWorld * ImportanceSampleLambert(vec2(x, y));
                    // BRDF = 1 / PI
                    // PDF = NdotL / PI
                    //
                    // Integrate { Li * BRDF * NdotL }
                    //
                    // F_N = 1/N * Sigma^N { Li * BRDF * NdotL / PDF }
                    //     = 1/N * Sigma^N { Li }
                    color += texCUBE(radianceCubeMap, L).rgb;

                    numSamples += 1.0;
                }
            }

            o_fragColor = vec4(color / numSamples, 1.0);
#else // Brute force way integration
            for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                for (int y = 0; y < radianceCubeMapSize; y++) {
                    for (int x = 0; x < radianceCubeMapSize; x++) {
                        vec3 L = FaceToGLCubeMapCoords(faceIndex, x, y, radianceCubeMapSize).xyz;
                        
                        float NdotL = max(dot(N, L), 0.0);

                        if (NdotL > 0.0) {
                            float dw = CubeMapTexelSolidAngle(x, y, radianceCubeMapSize);
                        
                            color += texCUBE(radianceCubeMap, L).rgb * NdotL * dw;
                        }
                    }
                }
            }

            float normFactor = 1.0 / PI;

            o_fragColor = vec4(normFactor * color, 1.0);
#endif
        }
    }
}
