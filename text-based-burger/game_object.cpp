#include "game_object.h"
#include "scripts.h"

using namespace std;

// GAME OBJECT

GameObject::GameObject(json data, ObjectIO& io) : io(io) {
	targetname = data["targetname"].get_ref<const string&>();

	update_script_name = data["update_script"].get_ref<const string&>();

	// For non rendering objects you can probably not even write these, if you know
	// you will never need to render it is probably fine if they are null.
	mesh = data["mesh"].get_ref<const string&>();
	position = make_pair(data["position"][0], data["position"][1]);
	//rotation = data["rotation"];
	render_scale = make_pair(data["scale"][0], data["scale"][1]);
	color = data["color"];
	return;
}

bool GameObject::update(ObjectUpdateData data) {
	// This is the update function. It is called every frame and should
	// return true if the object needs to be rerendered
	return false;
}

void GameObject::render(std::vector<float>& lines_list, std::vector<uint32_t> colors) {
	// Render function is called every frame. You are given a pointer to
	// an array and should append yourself to it if you need to be rendered.
	// Not appending yourself will cause you to not be rendered, even if you
	// were rendered the previous frame. The entire array is cleared. For optimization
	// you should only render if you are within some distance of the camera position
	// you get from updata data. Note that you should output NDC here not world space.
	return;
}