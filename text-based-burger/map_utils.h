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
	vec2 mid;

	float outer_rad;
	float inner_rad;

	int r_child = -1;
	int l_child = -1;
	int parent = -1;

	int layer = 0;

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

// Various collision

bool collide_aabb(
	MapBvNode& node,
	vec2& from, vec2& to
);

// Cohen–Sutherland algorithm
typedef int OutCode;

const int INSIDE = 0b0000;
const int LEFT = 0b0001;
const int RIGHT = 0b0010;
const int BOTTOM = 0b0100;
const int TOP = 0b1000;

OutCode ComputeOutCode(double x, double y, vec2& clip_from, vec2& clip_to);

bool CohenSutherlandLineClip(double& x0, double& y0, double& x1, double& y1, vec2& clip_from, vec2& clip_to);

// Collide given aabb with map geometry. Returns the normal force expereinced if
// there is a collision. 
vec2 collide_aabb_geometry(
	vec2& from, vec2& to,
	std::vector<MapBvNode>* bvh_nodes,
	std::vector<float>* lines
);