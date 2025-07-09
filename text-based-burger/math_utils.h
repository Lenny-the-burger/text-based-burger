#pragma once

#include "json.hpp"
using json = nlohmann::json;

#include <vector>

// A header only math library for various math things the engine uses.

#ifndef ENGINE_MATH
#define ENGINE_MATH

// vec2 helper struct
struct vec2 {
	float x, y;

	vec2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
	vec2(const std::pair<float, float>& p) : x(p.first), y(p.second) {}
	vec2(int x, int y) : x(static_cast<float>(x)), y(static_cast<float>(y)) {}
	vec2(const std::pair<int, int>& p) : x(static_cast<float>(p.first)), y(static_cast<float>(p.second)) {}
	// Truncate duobles to floats but whatever
	vec2(double x, double y) : x(static_cast<float>(x)), y(static_cast<float>(y)) {}

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
	vec2 operator-(const float scalar) const {
		return vec2(x - scalar, y - scalar);
	}
	vec2 operator-=(const vec2& other) {
		x -= other.x;
		y -= other.y;
		return *this; // Return the modified object
	}
	vec2 operator-() const {// Negate 
		return vec2(-x, -y);
	}
	// Overload the * operator for scalar multiplication
	vec2 operator*(float scalar) const {
		return vec2(x * scalar, y * scalar);
	}
	vec2 operator*=(float scalar) {
		x *= scalar;
		y *= scalar;
		return *this; // Return the modified object
	}
	// Overload the * operator for vec2 peicewise multiplication
	vec2 operator*(const vec2& other) const {
		return vec2(x * other.x, y * other.y);
	}
	vec2 operator*=(const vec2& other) {
		x *= other.x;
		y *= other.y;
		return *this; // Return the modified object
	}
	// Overload the / operator for scalar division
	vec2 operator/(float scalar) const {
		if (scalar != 0) {
			return vec2(x / scalar, y / scalar);
		}
		else {
			// Division by zero results in +infty
			return vec2(std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
		}
	}
	// division by 0 will result in +infinity
	vec2 operator/=(float scalar) {
		x /= scalar;
		y /= scalar;
		return *this; // Return the modified object
	}
	vec2 operator/=(vec2 other) {
		x /= other.x;
		y /= other.y;
		return *this; // Return the modified object
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
	bool is_zero() const {
		return fabs(x) < 1e-6f && fabs(y) < 1e-6f; // Check if both components are close to zero
	}
	void blank() {
		x = 0.0f;
		y = 0.0f;
	}
};

// Operators we need to declare outside
inline vec2 operator/(float scalar, const vec2& v) {
	return vec2(scalar / v.x, scalar / v.y);
}

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
// Peicewise min
inline vec2 minv(vec2 a, vec2 b) {
	return vec2(std::min(a.x, b.x), std::min(a.y, b.y));
}
// Peicewise max
inline vec2 maxv(vec2 a, vec2 b) {
	return vec2(std::max(a.x, b.x), std::max(a.y, b.y));
}
inline vec2 midv(vec2 a, vec2 b) {
	// Midpoint
	return vec2((a.x + b.x) / 2.0f, (a.y + b.y) / 2.0f);
}
inline float distance(vec2 a, vec2 b) {
	// Euclidean distance
	return (float)sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

#else
// skip this file if already included
#endif