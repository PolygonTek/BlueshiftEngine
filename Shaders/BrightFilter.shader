shader "BrightFilter" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
        $include "Colors.glsl"

		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D luminanceMap;
		uniform sampler2D tex0;

		uniform float brightLevel;
		uniform float brightThreshold;
		uniform float brightOffset;
		uniform float brightMax;

		void main() {
            float avgLum = tex2D(luminanceMap, vec2(0.0, 0.0)).x;

            vec3 color = tex2D(tex0, v2f_texCoord.st).rgb;

            //float magnitude = length(color);
            //color = min(magnitude, brightMax) * normalize(color);
            color *= (1.0 + (color / (brightMax * brightMax)));
            color *= brightLevel / (avgLum + 0.000001);
            color = max(color - vec3(brightThreshold), vec3(0.0));
            color /= (color + vec3(brightOffset));

            //color = max(color - vec3(brightThreshold), 0.0);
            //color /= color + brightOffset;

			o_fragColor = vec4(color, 1.0);
		}
	}
}
