#include "font_loader.h"

using namespace std;
using namespace glm;

vector<uint64_t> load_font(const string& font_path) {
	vector<uint64_t> font_data;

	ifstream font_file(font_path, ios::binary);
	if (!font_file.is_open()) {
		cerr << "Failed to open font file: " << font_path << endl;
		return font_data;
	}

	// Each character starts with a line of
	// ----- [c] (n) -----
	// Where c is the character and n is the character number, we dont need this for now.
	// The next 16 lines are 8 either " " ot "*" representing the pixels.
	// To convert into uints, we unrawp the first 8 lines into a 64 bit number, where each bit is a pixel,
	// and then the same for the next 8 lines.
	// Every glyph contains an upper and lower half, so we store them as a 64 bit number each.

	string line;

	while (getline(font_file, line)) {

		// Skip header lines
		if (line.size() > 8) {
			continue;
		}

		// Unwrap the first 8 lines into a 64 bit number
		uint64_t first = 0;
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				if (line[j] == '*') {
					first |= 1 << (i * 8 + j);
				}
			}
			getline(font_file, line);
		}

		font_data.push_back(first);

		// Unwrap the next 8 lines into a 64 bit number
		uint64_t second = 0;
		for (int i = 0; i < 8; i++) {
			for (int j = 0; j < 8; j++) {
				if (line[j] == '*') {
					second |= 1 << (i * 8 + j);
				}
			}
			getline(font_file, line);
		}

		font_data.push_back(second);
	}

	font_file.close();

	return font_data;
}