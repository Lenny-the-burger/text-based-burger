#pragma once

#include "json.hpp"
using json = nlohmann::json;

#include <vector>
#include <string>

// This file holds all the random ui things that are needed for the ui system to work
// but arent components themselves or the actual ui handler

struct UpdateData {
	int mouse_char_x;
	int mouse_char_y;
	int time;
	bool is_clicking;
};

// Error reporter class
class ErrorReporter {
public:
	// Constructor
	ErrorReporter();
	// Destructor
	~ErrorReporter();
	// Report an error
	void report_error(std::string error);
	// Get the error log
	std::vector<std::string> get_log();
	std::vector<int> get_repeats();
private:
	std::vector<std::string> error_log;
	std::vector<int> repeats;
};

// Generates a grid fragment from character, bg and fg color
uint32_t gen_frag(int character, int bg, int fg);

// All the various component events we can send to them
enum class ComponentEvent {
	CNT_ADD_CHILD,
	CNT_KILL_CHILD,

	LBL_UPDATE_TEXT,
	LBL_SET_COLOR,
};
