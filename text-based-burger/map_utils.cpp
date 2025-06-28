#include "map_utils.h"

#include "threading_utils.h"

#include <iostream>

using namespace std;

vector<MapBvNode> buildBVH(BVInput input, LongThreadState& tstate) {
	vector<MapBvNode> nodes;

	// Do some stuff
	std::cout << "Building BVH for " << input.lines->size() / 4 << " lines of type " << input.build_type << std::endl;

	int temp = 0;
	for (int i = 0; i < 1000000000; i++) {
		temp += rand();
		if (i % 10000000 == 0) {
			std::cout << "BVH "<< input.build_type << " progress: " << (i / 10000000) << "0M lines processed" << std::endl;
		}
		if (tstate.exit_now) {
			std::cout << "Exit early flag set" << std::endl;
			break;
		}
	}

	return nodes;
}