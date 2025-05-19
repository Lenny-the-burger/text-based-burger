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

using ComponentFactory = std::function<std::unique_ptr<UIComponent>(json, std::pair<int, int>, ComponentIO&)>;

std::unique_ptr<UIComponent> type_selector(json data, std::pair<int, int> offset, ComponentIO& reporter) {
	static const std::unordered_map<std::string, ComponentFactory> factory_map = {
		{"label", [](json d, std::pair<int, int> off, ComponentIO& rep) {
			return std::make_unique<Label>(d, off, rep);
		}},
		{"container", [](json d, std::pair<int, int> off, ComponentIO& rep) {
			return std::make_unique<Container>(d, off, rep);
		}},
		{"button", [](json d, std::pair<int, int> off, ComponentIO& rep) {
			return std::make_unique<Button>(d, off, rep);
		}}
	};

	std::string type = data["type"].get_ref<const std::string&>();

	auto it = factory_map.find(type);
	if (it != factory_map.end()) {
		return it->second(data, offset, reporter);
	}

	return std::make_unique<UIComponent>(data, offset, reporter);
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

	should_render = true;

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

	should_render = true;
}

void Label::update_text(string new_text) {
	text.clear();
	// truncate last and first characters away as it somehow reads the " characters
	// which should not be possible
	new_text = new_text.substr(1, new_text.size() - 2);
	for (char c : new_text) {
		text.push_back(char2int(c));
	}

	should_render = true;
}

void Label::change_fg_color(int new_color, bool relative) {
	if (relative) {
		foreground_color += new_color;
	}
	else {
		foreground_color = new_color;
	}
	should_render = true;
}

void Label::change_bg_color(int new_color, bool relative) {
	if (relative) {
		background_color += new_color;
	}
	else {
		background_color = new_color;
	}
	should_render = true;
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

bool Label::update(UpdateData data) {
	if (should_render) {
		// Maybe we animated the text or something
		should_render = false;
		return true;
	}
	return false;
}


// BUTTON

Button::Button(json data, pair<int, int> offset, ComponentIO& the_comp_io)
	: Label(data, offset, the_comp_io) {

	// Buttons are just labels with a script attached
	click_script_name = to_string(data["click_script"]);
	hover_script_name = to_string(data["hover_script"]);

	click_script_args = data["click_script_args"];
	hover_script_args = data["hover_script_args"];

	fire_only_once = data["fire_only_once"];

	// If bbox is set to text, then the bounding box is the size of the text
	if (data["bbox"] == "text") {
		bbox = make_pair(text.size(), 1);
	}
	else {
		bbox = make_pair(data["bbox"]["x"], data["bbox"]["y"]);
	}
	
	// Init to false
	is_hovering = false;
	is_clicking = false;



	return;
}

bool Button::update(UpdateData data) {
	// Check if mouse is within the bbox
	bool within_bbox = (data.mouse_char_x >= position.first && data.mouse_char_x < position.first + bbox.first &&
		data.mouse_char_y >= position.second && data.mouse_char_y < position.second + bbox.second);

	if (within_bbox) {
		if (!is_hovering) {
			is_hovering = true;
			on_hover();
		}

		// Click logic
		if (data.is_clicking && !is_clicking) {
			is_clicking = true;
			on_click();
		}
		else if (!data.is_clicking && is_clicking) {
			is_clicking = false;
			on_release();
		}
	}
	else {
		if (is_hovering) {
			is_hovering = false;
			is_clicking = false;
			on_exit();
		}
	}

	if (should_render) {
		// Maybe we animated the text or something
		should_render = false;
		return true;
	}
	return false;
}

void Button::set_click_script(string script) {
	click_script_name = script;
	return;
}

void Button::set_hover_script(string script) {
	hover_script_name = script;
	return;
}

void Button::on_hover() {
	// Call the hover script
	return;
}

void Button::on_click() {
	// Call the click script
	return;
}

void Button::on_release() {
	// Call the release script
	return;
}

void Button::on_exit() {
	// Call the exit script
	return;
}