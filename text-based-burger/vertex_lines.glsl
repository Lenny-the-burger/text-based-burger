#version 460 core
layout (location = 0) in vec2 aPos; // Position in NDC

out vec2 TexCoords;
uniform float aspectRatio;
uniform float aspectRatioSmall;

void main() {

    // Need to scale lines cords the same way we scale the small framebuffer, double work
    // but whatever. Other option is second intermediary buffer because lines need to be drawn
    // at a different resolution and i dont want to do that.

    vec2 scaledpos = aPos * vec2(1.0 / (aspectRatioSmall * aspectRatio), 1);
    gl_Position = vec4(scaledpos, 0.0, 1.0); // Pass position to clip space
    TexCoords = scaledpos; // Pass texture coordinates to fragment shader
}