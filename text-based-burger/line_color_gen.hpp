#include <cstdint>
#include <unordered_map>

#ifndef LINE_COLOR_GEN_HPP
#define LINE_COLOR_GEN_HPP

// Colors are stored as 32 bit integers in the format:
// bits 0-7: hue (0-255)
// bits 8-15: intensity (0-255)
// bits 16-23: alpha (0-255)
// bits 24-31: line width, note that this is the only paramater that is not treated as 0-1 but as a raw value.
// Width of 0 means default width, 1-127 is more thick, 128-255 is less thick.

// Line type presets you can use
enum LineTypePreset {
	LINE_COLOR_PRESET_CURSOR, // Mouse cursor lines
	LINE_COLOR_PRESET_NPC_FRIENDLY,
	LINE_COLOR_PRESET_NPC_FRIENDLY_SELECTED,
	LINE_COLOR_PRESET_NPC_ENEMY,
	LINE_COLOR_PRESET_WALL_GENERIC,
	LINE_COLOR_PRESET_WALL_SECONDARY,
	LINE_COLOR_PRESET_EDITOR_LINE,
	LINE_COLOR_PRESET_EDITOR_SELECTED
};

// Lookup table for presets - more efficient than switch statement
static const std::unordered_map<LineTypePreset, uint32_t> preset_colors = {
	{LINE_COLOR_PRESET_CURSOR,              64 | (0 << 8) | (255 << 16)}, // 255 hue, 127 intensity, 255 alpha

	{LINE_COLOR_PRESET_NPC_FRIENDLY,        0x00FF7FFF}, // 255 hue, 127 intensity, 255 alpha
	{LINE_COLOR_PRESET_NPC_FRIENDLY_SELECTED, 0x00FFFFFF}, // 255 hue, 255 intensity, 255 alpha
	{LINE_COLOR_PRESET_NPC_ENEMY,           0x00FF7FFF}, // 255 hue, 127 intensity, 255 alpha

	{LINE_COLOR_PRESET_WALL_GENERIC,        191 | (191 << 8) | (255 << 16)}, // 191 hue, 191 intensity, 255 alpha
	{LINE_COLOR_PRESET_WALL_SECONDARY,      191 | (191 << 8) | (127 << 16) | ((127 + int(0.5 * 127.0)) << 24)}, // 191 hue, 191 intensity, 127 alpha, -0.5 width

	{LINE_COLOR_PRESET_EDITOR_LINE,         191 | (191 << 8) | (255 << 16)}, // 191 hue, 191 intensity, 255 alpha - same as wall generic
	{LINE_COLOR_PRESET_EDITOR_SELECTED,     191 | (255 << 8) | (255 << 16) | (5 << 24)} // 191 hue, 255 intensity, 255 alpha, thicker
};

// First 8 bits are hue, next are intensity, next are alpha. 8 unused.
inline uint32_t generate_line_color(LineTypePreset preset) {
	auto it = preset_colors.find(preset);
	return (it != preset_colors.end()) ? it->second : 255 << 16;
}

// Generate color from given colors. Colors should be 0-1 range.
inline uint32_t generate_line_color(float hue, float intensity, float alpha, float thickness) {

	int converted_thickness;
	if (thickness < 0.0f) {
		// Negative thickness means thinner than default, so we convert it to a positive value
		converted_thickness = static_cast<int>(127.0f + (-1.0 * thickness) * 127.0f);
	} else {
		// Positive thickness means thicker than default, so we use it directly
		converted_thickness = static_cast<int>(thickness * 127.0f);
	}

	uint32_t col = 0;
	col |= static_cast<uint8_t>(hue * 255.0f);           // First 8 bits are hue
	col |= static_cast<uint8_t>(intensity * 255.0f) << 8; // Next 8 bits are intensity
	col |= static_cast<uint8_t>(alpha * 255.0f) << 16;    // Next 8 bits are alpha
	col |= static_cast<uint8_t>(converted_thickness) << 24; // Last 8 bits are thickness
	return col;
}

#endif // LINE_COLOR_GEN_HPP