#ifdef LOGLUV_HDR
$include "logluv.glsl"
#endif

vec4 tex2D_bilinear(sampler2D tex, in vec2 uv, in float textureSize) {
	float texelSize = 1.0 / textureSize;
	vec2 f = fract(uv.xy * textureSize);
	vec4 t00 = tex2D(tex, uv);
	vec4 t10 = tex2D(tex, uv + vec2(texelSize, 0.0));
	vec4 tA = mix(t00, t10, f.x);
	vec4 t01 = tex2D(tex, uv + vec2(0.0, texelSize));
	vec4 t11 = tex2D(tex, uv + vec2(texelSize, texelSize));
	vec4 tB = mix(t01, t11, f.x);
	return mix(tA, tB, f.y);
}

float getMipmapLevel(vec2 uv, vec2 textureSize) {
  vec2 dx = dFdx(uv * textureSize.x);
  vec2 dy = dFdy(uv * textureSize.y);
  float d = max(dot(dx, dx), dot(dy, dy));
  return 0.5 * log2(d);
}

float fresnel(vec3 eyeVec, vec3 normal, float R0) {
	float kOneMinusEdotN = 1.0 - abs(dot(eyeVec, normal));    // note: abs() makes 2-sided materials work

	// raise to 5th power
	float result = kOneMinusEdotN * kOneMinusEdotN;
	result = result * result;
	result = result * kOneMinusEdotN;
	result = R0 + (1.0 - R0) * result;

	return max(result, 0.0);
}

vec3 getNormal(sampler2D normalMap, in vec2 tc) {
#if defined(LATC_NORMAL)
	vec3 n = tex2D(normalMap, tc).xyy * 2.0 - 1.0;
	n.z = 0.0;
	n.z = sqrt(abs(1.0 - dot(n, n)));
#elif defined(ETC2_NORMAL)
    vec3 n = tex2D(normalMap, tc).xyy;
    n.z = 0.0;
    n.z = sqrt(abs(1.0 - dot(n, n)));
#elif defined(DXT5_XGBR_NORMAL)
	vec3 n = tex2D(normalMap, tc).wyz * 2.0 - 1.0;
#else
	vec3 n = tex2D(normalMap, tc).xyz * 2.0 - 1.0;
#endif
	return n;
}

//#ifdef _PARALLAX
vec2 offsetTexcoord(sampler2D heightMap, in vec2 st, in vec2 viewDir, in float parallaxOffset) {
	vec2 ost = st;
	float z = tex2D(heightMap, ost).x * 2.0 - 1.0;
	ost = st + viewDir * z * parallaxOffset;

	z += tex2D(heightMap, ost).x * 2.0 - 1.0;
	ost = st + viewDir * z * parallaxOffset;

	return ost;
}
//#endif

vec4 encodeFloatRGBA(float v) {
	vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;
	enc = fract(enc);
	enc -= enc.yzww * vec4(1.0/255.0, 1.0/255.0, 1.0/255.0, 0.0);
	return enc;
}

float decodeFloatRGBA(vec4 rgba) {
	return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/160581375.0));
}

vec2 encodeViewNormal(vec3 n) {
	vec2 enc = n.xy / (n.z + 1.0);
	enc /= 1.7777;;
	return enc * 0.5 + 0.5;
}

// enc4 is vec4, with .rg containing encoded normal
vec3 decodeViewNormal(vec4 enc4) {
	vec3 n = 1.7777 * (enc4.xyz * vec3(2.0, 2.0, 0.0) - vec3(1.0, 1.0, -1.0));
	float g = 2.0 / dot(n, n);
	n.xy *= g;
	n.z = g - 1.0;
	return n;
}
/*
void RGB2YCoCg(vec4 color, vec3 ycocg) {
	ycocg.x = dot(color, vec4(1.0, -1.0, 0.0, 1.0));
	ycocg.y = dot(color, vec4(0.0, 1.0, -0.5 * 256.0 / 255.0, 1.0));
	ycocg.z = dot(color, vec4(-1.0, -1.0, 1.0 * 256.0 / 255.0, 1.0));
}

void YCoCg2RGB(vec3 ycocg) {
	ycocg.z * (255.0 / 8.0) + 1.0;
}
*/
