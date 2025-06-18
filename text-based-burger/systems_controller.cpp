#include "systems_controller.h"

using namespace std;

ControllerErrorReporter::ControllerErrorReporter() {
	// Reporters will always start with an empty log
	error_log = vector<string>();
	return;
}

void ControllerErrorReporter::report_error(string error) {
	// Check if the previous reported error is the same as the current one
	if (!error_log.empty() && error_log.back() == error) {
		// If it is, increment the repeat counter
		repeats.back()++;
		return;
	}
	// For now only report string errors
	error_log.push_back(error);
	repeats.push_back(0);
	return;
}

vector<string> ControllerErrorReporter::get_log() {
	return error_log;
}

vector<int> ControllerErrorReporter::get_repeats() {
	return repeats;
}

SystemsController::SystemsController(RenderTargets render_targets, string ui_entry) {
	error_reporter = ControllerErrorReporter();

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
			show_error_log(ERROR_LOG_TYPE_UI);
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
	
	if (error_log_type != ERROR_LOG_TYPE_NONE) {
		render_log();
		// dont render any lines
		RenderData return_data;
		return_data.lines_counter = 0;
		return_data.stencil_state = 0;

		return return_data;

	} else {
		vector<vector<uint32_t>> screen = ui_handlers[active_ui_handler]->get_screen();
		for (int i = 0; i < CHAR_ROWS / 2; i++) {
			for (int j = 0; j < CHAR_COLS; j++) {
				char_grid[i * CHAR_COLS + j] = screen[i][j];
			}
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

	switch (error_log_type) {
	case ERROR_LOG_TYPE_UI:
		all_errors = ui_handlers[active_ui_handler]->get_io()->get_log();
		all_repeats = ui_handlers[active_ui_handler]->get_io()->get_repeats();
		break;
	case ERROR_LOG_TYPE_OBJECTS:
		all_errors = objects_io->get_log();
		all_repeats = objects_io->get_repeats();
		break;
	case ERROR_LOG_TYPE_CONTROLLER:
		all_errors = error_reporter.get_log();
		all_repeats = error_reporter.get_repeats();
		break;
	default:
		// Uh oh you set an invalid error log type
		all_errors = {
			"Invalid error log type set no." + get_error_type_name(error_log_type),
			"good job you broke the error reporter idiot" };
		all_repeats = { 0,0 };
		break;
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
				screen[line][i] = char_packed;
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
		error_reporter.report_error("ERROR: " + to_string(args["caller"]) + " tried to call a non existant script " + script_name);
		return;
	}
	// This will behave strangely if an inactive ui component attempts to call
	// a script but that should never happen.
	script(args, ScriptHandles{ this, ui_io[active_ui_handler], objects_io });
}