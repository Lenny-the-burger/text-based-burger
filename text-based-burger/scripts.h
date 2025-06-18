#pragma once

#include "json.hpp"
using json = nlohmann::json;

#include <vector>
#include <string>
#include <unordered_map>

// Various things that scripts might need
class UIComponent;
class UIComponentIO;
class UIHandler;

class GameObject;
class ObjectsHandler;
class ObjectIO;

class SystemsController;

struct ScriptHandles {
	SystemsController* controller;
	UIComponentIO* ui_io;
	ObjectIO* obj_io;
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
using Script = void(*)(const json data, ScriptHandles handles);

// Get the function pointer for the given script name
Script get_script(std::string name);

void test_button_script(json data, ScriptHandles handles);
void hover_reporter(json data, ScriptHandles handles);

void mouse_pos_shower(json data, ScriptHandles handles);

void basic_mover(json data, ScriptHandles handles);