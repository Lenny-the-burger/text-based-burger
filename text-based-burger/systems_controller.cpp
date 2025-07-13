#include "systems_controller.h"

using namespace std;

SystemsController::SystemsController(RenderTargets render_targets, string ui_entry) {
	// You get an error reporter and you get an error reporter, everybody gets an error reporter!
	controller_error_reporter = ErrorReporter();
	script_error_reporter = ErrorReporter();
	threading_error_reporter = ErrorReporter();

	// Init threading stuff, this should probably be in the initializer list
	// instead of unique ptrs but whatever
	long_thread_controller = make_unique<LongThreadController>(threading_error_reporter);

	// reserve at least 2 spots in the ui handlers for the standard handler slots
	ui_handlers.resize(2);
	ui_io.resize(2);

	// Create the default ui handler
	ui_handlers[UI_HANDLER_ENTRY] = make_unique<UIHandler>("gamedata\\ui\\map_selector.json", 120, 34, *this);
	ui_io[UI_HANDLER_ENTRY] = ui_handlers[0]->get_io();

	// Load the none map by default
	objects_handler = make_unique<ObjectsHandler>("gamedata\\maps\\none_map.json", *this);
	objects_io = objects_handler->get_io();
	map_manager = make_unique<MapManager>("gamedata\\maps\\none_map_geo.json");

	// set render targets
	char_grid = render_targets.char_grid;
	line_verts = render_targets.line_verts;
	line_colors = render_targets.line_colors;

	// TEMP: for testing
	load_metamap("mesh_editor");
}

void SystemsController::handle_misc_inputs(GLFWwindow* window) {

	// f11 to toggle fullscreen
	if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
		// Is the key already pressed?
		if (key_presses.count(GLFW_KEY_F11) > 0) {
			return; // Key already pressed, do nothing
		}
		// Key pressed, add to set
		key_presses.insert(GLFW_KEY_F11);

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		if (glfwGetWindowMonitor(window) == NULL) {
			glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		}
		else {
			glfwSetWindowMonitor(window, NULL, 100, 100, 1300, 720, 0);
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_RELEASE) {
		// Key released, remove from set
		key_presses.erase(GLFW_KEY_F11);
	}

	if(glfwGetKey(window, GLFW_KEY_F6) == GLFW_PRESS) {
		// Is the key already pressed?
		if (key_presses.count(GLFW_KEY_F6) > 0) {
			return; // Key already pressed, do nothing
		}
		// Key pressed, add to set
		key_presses.insert(GLFW_KEY_F6);

		if (error_log_type == ERROR_LOG_TYPE_NONE) {
			// If no error log is shown, show the UI error log
			show_error_log(ERROR_LOG_TYPE_ALL);
		}
		else {
			// If an error log is already shown, hide it
			error_log_type = ERROR_LOG_TYPE_NONE;
		}
	}
	else if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_RELEASE) {
		// Key released, remove from set
		key_presses.erase(GLFW_KEY_F6);
	}
}

void SystemsController::update(GLFWwindow* window, GlobalUpdateData global_update_data) {

	// Set time
	frame_time = glfwGetTime() - last_time;
	last_time = glfwGetTime();

	handle_misc_inputs(window); // do this first, this shouldnt have any side effects.

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
	update_data.scroll_delta = global_update_data.scroll_delta;

	// Should probably not reset this every frame, but its a pointer so whatever
	update_data.window = window;

	ObjectUpdateReturnData return_data;

	// Call update
	return_data = objects_handler->update(update_data);

	// Update camera position from ovjects handler
	update_data.camera_pos = return_data.camera_pos;

	map_manager->update(update_data);

	long_thread_controller->update();
}

