shader "GenDiffuseIrradianceCubeMap" {
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

        #define PI 3.1415926535897932384626433832795
        #define TWO_PI 6.283185307179586476925286766559
        #define HALF_PI 1.5707963267948966192313216916398

        in vec2 v2f_texCoord;

        out vec4 o_fragColor : FRAG_COLOR;

        uniform samplerCube radianceCubeMap;
        uniform int targetCubeMapSize;
        uniform int targetCubeMapFace;

        // Transform tangent space direction vector to local space direction vector
        vec3 rotateWithUpVector(vec3 tangentDir, vec3 tangentZ) {
            vec3 tangentY = abs(tangentZ.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
            vec3 tangentX = normalize(cross(tangentY, tangentZ));
            tangentY = cross(tangentZ, tangentX);
            return mat3(tangentX, tangentY, tangentZ) * tangentDir;
        }
        
        void main() {
            int targetFaceX = int(min(floor(v2f_texCoord.x * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));
            int targetFaceY = int(min(floor(v2f_texCoord.y * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));

            vec3 N = faceToGLCubeMapCoords(targetCubeMapFace, targetFaceX, targetFaceY, targetCubeMapSize).xyz;

            vec3 color = vec3(0.0);
#if 1
            float index = 0.0;

            for (float phi = 0.0; phi < TWO_PI; phi += 0.0125) {
                for (float theta = 0.0; theta < HALF_PI; theta += 0.05) {
                    float cosTheta = cos(theta);
                    float sinTheta = sin(theta);

                    vec3 L;
                    L.x = sinTheta * cos(phi);
                    L.y = sinTheta * sin(phi);
                    L.z = cosTheta;

                    color += texCUBE(radianceCubeMap, rotateWithUpVector(L, N)).rgb * cosTheta * sinTheta;

                    index += 1.0;
                }
            }

            o_fragColor = vec4(PI * color / index, 1.0);
#else
            for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                for (int y = 0; y < radianceCubeMapSize; y++) {
                    for (int x = 0; x < radianceCubeMapSize; x++) {
                        vec3 L = faceToGLCubeMapCoords(faceIndex, x, y, radianceCubeMapSize).xyz;
                        
                        float NdotL = max(dot(N, L), 0.0);

                        if (NdotL > 0.0) {
                            float dw = cubeMapTexelSolidAngle(x, y, radianceCubeMapSize);
                        
                            color += texCUBE(radianceCubeMap, L).rgb * pow(NdotL, specularPower) * dw;
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
