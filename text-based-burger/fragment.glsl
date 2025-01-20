out float FragColor;

in vec2 TexCoords;

void main() {

    // color based on fragment position
    FragColor =  0.5 * (ceil((gl_FragCoord.x / 640.0) * 10.0) / 10.0);
    FragColor += 0.5 * (ceil((gl_FragCoord.y / 480.0) * 10.0) / 10.0);

    // If we are a border pixel, make it black
    if (TexCoords.x < -0.99 || TexCoords.x > 0.99 || TexCoords.y < -0.99 || TexCoords.y > 0.99) {
		FragColor = 0.0;
	}

}
