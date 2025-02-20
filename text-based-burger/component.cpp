#include "component.h"
#include "char_lut.h"

using namespace std;

// Error Reporter
ErrorReporter::ErrorReporter() {
	// Reporters will always start with an empty log
	error_log = vector<string>();
	return;
}

ErrorReporter::~ErrorReporter() {
	error_log.clear();
	return;
}

void ErrorReporter::report_error(string error) {
	// For now only report string errors
	error_log.push_back(error);
	return;
}

vector<string> ErrorReporter::get_log() {
	return error_log;
}

uint32_t gen_frag(int character, int bg, int fg) {
	// Generate a fragment from a character, background color, and foreground color
	// This is a 32 bit integer with the first 8 bits being the character, the next
	// 8 bits being the background color, and the next 8 bits being the foreground color
	// The last 8 bits are unused
	uint32_t frag = 0;
	frag |= character;
	frag |= bg << 8;
	frag |= fg << 16;

	return frag;
}


// UI COMPONENT
UIComponent::UIComponent(ErrorReporter& the_error_reporter) : error_reporter(the_error_reporter) {
	name = "root";
	position = make_pair(0, 0);
	return;
};

UIComponent::UIComponent(json data, ErrorReporter& the_error_reporter) : error_reporter(the_error_reporter) {
	name = to_string(data["name"]); // This should always return a string name im not error checking
	position = make_pair(data["position"]["x"], data["position"]["y"]);

	// Recursivly build the tree
	for (json child : data["children"]) {
		contains(move(type_selector(child, error_reporter)));
	}

	return;
};

UIComponent::~UIComponent() {
	children.clear();
};

bool UIComponent::update(update_data data) {
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

std::unique_ptr<UIComponent> type_selector(json data, ErrorReporter& reporter) {
	// const char*, string, and whatever the fuck nlhomann json uses is going to
	// make me die i didnt want to use a switch anyway

	if (data["type"] == "label") {
		return make_unique<Label>(data, reporter);
	}
	if (data["type"] == "container") {
		return make_unique<Container>(data, reporter);
	}
	else {
		return make_unique<UIComponent>(data, reporter);
	}
}


// CONTAINER

Container::Container(json data, ErrorReporter& the_error_reporter) : UIComponent(data, the_error_reporter) {
	return;
}

// LABELS

Label::Label(json data, ErrorReporter& the_error_reporter) : UIComponent(data, the_error_reporter) {
	update_text(to_string(data["text"]));
	foreground_color = data["style"]["fg"];
	background_color = data["style"]["bg"];

	// Report any illegal colours, illegal text should never happen
	if (foreground_color < 0 || foreground_color > 255) {
		error_reporter.report_error(
			"Tried to create label " + name + " with illegal fg colour of " + to_string(foreground_color)
		);
	}
	if (background_color < 0 || background_color > 255) {
		error_reporter.report_error(
			"Tried to create label " + name + " with illegal bg colour of " + to_string(background_color)
		);
	}

	// Report an error if the fg and bg colours are the same
	if (foreground_color == background_color) {
		error_reporter.report_error(
			"Tried to create label " + name + " with fg and bg colours the same"
		);
	}

	return;
}

// Labels do not update, have special init function, and hold text
void Label::contains(std::unique_ptr<UIComponent>&& component) {
	// labels cannot get pregnant
	error_reporter.report_error("Tried to add a child to a label " + name);
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
	for (char i : new_text) {
		text.push_back(char2int(i));
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
		}
	}
	catch (out_of_range) {
		error_reporter.report_error("Label " + name + " tried to render out of bounds");
	}
	return;
}