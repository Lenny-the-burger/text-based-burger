#include "scripts.h"

// Best practice is to include every object class in one giant script file yes this is a good idea
#include "ui_systems.h"
#include "component.h"
#include "ui_handler.h"

#include "game_object.h"
#include "game_object_handler.h"

#include <iostream>
#include "json.hpp"


Script get_script(std::string name) {
    // Come get your scripts
    static const std::unordered_map<std::string, Script> script_map = {
        {"button_test", test_button_script},
		{"hover_reporter", hover_reporter},
		{"mousepos", mouse_pos_shower},
		{"basic_mover", basic_mover}
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

void test_button_script(json data, ScriptHandles handles) {
	// Dump data to console
	std::cout << "Test button script called with data:\n" << data.dump() << std::endl;

	return;
}

void hover_reporter(json data, ScriptHandles handles) {
	// Dump data to console
	std::cout << "Hovering over " << data["targetname"] << std::endl;
	return;
}

void mouse_pos_shower(json data, ScriptHandles handles) {
	// Get the caller component from the io
	DynLabel* lbl = dynamic_cast<DynLabel*>(handles.ui_io->get_component(data["targetname"]));

	if (lbl == nullptr) {
		handles.ui_io->report_error("ERROR: Mouse pos shower called on non existant component " + data["targetname"]);
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

void basic_mover(json data, ScriptHandles handles) {
	// Get the caller component from the io
	GameObject* obj = dynamic_cast<GameObject*>(handles.obj_io->get_object(data["targetname"]));

	if (obj == nullptr) {
		handles.obj_io->report_error("ERROR: Basic mover called on non existant object " + data["targetname"]);
		return;
	}
	
	obj->position = vec2(
		data["x"].get<float>(),
		data["y"].get<float>()
	);
	return;
}