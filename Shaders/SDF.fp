in LOWP vec4 v2f_color;

in MEDIUMP vec2 v2f_texCoord;

out vec4 o_fragColor : FRAG_COLOR;

uniform sampler2D sdfMap;

void main() {
    float dist = tex2D(sdfMap, v2f_texCoord).a;
    float alpha = smoothstep(u_buffer - u_gamma, u_buffer + u_gamma, dist);

    o_FragColor = vec4(v2f_color.rgb, alpha * v2f_color.a);

    o_fragColor = v2f_color * albedo * vec4(vec3(intensity), 1.0);
}
