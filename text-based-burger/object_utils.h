#pragma once

// For keyboard and mouse input
#include <GLFW/glfw3.h>

#include "json.hpp"
using json = nlohmann::json;

#include <unordered_dense.h>
#include "math_utils.h"

#include <vector>
#include <string>
#include <map>

// Forward declaration
class GameObject;
class SystemsController;

// Update data struct for the object system
struct ObjectUpdateData {
	float time;
	float frame_time;
	vec2 mouse_pos;
	bool is_clicking;

	// Pointer to actual window, do whatever you want with it
	GLFWwindow* window;

	vec2 camera_pos;
};

// Data that gets returned from the object handler once it has updated all
// objects to be used by other systems.
struct ObjectUpdateReturnData {
	vec2 camera_pos; // Camera position after update
};

// Objects all hold a pointer to an instance of this class and use it to call
// scripts, report errors etc. Basically the same as the UIComponentIO class but
// for game objects. Only reason it isnt reused is because of system specific
// types

class ObjectIO {
public:
	ObjectIO(SystemsController& controller);
	~ObjectIO();

	void report_error(std::string error);

	std::vector<std::string> get_log();
	std::vector<int> get_repeats();

	void register_object(std::string name, GameObject* object);

	// This should only be used by the script system
	GameObject* get_object(std::string name);

	// Points to the meshes data. The actual one is held by the handler
	json* meshes;

private:
	std::vector<std::string> error_log;
	std::vector<int> repeats;
	std::map<std::string, GameObject*> object_registry;

	SystemsController& controller;
};

// Split given path along "/" delimiter
std::vector<std::string> split_file_path(std::string path);