RenderData SystemsController::render() {

	// Render objects
	num_lines = objects_handler->render(line_verts, line_colors);
	num_lines = map_manager->render(line_verts, line_colors, num_lines);
	
	// Render the bvh if needed
	num_lines = map_manager->render_bvh(line_verts, line_colors, num_lines);

	//float renderscale = 1.0f;

	//// scale the non cursor line
	//for (int i = 100; i < num_lines * 4; i++) {
	//	line_verts[i] *= renderscale;
	//}

	RenderData return_data;

	return_data.lines_counter = num_lines;

	// Stencil regions should really only ever be uints as they are pixel coordinates
	return_data.stencil_regions = ui_handlers[active_ui_handler]->get_stencil_regions();
	return_data.stencil_state = ui_handlers[active_ui_handler]->get_stencil_state();

	ui_handlers[active_ui_handler]->rerender_all();

	if (error_log_type != ERROR_LOG_TYPE_NONE) {
		render_log();
		// stencil away the entire screen so we dont draw the map
		return_data.stencil_regions = { };
		return_data.stencil_state = 0; // stencil in

	}
	else {
		vector<vector<uint32_t>> screen = ui_handlers[active_ui_handler]->get_screen();
		for (int i = 0; i < CHAR_ROWS / 2; i++) {
			for (int j = 0; j < CHAR_COLS; j++) {
				char_grid[i * CHAR_COLS + j] = screen[i][j];
			}
		}
	}

	return return_data;
}

void SystemsController::show_error_log(ErrorLogType type) {
	error_log_type = type;
}

string get_error_type_name(ErrorLogType type) {
	switch (type) {
	case ERROR_LOG_TYPE_ALL: return "All";
	case ERROR_LOG_TYPE_NONE: return "None";
	case ERROR_LOG_TYPE_UI: return "UI";
	case ERROR_LOG_TYPE_OBJECTS: return "Objects";
	default: return to_string(type);
	}
}

void SystemsController::render_log() {
	vector<int> all_repeats;
	vector<string> all_errors;

	auto temp_log = ui_handlers[active_ui_handler]->get_io()->get_log();
	auto temp_repeats = ui_handlers[active_ui_handler]->get_io()->get_repeats();

	switch (error_log_type) {
	case ERROR_LOG_TYPE_ALL:
		all_errors = temp_log;
		all_repeats = temp_repeats;

		temp_log = objects_io->get_log();
		temp_repeats = objects_io->get_repeats();

		all_errors.insert(all_errors.end(), temp_log.begin(), temp_log.end());
		all_repeats.insert(all_repeats.end(), temp_repeats.begin(), temp_repeats.end());

		temp_log = controller_error_reporter.get_log();
		temp_repeats = controller_error_reporter.get_repeats();

		all_errors.insert(all_errors.end(), temp_log.begin(), temp_log.end());
		all_repeats.insert(all_repeats.end(), temp_repeats.begin(), temp_repeats.end());

		temp_log = script_error_reporter.get_log();
		temp_repeats = script_error_reporter.get_repeats();

		all_errors.insert(all_errors.end(), temp_log.begin(), temp_log.end());
		all_repeats.insert(all_repeats.end(), temp_repeats.begin(), temp_repeats.end());

		temp_log = threading_error_reporter.get_log();
		temp_repeats = threading_error_reporter.get_repeats();

		all_errors.insert(all_errors.end(), temp_log.begin(), temp_log.end());
		all_repeats.insert(all_repeats.end(), temp_repeats.begin(), temp_repeats.end());

		break;

	case ERROR_LOG_TYPE_UI:
		all_errors = temp_log;
		all_repeats = temp_repeats;
		break;
	case ERROR_LOG_TYPE_OBJECTS:
		all_errors = objects_io->get_log();
		all_repeats = objects_io->get_repeats();
		break;
	case ERROR_LOG_TYPE_CONTROLLER:
		all_errors = controller_error_reporter.get_log();
		all_repeats = controller_error_reporter.get_repeats();
		break;
	default:
		// Uh oh you set an invalid error log type
		all_errors = {
			"Invalid error log type set no." + get_error_type_name(error_log_type),
			"good job you broke the error reporter idiot" };
		all_repeats = { 0,0 };
		break;
	}

	// Check if we overflow the screen with errors, in that case only keep the last CHAR_ROWS errors
	if (all_errors.size() > CHAR_ROWS / 2) {
		all_errors.erase(all_errors.begin(), all_errors.end() - (CHAR_ROWS / 2));
		all_repeats.erase(all_repeats.begin(), all_repeats.end() - (CHAR_ROWS / 2));
	}

	// Render to temporary screen
	vector<vector<uint32_t>> screen = vector<vector<uint32_t>>(CHAR_ROWS / 2, vector<uint32_t>(CHAR_COLS, 0));

	int line = 0;
	for (string error : all_errors) {
		// Append repeat count if greater than 0
		int repeats = all_repeats[line];
		if (repeats > 0) {
			error += " x" + to_string(repeats);
		}

		// Render the error, split across multiple lines if needed
		size_t max_width = screen[0].size();
		size_t pos = 0;

		while (pos < error.size()) {
			string line_error = error.substr(pos, max_width);

			for (int i = 0; i < line_error.size(); i++) {
				int char_num = char2int(line_error[i]);
				uint32_t char_packed = gen_frag(char_num, 0, 255);
				try {
					screen[line][i] = char_packed;
				}
				catch (const out_of_range& e) {
					// If we try to write out of bounds, just skip this character
					controller_error_reporter.report_error("ERROR: Ran out of lines for errors (what did you do???)");
					continue;
				}
			}

			pos += max_width;
			line++;
		}
	}

	// If no errors print no errors found
	if (all_errors.empty()) {
		string no_errors = "No errors reported for type " + get_error_type_name(error_log_type);
		for (int i = 0; i < no_errors.size(); i++) {
			int char_num = char2int(no_errors[i]);
			uint32_t char_packed = gen_frag(char_num, 0, 255);
			screen[line][i] = char_packed;
		}
		line++;
	}

	// Render out to the char grid
	for (int i = 0; i < CHAR_ROWS / 2; i++) {
		for (int j = 0; j < CHAR_COLS; j++) {
			char_grid[i * CHAR_COLS + j] = screen[i][j];
		}
	}
}

