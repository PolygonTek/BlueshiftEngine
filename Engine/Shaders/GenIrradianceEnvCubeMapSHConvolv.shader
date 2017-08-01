shader "GenIrradianceEnvCubeMapSHConvolv" {
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

        uniform sampler2D incidentCoeffMap;
        uniform int targetCubeMapSize;
        uniform int targetCubeMapFace;
        uniform float lambertCoeff[4];

        void main() {
            vec3 incidentCoeff[16];

            float t = 0.5 / 4.0;
            for (int y = 0; y < 4; y++) {
                float s = 0.5 / 4.0;
                for (int x = 0; x < 4; x++) {
                    incidentCoeff[y * 4 + x] = tex2D(incidentCoeffMap, vec2(s, t)).xyz;
                    s += 1.0 / 4.0;
                }
                t += 1.0 / 4.0;
            }

            int x = int(min(floor(v2f_texCoord.x * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));
            int y = int(min(floor(v2f_texCoord.y * float(targetCubeMapSize)), float(targetCubeMapSize) - 1.0));

            // z-up world normal direction
            vec3 dir = faceToGLCubeMapCoords(targetCubeMapFace, x, y, targetCubeMapSize).zxy;
            vec3 dirSq = dir * dir;

            vec3 p1 = lambertCoeff[1] * SH_CONST2 * dir.yzx;
            vec3 p2 = lambertCoeff[2] * SH_CONST3 * dir.xyz * dir.yzx;

            vec3 color;
            color  = incidentCoeff[0] * lambertCoeff[0] * SH_CONST1;
            color -= incidentCoeff[1] * p1.x;
            color += incidentCoeff[2] * p1.y;
            color -= incidentCoeff[3] * p1.z;
            color += incidentCoeff[4] * p2.x;
            color -= incidentCoeff[5] * p2.y;
            color += incidentCoeff[6] * lambertCoeff[2] * SH_CONST4 * (3.0 * dirSq.z - 1.0);
            color -= incidentCoeff[7] * p2.z;
            color += incidentCoeff[8] * lambertCoeff[2] * SH_CONST5 * (dirSq.x - dirSq.y);

            o_fragColor = vec4(color, 1.0);
        }
    }
}
