out vec4 o_fragColor : FRAG_COLOR;

#ifdef PERFORATED
    in MEDIUMP vec2 v2f_texCoord;
    in LOWP float v2f_alpha;
#endif

#ifdef PERFORATED
    uniform sampler2D albedoMap;
    uniform LOWP float perforatedAlpha;
    uniform LOWP vec4 constantColor;
#endif

void main() {
#ifdef PERFORATED
    LOWP float alpha = tex2D(albedoMap, v2f_texCoord).a * v2f_alpha;
    if (alpha < perforatedAlpha) {
        discard;
    }
#endif

    o_fragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
