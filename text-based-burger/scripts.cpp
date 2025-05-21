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
        {"button_test", test_button_script}
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