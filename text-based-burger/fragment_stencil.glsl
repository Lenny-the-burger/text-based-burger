//! #version 460
// This is for the glsl addon the compiler adds the version

out vec4 FragColor;

in vec2 TexCoords;

uniform float aspectRatio;
uniform float aspectRatioSmall;

uniform int zone_count;

layout(std430, binding = 2) buffer StencilZones {
    vec2 zones[];
};

void main() {
	vec2 native_coords = (0.5 * TexCoords * vec2(aspectRatio * aspectRatioSmall, 1)) + vec2(0.5, 0.5);
	native_coords = native_coords * vec2(960.0, 536.0);

	// discard outside of screen bounds
	for (int i = 0; i < zone_count; i++) {
		vec2 z_start = zones[i * 2];
		vec2 z_end   = zones[i * 2 + 1];
		if (native_coords.x > z_start.x && native_coords.x < z_end.x &&
			native_coords.y > z_start.y && native_coords.y < z_end.y) {
			// If we are inside a zone, discard
			discard;
		}
	}
//	if (native_coords.x > 0.0 && native_coords.x < 960.0 && native_coords.y > 80.0 && native_coords.y < 536.0) {
//		discard;
//	}
	return;
}