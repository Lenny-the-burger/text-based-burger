#include "map_utils.h"

#include "threading_utils.h"

#include <iostream>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
using namespace std;

float bvArea(const MapBvNode& node) {
	// Assume from and to are always min and max
	return (node.from.x - node.to.x) * (node.from.y - node.to.y);
}

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
		node.from = maxv(v1, v2);
		node.to = minv(v1, v2);
		node.mid = midv(node.from, node.to);
		node.line_idx = i;
		input.bvh_nodes->push_back(node);
		work_q.push_back(i);
		std::this_thread::sleep_for(0.2s);
	}

	tstate.progress = 0;

	// Repeatedly merge nodes until only one remains
	while (work_q.size() > 1) {
		// Loop over the the work q and find the minimum area pair
		int min_area_idx = -1;
		float min_area = pow(2, 32);

		int cur_idx = work_q.front();

		MapBvNode* cur_node = &input.bvh_nodes->at(cur_idx);
		work_q.erase(work_q.begin());

		for (int i : work_q) {
			MapBvNode* candidate_node = &input.bvh_nodes->at(i);
			//float area = bvArea(*cur_node) + bvArea(*candidate_node);
			// For now use distance
			float area = distance(cur_node->mid, candidate_node->mid);
			if (area < min_area) {
				min_area = area;
				min_area_idx = i;
			}
		}

		MapBvNode* min_node = &input.bvh_nodes->at(min_area_idx);

		MapBvNode new_node;
		new_node.from = maxv(cur_node->from, min_node->from);
		new_node.to = minv(cur_node->to, min_node->to);
		new_node.mid = midv(new_node.from, new_node.to);
		new_node.l_child = cur_idx;
		new_node.r_child = min_area_idx;
		new_node.layer = max(cur_node->layer, min_node->layer) + 1;

		cur_node->parent = input.bvh_nodes->size();
		min_node->parent = input.bvh_nodes->size();

		input.bvh_nodes->push_back(new_node);

		// find and erase the minimum area node from the work queue
		work_q.erase(remove(work_q.begin(), work_q.end(), min_area_idx), work_q.end());

		// Add the new node to the work queue
		work_q.push_back(input.bvh_nodes->size() - 1);

		// Check if we are still running
		if (tstate.exit_now) {
			cout << "BVH build stopped by user" << endl;
			return -1; // Indicate that the build was stopped
		}

		tstate.progress++;

		cout << "Progress: " << tstate.progress << "/" << work_q.size() << " at layer " << new_node.layer << "\n";
		std::this_thread::sleep_for(0.2s);
	}

	cout << "Total size: " << input.bvh_nodes->size() << " nodes" << endl;

	return 0;
}