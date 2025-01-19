out float FragColor;

in vec2 TexCoords;

void main() {

    // color based on fragment position
    FragColor =  0.5 * (ceil((gl_FragCoord.x / 480.0) * 10.0) / 10.0);
    FragColor += 0.5 * (ceil((gl_FragCoord.y / 320.0) * 10.0) / 10.0);

}
