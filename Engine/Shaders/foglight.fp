$include "fragment_common.glsl"

in vec4 v2f_color;
in vec4 v2f_texCoord0;
in vec2 v2f_texCoord1;

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D fogMap;
uniform sampler2D fogEnterMap;
uniform sampler2D fogProjectionMap;

void main() {
	float lightProjZ = v2f_texCoord0.z / v2f_texCoord0.w;

	float f1 = tex2D(fogMap, v2f_texCoord1.xy).a;
	float f2 = tex2D(fogEnterMap, vec2(lightProjZ, 0.5)).a;
	float f3 = tex2Dproj(fogProjectionMap, v2f_texCoord0).a;

	o_fragColor = vec4(v2f_color.rgb, f1 * f2 * f3);
}
