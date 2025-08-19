#include "game_object.h"
#include "scripts.h"
#include <iostream>

#include "line_color_gen.hpp"

using namespace std;

// GAME OBJECT

GameObject::GameObject(json data, ObjectIO& io) : io(io) {
	targetname = data["targetname"].get_ref<const string&>();

	update_script_name = data["update_script"].get_ref<const string&>();

	// For non rendering objects you can probably not even write these, if you
	// know you will never need to render it is probably fine if they are null.
	mesh = data["mesh"].get_ref<const string&>();

	position = vec2(data["position"]);

	//rotation = data["rotation"];
	render_scale = vec2(data["scale"][0].get<int>(), data["scale"][1]);
	color = data["color"];

	// None by default
	collision_type = COLLISION_TYPE_NONE;

	return;
}

void GameObject::update(ObjectUpdateData data) {
	// This is the update function.

	return;
}

int GameObject::render(float* lines_list, int offset, uint32_t* colors, vec2 camera) {
	// Render function is called every frame. You are given a pointer to an array
	// and should append yourself to it if you need to be rendered. Not appending
	// yourself will cause you to not be rendered, even if you were rendered the
	// previous frame. The entire array is cleared. For optimization you should only
	// render if you are within some distance of the camera position you get from
	// updata data. Note that you should output NDC here not world space.

	// Hard coded screen size whatever
	float scrn_width = 960.0f;
	float scrn_height = 536.0f;

	// TODO: add distance to camera check if we need to render at all

	vec2 screen_position = position - camera; // Offset the position by the camera position
	// Centre the position on the screen
	screen_position.x += scrn_width / 2.0f;
	screen_position.y += scrn_height / 2.0f;

	int vert_count = io.meshes->at(mesh).size();

	// Loop over the mesh array of coordinates and transform and copy to lines list until done
	for (int i = 0; i < vert_count; i++) {

		// This should probably be a vec2 instead of one float at a time, but whatever
		float number = (*io.meshes)[mesh][i];
		
		// It goes x, y, x, y so if offset % 2 == 0 then we are at x
		if (offset % 2 == 0) {
			// Transform the x coordinate
			number = (number * render_scale.x) + screen_position.x;
			number = number / scrn_width; // Normalize to screen width
		}
		else {
			// Transform the y coordinate
			number = (number * render_scale.y) + screen_position.y;
			number = number / scrn_height; // Normalize to screen height
		}
		// Normalize to full screen ndc -1 to 1
		number = (number * 2.0f) - 1.0f; // Convert to NDC space

		lines_list[offset] = number;

		// Every 4th number is a complete line so add new color
		if (offset % 4 == 0) {
			colors[offset / 4] = color; // Set the color for the line
		}

		offset++;
	}

	return offset; // Return the new offset
}

void GameObject::move(vec2 velocity) {
	// Generic game objects always move
	position += velocity;
	return;
}

using ObjectFactory = std::function<std::unique_ptr<GameObject>(json, ObjectIO&)>;

std::unique_ptr<GameObject> object_type_selector(json data, ObjectIO& io) {
	static const std::map<std::string, ObjectFactory> factory_map = {
		{"generic", [](json data, ObjectIO& io) { 
			return std::make_unique<GameObject>(data, io); 
		}},
		{"point_view_control", [](json data, ObjectIO& io) { 
			return std::make_unique<PointViewControl>(data, io); 
		}},
		{"npc", [](json data, ObjectIO& io) {
			return std::make_unique<NPC>(data, io);
		}},
		{"line_canvas", [](json data, ObjectIO& io) {
			return std::make_unique<LineCanvas>(data, io);
		}},

		// Dont add types that shouldnt be spawned by the user like mouse renderer
	};

	auto it = factory_map.find(data["type"].get_ref<const string&>());
	if (it != factory_map.end()) {
		return it->second(data, io);
	}
	else {
		throw std::runtime_error("Unknown game object type: " + data["type"].get_ref<const string&>());
	}
}

// Mouse renderer

