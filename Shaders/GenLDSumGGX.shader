shader "GenLDSumGGX" {
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
        uniform float roughness;
        
        // Compute LD term (first sum on Real Shading in Unreal Engine 4 p6)
        void main() {
            int targetFaceX = int(min(floor(v2f_texCoord.x * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));
            int targetFaceY = int(min(floor(v2f_texCoord.y * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));

            vec3 N = FaceToGLCubeMapCoords(targetCubeMapFace, targetFaceX, targetFaceY, targetCubeMapSize).xyz;

            mat3 tangentToWorld = GetLocalFrame(N);

            // As we don't know beforehand the view direction when convoluting the environment map,
            // We make a further approximation by assuming the view direction is always equal to the surface normal.
            // So we don't get lengthy reflections at grazing angles.
            vec3 V = N;

            vec3 color = vec3(0.0);

            float linearRoughness = roughness * roughness;

            float totalWeights = 0.0;

            for (float y = 0.0; y < 1.0; y += 0.01) {
                for (float x = 0.0; x < 1.0; x += 0.01) {
                    vec3 H = tangentToWorld * ImportanceSampleGGX(vec2(x, y), linearRoughness);
                    vec3 L = 2.0 * dot(V, H) * H - V;

                    // Integrate { Li * NdotL }
                    float NdotL = max(dot(N, L), 0.0);

                    if (NdotL > 0.0) {
                        color += texCUBE(radianceCubeMap, L).rgb * NdotL;

                        // We have found weighting by cos(theta) achieves better results.
                        totalWeights += NdotL;
                    }
                }
            }

            o_fragColor = vec4(color / totalWeights, 1.0);
        }
    }
}
