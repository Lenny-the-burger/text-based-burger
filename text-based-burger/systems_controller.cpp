#include "systems_controller.h"

using namespace std;

SystemsController::SystemsController(RenderTargets render_targets, string ui_entry) {
	// Create the default ui handler
	ui_handlers.push_back(make_unique<UIHandler>(ui_entry, 120, 34, *this));
	ui_io.push_back(ui_handlers[0]->get_io());

	// Normally we wouldnt create an objects handler, that would be handled by a script called from the
	// ui.
	// TODO: create actual behavior
	objects_handler = make_unique<ObjectsHandler>("gamedata\\maps\\testmap.json", *this);
	objects_io = objects_handler->get_io();

	// same thing with the map manager
	map_manager = make_unique<MapManager>("gamedata\\maps\\testmap_temp.json");
}