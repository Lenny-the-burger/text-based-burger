#version 460 core

layout (location = 0) in vec2 aPos; // Position in NDC

out vec2 TexCoords;

void main() {

    gl_Position = vec4(aPos, 0.0, 1.0); // Pass position to clip space

	// Pass texture coordinates to fragment shader
	TexCoords = aPos;
}
