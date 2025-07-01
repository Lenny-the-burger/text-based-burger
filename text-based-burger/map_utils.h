#pragma once

#include "math_utils.h"

#include <vector>
#include <set>

struct LongThreadState;

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

// Functions running on long threads can only have one input
struct BVInput {
	std::vector<float>* lines;
	std::vector<int>* types;
	BVHType build_type;

	// Set this to what you want to work on
	std::vector<MapBvNode>* bvh_nodes;
};

// Build bvh for given lines.
int buildBVH(BVInput& input, LongThreadState& tstate);