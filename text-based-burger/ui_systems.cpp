#include "ui_systems.h"

using namespace std;

// Error Reporter
ErrorReporter::ErrorReporter() {
	// Reporters will always start with an empty log
	error_log = vector<string>();
	return;
}

ErrorReporter::~ErrorReporter() {
	error_log.clear();
	return;
}

void ErrorReporter::report_error(string error) {
	// Check if the previous reported error is the same as the current one
	if (!error_log.empty() && error_log.back() == error) {
		// If it is, increment the repeat counter
		repeats.back()++;
		return;
	}

	// For now only report string errors
	error_log.push_back(error);
	repeats.push_back(0);
	return;
}

vector<string> ErrorReporter::get_log() {
	return error_log;
}

vector<int> ErrorReporter::get_repeats() {
	return repeats;
}

uint32_t gen_frag(int character, int bg, int fg) {
	// Generate a fragment from a character, background color, and foreground color
	// This is a 32 bit integer with the first 8 bits being the character, the next
	// 8 bits being the background color, and the next 8 bits being the foreground color
	// The last 8 bits are unused
	uint32_t frag = 0;
	frag |= character;
	frag |= fg << 8;
	frag |= bg << 16;

	return frag;
}