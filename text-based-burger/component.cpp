#include "component.h"
#include "char_lut.h"

using namespace std;

UIComponent::UIComponent() {
	children = vector<UIComponent>();
	name = "UIComponent";
	return;
};

UIComponent::UIComponent(json data) {
	name = to_string(data["name"]); // This should always return a string name im not error checking
	position = make_pair(data["position"]["x"], data["position"]["y"]);

	children = vector<UIComponent>();

	// Recursivly build the tree
	for (json child : data["children"]) {
		contains(type_selector(child));
	}

	return;
};

UIComponent::~UIComponent() {
	children.clear();
};

void UIComponent::update(update_data data) {
	return;
}

void UIComponent::render() {
	return;
}

void UIComponent::contains(UIComponent component) {
	children.push_back(component);
	return;
}

vector<UIComponent> UIComponent::get_children() {
	return children;
}

vector<UIComponent> iterate_leaves(UIComponent component) {
	vector<UIComponent> leaves;
	vector<UIComponent> queue;
	queue.push_back(component);
	while (!queue.empty()) {
		UIComponent current = queue.back();
		queue.pop_back();
		vector<UIComponent> children = current.get_children();
		if (children.empty()) {
			leaves.push_back(current);
		}
		else {
			for (UIComponent child : children) {
				queue.push_back(child);
			}
		}
	}
	return leaves;
}

UIComponent type_selector(json data) {
	// const char*, string, and whatever the fuck nlhomann json uses is going to
	// make me die i didnt want to use a switch anyway

	if (data["type"] == "label") {
		return Label(data);
	}
	if (data["type"] == "container") {
		return Container(data);
	}
	else {
		return UIComponent(data);
	}
}


// CONTAINER

Container::Container(json data) {
	return;
}

// LABELS

Label::Label(json data) : UIComponent(data) {
	update_text(to_string(data["text"]));
	foreground_color = data["style"]["fg"];
	background_color = data["style"]["bg"];
	return;
}

// Labels do not update, have special init function, and hold text
void Label::contains() {
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