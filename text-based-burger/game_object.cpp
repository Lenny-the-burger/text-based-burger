#include "game_object.h"
#include "scripts.h"

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
	render_scale = make_pair(data["scale"][0], data["scale"][1]);
	color = data["color"];
	return;
}

void GameObject::update(ObjectUpdateData data) {
	// This is the update function.

	return;
}

int GameObject::render(float* lines_list, int offset, uint32_t* colors) {
	// Render function is called every frame. You are given a pointer to an array
	// and should append yourself to it if you need to be rendered. Not appending
	// yourself will cause you to not be rendered, even if you were rendered the
	// previous frame. The entire array is cleared. For optimization you should only
	// render if you are within some distance of the camera position you get from
	// updata data. Note that you should output NDC here not world space.

	// Hard coded screen size whatever
	float scrn_width = 640.0f;
	float scrn_height = 480.0f;

	// TODO: add siatcne to camera check if we need to render at all

	// Get the mesh from the io using json pointer
	json::json_pointer p("/" + mesh);

	json arr = io.meshes->at(p);

	// Loop over the mesh array of coordinates and transform and copy to lines list until done
	for (json num: arr) {
		float number = num.get<float>();
		
		// It goes x, y, x, y so if offset % 2 == 0 then we are at x
		if (offset % 2 == 0) {
			// Transform the x coordinate
			number = (number * render_scale.first) + position.x;
			number = number / scrn_width; // Normalize to screen width
		}
		else {
			// Transform the y coordinate
			number = (number * render_scale.second) + position.y;
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

using ObjectFactory = std::unique_ptr<GameObject>(*)(json data, ObjectIO& io);

std::unique_ptr<GameObject> object_type_selector(json data, ObjectIO& io) {
	static const std::map<std::string, ObjectFactory> factory_map = {
		{"generic", [](json data, ObjectIO& io) { return std::make_unique<GameObject>(data, io); }},

		// Add more game object types here
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

	mouse_state = MOUSE_NORMAL;
	return;
}

void MouseRenderer::update(ObjectUpdateData data) {
	// TODO: make all pointer meshes point from 0,0 so i dont have to do -19 here.
	position = vec2(float(data.mouse_x), float(480 - data.mouse_y - 19));

	if (data.is_clicking) {
		mouse_state = MOUSE_CLICKING;
	}
	else {
		mouse_state = MOUSE_NORMAL;
	}

	// Mouse state switcher. Modify mouse state to change the mesh.
	switch (mouse_state) {
		case MOUSE_NORMAL:
			mesh = "gen_props/pointers/point";
			break;
	
		case MOUSE_CLICKING:
			mesh = "gen_props/pointers/click";
			break;
	}

	return;
}
}