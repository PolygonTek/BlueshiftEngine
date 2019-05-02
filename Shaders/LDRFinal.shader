shader "LDRFinal" {
    glsl_vp {
        $include "clipQuad.vp"
    }

    glsl_fp {
        $include "Colors.glsl"

        in vec2 v2f_texCoord;

        out vec4 o_fragColor : FRAG_COLOR;

        uniform sampler2D colorSampler;

        void main() {
            o_fragColor = tex2D(colorSampler, v2f_texCoord.st);
        }
    }
}
