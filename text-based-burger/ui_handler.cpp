#include "ui_handler.h"
#include "json.hpp"
#include <fstream> // Include this header to use ifstream

using namespace std;
using json = nlohmann::json; // for convenience

UIHandler::UIHandler(string filename) {
	// Everything is stored in a json file

	// Load the json file
	ifstream f(filename);
	if (!f.is_open()) {
		throw runtime_error("Could not open file " + filename);
	}
	json data = json::parse(f);

	// Everything is in an array names "root". Those are the children of the root component
	json root_children = data["root"];

	// Build the component tree recursively
	root = UIComponent();
	for (json child : root_children) {
		root.contains(UIComponent(child));
	}
}

void UIHandler::update(update_data data) {
	// Update the root component
	vector<UIComponent> leaves = iterate_leaves(root);
	for (UIComponent leaf : leaves) {
		leaf.update(data);
	}
}

void UIHandler::rerender_all() {
	// Rerender all components
	vector<UIComponent> leaves = iterate_leaves(root);
	for (UIComponent leaf : leaves) {
		leaf.render();
	}
}