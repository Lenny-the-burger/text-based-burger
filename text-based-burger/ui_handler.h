#pragma once

#include "component.h"
#include "char_lut.h"
#include "ui_systems.h"
#include "math_utils.h"

#include <vector>
#include <string>
#include <map>

class SystemsController;

// This is the ui handler class. It handles ui operations, look drawing, updates, input handling, etc.

class UIHandler {

public:
	// Constructors
	UIHandler(std::string filename, int scrn_w, int scrn_h, SystemsController& controller);

	// Update the ui
	void update(UIUpdateData data);

	// Sometimes it may be useful to switch to a different screen
	// that is handled by a different ui handler, so this function
	// statically makes all components rerender without updating
	void rerender_all();

	// Clear the screen
	void cls();

	std::vector<std::vector<uint32_t>> get_screen();

	std::vector<vec2>& get_stencil_regions() {
		return stencil_regions;
	}

	int& get_stencil_state() {
		return stencil_state;
	}

	UIComponentIO* get_io() {
		return &component_io;
	}

private:

	// New screen to draw
	std::vector<std::vector<uint32_t>> screen;

	std::vector<vec2> stencil_regions;
	int stencil_state;

	UIComponentIO component_io;
	
	// Root component
	UIComponent root;
};