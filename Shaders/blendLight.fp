$include "fragment_common.glsl"

in vec4 v2f_color;
in vec4 v2f_texCoord;

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D blendProjectionMap;

void main() {
	vec4 C = tex2Dproj(blendProjectionMap, v2f_texCoord);

	C *= min((v2f_texCoord.z < 0.0 ? 0.0 : 10.0) * v2f_texCoord.z / v2f_texCoord.w, 1.0);

	o_fragColor = v2f_color * C;
}
