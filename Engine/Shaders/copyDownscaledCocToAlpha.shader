shader "copyDownscaledCocToAlpha" {
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
		uniform sampler2D depthMap;
		uniform vec2 sampleOffsets[4];

		float ComputeCoc(float viewDist) {
			float delta = viewDist - focalDistance;
			float deltaNear = maxBlurDistance.x - focalDistance;
			float deltaFar = maxBlurDistance.y - focalDistance;
			float coc =
				pow(clamp(delta / deltaFar - noBlurFraction, 0.0, 1.0), 0.8) +
				pow(clamp(delta / deltaNear - noBlurFraction, 0.0, 1.0), 0.8);

			return clamp(coc * cocScale, 0.0, 1.0);
		}

		void main() {
			float sumCoc = 0.0;
			float viewDist;
	/*
			for (int i = 0; i < 4; i++) {
				viewDist = tex2D(depthMap, v2f_texCoord.st + sampleOffsets[i]).x;
				sumCoc += ComputeCoc(viewDist);
			}*/

			viewDist = tex2D(depthMap, v2f_texCoord.st).x * zFar;
			sumCoc = ComputeCoc(viewDist);

			o_fragColor.a = sumCoc;
		}
	}
}
