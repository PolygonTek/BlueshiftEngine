shader "gammaCorrection" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D tex0;
		uniform float invGamma;

		float linearToSRGB(float cl) {
			float cs = 0.0;
			if (cl > 1.0) {
				cs = 1.0;
			} else if (cl < 0.0) {
				cs = 0.0;
			} else if (cl < 0.0031308) {
				cs = 12.92 * cl;
			} else {
				cs = 1.055 * pow(cl, 0.41666) - 0.055;
			}

			return cs;
		}

		void main() {
			vec4 color = tex2D(tex0, v2f_texCoord.st);

			//color.xyz = pow(color.xyz, vec3(invGamma, invGamma, invGamma));
			//color.x = linearToSRGB(color.x);
			//color.y = linearToSRGB(color.y);
			//color.z = linearToSRGB(color.z);

			o_fragColor = color;
		}
	}
}
