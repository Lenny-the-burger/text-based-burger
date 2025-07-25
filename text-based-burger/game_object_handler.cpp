#include "game_object_handler.h"

#include <fstream> // Include this header to use ifstream

using namespace std;


ObjectsHandler::ObjectsHandler(string filename, SystemsController& new_controller) : object_io(new_controller) {
	// How this works is that when a map is loaded the vdg file is given to the 
	// map loader, and the partner json file is sent here to load all the
	// various entities and other things. The map loader also parses the json
	// file for other things, but we are specifically in charge of handling
	// most entities, brushes for example are handles by both as they need to
	// have some presence with the geometry, but also need to interface with
	// other entities.

	// Entities only really exist in the context of maps, but can still be
	// dynamically created and destroyed, wich differs from the ui components
	// system. Game objects are also stored in a flat structure, game objects
	// usually do not have parents or children. If you think that something
	// needs children or parents to work, then you should probably rework
	// your script to not do that. For example dont create and destroy 
	// game objects for bullets that come from a gun, just do the calculation
	// in a script and render the visuals.

	

	// Create the mouse renderer object
	mouse_renderer = std::make_unique<MouseRenderer>(object_io);

	// Load the json file
	ifstream f(filename);
	if (!f.is_open()) {
		throw runtime_error("Could not open file " + filename);
	}
	json data = json::parse(f);

	possessor = std::make_unique<Possessor>(json::object({ // Dummy data that will never change on init
			{"victim", data["start_controllable"].get_ref<const string&>()}
		}), object_io);

	// By default, the handler will create at least one point_viewcontrol. You can create more and switch between them, but we need
	// at least one to be able to render the map.
	camera_controllers.push_back(std::make_unique<PointViewControl>(json::object({
		{"controller_num", 0},
		{"follow_target", data["start_controllable"].get_ref<const string&>()},
		{"mode", 2}
		}), object_io));

	for (auto entity_data : data["entities"]) {
		objects.push_back(move(object_type_selector(entity_data, object_io)));

		// TODO: point_viewcontrols should probably be specially acessible

		// Register the object in the io
		object_io.register_object(entity_data["targetname"].get_ref<const string&>(), objects.back().get());
	}

	vector<string> mesh_files_to_load;

	// Push back gen props file to files we should load as this should never be missing.
	// This may be renamed in the future to like util_meshes or something
	mesh_files_to_load.push_back("gen_props");

	// By default looks for meshes in the directory /gamedata/meshes/[filename].json
	// You can also specify a special search path in the map json file with "extra_mesh_paths"
	// but probably dont.
	vector<string> paths = { "gamedata/meshes/" };
	for (auto path : data["extra_mesh_paths"]) {
		paths.push_back(path);
	}

	// empty mesh
	meshes[""] = std::vector<float>();

	// Now load the meshes
	for (auto mesh_file : mesh_files_to_load) {
		// Load the mesh file
		for (auto path : paths) {
			ifstream f(path + mesh_file + ".json");
			// If we cant find the file with the last path then something went wrong
			if (!f.is_open() && path == paths.back()) {
				// You should really not have missing mesh files, that will break a lot of things.
				// Maybe you just mispelled a mesh name in the wrong place but i dont know that
				throw runtime_error("Could not find mesh file " + mesh_file + ".json");
			}
			else if (!f.is_open()) {
				continue; // Try the next path
			}

			// Loop over every mesh (they should be flat packed). Meshes follow format
			// "name": [1, 2, 3, 4] // where each two numbers are a vertex
			json mesh_data = json::parse(f);
			for (auto& mesh : mesh_data.items()) {
				meshes[mesh.key()] = mesh.value().get<std::vector<float>>();
			}
		}
	}

	// Give the ref for the meshes to the object io
	object_io.meshes = &meshes;
}

ObjectUpdateReturnData ObjectsHandler::update(ObjectUpdateData data) {
	mouse_renderer->update(data); // Update the mouse renderer
	possessor->update(data); // Update the possessor

	// Update all the objects
	for (auto& obj : objects) {
		obj->update(data);
	}

	camera_controllers[active_camera_controller]->update(data);
	ObjectUpdateReturnData ret_data;
	ret_data.camera_pos = camera_controllers[active_camera_controller]->position;

	return ret_data;
}

int ObjectsHandler::render(float* lines_list, uint32_t* colors) {
	// Render all the objects

	int counter = 0;

	// Render mouse cursor first
	int mouse_lines = mouse_renderer->render(lines_list, counter, colors, vec2(480.0f, 268.0f));
	mouse_lines /= 4;

	// Reserve the first 25 lines to be exclusivly used by the mouse. These 25
	// lines will not be stenciled like normal because we want to always see the
	// mouse cursor. The mouse cursor can render to any number of lines within
	// that number, if it uses <25 lines the rest are left empty.
	int reserved_lines = 25;

	// Set all lines from mouse_lines to reserved_lines to black in case on this frame
	// we went from a cursor with a lines to b lines where a > b. Dont need to rouch the 
	// actual vertex data, since it doesnt matter.
	for (int i = mouse_lines; i < reserved_lines; i++) {
		colors[i] = 0; // Black color
	}

	counter = reserved_lines * 4;

	vec2 camera_pos = camera_controllers[active_camera_controller]->position;

	for (std::unique_ptr<GameObject>& obj : objects) {
		counter = obj->render(lines_list, counter, colors, camera_pos);
	}

	return counter / 4; // Return the number of lines rendered
}