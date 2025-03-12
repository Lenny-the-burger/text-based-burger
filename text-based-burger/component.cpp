#include "component.h"
#include "char_lut.h"
#include "hash_fnv1a.h"

using namespace std;

// UI COMPONENT
UIComponent::UIComponent(ComponentIO& the_comp_io) : comp_io(the_comp_io) {
	targetname = "root";
	position = make_pair(0, 0); // This constructor should only be used for the root component
	return;
};

UIComponent::UIComponent(json data,pair<int, int> offset, ComponentIO& the_comp_io) : comp_io(the_comp_io) {
	targetname = to_string(data["targetname"]); // This should always return a string targetname im not error checking
	position = make_pair(data["position"]["x"], data["position"]["y"]);
	comp_io.register_component(targetname, this);

	// Parental offsets are computed statically, so components cannot be moved after creation
	// They must be destroyed and recreated, but ideally this should never happen. You should
	// use an animating component for that. Otherwise, this would require backtracking through
	// the entire ui tree on every update. This is probably fine perf wise but we still dont
	// want to do it.

	// Add the offset to the position
	position.first += offset.first;
	position.second += offset.second;

	// Recursivly build the tree
	for (json child : data["children"]) {
		contains(move(type_selector(child, position, comp_io)));
	}

	return;
};

UIComponent::~UIComponent() {
	children.clear();
};

bool UIComponent::update(UpdateData data) {
	// This is where you handle you pull events
	return false;
}

void UIComponent::render(std::vector<std::vector<uint32_t>>& screen) {
	return;
}

void UIComponent::contains(unique_ptr<UIComponent>&& component) {
	children.push_back(move(component));
	return;
}

vector<UIComponent*> UIComponent::get_children() {
	vector<UIComponent*> raw_children;
	for (auto& child : children) {
		raw_children.push_back(child.get()); // Extract raw pointer
	}
	return raw_children;
}

void UIComponent::call_event(ComponentEvent event, json data) {
	// This is where you would handle your push events
	return;
}

vector<UIComponent*> iterate_leaves(UIComponent* component) {
	vector<UIComponent*> leaves;
	vector<UIComponent*> queue;

	queue.push_back(component);

	while (!queue.empty()) {
		UIComponent* current = queue.back();
		queue.pop_back();

		vector<UIComponent*> children = current->get_children();

		if (children.empty()) {
			leaves.push_back(current);
		}
		else {
			for (UIComponent* child : children) {
				queue.push_back(child);
			}
		}
	}

	return leaves;
}

hash<string> hasher;

unique_ptr<UIComponent> type_selector(json data, pair<int, int> offset, ComponentIO& reporter) {
	std::string type_str = data["type"].get_ref<const std::string&>();
	const char* type = type_str.c_str();
	int len = (int)type_str.size();

	switch (hash_64_fnv1a(type, len)) {
		case hash_64_fnv1a_const("label"):
			return make_unique<Label>(data, offset, reporter);
		case hash_64_fnv1a_const("container"):
			return make_unique<Container>(data, offset, reporter);

		default:
			return make_unique<UIComponent>(data, offset, reporter);
	}

}


// CONTAINER

Container::Container(json data, pair<int, int> offset, ComponentIO& the_comp_io) 
	: UIComponent(data, offset, the_comp_io) {
	return;
}

// LABEL

Label::Label(json data, pair<int, int> offset, ComponentIO& the_comp_io) 
	: UIComponent(data, offset, the_comp_io) {
	update_text(to_string(data["text"]));
	foreground_color = data["style"]["fg"];
	background_color = data["style"]["bg"];

	// Report any illegal colours, illegal text should never happen
	if (foreground_color < 0 || foreground_color > 255) {
		comp_io.report_error(
			"ERROR: Tried to create label " + targetname + " with illegal fg colour of " + to_string(foreground_color)
		);
	}
	if (background_color < 0 || background_color > 255) {
		comp_io.report_error(
			"ERROR: Tried to create label " + targetname + " with illegal bg colour of " + to_string(background_color)
		);
	}

	// Report an error if the fg and bg colours are the same
	if (foreground_color == background_color) {
		comp_io.report_error(
			"WARN: Tried to create label " + targetname + " with fg and bg colours the same"
		);
	}

	return;
}

// Labels do not update, have special init function, and hold text
void Label::contains(std::unique_ptr<UIComponent>&& component) {
	// labels cannot get pregnant
	comp_io.report_error("FATAL: Tried to add a child to a label " + targetname);
	return;
}

void Label::update_text(vector<int> new_text) {
	text.clear();
	for (int i : new_text) {
		text.push_back(i);
	}
}

void Label::update_text(string new_text) {
	text.clear();
	// truncate last and first characters away as it somehow reads the " characters
	// which should not be possible
	new_text = new_text.substr(1, new_text.size() - 2);
	for (char c : new_text) {
		text.push_back(char2int(c));
	}
}

void Label::render(std::vector<std::vector<uint32_t>>& screen) {
	// Start at the root position and write in a straight line until you run out of text
	int x = position.first;
	int y = position.second;

	// Catch any out of bounds errors if we try to render off screen and report them
	try {
		for (int i : text) {
			// Dont do any fancy calculations just try to write to the coordinates
			screen.at(y).at(x) = gen_frag(i, background_color, foreground_color);
			x++;
		}
	}
	catch (out_of_range) {
		comp_io.report_error("WARN: Label " + targetname + " tried to render out of bounds");
	}
	return;
}


// BUTTON