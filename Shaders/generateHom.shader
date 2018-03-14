shader "generateHOM" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out float o_fragDepth : FRAG_DEPTH;

		uniform sampler2D lastMip;
		uniform vec2 texelSize;

		void main() {
			vec4 texels;
			texels.x = tex2D(lastMip, v2f_texCoord.st).x;
			texels.y = tex2D(lastMip, v2f_texCoord.st + vec2(-texelSize.x,  0)).x;
			texels.z = tex2D(lastMip, v2f_texCoord.st + vec2(-texelSize.x, -texelSize.y)).x;
			texels.w = tex2D(lastMip, v2f_texCoord.st + vec2( 0, -texelSize.y)).x;
			float max_z = max(max(texels.x, texels.y), max(texels.z, texels.w));
			o_fragDepth = max_z;
		}
	}
}
