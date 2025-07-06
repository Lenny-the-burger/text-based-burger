#include "scripts.h"

// Best practice is to include every object class in one giant script file yes this is a good idea
#include "ui_systems.h"
#include "component.h"
#include "ui_handler.h"

#include "game_object.h"
#include "game_object_handler.h"

#include "systems_controller.h"

#include "threading_utils.h"

#include <iostream>
#include "json.hpp"


Script get_script(std::string name) {
    // Come get your scripts
	static const std::unordered_map<std::string, Script> script_map = {
		{"button_test", test_button_script},
		{"hover_reporter", hover_reporter},
		{"mousepos", mouse_pos_shower},
		{"basic_mover", basic_mover},
		{"map_loader", map_loader},
		{"map_unloader", map_unloader},
		{"build_bvh", build_bvh},
		{"bvh_build_done", bvh_build_done},
		{"toggle_show_bvh", toggle_show_bvh},
		{"npc_move", npc_move}
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

void call_script(std::string script_name, json args, ScriptHandles handles) {
	Script script = get_script(script_name);
	if (script == nullptr) {
		handles.controller->script_error_reporter.report_error("ERROR: Script" + to_string(args["caller"]) + " tried to call a non existant script " + script_name);
		return;
	}
	script(args, handles);
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

void map_loader(json data, ScriptHandles handles) {
	// Get the map name from the data
	std::string map_name = data["map_name"].get<std::string>();
	// Load the map
	handles.controller->load_map(map_name);
	return;
}

void map_unloader(json data, ScriptHandles handles) {
	// Unload the current map
	handles.controller->unload_map();
	return;
}

void build_bvh(json data, ScriptHandles handles) {
	// Compile inputs
	BVInput input;
	MapGeometry* map_geom = handles.map_manager->get_geometry();

	// We should realy just pass the MapGeometry directly but whatever
	input.lines = map_geom->lines;
	input.types = map_geom->types;

	std::unordered_map<std::string, BVHType> bvh_type_map = {
		{"collision", BVH_COLLISION},
		{"cosmetic", BVH_COSMETIC}
	};

	std::string type_str = data["type"].get<std::string>();
	if (bvh_type_map.find(type_str) != bvh_type_map.end()) {
		input.build_type = bvh_type_map[type_str];
	}
	else {
		handles.controller->script_error_reporter.report_error("ERROR: Requested invalid BVH type to build '" + type_str + "'");
		return;
	}

	switch (input.build_type) {
	case BVH_COLLISION:
		input.bvh_nodes = map_geom->bvh_collision_nodes;
		break;
	case BVH_COSMETIC:
		input.bvh_nodes = map_geom->bvh_cosmetic_nodes;
		break;
	}

	// Dont collide while building
	handles.map_manager->has_collision_bvh = false;

	// Launch a long thread to build the bvh using map utils buildBVH()
	handles.long_thread_controller->launch("compile_quadtree_" + type_str, input,
		[](LongThreadState& state) {
			BVInput input = std::any_cast<BVInput>(state.input);
			auto result = buildBVH(input, state);
			state.output = std::move(result);
			state.progress = 100;
		},
		[data = data, handles]() mutable {
			call_script("bvh_build_done", {
				{"caller", "build_bvh"},
				{"type", data["type"]}
				}, handles);
		}
	);

	return;
}

void bvh_build_done(json data, ScriptHandles handles) {
	// We did it
	std::cout << "BVH build done for type: " << data["type"].get<std::string>() << std::endl;
	handles.map_manager->has_collision_bvh = true;

	return;
}

void toggle_show_bvh(json data, ScriptHandles handles) {
	// Toggle the draw bvh flag in the map manager
	handles.map_manager->toggle_render_bvh();
	return;
}

void npc_move(json data, ScriptHandles handles) {
	// This
	std::string name = data["targetname"].get<std::string>();
	NPC* npc = dynamic_cast<NPC*>(handles.obj_io->get_object(name));
	if (npc == nullptr) {
		handles.obj_io->report_error("ERROR: NPC move called on non existant npc " + name + " by " + data["caller"].get<std::string>());
		return;
	}
	vec2 move_vel = vec2(data); // vec2 can hoover up any json object with x and y

	// if bvh does not exist dont collide with anything
	if (!handles.map_manager->has_collision_bvh) {
		// No collision, just move
		npc->position += move_vel;
		return;
	}

	// NPCs should always be 16*16
	vec2 npc_from = npc->position - 16;
	vec2 npc_to = npc->position + 16;

	// Aspirational position
	npc_from += move_vel;
	npc_to += move_vel;

	vec2 normal_force = collide_aabb_geometry(
		npc_from, npc_to,
		handles.map_manager->get_geometry()->bvh_collision_nodes,
		handles.map_manager->get_geometry()->lines
	);

	bool temp = normal_force.is_zero();

	normal_force *= 10.0f;

	move_vel += normal_force;

	// Set new position
	npc->position += move_vel;
}