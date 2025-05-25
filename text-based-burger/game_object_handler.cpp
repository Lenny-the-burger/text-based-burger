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
		// Mesh names are file.folder.mesh with any number of folders possible
		// so we only need the first one. Using "." as delimiter because why would 
		// you make your file delimiter a control character thats stupid.
		string filename = split_file_path(entity_data["mesh"].get_ref<const string&>())[0];

		if (find(mesh_files_to_load.begin(), mesh_files_to_load.end(), filename) == mesh_files_to_load.end()) {
			mesh_files_to_load.push_back(filename);
		}
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
				// You should not have missing mesh files, if an object cant find its specific
				// mesh it just gets reported as an error, but you should really not be mispelling
				// file names.
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