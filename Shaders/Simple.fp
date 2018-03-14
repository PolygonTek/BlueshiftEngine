in MEDIUMP vec2 v2f_texCoord;
in LOWP vec4 v2f_color;

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D albedoMap;

void main() {
	o_fragColor = v2f_color * tex2D(albedoMap, v2f_texCoord);
}