void SystemsController::call_script(string script_name, json args) {
	Script script = get_script(script_name);
	if (script == nullptr) {
		controller_error_reporter.report_error("ERROR: " + to_string(args["caller"]) + " tried to call a non existant script " + script_name);
		return;
	}
	// This will behave strangely if an inactive ui component attempts to call
	// a script but that should not be possible.
	int temp = sizeof(args);
	script(args, ScriptHandles{ 
		this, 
		ui_io[active_ui_handler], 
		objects_io, 
		map_manager.get(),
		long_thread_controller.get()
	});
}

void SystemsController::unload_map() {
	// Not actually unloads the map, just loads the none map
	objects_handler = make_unique<ObjectsHandler>("gamedata\\maps\\none_map.json", *this);
	objects_io = objects_handler->get_io();

	map_manager = make_unique<MapManager>("gamedata\\maps\\none_map_geo.json");

	active_ui_handler = UI_HANDLER_ENTRY;
}

void SystemsController::load_map(string map_name) {
	// Unload current map
	unload_map();
	// Load new map
	objects_handler = make_unique<ObjectsHandler>("gamedata\\maps\\" + map_name + ".json", *this);
	objects_io = objects_handler->get_io();
	map_manager = make_unique<MapManager>("gamedata\\maps\\" + map_name + "_geo.json");

	// Load gameplay ui
	ui_handlers[UI_HANDLER_GAMEPLAY] = (make_unique<UIHandler>("gamedata\\ui\\gameplay_ui.json", 120, 34, *this));
	ui_io[UI_HANDLER_GAMEPLAY] = (ui_handlers[UI_HANDLER_GAMEPLAY]->get_io());

	active_ui_handler = UI_HANDLER_GAMEPLAY;
}

void SystemsController::load_metamap(string map_name) {
	// Unload current map
	unload_map();
	// Load new map
	objects_handler = make_unique<ObjectsHandler>("gamedata\\maps\\" + map_name + ".json", *this);
	objects_io = objects_handler->get_io();

	// no map geometry
	map_manager = make_unique<MapManager>("gamedata\\maps\\none_map_geo.json");

	// metamaps should have custom ui aswell
	ui_handlers[UI_HANDLER_GAMEPLAY] = (make_unique<UIHandler>("gamedata\\ui\\" + map_name + "_ui.json", 120, 34, *this));
	ui_io[UI_HANDLER_GAMEPLAY] = (ui_handlers[UI_HANDLER_GAMEPLAY]->get_io());
	active_ui_handler = UI_HANDLER_GAMEPLAY;
}

void SystemsController::clean_up_threads() {
	long_thread_controller->clean_up_threads();
}