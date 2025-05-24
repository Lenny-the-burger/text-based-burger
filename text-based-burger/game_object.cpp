#include "game_object.h"
#include "scripts.h"

using namespace std;

// GAME OBJECT

GameObject::GameObject(json data, ObjectIO& io) : io(io) {
	targetname = to_string(data["targetname"]);

	update_script_name = data["update_script"].get_ref<const string&>();

	// For non rendering objects you can probably not even write these, if you know
	// you will never need to render it is probably fine if they are null.
	render_name = data["render_name"].get_ref<const string&>();
	position = make_pair(data["position"]["x"], data["position"]["y"]);
	rotation = data["rotation"];
	render_scale = make_pair(data["scale"]["x"], data["scale"]["y"]);
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