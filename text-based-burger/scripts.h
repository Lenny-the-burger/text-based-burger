#pragma once

#include "json.hpp"
using json = nlohmann::json;

#include <vector>

// "scripts" are more just general subroutines that can be called. Scripts can be called by anything,
// and can do anything, but do not exist as objects and do not have a state. A use for a script could 
// be interfacing between on screen ui, such as calling in an air strike, which would originate from 
// a ui component, but would need to interface with the game state, which are two completly seperate 
// systems. 
// Scripts let you do anything you want, this is to make it so you dont need to go through 200 layers
// of abstraction to do a very specific thing, but makes it very easy to explode everything if you are 
// not careful.
// Scripts can even interface with the gpu through glfw but probably dont do that. They are statically
// compiled with the rest of the program so should not be a security risk as no arbitrary code can be
// run.

// Get the function pointer for the given script name
void* get_script(std::string name);

// Various things that scripts might need
class UIComponent; // Forward declaration
class ComponentIO; // Forward declaration
class UIHandler; // Forward declaration


void test_label_script(json data);