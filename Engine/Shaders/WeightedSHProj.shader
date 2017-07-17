shader "WeightedSHProj" {
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
        $include "sh_common.glsl"
        $include "fragment_common.glsl"

        in vec2 v2f_texCoord;

        out vec4 o_fragColor : FRAG_COLOR;

        uniform samplerCube radianceCubeMap;
        uniform sampler2D weightMap[6];
        uniform int radianceCubeMapSize;
        uniform float radianceScale;

        void main() {
            vec3 shCoeff = vec3(0.0, 0.0, 0.0);

            float weightMapInvSize = 1.0 / float(radianceCubeMapSize * 4);

            for (int faceIndex = 0; faceIndex < 6; faceIndex++) {
                for (int y = 0; y < radianceCubeMapSize; y++) {
                    for (int x = 0; x < radianceCubeMapSize; x++) {
                        ivec2 blockCoords = ivec2(min(floor(v2f_texCoord * 4.0), vec2(3.0)));

                        vec2 weightCoords = vec2(float(blockCoords.s * radianceCubeMapSize + x), float(blockCoords.t * radianceCubeMapSize + y)) * weightMapInvSize;
                        float weight = tex2D(weightMap[faceIndex], weightCoords).x;

                        vec3 radianceDir = faceToGLCubeMapCoords(faceIndex, x, y, radianceCubeMapSize).xyz;
                        vec3 radianceColor = texCUBE(radianceCubeMap, radianceDir).xyz * radianceScale;

                        shCoeff += radianceColor * weight;
                    }
                }
            }            

            o_fragColor = vec4(shCoeff, 1.0);
        }
    }
}
