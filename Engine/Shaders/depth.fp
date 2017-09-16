in MEDIUMP vec2 v2f_texCoord;

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D albedoMap;
uniform LOWP float perforatedAlpha;
uniform LOWP vec4 constantColor;

void main() {
#ifdef PERFORATED
	vec4 diffuse = tex2D(albedoMap, v2f_texCoord);
	if (diffuse.w < perforatedAlpha) {
		discard;
	}
#endif
	o_fragColor = constantColor;
}
