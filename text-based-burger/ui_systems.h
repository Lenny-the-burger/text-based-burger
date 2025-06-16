#pragma once

#include "json.hpp"
using json = nlohmann::json;

#include <vector>
#include <string>
#include <map>

// This file holds all the random ui things that are needed for the ui system to work
// but arent components themselves or the actual ui handler

struct UIUpdateData {
	int mouse_char_x;
	int mouse_char_y;
	int time;
	bool is_clicking;
};

// Forward declaration
class UIComponent;
class SystemsController;

// Error reporter class
class UIComponentIO {
public:
	// Constructor
	UIComponentIO(SystemsController& controller);
	// Destructor
	~UIComponentIO();

	// Report an error
	void report_error(std::string error);
	// Get the error log
	std::vector<std::string> get_log();
	std::vector<int> get_repeats();

	void register_component(std::string name, UIComponent* component);

	// This can only be used by the script system
	UIComponent* get_component(std::string name);
private:
	std::vector<std::string> error_log;
	std::vector<int> repeats;

	std::map<std::string, UIComponent*> component_registry;

	SystemsController& controller;
};

// Generates a grid fragment from character, bg and fg color
uint32_t gen_frag(int character, int bg, int fg);