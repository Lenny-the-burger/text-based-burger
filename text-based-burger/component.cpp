#include "component.h"
#include "char_lut.h"

using namespace std;

// Component io handler
ComponentIO::ComponentIO() {
	// Reporters will always start with an empty error log
	error_log = vector<string>();
	return;
}

ComponentIO::~ComponentIO() {
	error_log.clear();
	return;
}

void ComponentIO::report_error(string error) {
	// Check if the previous reported error is the same as the current one
	if (!error_log.empty() && error_log.back() == error) {
		// If it is, increment the repeat counter
		repeats.back()++;
		return;
	}

	// For now only report string errors
	error_log.push_back(error);
	repeats.push_back(0);
	return;
}

vector<string> ComponentIO::get_log() {
	return error_log;
}

vector<int> ComponentIO::get_repeats() {
	return repeats;
}

uint32_t gen_frag(int character, int bg, int fg) {
	// Generate a fragment from a character, background color, and foreground color
	// This is a 32 bit integer with the first 8 bits being the character, the next
	// 8 bits being the background color, and the next 8 bits being the foreground color
	// The last 8 bits are unused
	uint32_t frag = 0;
	frag |= character;
	frag |= fg << 8;
	frag |= bg << 16;

	return frag;
}


// UI COMPONENT
UIComponent::UIComponent(ComponentIO& the_comp_io) : comp_io(the_comp_io) {
	targetname = "root";
	position = make_pair(0, 0); // This constructor should only be used for the root component
	return;
};

UIComponent::UIComponent(json data,pair<int, int> offset, ComponentIO& the_comp_io) : comp_io(the_comp_io) {
	targetname = to_string(data["targetname"]); // This should always return a string targetname im not error checking
	position = make_pair(data["position"]["x"], data["position"]["y"]);

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

unique_ptr<UIComponent> type_selector(json data, pair<int, int> offset, ComponentIO& reporter) {
	// const char*, string, and whatever the fck nlhomann json uses is going to
	// make me die i didnt want to use a switch anyway

	if (data["type"] == "label") {
		return make_unique<Label>(data, offset, reporter);
	}
	if (data["type"] == "container") {
		return make_unique<Container>(data, offset, reporter);
	}
	else { // Why are you creating a default component?
		return make_unique<UIComponent>(data, offset, reporter);
	}
}


// CONTAINER

Container::Container(json data, pair<int, int> offset, ComponentIO& the_comp_io) 
	: UIComponent(data, offset, the_comp_io) {
	return;
}

// LABELS

Label::Label(json data, pair<int, int> offset, ComponentIO& the_comp_io) 
	: UIComponent(data, offset, the_comp_io) {
	update_text(to_string(data["text"]));
	foreground_color = data["style"]["fg"];
	background_color = data["style"]["bg"];

	// Report any illegal colours, illegal text should never happen
	if (foreground_color < 0 || foreground_color > 255) {
		comp_io.report_error(
			"Tried to create label " + targetname + " with illegal fg colour of " + to_string(foreground_color)
		);
	}
	if (background_color < 0 || background_color > 255) {
		comp_io.report_error(
			"Tried to create label " + targetname + " with illegal bg colour of " + to_string(background_color)
		);
	}

	// Report an error if the fg and bg colours are the same
	if (foreground_color == background_color) {
		comp_io.report_error(
			"Tried to create label " + targetname + " with fg and bg colours the same"
		);
	}

	return;
}

// Labels do not update, have special init function, and hold text
void Label::contains(std::unique_ptr<UIComponent>&& component) {
	// labels cannot get pregnant
	comp_io.report_error("Tried to add a child to a label " + targetname);
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
		comp_io.report_error("Label " + targetname + " tried to render out of bounds");
	}
	return;
}