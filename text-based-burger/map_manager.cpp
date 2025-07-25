#include "map_manager.h"

#include "json.hpp"
#include <fstream> // Include this header to use ifstream 
#include <iostream>

using namespace std;
using json = nlohmann::json; // for convenience

// Maps are encoded in a binary format (for once not json can you imagine?)
// There will probably be a header eventually but i dont know what to put in
// it right now. Map files only contain the geometry in a .vdg file, (vector
// display geometry) and the rest of the data is in a .json file, like the
// entities, brushes, etc.
// 
// You can just load a raw vdg file but it will be missing everything that
// makes it an actual map. See dev documentation for more details on this.
// 
// Format is per line that gets rendered, (parralax lines dont count as they
// are generated dynamically).
// 
// ---- format ----
// 
// 32 bit uint: x component of first vertex
// 32 bit uint: y component of first vertex
// 32 bit uint: x component of second vertex
// 32 bit uint: y component of second vertex
// 
// Lines only have a flat height, and shouldnt reach very high. Keep in mind
//     that this is a purely cosmetic effect and has no effect on gameplay.
// 8 bit uint: top z component of entire line
// 
// If you want to have you lines float above the field (or below) use this.
// Should only be used on cosmetic lines though, will probably break things if
// you try to collide with it.
// 8 bit uint: bottom z component of entire line
//
// We only have 8 bit monochrome color.
// 8 bit uint: color of line
// 
// Lines have several basic types:
// 0: normal line (collision, rendered)
// 1: cosmetic line (no collision, no parralax)
// 2: cosmetic parralax line (no collision, rendered, parralax)
// 3: technical line (no collision, not rendered)
// 
// More advanced types should be handled by brush entities, this is mostly
// intended for general purpouse brushless lines.
// 8 bit uint: line type
// 
// 255 misc flags for whatever might be needed, like maybe dashed line?
// 8 bit uint: misc flags
// 
// 32 bit uint: Id of brush it is part of. Brushes are groups of lines that can
//     have special properties, like being a door or a window. They are a type
//	   of entity and are stored in the json file. Id of 2^32 means no brush.
//
// ---- end ----
//
// Right now lines only take up 192 bits, and 64 at the end are tacked on as
// "unused" space. This is just for future proofing, if you are doing something
// that requires more flags then use these (if misc flags isnt enough).
//
// Lines are packed like this and one map may have several thousand lines, 
// especially the more expansive ones.

MapManager::MapManager(std::string filename) {
	// TODO: Actually load the binary file

	// for now we just read from json :))))))
	ifstream f(filename);
	if (!f.is_open()) {
		throw runtime_error("Could not open file " + filename);
	}
	json data = json::parse(f);

	float map_scale = 2.0f;

	// TODO: This will eventually be replaced with binary files
	// Loop through the coords and add them to the actual array
	for (auto& val : data["coords"]) {
		lines.push_back(val.get<int>() * map_scale);
	}
	num_lines = lines.size() / 4; // Each line has 4 coordinates

	geometry = MapGeometry();

	geometry.num_lines = num_lines;
	geometry.lines = &lines;
	geometry.lines_z = &lines_z;
	geometry.colors = &colors;
	geometry.types = &types;
	geometry.misc_flags = &misc_flags;
	geometry.brush_ids = &brush_ids;

	geometry.bvh_collision_nodes = &bvh_collision_nodes;
	geometry.bvh_cosmetic_nodes = &bvh_cosmetic_nodes;
}

void MapManager::update(ObjectUpdateData data) {
	camera_pos = data.camera_pos;
}

MapGeometry* MapManager::get_geometry() {
	return &geometry;
}

float fov_scale(float fov) {
	// Precompute the expensive parts since were calling project_point a lot.
	return 1.0f / tan(fov * 0.5f * (3.14159265359f / 180.0f));
}

// Project point in 3d. FOV should be in degrees.
float project_point(float val, float depth, float scale) {
	// Remember that depth should be orthographic depth. For actual 3D models
	// use the full distance field camera setup, see: https://www.desmos.com/calculator/3b0d55bd34.
	//
	// Also see: https://www.desmos.com/calculator/20c23f2dea

	return val / (depth * scale);
}

