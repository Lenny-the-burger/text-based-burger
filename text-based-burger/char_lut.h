#pragma once

#include <vector>
#include <string>

// Character lookup, converts characters to numbers or numbers to characters
// Due to encoding, this only converts a subset of characters, but this is 
// mostly only for easier human interaction.

std::string int2char(int i);

int char2int(char c);