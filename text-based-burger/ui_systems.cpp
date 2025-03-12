#include "ui_systems.h"
#include "component.h"

using namespace std;

// Error Reporter
ComponentIO::ComponentIO() {
	// Reporters will always start with an empty log
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

void ComponentIO::register_component(string name, UIComponent* component) {
	// First check if the component is already registered
	if (component_registry.find(name) != component_registry.end()) {
		// If it is, report an error
		// this should never happen and will be a nightmare to debug
		report_error("FATAL: Tried to register component " + name + " twice");
		return;
	}
	
	component_registry[name] = component;
	return;
}

void ComponentIO::send_event(string target, ComponentEvent event, json data) {
	// Check if the target component is registered
	if (component_registry.find(target) == component_registry.end()) {
		// If it is not, report an error
		report_error("FATAL: Tried to send event to unregistered component " + target);
		return;
	}
	// Get the target component
	UIComponent* target_component = component_registry[target];

	// Call the event on the target component
	target_component->call_event(event, data);

	return;
}

UIComponent* ComponentIO::get_component(string name) {
	// Check if the target component is registered
	if (component_registry.find(name) == component_registry.end()) {
		// If it is not, report an error
		report_error("FATAL: Tried to get unregistered component " + name);
		return nullptr;
	}
	// Get the target component
	return component_registry[name];
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