#pragma once

// Header only generic error reporter class. Bigger systems like the ui handler
// and objects handler dont use this because it is already integrated into their
// io classes (for now).

#include <string>
#include <vector>

class ErrorReporter {
public:
	// Constructor
	ErrorReporter() = default;
	// Report an error
	void report_error(const std::string& error) {
		error_log.push_back(error);
		repeats.push_back(1); // Initialize repeat count to 1
	}
	// Get the error log
	const std::vector<std::string>& get_log() const {
		return error_log;
	}
	// Get the repeat counts for each error
	const std::vector<int>& get_repeats() const {
		return repeats;
	}

private:
	std::vector<std::string> error_log; // Log of errors
	std::vector<int> repeats; // Count of how many times each error has been reported
};