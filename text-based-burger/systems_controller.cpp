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

	// set render targets
	char_grid = render_targets.char_grid;
	line_verts = render_targets.line_verts;
	line_colors = render_targets.line_colors;
}

void SystemsController::update(GLFWwindow* window, GlobalUpdateData global_update_data) {

	// Set time
	frame_time = glfwGetTime() - last_time;
	last_time = glfwGetTime();

	UIUpdateData frame_data;
	frame_data.mouse_char_x = global_update_data.mouse_pos_char.x;
	frame_data.mouse_char_y = global_update_data.mouse_pos_char.y;
	frame_data.time = (int)glfwGetTime();
	frame_data.is_clicking = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

	// For now just update all the ui and plop the whole screen on the gpu, 
	// this will eventually happen on a sperate thread (probably)
	ui_handlers[active_ui_handler]->update(frame_data);

	// --- update objects ---
	ObjectUpdateData update_data;
	update_data.time = glfwGetTime();
	update_data.frame_time = frame_time;
	update_data.mouse_pos = global_update_data.mouse_pos_native;
	update_data.is_clicking = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

	// Should probably not reset this every frame, but its a pointer so whatever
	update_data.window = window;

	ObjectUpdateReturnData return_data;

	// Call update
	return_data = objects_handler->update(update_data);

	// Update camera position from ovjects handler
	update_data.camera_pos = return_data.camera_pos;

	// Render map first
	map_manager->update(update_data);
}

RenderData SystemsController::render() {
	// For now just blindly copy the screen to the char grid
	vector<vector<uint32_t>> screen = ui_handlers[active_ui_handler]->get_screen();
	for (int i = 0; i < CHAR_ROWS / 2; i++) {
		for (int j = 0; j < CHAR_COLS; j++) {
			char_grid[i * CHAR_COLS + j] = screen[i][j];
		}
	}

	num_lines = map_manager->render(line_verts, line_colors);

	// Render objects
	num_lines = objects_handler->render(line_verts, line_colors, num_lines);

	RenderData return_data;

	return_data.lines_counter = num_lines;
	return_data.stencil_state = 0;

	return return_data;
}