MouseRenderer::MouseRenderer(ObjectIO& io) : GameObject(json::object(
	{  // Dummy data that will never change on init
		{"targetname", "mouse_renderer"},
		{"position", {0, 0}},
		{"scale", {1, 1}},
		{"mesh", "gen_props/pointers/point"},
		{"color", generate_line_color(LINE_COLOR_PRESET_CURSOR)},
		{"update_script", "none"}

	}), io) {

	collision_type = COLLISION_TYPE_NONE;

	mouse_state = MOUSE_NORMAL;
	return;
}

void MouseRenderer::update(ObjectUpdateData data) {
	position = vec2(data.mouse_pos.x, 536.0f - data.mouse_pos.y);

	if (data.is_clicking) {
		mouse_state = MOUSE_CLICKING;
	}
	else {
		mouse_state = MOUSE_NORMAL;
	}

	// Mouse state switcher. Modify mouse state to change the mesh.
	switch (mouse_state) {
		case MOUSE_NORMAL:
			mesh = "pointer_aim";
			break;
	
		case MOUSE_CLICKING:
			mesh = "pointer_click";
			break;
	}

	return;
}

// Point view control
PointViewControl::PointViewControl(json data, ObjectIO& io) : GameObject(json::object(
	{  // Dummy data that will never change on init
		{"targetname", "view_controller_" + data["controller_num"]},
		{"position", {0, 0}},
		{"scale", {1, 1}},
		{"mesh", ""},
		{"color", 0},
		{"update_script", "none"}

	}), io) {
	follow_name = data["follow_target"].get_ref<const string&>();
	switch (data["mode"].get<int>()) {
	case 0:
		mode = CAMERA_MODE_WELD;
		break;
	case 1:
		mode = CAMERA_MODE_FOLLOW;
		break;
	case 2:
		mode = CAMERA_MODE_GAMEPLAY;
		break;
	default:
		// will be using this one most of the time, you dont have to specify it
		mode = CAMERA_MODE_GAMEPLAY;
		break;
	}

	// initialize position to dummy value because the thing we are tracking may
	// not exist yet. You probably wont ever get to this coordinate naturally,
	// but if you do it will just snap the camera for a frame
	prev_position = vec2(0xB00B1E5);

	rotation = 0.0f;
	return;
}

static vec2 smooth_follow(vec2 current_pos, vec2 target_pos, vec2 aimpos, float dtime, bool extended_look) {
	// Follow the given target position from the current position with some fake inertia.
	vec2 output = vec2();
	vec2 aimdir = aimpos - vec2(480, 268); // screen space
	aimdir *= vec2(1.0f, -1.0f);

	// Offset the target in the aim direction, unnormalized. This means that if aimdir is 00
	// then it will not offset.
	float max_offset = extended_look ? 150.0f : 40.0f;
	if (aimdir.mag() > max_offset) {
		aimdir = aimdir.unit() * max_offset;
	}

	target_pos += aimdir;


	float distance = mag(target_pos - current_pos);
	if (distance < 0.01f) {
		return target_pos;
	}

	vec2 direction = target_pos - current_pos;
	direction = direction.unit();

	// How far should the camera lag back at most
	float max_lag = 30.0f;
	distance = min(distance, max_lag);
	distance /= max_lag;

	// Equivalent to move speed
	float stepsize = 4.5f * 100.0f;
	float step = stepsize * dtime;
	// The smooth function
	step *= max(0.0f, min(1.0f, 
		distance * distance * ( 3 - 2 * distance)
		));

	output = current_pos + (direction * step);
	return output;
}

void PointViewControl::update(ObjectUpdateData data) {
	// If no follow name we probably arent active. Hopefully you didnt forget to
	// set a followname and spent an hour chasing a bug to find this comment and
	// realize your mistake.
	if (follow_name.empty()) {
		return;
	}

	// Get the object we are following
	auto obj = io.get_object(follow_name);
	if (!obj) {
		io.report_error("PointViewControl: Object to follow '" + follow_name + "' does not exist.");
		return;
	}

	// Get the position of the object we are following
	vec2 new_pos = obj->position;

	if (new_pos == prev_position) {
		// If the position has not changed, we dont need to update anything
		return;
	}

	if (prev_position == vec2(0xB00B1E5)) {
		// If this is the first update just snap the camera
		prev_position = new_pos;
		position = new_pos;
		return;
	}
	
	bool extended_look = false;
	if (glfwGetKey(data.window, GLFW_KEY_B) == GLFW_PRESS) { // bzoom
		extended_look = true;
	}

	// Now we actually update the camera based on the mode
	switch (mode) {
	case CAMERA_MODE_WELD:
		// Snap follow
		prev_position = position;
		position = new_pos;
		break;

	case CAMERA_MODE_FOLLOW:
		// Follow target but smoothly
		prev_position = position;
		position = smooth_follow(position, new_pos, new_pos, data.frame_time, extended_look);
		break;

	case CAMERA_MODE_GAMEPLAY:
		// Same as follow cam but we actually supply aim position
		prev_position = position;
		position = smooth_follow(position, new_pos, data.mouse_pos, data.frame_time, extended_look);
		break;
	default:
		io.report_error("PointViewControl: Unknown camera mode.");
		return;
	}

	return;
}

