shader "brightFilter" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D luminanceMap;
		uniform sampler2D tex0;
		//uniform sampler2D tex1;
		//uniform vec2 texelOffset;
		uniform float brightLevel;
		uniform float brightThrehold;
		uniform float brightOffset;
		uniform float brightMax;

		const vec3 lumVector = vec3(0.2125, 0.7154, 0.0721);

		void main() {
			float adaptedLum = tex2D(luminanceMap, vec2(0.0, 0.0)).x;
			vec3 c = tex2D(tex0, v2f_texCoord.st).rgb;
			//c += tex2D(tex0, v2f_texCoord.st + vec2(texelOffset.x, 0.0));
			//c += tex2D(tex0, v2f_texCoord.st + vec2(texelOffset.x, texelOffset.y));
			//c += tex2D(tex0, v2f_texCoord.st + vec2(0.0, texelOffset.y));
			//c *= 0.25;

			float magnitude = length(c);
			c = min(magnitude, brightMax) * normalize(c);

			c *= brightLevel / (adaptedLum + 0.001);

			//float Lw = dot(c, lumVector);
			//float Ls = Lw * brightLevel / (adaptedLum + 0.001);
			//c = c * Ls / (Lw + 0.001);

			c = max(c - brightThrehold, 0.0);
			c /= (c + brightOffset);

			o_fragColor = vec4(c, 1.0);
		}
	}
}
