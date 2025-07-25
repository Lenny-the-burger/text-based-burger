#pragma once

// This class handles the loading and rendering of a map. It also handles
// fetching relevant geometry for other system for things like collision. actual
// collision is handled by scripts.

#include "object_utils.h"
#include "map_utils.h"

// Lightweight struct to pass when someone wants access to the map geometry
struct MapGeometry {
	int num_lines;

	std::vector<float>* lines;
	std::vector<float>* lines_z;
	std::vector<int>* colors;
	std::vector<int>* types;
	std::vector<int>* misc_flags;
	std::vector<int>* brush_ids;

	std::vector<MapBvNode>* bvh_collision_nodes; // BVH for collision lines
	std::vector<MapBvNode>* bvh_cosmetic_nodes; // BVH for cosmetic lines
};

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

	MapGeometry* get_geometry();

	void toggle_render_bvh();

	int render_bvh(float* lines_list, uint32_t* colors, int offset);

	bool has_collision_bvh = false; // If we have a collision BVH

private:

	bool draw_bvh = false;

	MapGeometry geometry;

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
	std::vector<int> colors;

	// Per line line types.
	std::vector<int> types;

	// Per line misc flags.
	std::vector<int> misc_flags;

	// Per line brush ids. 2^32 means no brush.
	std::vector<int> brush_ids;

	std::vector<MapBvNode> bvh_collision_nodes; // BVH for collision lines
	std::vector<MapBvNode> bvh_cosmetic_nodes; // BVH for cosmetic lines

	// Error log
	std::vector<std::string> error_log;

	vec2 camera_pos;

	float mapz = 0.8f; // Default map z position
	float map_z_fov = 90.0f; // Default map z fov
};