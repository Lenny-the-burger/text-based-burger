//! #version 460
// This is for the glsl addon the compiler adds the version

out float FragColor;

in vec2 TexCoords;

uniform uvec4 glyphs[256];
uniform int mouse_char_x;
uniform int mouse_char_y;



// Character grid ssbo
layout(std430, binding = 0) buffer CharacterGrid {
	uint character_grid[];
};

void main() {

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

    // Get the character data 32 bit uint word from the character grid
    uint char_word = character_grid[character_position.y * 80 + character_position.x];

    // Unpack
    uint charnum = (char_word & 0xFFu);
    uint fg = (char_word >> 8u ) & 0xFFu;
    uint bg = (char_word >> 16u) & 0xFFu;

    float fg_float = float(fg) / 255.0;
    float bg_float = float(bg) / 255.0;

    // Get the glyph
    uvec4 character = glyphs[charnum];

    uint block = character[sector];

    // Get the bit
    uint bit = (block >> block_position) & 1u;

    // Either we draw the foreground or background color
    FragColor = (float(bit)  * fg_float) + ((1.0 - float(bit)) * bg_float);

    // If we are the mouse cursor, draw glyph 165, white on black
    if (character_position.x == mouse_char_x && character_position.y == mouse_char_y) {
        uvec4 character = glyphs[165];
        uint block = character[sector];
        uint bit = (block >> block_position) & 1u;
        FragColor = (float(bit)  * 1.0) + ((1.0 - float(bit)) * 0.0);
    }
}
