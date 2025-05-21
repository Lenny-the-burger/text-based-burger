#include "scripts.h"

// Best practice is to include every object class in one giant script file yes this is a good idea
#include "ui_systems.h"
#include "component.h"
#include "ui_handler.h"

#include <iostream>
#include "json.hpp"


Script get_script(std::string name) {
    // Come get your scripts
    static const std::unordered_map<std::string, Script> script_map = {
        {"button_test", test_button_script},
		{"hover_reporter", hover_reporter},
		{"mousepos", mouse_pos_shower},
    };

    auto it = script_map.find(name);
    if (it != script_map.end()) {
        return it->second;
    }
    else {
		// Script does not exist, you should report this as an error
        return nullptr;
    }
}

void test_button_script(json data, ComponentIO& io) {
	// Dump data to console
	std::cout << "Test button script called with data:\n" << data.dump() << std::endl;

	return;
}

void hover_reporter(json data, ComponentIO& io) {
	// Dump data to console
	std::cout << "Hovering over " << data["targetname"] << std::endl;
	return;
}

void mouse_pos_shower(json data, ComponentIO& io) {
	// Get the caller component from the io
	DynLabel* lbl = dynamic_cast<DynLabel*>(io.get_component(data["targetname"]));

	if (lbl == nullptr) {
		io.report_error("ERROR: Mouse pos shower called on non existant component " + data["targetname"]);
		return;
	}

	std::string new_text = "";

	// Ignore negative values
	int mouse_x = abs(data["mouse_char_x"].get<int>());
	int mouse_y = abs(data["mouse_char_y"].get<int>());

	// Pad with 0s
	if (mouse_x < 100) {
		new_text += "0";
	} 
	if (mouse_x < 10) {
		new_text += "0";
	}
	new_text += std::to_string(mouse_x) + ",";

	if (mouse_y < 100) {
		new_text += "0";
	}
	if (mouse_y < 10) {
		new_text += "0";
	}
	new_text += std::to_string(mouse_y);

	lbl->update_text(new_text);

    return;
}