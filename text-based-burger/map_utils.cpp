#include "map_utils.h"

#include "threading_utils.h"

#include <iostream>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
using namespace std;

int buildBVH(BVInput& input, LongThreadState& tstate) {

	int num_lines = input.lines->size() / 4;

	cout << "Building BVH for " << num_lines << " lines of type " << input.build_type << endl;

	vector<int> work_q;

	input.bvh_nodes->clear();

	// Loop over all the lines and create leaf node
	for (int i = 0; i < num_lines; i++) {
		// TODO: Check line type here

		vec2 v1 = vec2((*input.lines)[i * 4 + 0], (*input.lines)[i * 4 + 1]);
		vec2 v2 = vec2((*input.lines)[i * 4 + 2], (*input.lines)[i * 4 + 3]);

		MapBvNode node;
		node.from = minv(v1, v2);
		node.to = maxv(v1, v2);
		node.mid = midv(node.from, node.to);
		node.line_idx = i;
		input.bvh_nodes->push_back(node);
		work_q.push_back(i);
		//std::this_thread::sleep_for(0.2s);
	}

	tstate.progress = 0;

	// Repeatedly merge nodes until only one remains
	while (work_q.size() > 1) {
		// Loop over the the work q and find the minimum area pair
		int min_dist_idx = -1;
		float min_dist = pow(2, 32);

		int cur_idx = work_q.front();

		MapBvNode* cur_node = &input.bvh_nodes->at(cur_idx);
		work_q.erase(work_q.begin());

		for (int i : work_q) {
			MapBvNode* candidate_node = &input.bvh_nodes->at(i);
			float area = distance(cur_node->mid, candidate_node->mid);
			if (area < min_dist) {
				min_dist = area;
				min_dist_idx = i;
			}
		}

		MapBvNode* min_node = &input.bvh_nodes->at(min_dist_idx);

		MapBvNode new_node;
		new_node.from = minv(cur_node->from, min_node->from);
		new_node.to = maxv(cur_node->to, min_node->to);
		new_node.mid = midv(new_node.from, new_node.to);
		new_node.l_child = cur_idx;
		new_node.r_child = min_dist_idx;
		new_node.layer = max(cur_node->layer, min_node->layer) + 1;

		cur_node->parent = input.bvh_nodes->size();
		min_node->parent = input.bvh_nodes->size();

		input.bvh_nodes->push_back(new_node);

		// find and erase the minimum area node from the work queue
		work_q.erase(remove(work_q.begin(), work_q.end(), min_dist_idx), work_q.end());

		// Add the new node to the work queue
		work_q.push_back(input.bvh_nodes->size() - 1);

		// Check if we are still running
		if (tstate.exit_now) {
			cout << "BVH build stopped by user" << endl;
			return -1; // Indicate that the build was stopped
		}

		tstate.progress++;

		//cout << "Progress: " << tstate.progress << "/" << work_q.size() << " at layer " << new_node.layer << "\n";
		//std::this_thread::sleep_for(0.2s);
	}

	cout << "Total size: " << input.bvh_nodes->size() << " nodes" << endl;

	return 0;
}

bool collide_aabb(
	MapBvNode& node,
	vec2& from, vec2& to
) {
	return (from.x <= node.to.x && to.x >= node.from.x &&
		    from.y <= node.to.y && to.y >= node.from.y);
}

// Compute the bit code for a point (x, y) using the clip rectangle
// bounded diagonally by (xmin, ymin), and (xmax, ymax)

// ASSUME THAT xmax, xmin, ymax and ymin are global constants.

OutCode ComputeOutCode(double x, double y, vec2& clip_from, vec2& clip_to)
{
	OutCode code = INSIDE;  // initialised as being inside of clip window
	double xmin = clip_from.x;
	double ymin = clip_from.y;
	double xmax = clip_to.x;
	double ymax = clip_to.y;

	if (x < xmin)           // to the left of clip window
		code |= LEFT;
	else if (x > xmax)      // to the right of clip window
		code |= RIGHT;
	if (y < ymin)           // below the clip window
		code |= BOTTOM;
	else if (y > ymax)      // above the clip window
		code |= TOP;

	return code;
}