int MapManager::render(float* lines_list, uint32_t* colors, int offset) {

	// To render things, the centre of camera is at 0,0, but that translates to 480, 268 on the actual screen.
	// The raw camera position is fed to update(), and then update sends it through some function for things like
	// camera follow inertia, and writes it to camera_x and camera_y. These are the absoulte camera position so dont need to be
	// transformed. 
	//
	// For parralax, the camera is at a constant height above 0.
	float camera_height = 10;

	float scrn_width = 960.0f;
	float scrn_height = 536.0f;

	int lines_counter = offset;

	// Dont want to refactor all of this itll probably get scrapped anyway
	float camera_x = camera_pos.x;
	float camera_y = camera_pos.y;

	for (int i = 0; i < num_lines; i++) {
		// Get the line coordinates, these are in 32 bit ints
		int x1 = lines[i * 4 + 0];
		int y1 = lines[i * 4 + 1];
		int x2 = lines[i * 4 + 2];
		int y2 = lines[i * 4 + 3];

		// Do distance cullnig etc etc

		// Moves lines to camera position
		x1 -= camera_x;
		y1 -= camera_y;
		x2 -= camera_x;
		y2 -= camera_y;

		// Generate the floating line. Eventually we will also project the bottom lines, but for now only project top line
		// to height of 5. Keep in mind camera_heigh when calculating depth.
		float z_x1 = project_point((float)x1, mapz, fov_scale(map_z_fov));
		float z_y1 = project_point((float)y1, mapz, fov_scale(map_z_fov));
		float z_x2 = project_point((float)x2, mapz, fov_scale(map_z_fov));
		float z_y2 = project_point((float)y2, mapz, fov_scale(map_z_fov));

		// Move lines to centre of screen
		x1 += scrn_width / 2.0f;
		y1 += scrn_height / 2.0f;
		x2 += scrn_width / 2.0f;
		y2 += scrn_height / 2.0f;

		z_x1 += scrn_width / 2.0f;
		z_y1 += scrn_height / 2.0f;
		z_x2 += scrn_width / 2.0f;
		z_y2 += scrn_height / 2.0f;

		// Normalize to ndc
		float f_x1 = x1 / scrn_width;
		float f_y1 = y1 / scrn_height;
		float f_x2 = x2 / scrn_width;
		float f_y2 = y2 / scrn_height;

		f_x1 = (f_x1 * 2.0f) - 1.0f;
		f_y1 = (f_y1 * 2.0f) - 1.0f;
		f_x2 = (f_x2 * 2.0f) - 1.0f;
		f_y2 = (f_y2 * 2.0f) - 1.0f;

		z_x1 /= scrn_width;
		z_y1 /= scrn_height;
		z_x2 /= scrn_width;
		z_y2 /= scrn_height;

		z_x1 = (z_x1 * 2.0f) - 1.0f;
		z_y1 = (z_y1 * 2.0f) - 1.0f;
		z_x2 = (z_x2 * 2.0f) - 1.0f;
		z_y2 = (z_y2 * 2.0f) - 1.0f;

		// Now draw the 4 lines to lines list:
		// Bottom:
		lines_list[lines_counter * 4 + 0] = f_x1; // x1
		lines_list[lines_counter * 4 + 1] = f_y1; // y1
		lines_list[lines_counter * 4 + 2] = f_x2; // x2
		lines_list[lines_counter * 4 + 3] = f_y2; // y2
		colors[lines_counter] = 200;
		lines_counter++;

		// if we are drawing bvh dont tesselate
		if (draw_bvh) {
			colors[lines_counter - 1] = 50;
			continue; // Skip the rest of the lines
		}

		// Top:
		lines_list[lines_counter * 4 + 0] = z_x1; // x1
		lines_list[lines_counter * 4 + 1] = z_y1; // y1
		lines_list[lines_counter * 4 + 2] = z_x2; // x2
		lines_list[lines_counter * 4 + 3] = z_y2; // y2
		colors[lines_counter] = 127; // Color is always white for now
		lines_counter++;

		// Side 1:
		lines_list[lines_counter * 4 + 0] = f_x1; // x1
		lines_list[lines_counter * 4 + 1] = f_y1; // y1
		lines_list[lines_counter * 4 + 2] = z_x1; // x2
		lines_list[lines_counter * 4 + 3] = z_y1; // y2
		colors[lines_counter] = 127; // Color is always white for now
		lines_counter++;

		//// Side 2:
		//lines_list[lines_counter * 4 + 0] = f_x2; // x1
		//lines_list[lines_counter * 4 + 1] = f_y2; // y1
		//lines_list[lines_counter * 4 + 2] = z_x2; // x2
		//lines_list[lines_counter * 4 + 3] = z_y2; // y2
		//colors[lines_counter] = 127; // Color is always white for now
		//lines_counter++;

		// Need to deduplicate the side lines as every line draws both ends.
		// for now just only draw one side ig.
	}

	// Return how many lines weve rendered. Note that this isnt just num_lines
	// because not the entire map may be rendered at once, but also because of
	// parralax lines.
	return lines_counter;
}

