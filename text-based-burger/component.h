#pragma once

#include "json.hpp"
using json = nlohmann::json;

#include "ui_systems.h"
#include "math_utils.h"

#include <algorithm>
#include <vector>
#include <string>

// The root ui component class. All ui components inherit from this class.

class UIComponent {

public:
	// Constructor
	UIComponent(UIComponentIO& the_comp_io); // Construct an empty component
	UIComponent(json data, std::pair<int, int> offset, UIComponentIO& the_comp_io); // Construct from json data


	// THESE CAN ONLY BE MOVED DO NOT EVER EVER COPY THEM
	// ESPECIALLY IN THE INITIALIZER LIST EVERYTHING WILL EXPLODE
	// INCLUDING THE COMPILER ERROR REPORTER
	
	// Delete copy constructor and assignment
	UIComponent(const UIComponent&) = delete;
	UIComponent& operator=(const UIComponent&) = delete;

	// Add move constructor and assignment
	UIComponent(UIComponent&&) = default;
	UIComponent& operator=(UIComponent&&) = default;

	// Destructor
	~UIComponent();

	// Update function, returns true if the component should rerender
	virtual bool update(UIUpdateData data);

	// Tell the component to render to the given screen
	// If it runs out of bounds it just clips
	virtual void render(std::vector<std::vector<uint32_t>>&screen);

	// This may be overridden by some components that cannot contain children
	virtual void contains(std::unique_ptr<UIComponent>&& component);

	std::vector<UIComponent*> get_children();

	std::string targetname;

protected:
	// These are all common to all components

	std::pair<int, int> position;

	std::vector<std::unique_ptr<UIComponent>> children;

	UIComponentIO& comp_io;

};

// Function to iterate over every leaf in component tree from some given component
// Returns iterator that returns leaves in bfs order
std::vector<UIComponent*> iterate_leaves(UIComponent* component);

std::unique_ptr<UIComponent> component_type_selector(json data, std::pair<int, int>, UIComponentIO& reporter);

class Container : public UIComponent {
public:
	Container(json data, std::pair<int, int> offset, UIComponentIO& the_comp_io);

	virtual void render(std::vector<std::vector<uint32_t>>& screen) override;

protected:
	bool colored_container = false;
	int bg_color = 0;
	vec2 bbox_from;
	vec2 bbox_to;
};


// One very important feature: Because keyboards do not have 256 characters, we would need a
// look up table to do that, but because of microsoft's dedication to backwards compatibility,
// the characters can be entered using alt codes, and they are 1:1 to cp437. This means that 
// to interact with labels as a human, you only need to hold alt and type the number on the numpad.
class Label : public UIComponent {
public:
	Label(json data, std::pair<int, int> offset, UIComponentIO& the_comp_io);

	virtual void contains(std::unique_ptr<UIComponent>&& component) override;

	virtual void render(std::vector<std::vector<uint32_t>>& screen) override;

	virtual bool update(UIUpdateData data) override;
	
	void update_text(std::string new_text);
	void update_text(std::vector<int> new_text);

	void change_fg_color(int new_color, bool relative);
	void change_bg_color(int new_color, bool relative);
protected:
	std::vector<int> text;
	int foreground_color;
	int background_color;
	bool should_render;
};

class Button : public Label {
public:
	Button(json data, std::pair<int, int> offset, UIComponentIO& the_comp_io);

	virtual bool update(UIUpdateData data) override;

	void set_click_script(std::string script);
	void set_hover_script(std::string script);

protected:
	bool is_hovering;
	bool is_clicking;
	bool fire_only_once;
	bool have_already_fired;
	bool is_click_start_inside;

	int hover_color_fg = 0;
	int hover_color_bg = 0;

	std::pair<int, int> bbox;

	std::string click_script_name;
	std::string hover_script_name;

	json click_script_args;
	json hover_script_args;

	void on_hover();
	void on_press();
	void on_click();
	void on_release();
	void on_exit();
};

// Idenitical to label but executes script on every frame. Should only be used for
// simple displays like an fps counter.
class DynLabel : public Label {
public:
	DynLabel(json data, std::pair<int, int> offset, UIComponentIO& the_comp_io);

	virtual bool update(UIUpdateData data) override;

protected:

	std::string script_name;
};

class TextField : public UIComponent {
public:
	TextField(json data, std::pair<int, int> offset, UIComponentIO& the_comp_io);

	virtual bool update(UIUpdateData data) override;
	virtual void render(std::vector<std::vector<uint32_t>>& screen) override;
	virtual void contains(std::unique_ptr<UIComponent>&& component) override;

	std::string get_text() const { return current_text; }
	void set_text(const std::string& text);
	float get_float_value() const;
	void set_float_value(float value);

protected:
	std::string current_text;
	std::string placeholder_text;
	bool is_focused;
	bool is_hovering;
	size_t cursor_position;
	int foreground_color;
	int background_color;
	int focused_bg_color;
	int hover_bg_color;
	std::pair<int, int> bbox;
	bool is_numeric_only;
	
	void handle_key_input(int key, int action);
	bool is_valid_numeric_char(char c) const;
};