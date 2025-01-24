#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <glm/glm.hpp>


// Loads the font file and returns a list of vec2s containing the upper and lower halves of the characters as a 
// 64 bit number representation. If your dont isnt 8*16 black and white you will need to modify this function.

std::vector<uint32_t> load_font(const std::string& font_path);