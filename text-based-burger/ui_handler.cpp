#include "ui_handler.h"
#include "json.hpp"
#include <fstream> // Include this header to use ifstream

using namespace std;
using json = nlohmann::json; // for convenience

UIHandler::UIHandler(string filename, int scrn_w, int scrn_h, SystemsController& new_controller)
	: component_io(new_controller), root(move(UIComponent(component_io))) {

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
		root.contains(move(component_type_selector(child, make_pair(0, 0), component_io)));
	}

	// Read the stencil regions if they exist
	if (data.contains("stencil_regions")) {
		json stencil_data = data["stencil_regions"];
		for (const auto& region : stencil_data) {
			// stencils should be stored as an array of 2 number arrays
			stencil_regions.push_back(vec2(region)); // vec2 eat json nom nom
		}
		try {
			// If the stencil state is not present, default to 1 (stencil out)
			stencil_state = data["stencil_state"].get<int>();
		}
		catch (const json::exception& e) {
			// you probably meant stencil in
			stencil_state = 0;
		}
	}
	else {
		// stencil the entire screen by default
		stencil_regions = {};
		stencil_state = 1; // set mode to stencil out so everything passes
	}
}

void UIHandler::update(UIUpdateData data) {
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

void UIHandler::cls() {
	// Clear the screen
	screen = vector<vector<uint32_t>>(screen.size(), vector<uint32_t>(screen[0].size(), 0));
}

std::vector<std::vector<uint32_t>> UIHandler::get_screen() {
	return screen;
}