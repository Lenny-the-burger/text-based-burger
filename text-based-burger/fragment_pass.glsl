in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D screenTexture;

uniform float aspectRatio;
uniform float aspectRatioSmall;

void main() {
	// just pass through the texture
	float scale = 1.0 / 3.0;
	float col = texture(screenTexture, (scale * TexCoords * vec2(aspectRatio, aspectRatioSmall)) + vec2(0.5, 0.5), 0.0).r;
	FragColor = vec4(col, col, col, 1.0);
	//FragColor = vec4(TexCoords.x, TexCoords.y, 0.0, 1.0);
}