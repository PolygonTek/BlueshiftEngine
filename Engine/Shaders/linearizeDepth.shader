shader "linearizeDepth" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		uniform sampler2D depthMap;
		uniform vec2 depthRange;	// zNear, zFar

		void main() {
			float depth = tex2D(depthMap, v2f_texCoord.st).r;
			o_fragColor = vec4(depthRange.x / (depthRange.y - depth * (depthRange.y - depthRange.x)));
		}
	}
}
