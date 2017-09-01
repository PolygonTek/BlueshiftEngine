shader "GenPhongSpecularPrefilteredEnvCubeMap" {
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
        uniform float specularPower;
        
        void main() {
            int targetFaceX = int(min(floor(v2f_texCoord.x * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));
            int targetFaceY = int(min(floor(v2f_texCoord.y * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));

            vec3 S = faceToGLCubeMapCoords(targetCubeMapFace, targetFaceX, targetFaceY, targetCubeMapSize).xyz;

            vec3 color = vec3(0.0);
#if 1 // Quasi Monte Carlo integration
            float numSamples = 0.0;

            for (float y = 0.0; y < 1.0; y += 0.01) {
                for (float x = 0.0; x < 1.0; x += 0.01) {
                    vec3 sampleDir = importanceSamplePhongSpecular(vec2(x, y), specularPower, S);

                    // BRDF = (power + 2) * pow(LdotS, power) / (NdotL * TWO_PI)
                    // PDF = (power + 1) * pow(LdotS, power) / TWO_PI
                    //
                    // Integrate { Li * BRDF * NdotL }
                    // F_N = 1/N * Sigma^N { Li * BRDF * NdotL / PDF }
                    // = 1/N * Sigma^N { Li * (power + 2) / (power + 1) }
                    color += texCUBE(radianceCubeMap, sampleDir).rgb;

                    numSamples += 1.0;
                }
            }

            o_fragColor = vec4((specularPower + 2.0) / (specularPower + 1.0) * color / numSamples, 1.0);
#else
            for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                for (int y = 0; y < radianceCubeMapSize; y++) {
                    for (int x = 0; x < radianceCubeMapSize; x++) {
                        vec3 L = faceToGLCubeMapCoords(faceIndex, x, y, radianceCubeMapSize).xyz;
                        
                        float SdotL = max(dot(S, L), 0.0);

                        if (SdotL > 0.0) {
                            float dw = cubeMapTexelSolidAngle(x, y, radianceCubeMapSize);
                        
                            color += texCUBE(radianceCubeMap, L).rgb * pow(SdotL, specularPower) * dw;
                        }
                    }
                }
            }

            float normFactor = (specularPower + 2.0) / TWO_PI;

            o_fragColor = vec4(normFactor * color, 1.0);
#endif
        }
    }
}
