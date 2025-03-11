#include "ui_handler.h"
#include "json.hpp"
#include <fstream> // Include this header to use ifstream

using namespace std;
using json = nlohmann::json; // for convenience

UIHandler::UIHandler() : error_log(), root(move(UIComponent(error_log))) {
	// Set error log to empty
	error_log = ErrorReporter();
}

UIHandler::UIHandler(string filename, int scrn_w, int scrn_h)
	: error_log(), root(move(UIComponent(error_log))) {
	// Set error log to empty
	error_log = ErrorReporter();

	// Initialize the screen to all 0s
	screen = vector<vector<uint32_t>>(scrn_h, vector<uint32_t>(scrn_w, 0));

	// Everything is stored in a json file

	// Load the json file
	ifstream f(filename);
	if (!f.is_open()) {
		throw runtime_error("Could not open file " + filename);
	}
	json data = json::parse(f);

	// Everything is in an array names "root". Those are the children of the root component
	json root_children = data["root"];

	for (json child : root_children) {
		// Children of root will always have no offset, as root is always at 0, 0
		root.contains(move(type_selector(child, make_pair(0, 0), error_log)));
	}

	// Set the root component
}

void UIHandler::update(UpdateData data) {
	// Update the root component
	vector<UIComponent*> leaves = iterate_leaves(&root);
	for (UIComponent* leaf : leaves) {
		if (leaf->update(data)) {
			leaf->render(screen);
		}
	}
}

void UIHandler::rerender_all() {
	// Rerender all components
	vector<UIComponent*> leaves = iterate_leaves(&root);
	for (UIComponent* leaf : leaves) {
		leaf->render(screen);
	}
}

void UIHandler::toggle_error_log() {
	show_error_log = !show_error_log;
}

void UIHandler::render_error_log() {
	// Render the error log

	// Clear the screen
	screen = vector<vector<uint32_t>>(screen.size(), vector<uint32_t>(screen[0].size(), 0));

	int line = 0;
	vector<int> all_repeats = error_log.get_repeats();
	for (string error : error_log.get_log()) {

		// Check if the error is repeated and if >0 render the number of repeats
		int repeats = all_repeats[line];

		// append the number of repeats to the error
		if (repeats > 0) {
			error += " x" + to_string(repeats);
		}

		// Render the error
		for (int i = 0; i < error.size(); i++) {
			int char_num = char2int(error[i]);
			uint32_t char_packed = gen_frag(char_num, 0, 255);
			screen[line][i] = char_packed;
		}

		line++;
	}
}

pair<int, int> get_update_window() {
	// What part of the screen actually needs to be updated?
	
	// Loop through the should_update array in flattened order and record the first and last
	// places that need to be updated

	return make_pair(0, 0);
}

void UIHandler::cls() {
	// Clear the screen
	screen = vector<vector<uint32_t>>(screen.size(), vector<uint32_t>(screen[0].size(), 0));
}

std::vector<std::vector<uint32_t>> UIHandler::get_screen() {
	if (show_error_log) {
		// This is innefficient but you probably won't be rendering the error log all the time
		render_error_log();
	}
	return screen;
}