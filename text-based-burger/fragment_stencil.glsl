//! #version 460
// This is for the glsl addon the compiler adds the version

out vec4 FragColor;

in vec2 TexCoords;

uniform float aspectRatio;
uniform float aspectRatioSmall;

void main() {
	vec2 native_coords = (0.5 * TexCoords * vec2(aspectRatio * aspectRatioSmall, 1)) + vec2(0.5, 0.5);
	native_coords = native_coords * vec2(960.0, 536.0);

	// discard outside of screen bounds
	// Cut off the bottom 9 lines for ui (9*16 = 144)
	if (native_coords.x > 0.0 && native_coords.x < 960.0 && native_coords.y > 80.0 && native_coords.y < 536.0) {
		discard;
	}
}