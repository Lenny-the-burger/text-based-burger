#pragma once

#include "component.h"

#include <vector>
#include <string>

// This is the ui handler class. It handles ui operations, look drawing, updates, input handling, etc.

class UIHandler {

public:
	// Constructors
	UIHandler(std::string filename);

	// Update the ui
	void update(update_data data);

	// Sometimes it may be useful to switch to a different screen
	// that is handled by a different ui handler, so this function
	// statically makes all components rerender without updating
	void rerender_all();

private:

	// New screen to draw
	std::vector<uint32_t> screen;
	std::vector<bool> should_update;
	
	// Root component
	UIComponent root;
	
};