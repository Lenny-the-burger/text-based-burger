#pragma once

#include "component.h"

#include <vector>
#include <string>

// This is the ui handler class. It handles ui operations, look drawing, updates, input handling, etc.

class UIHandler {

public:
	// Constructors
	UIHandler(std::string filename, int scrn_w, int scrn_h);

	// Update the ui
	void update(update_data data);

	// Sometimes it may be useful to switch to a different screen
	// that is handled by a different ui handler, so this function
	// statically makes all components rerender without updating
	void rerender_all();

	std::vector<std::vector<uint32_t>> get_screen();

private:

	// New screen to draw
	std::vector<std::vector<uint32_t>> screen;
	std::vector<std::vector<bool>> should_update;

	ErrorReporter error_log;
	
	// Root component
	UIComponent root;
};