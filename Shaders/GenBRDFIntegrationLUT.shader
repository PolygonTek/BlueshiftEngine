shader "GenBRDFIntegrationLUT" {
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

        vec2 integrateBRDF(float NdotV, float roughness) {
            vec3 N = vec3(0.0, 0.0, 1.0);

            vec3 V;
            V.x = sqrt(1.0 - NdotV * NdotV);
            V.y = 0.0;
            V.z = NdotV;

            float A = 0.0;
            float B = 0.0;

            float numSamples = 0.0;

            float k = roughness * roughness * 0.5; // k for IBL

            for (float y = 0.0; y < 1.0; y += 0.01) {
                for (float x = 0.0; x < 1.0; x += 0.01) {
                    vec3 H = importanceSampleGGX(vec2(x, y), roughness, N);
                    //vec3 H = importanceSampleBeckmann(vec2(x, y), roughness, N);
                    vec3 L = normalize(2.0 * dot(V, H) * H - V);

                    float NdotL = max(L.z, 0.0);
                    float NdotH = max(H.z, 0.0);
                    float VdotH = max(dot(V, H), 0.0);

                    if (NdotL > 0.0) {
                        float G = G_SchlickGGX(NdotV, NdotL, k);
                        float G_Vis = G * NdotL * VdotH / NdotH;
                        float Fc = pow5(1.0 - VdotH);

                        A += (1.0 - Fc) * G_Vis;
                        B += Fc * G_Vis;
                    }

                    numSamples += 1.0;
                }
            }

            return vec2(A, B) / numSamples;
        }

        void main() {
            vec2 integratedBRDF = integrateBRDF(v2f_texCoord.x, v2f_texCoord.y);
            o_fragColor = vec4(integratedBRDF, 0.0, 1.0);
        }
    }
}
