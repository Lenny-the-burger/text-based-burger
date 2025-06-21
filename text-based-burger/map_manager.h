#pragma once

// This class handles the loading and rendering of a map. It also handles
// fetching relevant geometry for other system for things like collision. actual
// collision is handled by scripts.

#include "object_utils.h"

class MapManager {
public:
	// Constructor
	MapManager(std::string filename);
	// Update the map
	void update(ObjectUpdateData data);
	// Render the map
	int render(float* lines_list, uint32_t* colors, int offset);
	// Get the error log
	std::vector<std::string> get_error_log();

	// There should probably be a more elegant solution to acessing map geometry
	// information, but map_manager.lines[n] is probably fine and im not writing
	// 10 different getters. were only passing around pointers anyway.

	// Master line counter
	int num_lines;

	// Some of these are stored as 8 bit, some are 32 bit, but everything here
	// is normalized to 32 bit for simplicity. You only have to worry about
	// writing float ndc coordinates for geometry and 32 bit colors anyway.

	// Coordinates of actual map geometry. Stored as [x1, y1, x2, y2]. Not
	// indexed. Length should be num_lines * 4.
	std::vector<float> lines;

	// Z components of map geometry. Stored per line as [top_z, bottom_z]. Not
	// indexed. Length should be num_lines * 2.
	std::vector<float> lines_z;

	// Per lines colors. 
	std::vector<float> colors;

	// Per line line types.
	std::vector<float> types;

	// Per line misc flags.
	std::vector<float> misc_flags;

	// Per line brush ids. 2^32 means no brush.
	std::vector<float> brush_ids;

private:

	// Error log
	std::vector<std::string> error_log;

	vec2 camera_pos;

	float mapz = 0.8f; // Default map z position
	float map_z_fov = 90.0f; // Default map z fov
};