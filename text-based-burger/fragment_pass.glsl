in vec2 TexCoords;

out vec4 FragColor;

uniform sampler2D screenTexture;

uniform float aspectRatio;

void main() {
	// just pass through the texture
	float col = texture(screenTexture, (0.5 * TexCoords * vec2(aspectRatio, 1)) + vec2(0.5, 0.5), 0.0).r;
	FragColor = vec4(col, col, col, 1.0);
	//FragColor = vec4(TexCoords.x, TexCoords.y, 0.0, 1.0);
}