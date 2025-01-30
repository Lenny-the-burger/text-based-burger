#pragma once

#include <vector>

// This is the ui handler class. It handles ui operations, look drawing, updates, input handling, etc.

class UIHandler {

public:
	// Constructor
	UIHandler();
	// Destructor
	~UIHandler();
	// Initialize the ui handler
	void init();
	// Update the ui
	void update();

	// New screen to draw
	std::vector<uint32_t> screen;
	std::vector<bool> should_update;

private:


};