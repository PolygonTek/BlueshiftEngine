shader "copyColorAndCoc" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform vec2 maxBlurDistance;
		uniform float focalDistance;
		uniform float noBlurFraction;
		uniform float cocScale;
		uniform float zFar;
		uniform sampler2D colorMap;
		uniform sampler2D depthMap;

		float ComputeCoc(float viewDist) {
			float delta = viewDist - focalDistance;
			float deltaNear = maxBlurDistance.x - focalDistance;
			float deltaFar = maxBlurDistance.y - focalDistance;
			float coc = pow(clamp(delta / deltaFar - noBlurFraction, 0.0, 1.0), 0.8) +
				pow(clamp(delta / deltaNear - noBlurFraction, 0.0, 1.0), 0.8);

			return clamp(coc * cocScale, 0.0, 1.0);
		}

		void main() {
			vec3 color = tex2D(colorMap, v2f_texCoord.st).xyz;
			float viewDist = tex2D(depthMap, v2f_texCoord.st).x * zFar;
			float coc = ComputeCoc(viewDist);

			o_fragColor = vec4(color, coc);
		}
	}
}
