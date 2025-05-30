//! #version 460
// This is for the glsl addon the compiler adds the version

out vec3 FragColor;

in vec2 TexCoords;

// All we need to do is take the primitive id, lookup what color we should
// draw, and then draw it.


layout(std430, binding = 1) buffer LineColors {
    uint colors[];
};

void main() {
    // Just look up the color and draw it
    uint colorVal = colors[gl_PrimitiveID];
    FragColor = vec3(colorVal, colorVal, colorVal) / 255.0;
}