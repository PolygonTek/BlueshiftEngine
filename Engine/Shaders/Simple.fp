in vec2 v2f_texCoord;
in vec4 v2f_color;

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D mainMap;

void main() {
	o_fragColor = v2f_color * tex2D(mainMap, v2f_texCoord);
}
