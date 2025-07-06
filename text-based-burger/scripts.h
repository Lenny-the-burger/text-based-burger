#pragma once

#include "json.hpp"
using json = nlohmann::json;

#include <vector>
#include <string>
#include <unordered_map>

// Includes for script only interfaces. This includes basically everything that
// doesnt run on the main thread.
#include "map_utils.h"

// Various things that scripts might need
class UIComponent;
class UIComponentIO;
class UIHandler;

class GameObject;
class ObjectsHandler;
class ObjectIO;
class MapManager;

class SystemsController;

class LongThreadController;

struct ScriptHandles {
	SystemsController* controller;

	UIComponentIO* ui_io;
	ObjectIO* obj_io;
	MapManager* map_manager;

	LongThreadController* long_thread_controller;
};

// Scripts are statically registered C++ functions that act as generic, global subroutines.
// They can be triggered from anywhere in the engine — typically from UI elements — and are
// used to interface with other systems such as game state, audio, rendering, or I/O.
// 
// Scripts are stateless, freely callable, and compiled directly into the binary,
// meaning they do not require any form of registration at runtime.
//
// Use cases include:
// - Triggering game events from UI (e.g. launching an airstrike, spawning entities)
// - Executing complex or one-off logic without needing to wire through multiple layers
// - Interfacing between isolated systems (UI, world, engine)
//
// Scripts should be used with caution: they have full access to engine internals
// and can perform destructive or unsafe operations if misused.
//
// Function signature:
//     void my_script(json data);
//
// The 'data' parameter is an arbitrary JSON payload that the caller provides,
// allowing for flexible argument passing without requiring a fixed function interface.
//
// When calling scripts you usually call some call_script() function on your
// local io class, so you dont need to worry about assigning the handler
// yourself, this is done by the systems controller.
using Script = void(*)(const json data, ScriptHandles handles);

// Get the function pointer for the given script name
Script get_script(std::string name);

// If a script needs to call another script, or send off a lambda to a thread
void call_script(std::string script_name, json args, ScriptHandles handles);

void test_button_script(json data, ScriptHandles handles);
void hover_reporter(json data, ScriptHandles handles);

void mouse_pos_shower(json data, ScriptHandles handles);

void basic_mover(json data, ScriptHandles handles);

void map_loader(json data, ScriptHandles handles);
void map_unloader(json data, ScriptHandles handles);

void build_bvh(json data, ScriptHandles handles);
void bvh_build_done(json data, ScriptHandles handles);
void toggle_show_bvh(json data, ScriptHandles handles);

void npc_move(json data, ScriptHandles handles);