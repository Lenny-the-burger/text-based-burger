#pragma once

#include "math_utils.h"

#include <vector>

// Map utils, this is mostly used for building the map in the editor.

enum LineTypes {
	LINE_TYPE_NORMAL = 0, // Normal line, collision, rendered
	LINE_TYPE_COSMETIC = 1, // Cosmetic line, no collision, no parallax
	LINE_TYPE_COSMETIC_PARALLAX = 2, // Cosmetic line, no collision, parallax
	LINE_TYPE_TECHNICAL = 3, // Technical line, no collision, not rendered
};

struct MapBvNode {
	// Bounds
	vec2 from;
	vec2 to;
	vec2 centre;

	int r_child = -1;
	int l_child = -1;
	int parent = -1;

	// Only if we are a leaf node
	int line_idx = -1;
};

enum BVHType {
	BVH_COLLISION,
	BVH_COSMETIC,
};

// Build bvh for given lines.
std::vector<MapBvNode> buildBVH(
	const std::vector<float>& line_coords,
	std::vector<int> types,
	BVHType build_type
);