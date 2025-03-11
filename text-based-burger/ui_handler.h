#pragma once

#include "component.h"
#include "char_lut.h"
#include "ui_systems.h"

#include <vector>
#include <string>
#include <map>

// This is the ui handler class. It handles ui operations, look drawing, updates, input handling, etc.

class UIHandler {

public:
	// Constructors
	UIHandler();
	UIHandler(std::string filename, int scrn_w, int scrn_h);

	// Update the ui
	void update(update_data data);

	// Sometimes it may be useful to switch to a different screen
	// that is handled by a different ui handler, so this function
	// statically makes all components rerender without updating
	void rerender_all();

	// Toggle the error log
	void toggle_error_log();

	// Clear the screen
	void cls();

	std::vector<std::vector<uint32_t>> get_screen();

private:

	// Render the error log
	void render_error_log();

	// New screen to draw
	std::vector<std::vector<uint32_t>> screen;
	std::vector<std::vector<bool>> should_update;

	bool show_error_log = false;
	ErrorReporter error_log;
	
	// Root component
	UIComponent root;

	// Map of targetname to component pointer
	std::map<std::string, UIComponent*> component_registry;
};