// Cohen–Sutherland clipping algorithm clips a line from
// P0 = (x0, y0) to P1 = (x1, y1) against a rectangle with 
// diagonal from (xmin, ymin) to (xmax, ymax).
bool CohenSutherlandLineClip(
	double& x0, double& y0, double& x1, double& y1,
	vec2& clip_from, vec2& clip_to
)
{
	// compute outcodes for P0, P1, and whatever point lies outside the clip rectangle
	OutCode outcode0 = ComputeOutCode(x0, y0, clip_from, clip_to);
	OutCode outcode1 = ComputeOutCode(x1, y1, clip_from, clip_to);
	bool accept = false;

	double xmin = clip_from.x;
	double ymin = clip_from.y;
	double xmax = clip_to.x;
	double ymax = clip_to.y;

	while (true) {
		if (!(outcode0 | outcode1)) {
			// bitwise OR is 0: both points inside window; trivially accept and exit loop
			accept = true;
			break;
		}
		else if (outcode0 & outcode1) {
			// bitwise AND is not 0: both points share an outside zone (LEFT, RIGHT, TOP,
			// or BOTTOM), so both must be outside window; exit loop (accept is false)
			break;
		}
		else {
			// failed both tests, so calculate the line segment to clip
			// from an outside point to an intersection with clip edge
			double x, y;

			// At least one endpoint is outside the clip rectangle; pick it.
			OutCode outcodeOut = outcode1 > outcode0 ? outcode1 : outcode0;

			// Now find the intersection point;
			// use formulas:
			//   slope = (y1 - y0) / (x1 - x0)
			//   x = x0 + (1 / slope) * (ym - y0), where ym is ymin or ymax
			//   y = y0 + slope * (xm - x0), where xm is xmin or xmax
			// No need to worry about divide-by-zero because, in each case, the
			// outcode bit being tested guarantees the denominator is non-zero
			if (outcodeOut & TOP) {           // point is above the clip window
				x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
				y = ymax;
			}
			else if (outcodeOut & BOTTOM) { // point is below the clip window
				x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
				y = ymin;
			}
			else if (outcodeOut & RIGHT) {  // point is to the right of clip window
				y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
				x = xmax;
			}
			else if (outcodeOut & LEFT) {   // point is to the left of clip window
				y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
				x = xmin;
			}

			// Now we move outside point to intersection point to clip
			// and get ready for next pass.
			if (outcodeOut == outcode0) {
				x0 = x;
				y0 = y;
				outcode0 = ComputeOutCode(x0, y0, clip_from, clip_to);
			}
			else {
				x1 = x;
				y1 = y;
				outcode1 = ComputeOutCode(x1, y1, clip_from, clip_to);
			}
		}
	}
	return accept;
}

// Aspirationally collide aabb with map geometry. It will then return the normal
// force telling you exactly why your dreams will not happen.
vec2 collide_aabb_geometry(
	vec2& from, vec2& to,
	std::vector<MapBvNode>* bvh_nodes,
	std::vector<float>* lines
) {
	// This is a simple AABB collision detection with the map geometry
	vec2 normal_force = vec2();
	vector<int> work_stack;
	vector<int> lines_to_check;

	// Root
	work_stack.push_back(bvh_nodes->size() - 1);
	while (!work_stack.empty()) {
		int node_idx = work_stack.back();
		work_stack.pop_back();

		MapBvNode& node = bvh_nodes->at(node_idx);

		if (!collide_aabb(node, from, to)) {
			continue; // No collision with this node
		}
		if (node.line_idx >= 0) {
			// Leaf node, check the line
			lines_to_check.push_back(node.line_idx);
			continue;
		}
		// Non-leaf node, add children to the stack
		if (node.l_child >= 0) {
			work_stack.push_back(node.l_child);
		}
		if (node.r_child >= 0) {
			work_stack.push_back(node.r_child);
		}
	}

	vec2 collision_srs = vec2();
	int collision_count = 0;

	// Check the lines for actual collision
	while (!lines_to_check.empty()) {
		int line_idx = lines_to_check.back();
		lines_to_check.pop_back();
		vec2 v1 = vec2((*lines)[line_idx * 4 + 0], (*lines)[line_idx * 4 + 1]);
		vec2 v2 = vec2((*lines)[line_idx * 4 + 2], (*lines)[line_idx * 4 + 3]);

		double x0 = v1.x;
		double y0 = v1.y;
		double x1 = v2.x;
		double y1 = v2.y;


		// Check if the line intersects with the AABB
		if (CohenSutherlandLineClip(x0, y0, x1, y1, from, to)) {
			collision_srs += midv(vec2(x0, y0), vec2(x1, y1));
			collision_count++;
		}
	}

	if (collision_count == 0) {
		return vec2(); // No collision
	}

	collision_srs /= collision_count;
	normal_force = collision_srs - midv(from, to);

	// Inverse square for normal force
	normal_force *= 1.0f / pow(normal_force.mag(), 2);

	return normal_force;
}