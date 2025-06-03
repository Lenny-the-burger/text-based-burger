#pragma once

#include "json.hpp"
using json = nlohmann::json;

#include "object_utils.h"

#include <vector>
#include <string>
#include <map>

// This is all the game objects. Game objects are responcible for their own
// interaction and submitting their own rendering data to the handler.

class GameObject {
public:
	// Game objects only have once constructor because they dont need
	// Recursive construction like components do
	GameObject(json data, ObjectIO& io);

	// This is the update function. It is called every frame and should
	// return true if the object needs to be rerendered
	virtual bool update(ObjectUpdateData data);

	// Render function is called every frame. You are given a pointer to
	// an array and should append yourself to it if you need to be rendered.
	// Not appending yourself will cause you to not be rendered, even if you
	// were rendered the previous frame. The entire array is cleared. For optimization
	// you should only render if you are within some distance of the camera position
	// you get from updata data. Note that you should output NDC here not world space.
	virtual int render(float* lines_list, int offset, uint32_t* colors);

	std::string targetname;

	// This is the color of the object
	int color;

	void set_position(float x, float y) {
		position = std::make_pair(x, y);
	}

protected:
	// This is the object io instance. It is used to call scripts and report errors
	ObjectIO& io;

	// Name of the (mesh?) that this object uses
	std::string mesh;

	std::string update_script_name;

	// Safe to assume all objects have a world space position
	std::pair<float, float> position;

	// This is the rotation of the object in radians
	float rotation;

	// This is the scale of the object
	std::pair<float, float> render_scale;
};

// Type selctor for game objects
std::unique_ptr<GameObject> type_selector(json data, ObjectIO& io);

enum MouseState {
	MOUSE_NORMAL,
	MOUSE_CLICKING,
	MOUSE_DRAGGING
};

// Just renders the mouse cursor. This object cannot be created from JSON
// and only one exists, specially held by the ObjectsHandler
class MouseRenderer : public GameObject {
public:
	MouseRenderer(ObjectIO& io);

	virtual bool update(ObjectUpdateData data) override;

	MouseState mouse_state;
};