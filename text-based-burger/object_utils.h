#pragma once

// For keyboard and mouse input
#include <GLFW/glfw3.h>

#include "json.hpp"
using json = nlohmann::json;

#include <vector>
#include <string>
#include <map>

// Forward declaration
class GameObject;

// Update data struct for the object system
struct ObjectUpdateData {
	float time;
	float frame_time;
	int mouse_x;
	int mouse_y;
	bool is_clicking;

	// Pointer to actual window, do whatever you want with it
	GLFWwindow* window;

	double camera_x;
	double camera_y;

	float mapz;
	float map_z_fov;
};

// Objects all hold a pointer to an instance of this class and use it to call
// scripts, report errors etc. Basically the same as the UIComponentIO class but
// for game objects. Only reason it isnt reused is because of system specific
// types

class ObjectIO {
public:
	ObjectIO();
	~ObjectIO();

	void report_error(std::string error);

	std::vector<std::string> get_log();
	std::vector<int> get_repeats();

	void register_object(std::string name, GameObject* object);

	// This should only be used by the script system
	GameObject* get_object(std::string name);

	// Points to the meshes data. The actual one is held by the handler
	json* meshes;

private:
	std::vector<std::string> error_log;
	std::vector<int> repeats;
	std::map<std::string, GameObject*> object_registry;
};

// Split given path along "/" delimiter
std::vector<std::string> split_file_path(std::string path);

// Include gaurd because vec2 likes to explode
#ifndef TBB_VEC2
#define TBB_VEC2

// vec2 helper struct
struct vec2 {
	float x;
	float y;
	vec2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
	vec2(const std::pair<float, float>& p) : x(p.first), y(p.second) {}
	vec2(int x, int y) : x(static_cast<float>(x)), y(static_cast<float>(y)) {}
	vec2(const std::pair<int, int>& p) : x(static_cast<float>(p.first)), y(static_cast<float>(p.second)) {}

	vec2(int num) : x(static_cast<float>(num)), y(static_cast<float>(num)) {}
	vec2(float num) : x(num), y(num) {}

	// json constrtuctor
	vec2(const json& data) {
		if (data.is_array() && data.size() == 2) {
			x = data[0].get<float>();
			y = data[1].get<float>();
		}
		else if (data.is_object()) {
			x = data["x"].get<float>();
			y = data["y"].get<float>();
		}
		else {
			throw std::invalid_argument("Invalid json format for vec2");
		}
	}

	std::pair<float, float> to_pair() const {
		return std::make_pair(x, y);
	}

	// Overload the == operator for vec2 equality check
	bool operator==(const vec2& other) const {
		return fabs(x - other.x) < 1e-6f && fabs(y - other.y) < 1e-6f;
	}
	// Overload the + operator for vec2 addition
	vec2 operator+(const vec2& other) const {
		return vec2(x + other.x, y + other.y);
	}
	vec2 operator+=(const vec2& other) {
		x += other.x;
		y += other.y;
		return *this; // Return the modified object
	}
	// Overload the - operator for vec2 subtraction
	vec2 operator-(const vec2& other) const {
		return vec2(x - other.x, y - other.y);
	}
	// Overload the * operator for scalar multiplication
	vec2 operator*(float scalar) const {
		return vec2(x * scalar, y * scalar);
	}
	// Overload the * operator for vec2 peicewise multiplication
	vec2 operator*(const vec2& other) const {
		return vec2(x * other.x, y * other.y);
	}
	// Overload the / operator for scalar division
	vec2 operator/(float scalar) const {
		return vec2(x / scalar, y / scalar);
	}

	float mag() {
		return sqrt(x * x + y * y);
	}
	float dot(const vec2& other) const {
		return x * other.x + y * other.y;
	}
	float cross(const vec2& other) const {
		return x * other.y - y * other.x; // evil 2D cross product
	}
	float angle() const {
		return atan2(y, x);
	}
	vec2 unit() {
		float magnitude = mag();
		if (magnitude != 0) {
			return vec2(x / magnitude, y / magnitude);
		}
		else {
			return vec2(); // Return zero vector if magnitude is zero
		}
	}
};

// If you want to use them as globals
inline float mag(vec2 v) {
	return v.mag();
}
inline vec2 unit(vec2 v) {
	return v.unit();
}
inline float dot(vec2 a, vec2 b) {
	return a.dot(b);
}
inline float cross(vec2 a, vec2 b) {
	return a.cross(b);
}

#else
// skip this file if already included
#endif