void PointViewControl::set_mode(CameraMode new_mode) {
	// Set the camera mode
	mode = new_mode;
	return;
}

void PointViewControl::set_target(std::string targetname) {
	// Set the target to follow
	follow_name = targetname;
	return;
}

// Possesor
Possessor::Possessor(json data, ObjectIO& io) : GameObject(json::object(
	{  // Dummy data that will never change on init
		{"targetname", "possesor"},
		{"position", {0, 0}},
		{"scale", {1, 1}},
		{"mesh", ""},
		{"color", 0},
		{"update_script", "none"}

	}), io) {
	// Since this is a meta entity, it should only ever be created by the objects habndler
	// similarly to MouseRenderer
	victim_name = data["victim"].get_ref<const string&>();
	return;
}

void Possessor::update(ObjectUpdateData data) {
	// If we have no victim, we are not possessing anything
	if (victim_name.empty()) {
		return;
	}
	// Get the object we are possessing
	auto obj = io.get_object(victim_name);
	if (!obj) {
		io.report_error("Possessor cannot posses '" + victim_name + "' has invisibility cloak");
		return;
	}
	
	vec2 move = vec2();

	// Handle camera movement
	if (glfwGetKey(data.window, GLFW_KEY_W) == GLFW_PRESS) {
		move += vec2(0.0f, 1.0f);
	}
	if (glfwGetKey(data.window, GLFW_KEY_S) == GLFW_PRESS) {
		move += vec2(0.0f, -1.0f);
	}
	if (glfwGetKey(data.window, GLFW_KEY_A) == GLFW_PRESS) {
		move += vec2(-1.0f, 0.0f);
	}
	if (glfwGetKey(data.window, GLFW_KEY_D) == GLFW_PRESS) {
		move += vec2(1.0f, 0.0f);
	}

	move = move.unit();

	float move_speed = 4.0f * 100.0f;

	// are we crouching
	if (glfwGetKey(data.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		move_speed *= 0.4f; // Crouch speed
	}

	move *= move_speed * data.frame_time;

	// Attempt to move
	obj->move(move);

	return;
}

void Possessor::set_target(std::string targetname) {
	// Set the target to possess
	victim_name = targetname;
	return;
}

NPC::NPC(json data, ObjectIO& io) : GameObject(data, io) {

	// TEMP
	color = generate_line_color(LINE_COLOR_PRESET_NPC_FRIENDLY);

	return;
}

void NPC::update(ObjectUpdateData data) {

	// Try to move
	if (!move_velocity.is_zero()) {
		io.call_script("npc_move", {
			{"caller", targetname + " update"},
			{"targetname", targetname},
			{"x", move_velocity.x},
			{"y", move_velocity.y}
			});
		move_velocity.blank();
	}

	return;
}

void NPC::move(vec2 velocity) {
	// All we do here is actually just set the velocity, the actual moving is
	// handled in the update function, its safe to assume that npcs will be
	// moving on most update ticks.
	move_velocity = velocity;

	return;
}

void NPC::aim(vec2 direction) {

	return;
}

void NPC::attack() {

	return;
}

LineCanvas::LineCanvas(json data, ObjectIO& io) : GameObject(json::object(
	{  // Dummy data that will never change on init
		{"targetname", "line_canvas"},
		{"position", {0, 0}},
		{"scale", {1, 1}},
		{"mesh", ""},
		{"color", generate_line_color(LINE_COLOR_PRESET_EDITOR_LINE)},
		{"update_script", "none"}
	}), io) {
	collision_type = COLLISION_TYPE_NONE;
	active_color = generate_line_color(LINE_COLOR_PRESET_EDITOR_LINE);
	return;
}

void LineCanvas::update(ObjectUpdateData data) {
	data.mouse_pos.y = 536.0f - data.mouse_pos.y;
	
	// Handle delete key for selected lines (works in both select and edit modes)
	if ((tool == CANVAS_TOOL_SELECT || tool == CANVAS_TOOL_EDIT) && selected_line >= 0) {
		if (glfwGetKey(data.window, GLFW_KEY_DELETE) == GLFW_PRESS) {
			// Delete the selected line
			int start_index = selected_line * 2;
			if (start_index + 1 < canvas_lines.size() && selected_line < canvas_colors.size()) {
				// Remove the two vertices for this line
				canvas_lines.erase(canvas_lines.begin() + start_index, canvas_lines.begin() + start_index + 2);
				// Remove the color for this line
				canvas_colors.erase(canvas_colors.begin() + selected_line);
				// Clear selection since the line is deleted
				selected_line = -1;
				selected_vertex = -1;
				is_dragging = false;
			}
		}
	}

	// for now draw on the entire screen
	bool within_bbox = true;

	// Click tracking logic
	if (data.is_clicking) {
		if (!is_clicking && within_bbox) {
			is_clicking = true;
			is_click_start_inside = true;

			have_already_fired = false;

			on_press(data);
		}
		if (are_drawing && tool == CANVAS_TOOL_DRAW_LINE) {
			// if we are mid draw update the last element in the canvas_lines vector (with snapping)
			vec2 snapped_end_pos = find_snap_position(data.mouse_pos);
			canvas_lines[canvas_lines.size() - 1] = snapped_end_pos;
		}
		else if (is_dragging && tool == CANVAS_TOOL_EDIT && selected_line >= 0 && selected_vertex >= 0) {
			// Update vertex position while dragging (with snapping, excluding the current line)
			int vertex_index = selected_line * 2 + selected_vertex;
			if (vertex_index < canvas_lines.size()) {
				vec2 snapped_pos;
				if (snapping_enabled) {
					// Find nearest vertex excluding the current line being edited
					vec2 nearest_vertex = find_nearest_vertex(data.mouse_pos, selected_line);
					if (mag(data.mouse_pos - nearest_vertex) <= snap_radius) {
						snapped_pos = nearest_vertex;
					} else {
						snapped_pos = data.mouse_pos;
					}
				} else {
					snapped_pos = data.mouse_pos;
				}
				canvas_lines[vertex_index] = snapped_pos;
			}
		}
	}
	else {
		if (is_clicking) {
			// Only count as a full click if press and release both happen inside
			if (is_click_start_inside && within_bbox) {
				on_click(data);
			}
			else {
				on_release(data);
			}
			is_clicking = false;
			is_click_start_inside = false;
			
			// Stop dragging when mouse is released
			if (is_dragging) {
				is_dragging = false;
			}
		}
	}
	
	return;
}

void LineCanvas::on_press(ObjectUpdateData data) {
	vec2 snapped_start_pos;

	switch (tool) {
	case CANVAS_TOOL_DRAW_LINE: {
		// Start drawing
		are_drawing = true;

		// add the initial points to the canvas (with snapping)
		snapped_start_pos = find_snap_position(data.mouse_pos);
		canvas_lines.push_back(snapped_start_pos);
		canvas_lines.push_back(snapped_start_pos);

		canvas_colors.push_back(active_color);

		active_line = canvas_colors.size() - 1;

		return;
	}
	case CANVAS_TOOL_SELECT:
		// Make sure we're not in drawing mode
		are_drawing = false;
		active_line = -1;
		
		// Find line at click position
		selected_line = find_line_at_position(data.mouse_pos);
		selected_vertex = -1;
		return;
		
	case CANVAS_TOOL_EDIT: {
		// Make sure we're not in drawing mode
		are_drawing = false;
		active_line = -1;
		
		// First check if we're clicking near a vertex of the selected line
		if (selected_line >= 0) {
			selected_vertex = find_vertex_at_position(data.mouse_pos, selected_line, 15.0f);
			if (selected_vertex >= 0) {
				// Store original vertex position for potential revert
				int vertex_index = selected_line * 2 + selected_vertex;
				if (vertex_index < canvas_lines.size()) {
					original_vertex_pos = canvas_lines[vertex_index];
				}
				// Start dragging the vertex
				is_dragging = true;
				drag_start_pos = data.mouse_pos;
				return;
			}
		}
		
		// If not dragging a vertex, try to select a new line
		selected_line = find_line_at_position(data.mouse_pos);
		selected_vertex = -1;
		return;
	}
	}
}

void LineCanvas::on_click(ObjectUpdateData data) {
	switch (tool) {
	case CANVAS_TOOL_DRAW_LINE:
		// We are done drawing
		are_drawing = false;
		active_line = -1;
		return;
		
	case CANVAS_TOOL_SELECT:
		// Selection is handled in on_press, nothing to do here
		return;
		
	case CANVAS_TOOL_EDIT:
		// If we were dragging, we're now done
		if (is_dragging) {
			is_dragging = false;
		}
		return;
	}
}

void LineCanvas::on_release(ObjectUpdateData data) {
	switch (tool) {
	case CANVAS_TOOL_DRAW_LINE:
		// We are done drawing but we have cancelled the draw so remove the last two points
		if (canvas_lines.size() >= 2) {
			canvas_lines.pop_back(); // Remove the last point
			canvas_lines.pop_back(); // Remove the second last point
			canvas_colors.pop_back(); // Remove the last color
			active_line = -1; // Reset the active line
		}
		return;
		
	case CANVAS_TOOL_SELECT:
	case CANVAS_TOOL_EDIT:
		// Clear any dragging state when releasing outside the canvas bounds
		is_dragging = false;
		return;
	}
}

int LineCanvas::render(float* lines_list, int offset, uint32_t* colors, vec2 camera) {
	// Render function is called every frame. You are given a pointer to an array
	// and should append yourself to it if you need to be rendered. Not appending
	// yourself will cause you to not be rendered, even if you were rendered the
	// previous frame. The entire array is cleared. For optimization you should only
	// render if you are within some distance of the camera position you get from
	// updata data. Note that you should output NDC here not world space.

	// Hard coded screen size whatever;
	vec2 screen_size = vec2(960, 536);

	vec2 screen_position = position - camera; // Offset the position by the camera position
	// Centre the position on the screen
	screen_position += screen_size / 2.0f;

	int vert_count = canvas_lines.size();

	// Loop over the mesh array of coordinates and transform and copy to lines list until done
	for (int i = 0; i < vert_count; i++) {

		vec2 vertex = canvas_lines[i];

		vertex = vertex * render_scale;
		vertex = vertex / screen_size;

		// Normalize to full screen ndc -1 to 1
		vertex = (vertex * 2.0f) - 1.0f; // Convert to NDC space

		lines_list[offset] = vertex.x;
		offset++;
		lines_list[offset] = vertex.y;
		offset++;

		// Every 2nd nmuber is a complete line so add new color
		if (i % 2 == 0) {
			int line_index = i / 2;
			uint32_t line_color = canvas_colors[line_index];
			
			// Highlight selected line with preset color
			if (line_index == selected_line) {
				line_color = generate_line_color(LINE_COLOR_PRESET_EDITOR_SELECTED);
			}
			
			colors[offset / 4] = line_color;
		}
	}
	
	// Add vertex markers in edit mode for selected line
	if (tool == CANVAS_TOOL_EDIT && selected_line >= 0 && selected_line * 2 + 1 < canvas_lines.size()) {
		vec2 start_vertex = canvas_lines[selected_line * 2];
		vec2 end_vertex = canvas_lines[selected_line * 2 + 1];
		
		float marker_size = 10.0f;
		uint32_t marker_color = generate_line_color(LINE_COLOR_PRESET_CURSOR);
		
		// Transform vertices to NDC
		auto transform_vertex = [&](vec2 v) -> vec2 {
			v = v * render_scale;
			v = v / screen_size;
			return (v * 2.0f) - 1.0f;
		};
		
		vec2 start_ndc = transform_vertex(start_vertex);
		vec2 end_ndc = transform_vertex(end_vertex);
		
		// Add cross markers for vertices

		// Start vertex cross
		lines_list[offset++] = start_ndc.x - marker_size / screen_size.x * 2.0f;
		lines_list[offset++] = start_ndc.y;
		lines_list[offset++] = start_ndc.x + marker_size / screen_size.x * 2.0f;
		lines_list[offset++] = start_ndc.y;
		colors[offset / 4 - 1] = marker_color;
			
		lines_list[offset++] = start_ndc.x;
		lines_list[offset++] = start_ndc.y - marker_size / screen_size.y * 2.0f;
		lines_list[offset++] = start_ndc.x;
		lines_list[offset++] = start_ndc.y + marker_size / screen_size.y * 2.0f;
		colors[offset / 4 - 1] = marker_color;
			
		// End vertex cross
		lines_list[offset++] = end_ndc.x - marker_size / screen_size.x * 2.0f;
		lines_list[offset++] = end_ndc.y;
		lines_list[offset++] = end_ndc.x + marker_size / screen_size.x * 2.0f;
		lines_list[offset++] = end_ndc.y;
		colors[offset / 4 - 1] = marker_color;
			
		lines_list[offset++] = end_ndc.x;
		lines_list[offset++] = end_ndc.y - marker_size / screen_size.y * 2.0f;
		lines_list[offset++] = end_ndc.x;
		lines_list[offset++] = end_ndc.y + marker_size / screen_size.y * 2.0f;
		colors[offset / 4 - 1] = marker_color;
	}

	return offset; // Return the new offset
}

int LineCanvas::find_line_at_position(vec2 pos, float tolerance) {
	for (int i = 0; i < canvas_lines.size(); i += 2) {
		if (i + 1 >= canvas_lines.size()) break;
		
		vec2 line_start = canvas_lines[i];
		vec2 line_end = canvas_lines[i + 1];
		
		float distance = distance_point_to_line(pos, line_start, line_end);
		if (distance <= tolerance) {
			return i / 2; // Return line index
		}
	}
	return -1;
}

int LineCanvas::find_vertex_at_position(vec2 pos, int line_index, float tolerance) {
	if (line_index < 0 || line_index * 2 + 1 >= canvas_lines.size()) {
		return -1;
	}
	
	vec2 start_vertex = canvas_lines[line_index * 2];
	vec2 end_vertex = canvas_lines[line_index * 2 + 1];
	
	float dist_to_start = mag(pos - start_vertex);
	float dist_to_end = mag(pos - end_vertex);
	
	if (dist_to_start <= tolerance && dist_to_start <= dist_to_end) {
		return 0; // Start vertex
	}
	else if (dist_to_end <= tolerance) {
		return 1; // End vertex
	}
	
	return -1;
}

float LineCanvas::distance_point_to_line(vec2 point, vec2 line_start, vec2 line_end) {
	vec2 line_vec = line_end - line_start;
	vec2 point_vec = point - line_start;
	
	float line_len_squared = line_vec.x * line_vec.x + line_vec.y * line_vec.y;
	if (line_len_squared == 0.0f) {
		return mag(point - line_start);
	}
	
	float t = std::max(0.0f, std::min(1.0f, (point_vec.x * line_vec.x + point_vec.y * line_vec.y) / line_len_squared));
	vec2 projection = line_start + line_vec * t;
	
	return mag(point - projection);
}

vec2 LineCanvas::find_snap_position(vec2 pos) {
	if (!snapping_enabled) {
		return pos;
	}
	
	vec2 nearest_vertex = find_nearest_vertex(pos);
	if (mag(pos - nearest_vertex) <= snap_radius) {
		return nearest_vertex;
	}
	
	return pos;
}

vec2 LineCanvas::find_nearest_vertex(vec2 pos, int exclude_line) {
	vec2 nearest_vertex = pos;
	float min_distance = snap_radius + 1.0f; // Start with distance larger than snap radius
	
	for (int i = 0; i < canvas_lines.size(); i++) {
		// Skip vertices from the excluded line
		int line_index = i / 2;
		if (line_index == exclude_line) {
			continue;
		}
		
		vec2 vertex = canvas_lines[i];
		float distance = mag(pos - vertex);
		
		if (distance < min_distance) {
			min_distance = distance;
			nearest_vertex = vertex;
		}
	}
	
	return nearest_vertex;
}