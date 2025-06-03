#include "game_object_handler.h"

#include <fstream> // Include this header to use ifstream

using namespace std;


ObjectsHandler::ObjectsHandler(string filename) : object_io() {
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

	// As we build objects add the files that meshes they need are stored in
	// and then at the end read them all in at once.
	vector<string> mesh_files_to_load;

	// Create the mouse renderer object
	mouse_renderer = std::make_unique<MouseRenderer>(object_io);

	// Push back gen props file to files we should load as this should never be missing.
	// This may be renamed in the future to like util_meshes or something
	mesh_files_to_load.push_back("gen_props");

	// Load the json file
	ifstream f(filename);
	if (!f.is_open()) {
		throw runtime_error("Could not open file " + filename);
	}
	json data = json::parse(f);

	for (auto entity_data : data["entities"]) {
		objects.push_back(move(type_selector(entity_data, object_io)));

		// Check the object mesh and if we dont have the filename already
		// add it to the list. 
		// Mesh names are file/folder/mesh with any number of folders possible
		// "/" as a delimiter was not chosen by me it is what the fuckass json pointers use
		string filename = split_file_path(entity_data["mesh"].get_ref<const string&>())[0];

		if (find(mesh_files_to_load.begin(), mesh_files_to_load.end(), filename) == mesh_files_to_load.end()) {
			mesh_files_to_load.push_back(filename);
		}

		// Register the object in the io
		object_io.register_object(entity_data["targetname"].get_ref<const string&>(), objects.back().get());
	}

	// By default looks for meshes in the directory /gamedata/meshes/[filename].json
	// You can also specify a special search path in the map json file with "extra_mesh_paths"
	// but probably dont.
	vector<string> paths = { "gamedata/meshes/" };
	for (auto path : data["extra_mesh_paths"]) {
		paths.push_back(path);
	}

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
			json mesh_data = json::parse(f);
			meshes[mesh_file] = mesh_data;
		}
	}

	// Give the ref for the meshes to the object io
	object_io.meshes = &meshes;

	GameObject* test = objects[0].get();
}

void ObjectsHandler::update(ObjectUpdateData data) {
	mouse_renderer->update(data); // Update the mouse renderer

	// Update all the objects
	for (auto& obj : objects) {
		if (obj->update(data)) {
			// If the object needs to be rerendered, we can handle that later
			// For now we just update the object
		}
	}
}

int ObjectsHandler::render(float* lines_list, uint32_t* colors) {
	// Render all the objects

	// counter to where we should write in the lines list
	int counter = 0;

	// Render mouse cursor first
	counter = mouse_renderer->render(lines_list, counter, colors);

	int cols_counter = 0;
	for (std::unique_ptr<GameObject>& obj : objects) {
		counter = obj->render(lines_list, counter, colors);
	}

	return counter / 4; // Return the number of lines rendered
}