void MapManager::toggle_render_bvh() {
	draw_bvh = !draw_bvh;
}

int MapManager::render_bvh(float* lines_list, uint32_t* colors, int offset) {
	if (!draw_bvh) {
		return offset; // Nothing to render
	}
	int lines_counter = offset;

	float scrn_width = 960.0f;
	float scrn_height = 536.0f;

	// Render collision BVH
	for (const auto& node : bvh_collision_nodes) {

		// Draw the bounds of the node
		vec2 from = node.from - camera_pos;
		vec2 to = node.to - camera_pos;

		float x1 = from.x;
		float y1 = from.y;
		float x2 = to.x;
		float y2 = to.y;

		// Move lines to centre of screen
		from += vec2(scrn_width, scrn_height) / 2.0f;
		to += vec2(scrn_width, scrn_height) / 2.0f;

		// Normalize to ndc
		from /= vec2(scrn_width, scrn_height);
		to /= vec2(scrn_width, scrn_height);

		from = (from * 2.0f) - 1.0f;
		to = (to * 2.0f) - 1.0f;

		int col = 70;

		// Right vertical
		lines_list[lines_counter * 4 + 0] = to.x; // x1
		lines_list[lines_counter * 4 + 1] = from.y; // y1
		lines_list[lines_counter * 4 + 2] = to.x; // x2
		lines_list[lines_counter * 4 + 3] = to.y; // y2
		colors[lines_counter++] = col;

		// Bottom horizontal
		lines_list[lines_counter * 4 + 0] = to.x;
		lines_list[lines_counter * 4 + 1] = from.y;
		lines_list[lines_counter * 4 + 2] = from.x;
		lines_list[lines_counter * 4 + 3] = from.y;
		colors[lines_counter++] = col;

		// Left vertical
		lines_list[lines_counter * 4 + 0] = from.x;
		lines_list[lines_counter * 4 + 1] = from.y;
		lines_list[lines_counter * 4 + 2] = from.x;
		lines_list[lines_counter * 4 + 3] = to.y;
		colors[lines_counter++] = col;

		// Top horizontal
		lines_list[lines_counter * 4 + 0] = from.x;
		lines_list[lines_counter * 4 + 1] = to.y;
		lines_list[lines_counter * 4 + 2] = to.x;
		lines_list[lines_counter * 4 + 3] = to.y;
		colors[lines_counter++] = col;
	}

	/*for (int i = 0; i < lines_counter / 2; i++) {
		std::cout <<
			lines_list[i * 4 + 0] << "," <<
			lines_list[i * 4 + 1] << "," << "\n" <<
			lines_list[i * 4 + 2] << "," <<
			lines_list[i * 4 + 3] << "," << "\n";
	}*/

	return lines_counter; // Return how many lines we rendered
}