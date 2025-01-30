#include "component.h"

using namespace std;

UIComponent::UIComponent() {
	children = vector<UIComponent>();
};

UIComponent::~UIComponent() {
	children.clear();
};

void UIComponent::init() {
	return;
}

void UIComponent::update(std::pair<int, int> mouse_pos, int time, bool is_clicking) {
	return;
}

void UIComponent::render() {
	return;
}

void UIComponent::contains(UIComponent component) {
	children.push_back(component);
	return;
}

// Labels do not update, have special init function, and hold text
void Label::contains() {
	return;
}

void Label::init(std::string text) {
	// Go through look up table and convert to numbers
	return;
}