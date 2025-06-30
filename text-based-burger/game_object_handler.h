#pragma once

// Handles all the game objects

#include "object_utils.h"
#include "game_object.h"

#include <unordered_dense.h>

#include <string>
#include <vector>
#include <map>

#include "json.hpp"
using json = nlohmann::json;

class SystemsController;

class ObjectsHandler {

public:
	// Constructor
	ObjectsHandler(std::string filename, SystemsController& new_controller);

	// Update the objects
	ObjectUpdateReturnData update(ObjectUpdateData data);

	// Render the objects
	// !! this must be the first thing rendered to not screw up cursor rendering !!
	int render(float* lines_list, uint32_t* colors);

	// Cant just directly render the error log, so just return strings
	std::vector<std::string> get_error_log();

	// Get the object io
	ObjectIO* get_io() {
		return &object_io;
	}

private:

	// The common object io
	ObjectIO object_io;

	// Meta objects
	std::unique_ptr<MouseRenderer> mouse_renderer;
	std::unique_ptr<Possessor> possessor;

	std::vector<std::unique_ptr<PointViewControl>> camera_controllers;
	int active_camera_controller = 0; // Index of the active camera controller

	// Holds all the game objects. Note that this is just a flat array, and if
	// you want get objects by targetname you should use the io, as that holds
	// the object registry.
	std::vector<std::unique_ptr<GameObject>> objects;

	// Follows file.folder.mesh, like meshes[file][folder][mesh][1...n]
	json meshes;

	ankerl::unordered_dense::map<std::string, std::vector<int>> mesh_map;
};