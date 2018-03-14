shader "queryHOM" {
	glsl_vp {
		in vec2 position : POSITION;
		in vec3 center : TEXCOORD0;
		in vec3 extents : TEXCOORD1;

		out float v2f_visible;

		uniform mat4 viewProjectionMatrix;
		uniform vec2 viewportSize;
		uniform int maxLevel;
		uniform sampler2D homap;

		void main() {
			vec4 clip_p[8];
			clip_p[0] = viewProjectionMatrix * vec4(center + vec3( extents.x,  extents.y,  extents.z), 1.0);
			clip_p[1] = viewProjectionMatrix * vec4(center + vec3(-extents.x,  extents.y,  extents.z), 1.0);
			clip_p[2] = viewProjectionMatrix * vec4(center + vec3( extents.x, -extents.y,  extents.z), 1.0);
			clip_p[3] = viewProjectionMatrix * vec4(center + vec3(-extents.x, -extents.y,  extents.z), 1.0);
			clip_p[4] = viewProjectionMatrix * vec4(center + vec3( extents.x,  extents.y, -extents.z), 1.0);
			clip_p[5] = viewProjectionMatrix * vec4(center + vec3(-extents.x,  extents.y, -extents.z), 1.0);
			clip_p[6] = viewProjectionMatrix * vec4(center + vec3( extents.x, -extents.y, -extents.z), 1.0);
			clip_p[7] = viewProjectionMatrix * vec4(center + vec3(-extents.x, -extents.y, -extents.z), 1.0);

			vec3 smin = vec3(1.0, 1.0, 1.0);
			vec3 smax = vec3(0.0, 0.0, 0.0);

			for (int i = 0; i < 8; i++) {
				vec3 ndc = clip_p[i].xyz / clip_p[i].w;
				ndc = (ndc + vec3(1.0)) * 0.5;
				smin = min(ndc, smin);
				smax = max(ndc, smax);
			}

			smin = clamp(smin, 0.0, 1.0);
			smax = clamp(smax, 0.0, 1.0);

			vec2 size = (smax.xy - smin.xy) * viewportSize.xy;
			float level = clamp(ceil(log2(max(size.x, size.y) / 2.0)), 0.0, float(maxLevel));

			vec4 rect = vec4(smin.xy, smax.xy);

			vec4 samples;
			samples.x = tex2Dlod(homap, vec4(rect.xy, 0.0, level)).x;
			samples.y = tex2Dlod(homap, vec4(rect.xw, 0.0, level)).x;
			samples.z = tex2Dlod(homap, vec4(rect.zy, 0.0, level)).x;
			samples.w = tex2Dlod(homap, vec4(rect.zw, 0.0, level)).x;
			float max_depth = max(max(samples.x, samples.y), max(samples.z, samples.w));
			v2f_visible = smin.z <= max_depth ? 1.0 : 0.0;

			gl_Position = vec4(position.xy, 0.0, 1.0);
		}
	}

	glsl_fp {
		in float v2f_visible;

		out vec4 o_fragColor : FRAG_COLOR;

		void main() {
			o_fragColor = vec4(v2f_visible);
		}
	}
}
