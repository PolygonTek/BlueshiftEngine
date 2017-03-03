in vec2 v2f_texCoord;

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D diffuseMap;
uniform vec4 constantColor;
uniform vec3 id;

void main() {
#ifdef PERFORATED
	float alpha = tex2D(diffuseMap, v2f_texCoord).a;
	if (alpha < 0.5) {
		discard;
	}
#endif
	o_fragColor = vec4(id, 1.0f);
}
