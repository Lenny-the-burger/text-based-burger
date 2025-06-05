#pragma once

// For keyboard and mouse input
#include <GLFW/glfw3.h>

#include "json.hpp"
using json = nlohmann::json;

#include <vector>
#include <string>
#include <map>

// Forward declaration
class GameObject;

// Update data struct for the object system
struct ObjectUpdateData {
	int time;
	int mouse_x;
	int mouse_y;
	bool is_clicking;

	// Pointer to actual window, do whatever you want with it
	GLFWwindow* window;

	double camera_x;
	double camera_y;

	float mapz;
	float map_z_fov;
};

// Objects all hold a pointer to an instance of this class and use it to call
// scripts, report errors etc. Basically the same as the UIComponentIO class but
// for game objects. Only reason it isnt reused is because of system specific
// types

class ObjectIO {
public:
	ObjectIO();
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
};

// Split given path along "/" delimiter
std::vector<std::string> split_file_path(std::string path);

// Helper function to print out a json data for the debugger or immediate window
