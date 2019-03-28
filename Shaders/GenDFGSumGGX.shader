shader "GenDFGSumGGX" {
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
        $include "IBL.glsl"

        in vec2 v2f_texCoord;

        out vec4 o_fragColor : FRAG_COLOR;

        void main() {
            vec2 integratedDFG = IntegrateDFG(v2f_texCoord.x, v2f_texCoord.y);

            o_fragColor = vec4(integratedDFG, 0.0, 1.0);
        }
    }
}
