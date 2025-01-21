out float FragColor;

in vec2 TexCoords;

void main() {

    // color based on fragment position
    FragColor =  0.5 * ( mod(gl_FragCoord.x + 4.0, 8.0 ) / 8.0 );
    FragColor += 0.5 * ( mod(gl_FragCoord.y, 16.0) / 16.0);

    // If we are a border pixel, make it black
    if (TexCoords.x < -0.99 || TexCoords.x > 0.99 || TexCoords.y < -0.99 || TexCoords.y > 0.99) {
		FragColor = 0.0;
	}

}
