#include "object_utils.h"
#include "game_object.h"

using namespace std;

// This is NOT double work with the ui equivalent, these two classes just happen to behave similrly
// These two are distnct classes and should not be assumed to have interoperability

ObjectIO::ObjectIO() {
	// Reporters will always start with an empty log
	error_log = vector<string>();
	return;
}

ObjectIO::~ObjectIO() {
	error_log.clear();
	return;
}

void ObjectIO::report_error(string error) {
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

vector<string> ObjectIO::get_log() {
	return error_log;
}

vector<int> ObjectIO::get_repeats() {
	return repeats;
}

void ObjectIO::register_object(string name, GameObject* object) {
	// First check if the component is already registered
	if (object_registry.find(name) != object_registry.end()) {
		// If it is, report an error
		// this should never happen and will be a nightmare to debug
		report_error("FATAL: Tried to register object " + name + " twice");
		return;
	}

	object_registry[name] = object;
	return;
}

GameObject* ObjectIO::get_object(string name) {
	// Check if the target component is registered
	if (object_registry.find(name) == object_registry.end()) {
		// If it is not, report an error
		report_error("FATAL: Tried to get unregistered object " + name);
		return nullptr;
	}
	// Get the target component
	return object_registry[name];
}

vector<string> split_file_path(string path) {
	// Split the path along the "." delimiter
	vector<string> result;
	size_t pos = 0;
	while ((pos = path.find('.')) != string::npos) {
		result.push_back(path.substr(0, pos));
		path.erase(0, pos + 1);
	}
	result.push_back(path);
	return result;
}