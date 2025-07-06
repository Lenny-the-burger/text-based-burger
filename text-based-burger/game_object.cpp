#include "game_object.h"
#include "scripts.h"
#include <iostream>

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

	// TODO: add siatcne to camera check if we need to render at all

	// TODO: Eventually this will use just a flat map

	// Get the mesh from the io using json pointer
	json::json_pointer p("/" + mesh);

	json arr = io.meshes->at(p);

	vec2 screen_position = position - camera; // Offset the position by the camera position
	// Centre the position on the screen
	screen_position.x += scrn_width / 2.0f;
	screen_position.y += scrn_height / 2.0f;

	// Loop over the mesh array of coordinates and transform and copy to lines list until done
	for (json num: arr) {
		float number = num.get<float>();
		
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
		{"color", 255},
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
			mesh = "gen_props/pointers/aim";
			break;
	
		case MOUSE_CLICKING:
			mesh = "gen_props/pointers/click";
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
		{"color", 255},
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
	float stepsize = 3.0f * 100.0f;
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
	if (glfwGetKey(data.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
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
		{"color", 255},
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

	float move_speed = 3.0f * 100.0f;

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