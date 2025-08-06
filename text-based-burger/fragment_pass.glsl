//! #version 460

in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D screenTexture;

uniform float aspectRatio;
uniform float aspectRatioSmall;

void main() {
	// just pass through the texture
	float col = texture(screenTexture, (0.5 * TexCoords * vec2(aspectRatio * aspectRatioSmall, 1)) + vec2(0.5, 0.5)).r;
	FragColor = vec4(col, col, col, 1.0);
}