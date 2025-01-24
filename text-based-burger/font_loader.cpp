#include "font_loader.h"

using namespace std;
using namespace glm;

vector<uint32_t> load_font(const string& font_path) {
    // Each character starts with a line of
    // ----- [c] (n) -----
    // Where c is the character and n is the character number, we dont need this for now.
    // The next 16 lines are 8 either " " ot "" representing the pixels.
    // To convert into uints, we unrawp the first 4 lines into a 64 bit number, where each bit is a pixel,
    // and then the same for the next lines. The glyphs are divided into 4 32 bit numbers.

    vector<uint32_t> font_data;
    ifstream font_file(font_path);

    if (!font_file.is_open()) {
		cerr << "Failed to open font file: " << font_path << endl;
		exit(1);
    }

    string line;
    while (getline(font_file, line)) {
        // Skip header lines
        if (line.length() > 8) {
            continue;
        }

        uint32_t out = 0;
        for (int k = 0; k < 4; k++) {
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 8; j++) {
                    if (line[j] == '*') {
                        out |= 1 << (k * 32 + i * 8 + j);
                    }
                }
                getline(font_file, line);
            }
        }
        font_data.push_back(out);
    }

    font_file.close();
    return font_data;
}