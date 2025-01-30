#pragma once

#include <vector>
#include <string>

// The root ui component class. All ui components inherit from this class.

class UIComponent {

public:
	// Constructor
	UIComponent();
	// Destructor
	~UIComponent();

	// Initialize the ui component
	void init();

	// Update function
	void update(std::pair<int, int>, int time, bool is_clicking);

	// Tell the component to render
	void render();

	void contains(UIComponent component);

private:

	std::vector<UIComponent> children;

};


class Container : UIComponent {};

class Label : UIComponent {
public:
	void contains();

	void init(std::string text);
private:
	std::vector<int> text;
};