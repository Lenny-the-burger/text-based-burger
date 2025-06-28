#include "threading_utils.h"

using namespace std;

// Long threads

void LongThreadController::launch(
	const std::string& name,
	std::any input, 
	std::function<void(LongThreadState&)> task, 
	std::function<void()> on_complete
) {

	std::lock_guard<std::mutex> lock(thread_mutex);

	if (threads.count(name)) {
		error_reporter.report_error("ERROR: Attempted to launch a long thread with name '" + name + "' that already exists.");
		return;
	};

	LongThreadState& state = threads[name];

	state.input = std::move(input);
	state.on_complete = std::move(on_complete);

	state.worker = std::thread([task, &state]() {
		task(state);
		state.done = true;
		});
}

void LongThreadController::update() {
	std::lock_guard<std::mutex> lock(thread_mutex);

	for (auto it = threads.begin(); it != threads.end(); ) {
		LongThreadState& state = it->second;

		if (state.done) {
			if (state.worker.joinable())
				state.worker.join();

			if (state.on_complete)
				state.on_complete();

			it = threads.erase(it);
		}
		else {
			++it;
		}
	}
}

LongThreadState* LongThreadController::get(const std::string& name) {
	std::lock_guard<std::mutex> lock(thread_mutex);
	auto it = threads.find(name);
	return (it != threads.end()) ? &it->second : nullptr;
}

bool LongThreadController::is_running(const std::string& name) const {
	std::lock_guard<std::mutex> lock(thread_mutex);
	auto it = threads.find(name);
	return (it != threads.end() && !it->second.done);
}

void LongThreadController::remove(const std::string& name) {
	std::lock_guard<std::mutex> lock(thread_mutex);
	threads.erase(name);
}

void LongThreadController::clean_up_threads() {
	std::lock_guard<std::mutex> lock(thread_mutex);

	for (auto& [name, state] : threads) {
		state.exit_now = true;
	}

	// Make sure your functions care about the exit_now flag because I cant
	// force kill for some reason.
	for (auto& [name, state] : threads) {
		if (state.worker.joinable()) {
			state.worker.join();
		}
	}

	// Step 3: Clear all state
	threads.clear();
}