#pragma once

#include "json.hpp"
using json = nlohmann::json;

#include <vector>
#include <string>


struct update_data {
	std::pair<int, int> mouse_position;
	int time;
	bool is_clicking;
};

// The root ui component class. All ui components inherit from this class.

class UIComponent {

public:
	// Constructor
	UIComponent();
	UIComponent(json data); // Construct from json data

	// Destructor
	~UIComponent();

	// Update function
	void update(update_data data);

	// Tell the component to render
	void render();

	void contains(UIComponent component);

	std::vector<UIComponent> get_children();

	std::string name;

private:

	std::pair<int, int> position;

	std::vector<UIComponent> children;

};

// Function to iterate over every leaf in component tree from some given component
// Returns iterator that returns leaves in bfs order
std::vector<UIComponent> iterate_leaves(UIComponent component);

UIComponent type_selector(json data);


class Container : public UIComponent {
public:
	Container(json data);
};


// One very important feature: Because keyboards do not have 256 characters, we would need a
// look up table to do that, but because of microsoft's dedication to backwards compatibility,
// the characters can be entered using alt codes, and they are 1:1 to cp437. This means that 
// to interact with labels as a human, you only need to hold alt and type the number on the numpad.
class Label : public UIComponent {
public:
	Label(json data);

	void contains();
	
	void update_text(std::string new_text);
	void update_text(std::vector<int> new_text);
private:
	std::vector<int> text;
	int foreground_color;
	int background_color;
};