#pragma once

// The systems controller is the highest level class in game. Only one exists
// per application. It holds the ui, objects, and map handlers and it is how the
// systems interact with each other or get destroyed and created. 
// 
// For now only one of each can exist, if you want to multitask you have 120
// characters to do so, so most things should fit on one screen (eg 80 chars for
// a bbs terminal, 40 for everything else)
//
// For most game objects, scripts are not needed. An npc will always act like an
// npc and will always have the same n types and n states. Scripts are mostly
// used by ui components because a lot of the time you have the same 5
// components, but they do completly different things depending on the ui scene
// active. For example a button will always be a button and behave like a
// button, but you might want it to load a specific document in one context, or
// spawn a terminal and connect to a specific bbs in another scene, so scripts
// are used there. Another example is cross-system communication. One very basic
// example is loading a map from the ui. This requires a button to call a script
// that could query a text field, and then call the systems controller to load a
// map with that name, and since this requires actions outside the scope of the
// ui handler it should be done with a script.

// Include everything
#include "ui_handler.h"
#include "ui_systems.h"
#include "game_object_handler.h"
#include "object_utils.h"
#include "map_manager.h"

#include "scripts.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <map>
#include <set>

using json = nlohmann::json;

// Reserved ui handler spots
#define UI_HANDLER_ENTRY 0
#define UI_HANDLER_GAMEPLAY 1

enum ErrorLogType {
	ERROR_LOG_TYPE_ALL,
	ERROR_LOG_TYPE_NONE,
	ERROR_LOG_TYPE_UI,
	ERROR_LOG_TYPE_OBJECTS,
	ERROR_LOG_TYPE_CONTROLLER
};

// Pass this to the systems controller to set up where you want it to render stuff to
struct RenderTargets {
	uint32_t* char_grid;
	float* line_verts;
	uint32_t* line_colors;
};

// Misc data various systems emit for rendering
struct RenderData {
	// The first mouse_counter lines are rendered without stenciling as we dont
	// want to stencil the cursor. The other lines_counter - mouse_counter are 
	// stenciled like normal.
	int lines_counter;
	int mouse_counter;

	// Regions that are stenciled out (discarded) when rendering lines. Mouse cursor is
	// not stenciled ever. [x1, y1, x2, y2] coordinates in native space.
	std::vector<glm::vec4> stencil_regions;

	// glStencilFunc(GL_EQUAL, stencil_state, 0xFF);
	// By default 0, meaning we only render things within a perscribed render region.
	// Set it to 1 if you only want to render things outside the stencil regions.
	int stencil_state;
};

// Miscellaneous game data that various systems might need but dont have a place within
// a specific system. For example the player's current amount of discretionary budget,
// what day we are on, their name etc. This is mostly interacted with by scripts, and
// if you want to send game data to a script do args["send_game_data"] = true in the payload.
struct MiscGameData {

};

struct GlobalUpdateData {
	vec2 mouse_pos_native; // position in native
	vec2 mouse_pos_char;
};

class ControllerErrorReporter {
public:
	// Constructor
	ControllerErrorReporter();
	// Report an error
	void report_error(std::string error);
	// Get the error log
	std::vector<std::string> get_log();
	std::vector<int> get_repeats();

private:
	std::vector<std::string> error_log;
	std::vector<int> repeats;
};

class SystemsController {
public:
	// Constructor
	SystemsController(RenderTargets render_targets, std::string ui_entry);

	// Tell the systems controller to make things update
	void update(GLFWwindow* window, GlobalUpdateData global_update_data);

	// Tell systems to render to internal screens
	RenderData render();

	// Render a specific error log instead of what you would normally render.
	void show_error_log(ErrorLogType type);
	
	void call_script(std::string script_name, json args);

	// Unload the current map, if there is one (this also gets called when loading a new map)
	void unload_map();

	// Unload current map (if there is one) and load a new one
	void load_map(std::string map_name);

private:

	// Controller error reporter, similar to how the handlers have io classes to
	// report to, but this just reports errors from the controller itself.
	ControllerErrorReporter error_reporter;

	// Handles inputs that are not related to anything, global shortcuts.
	// This probably will not make it into production only really useful for 
	// debugging.
	void handle_misc_inputs(GLFWwindow* window);
	std::set<int> key_presses;

	// Function that actually renders the error log
	void render_log();
	ErrorLogType error_log_type = ERROR_LOG_TYPE_NONE;

	// The ui handler
	std::vector<std::unique_ptr<UIHandler>> ui_handlers;
	std::vector<UIComponentIO*> ui_io;
	int active_ui_handler = UI_HANDLER_ENTRY;

	// The objects handler
	std::unique_ptr<ObjectsHandler> objects_handler;
	ObjectIO* objects_io;

	// The map manager
	std::unique_ptr<MapManager> map_manager;

	// Random stuff we need to keep track of
	vec2 mouse_native_pos, mouse_char_pos = vec2();
	double last_time, frame_time = 0.0f;
	int CHAR_COLS = 120;
	int CHAR_ROWS = 68;

	// Internal render targets, these are actually held in main.cpp
	uint32_t* char_grid;
	float* line_verts;
	uint32_t* line_colors;

	int num_lines = 0;
};