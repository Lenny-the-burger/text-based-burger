//! #version 460

out float FragColor;

in vec2 TexCoords;

uniform uvec4 glyphs[256];

void main() {

    // color based on fragment position
    //FragColor =  0.5 * ( mod(gl_FragCoord.x + 4.0, 8.0 ) / 8.0 );
    //FragColor += 0.5 * ( mod(gl_FragCoord.y, 16.0) / 16.0);

    vec2 frag_coord = vec2(gl_FragCoord.x, 480.0 - gl_FragCoord.y);

    // Where in the character grid are we?
    uvec2 character_position = uvec2( 
                    floor(frag_coord.x * (1.0 / 8.0 )), 
                    floor(frag_coord.y * (1.0 / 16.0)) 
    );

    // Where in the texture are we?
    uvec2 texel_position = uvec2(
                    floor(mod(frag_coord.x, 8.0 )),
                    floor(mod(frag_coord.y, 16.0))
    );

    // Which of the 4 32 bit sectors are we in
    uint sector = uint(floor(texel_position.y / 4.0));

    // Where are we inside of the 32 bit block?
    uint block_position = (texel_position.y % 4) * 8 + texel_position.x;

    uint charnum = (character_position.y * 80 + character_position.x) % 256;

    // Get the glyph
    uvec4 character = glyphs[charnum];

    uint block = character[sector];

    // Get the bit
    uint bit = (block >> block_position) & 1u;

    FragColor = float(bit);

    // if we are a border pixel, make it 0
    if (gl_FragCoord.x < 1.0 || gl_FragCoord.x > 639.0 || gl_FragCoord.y < 1.0 || gl_FragCoord.y > 479.0) {
		FragColor = 0.0;
	}

}
