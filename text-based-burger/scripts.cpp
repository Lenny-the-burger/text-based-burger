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
		{"metamap_loader", metamap_loader},
		{"map_unloader", map_unloader},
		{"build_bvh", build_bvh},
		{"bvh_build_done", bvh_build_done},
		{"toggle_show_bvh", toggle_show_bvh},
		{"npc_move", npc_move},
		{"set_canvas_tool", set_canvas_tool},
		{"toggle_snapping", toggle_snapping},
		{"toggle_grid_snapping", toggle_grid_snapping},
		{"update_canvas_color_from_ui", update_canvas_color_from_ui},
		{"save_mesh_file", save_mesh_file},
		{"load_mesh_file", load_mesh_file},
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
	
	// So it doesnt freak out when you minimize the window
	mouse_x = std::min(mouse_x, 999);
	mouse_y = std::min(mouse_y, 999);

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
	
	obj->position = vec2(data);
	return;
}

void map_loader(json data, ScriptHandles handles) {
	// Get the map name from the data
	std::string map_name = data["map_name"].get<std::string>();
	// Load the map
	handles.controller->load_map(map_name);
	return;
}

void metamap_loader(json data, ScriptHandles handles) {
	// Get the map name from the data
	std::string map_name = data["map_name"].get<std::string>();
	// Load the metamap
	handles.controller->load_metamap(map_name);
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

	move_vel += normal_force;

	// Set new position
	npc->position += move_vel;
}

void set_canvas_tool(json data, ScriptHandles handles) {
	std::string tool_type = data["type"].get<std::string>();
	
	// Find the canvas object - look for the mesh viewer canvas
	LineCanvas* canvas = dynamic_cast<LineCanvas*>(handles.obj_io->get_object("mesh viewer"));
	if (canvas == nullptr) {
		handles.obj_io->report_error("ERROR: set_canvas_tool could not find mesh viewer canvas object");
		return;
	}
	
	CanvasTool new_tool;
	if (tool_type == "draw") {
		new_tool = CANVAS_TOOL_DRAW_LINE;
	}
	else if (tool_type == "select") {
		new_tool = CANVAS_TOOL_SELECT;
	}
	else if (tool_type == "edit") {
		new_tool = CANVAS_TOOL_EDIT;
	}
	else {
		handles.obj_io->report_error("ERROR: set_canvas_tool received invalid tool type: " + tool_type);
		return;
	}
	
	canvas->set_active_tool(new_tool);
	
	return;
}

void toggle_snapping(json data, ScriptHandles handles) {
	// Find the canvas object
	LineCanvas* canvas = dynamic_cast<LineCanvas*>(handles.obj_io->get_object("mesh viewer"));
	if (canvas == nullptr) {
		handles.obj_io->report_error("ERROR: toggle_snapping could not find mesh viewer canvas object");
		return;
	}
	
	// Toggle snapping state
	canvas->toggle_snapping();
	bool snapping_enabled = canvas->get_snapping_enabled();
	
	// Update button text to reflect current state
	Button* snap_button = dynamic_cast<Button*>(handles.ui_io->get_component("toggle snapping button"));
	if (snap_button != nullptr) {
		std::string button_text = snapping_enabled ? "Snap: On" : "Snap: Off";
		snap_button->update_text(button_text);
	}
	
	return;
}

void toggle_grid_snapping(json data, ScriptHandles handles) {
	// Find the canvas object
	LineCanvas* canvas = dynamic_cast<LineCanvas*>(handles.obj_io->get_object("mesh viewer"));
	if (canvas == nullptr) {
		handles.obj_io->report_error("ERROR: toggle_grid_snapping could not find mesh viewer canvas object");
		return;
	}
	
	// Toggle grid snapping state
	canvas->toggle_grid_snapping();
	bool grid_snapping_enabled = canvas->get_grid_snapping_enabled();
	
	// Update button text to reflect current state
	Button* grid_button = dynamic_cast<Button*>(handles.ui_io->get_component("toggle grid snapping button"));
	if (grid_button != nullptr) {
		std::string button_text = grid_snapping_enabled ? "Grid: On" : "Grid: Off";
		grid_button->update_text(button_text);
	}
	
	return;
}

void update_canvas_color_from_ui(json data, ScriptHandles handles) {
	// Find the canvas object
	LineCanvas* canvas = dynamic_cast<LineCanvas*>(handles.obj_io->get_object("mesh viewer"));
	if (canvas == nullptr) {
		handles.obj_io->report_error("ERROR: update_canvas_color_from_ui could not find mesh viewer canvas object");
		return;
	}
	
	// Get the text field values
	TextField* hue_field = dynamic_cast<TextField*>(handles.ui_io->get_component("hue field"));
	TextField* intensity_field = dynamic_cast<TextField*>(handles.ui_io->get_component("intensity field"));
	TextField* alpha_field = dynamic_cast<TextField*>(handles.ui_io->get_component("alpha field"));
	TextField* thickness_field = dynamic_cast<TextField*>(handles.ui_io->get_component("thickness field"));
	
	if (hue_field != nullptr) {
		canvas->set_selected_line_hue(hue_field->get_float_value());
	}
	
	if (intensity_field != nullptr) {
		canvas->set_selected_line_intensity(intensity_field->get_float_value());
	}
	
	if (alpha_field != nullptr) {
		canvas->set_selected_line_alpha(alpha_field->get_float_value());
	}
	
	if (thickness_field != nullptr) {
		canvas->set_selected_line_thickness(thickness_field->get_float_value());
	}
	
	return;
}

void save_mesh_file(json data, ScriptHandles handles) {
	// Find the canvas object
	LineCanvas* canvas = dynamic_cast<LineCanvas*>(handles.obj_io->get_object("mesh viewer"));
	if (canvas == nullptr) {
		handles.obj_io->report_error("ERROR: save_mesh_file could not find mesh viewer canvas object");
		return;
	}
	
	// For now, save to a default location in gamedata/meshes/
	// In a real implementation, this would use a file dialog
	std::string filename = "gamedata/meshes/saved_mesh.json";
	
	if (canvas->save_mesh_to_file(filename)) {
		std::cout << "Mesh saved successfully to " << filename << std::endl;
	} else {
		handles.obj_io->report_error("ERROR: Failed to save mesh to " + filename);
	}
	
	return;
}

void load_mesh_file(json data, ScriptHandles handles) {
	// Find the canvas object
	LineCanvas* canvas = dynamic_cast<LineCanvas*>(handles.obj_io->get_object("mesh viewer"));
	if (canvas == nullptr) {
		handles.obj_io->report_error("ERROR: load_mesh_file could not find mesh viewer canvas object");
		return;
	}
	
	// For now, load from a default location in gamedata/meshes/
	// In a real implementation, this would use a file dialog
	std::string filename = "gamedata/meshes/saved_mesh.json";
	
	if (canvas->load_mesh_from_file(filename)) {
		std::cout << "Mesh loaded successfully from " << filename << std::endl;
	} else {
		handles.obj_io->report_error("ERROR: Failed to load mesh from " + filename);
	}
	
	return;
}