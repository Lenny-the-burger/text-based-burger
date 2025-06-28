#pragma once

#include <any>
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "scripts.h" 
#include "error_reporter.hpp"

// threading_utils.h: utils that be threading

// Long threads are threads that run for a while in the background and are not
// necessarily doing parralel work. Due to their simplicity long threads should
// only really be used for things in the editor, like building navmeshes, but
// can also be used to load maps for example. Things that you are okay with the
// player waiting for. They get created, do their work, and then get destroyed.

struct LongThreadState {
	// Max can be set to whatever you want if you want more granularity in
	// updates. This is usally read by a progress bar that can have a variable
	// width and number of steps, so you could have more than a 100 steps. Do
	// not normalize to 100 unless it makes sense.
	std::atomic<int> progress = 0;
	std::atomic<int> max = 100;

	std::atomic<bool> done = false;

	std::atomic<bool> exit_now = false;

	std::thread worker;
	std::any input;
	std::any output;

	// This should be a call_script("end_script_name") lambda on your local io.
	std::function<void()> on_complete;
};

class LongThreadController {
public:

	LongThreadController(ErrorReporter& error_reporter) : error_reporter(error_reporter) {}; 

	// Launch a named thread
	void launch(
		const std::string& name, 
		std::any input, 
		std::function<void(LongThreadState&)> task, 
		std::function<void()> on_complete = nullptr
	);

	// Called periodically to join completed threads and run callbacks
	void update();

	// Get thread state by name (for progress bars etc)
	LongThreadState* get(const std::string& name);

	// Check if thread is running
	bool is_running(const std::string& name) const;

	// Forcefully remove a thread (must be joined manually beforehand)
	void remove(const std::string& name);

	void clean_up_threads();

private:
	// All controllers share one error reporter you dont each need your own
	ErrorReporter& error_reporter;

	std::unordered_map<std::string, LongThreadState> threads;
	mutable std::mutex thread_mutex; // Protects the map
};