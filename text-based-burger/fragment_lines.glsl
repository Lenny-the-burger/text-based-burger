//! #version 460
// This is for the glsl addon the compiler adds the version

out vec4 FragColor;

in vec2 TexCoords;

// All we need to do is take the primitive id, lookup what color we should
// draw, and then draw it.


layout(std430, binding = 1) buffer LineColors {
    uint colors[];
};

void main() {
    // Just look up the color and draw it
    float colorVal = colors[gl_PrimitiveID] / 255.0;
    FragColor = vec4(colorVal, colorVal, colorVal, 0.7);
}