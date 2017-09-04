in LOWP vec4 v2f_color;
in MEDIUMP vec3 v2f_texCoord;

out vec4 o_fragColor : FRAG_COLOR;
out float o_fragDepth : FRAG_DEPTH;

uniform LOWP vec4 tint;
uniform MEDIUMP float exposure;

uniform HIGHP sampler2D skyBackMap;
uniform HIGHP sampler2D skyFrontMap;
uniform HIGHP sampler2D skyRightMap;
uniform HIGHP sampler2D skyLeftMap;
uniform HIGHP sampler2D skyTopMap;
uniform HIGHP sampler2D skyDownMap;

void main() {
    vec3 dir = normalize(v2f_texCoord);
    vec3 absDir = abs(dir);

    vec3 color = vec3(1.0, 1.0, 1.0);
    vec2 st;

    if (absDir.x > absDir.y && absDir.x > absDir.z) {
        if (dir.x > 0.0) {
            st = vec2(-dir.z, -dir.y) / absDir.x * vec2(0.5) + vec2(0.5);
            color = tex2D(skyRightMap, st).rgb;
        } else {
            st = vec2(dir.z, -dir.y) / absDir.x * vec2(0.5) + vec2(0.5);
            color = tex2D(skyLeftMap, st).rgb;
        }
    } else {
        if (absDir.y > absDir.z) {
            if (dir.y > 0.0) {
                st = vec2(dir.x, dir.z) / absDir.y * vec2(0.5) + vec2(0.5);
                color = tex2D(skyTopMap, st).rgb;
            } else {
                st = vec2(dir.x, -dir.z) / absDir.y * vec2(0.5) + vec2(0.5);
                color = tex2D(skyDownMap, st).rgb;
            }
        } else {
            if (dir.z > 0.0) {
                st = vec2(dir.x, -dir.y) / absDir.z * vec2(0.5) + vec2(0.5);
                color = tex2D(skyBackMap, st).rgb;
            } else {
                st = vec2(-dir.x, -dir.y) / absDir.z * vec2(0.5) + vec2(0.5);
                color = tex2D(skyFrontMap, st).rgb;
            }
        }
    }

    color *= tint.rgb * exposure;
    o_fragColor = v2f_color * vec4(color, 1.0);
    o_fragDepth = 1.0;
}
