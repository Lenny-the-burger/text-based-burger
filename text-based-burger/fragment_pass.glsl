in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D screenTexture;

uniform float aspectRatio;
uniform float aspectRatioSmall;

uniform float scale;
uniform float translationX;
uniform float translationY;

void main() {
	// just pass through the texture
	float col = texture(screenTexture, (1.0 / scale) * (0.5 * TexCoords * vec2(aspectRatio * aspectRatioSmall, 1)) + vec2(0.5, 0.5) + vec2(translationX, translationY), 0.0).r;
	FragColor = vec4(col, col, col, 1.0);
	//FragColor = vec4(TexCoords.x, TexCoords.y, 0.0, 1.0);
}