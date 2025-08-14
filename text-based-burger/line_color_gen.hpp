#include <cstdint>
#include <unordered_map>

#ifndef LINE_COLOR_GEN_HPP
#define LINE_COLOR_GEN_HPP

// Line type presets you can use
enum LineTypePreset {
	LINE_COLOR_PRESET_CURSOR, // Mouse cursor lines
	LINE_COLOR_PRESET_NPC_FRIENDLY,
	LINE_COLOR_PRESET_NPC_FRIENDLY_SELECTED,
	LINE_COLOR_PRESET_NPC_ENEMY,
	LINE_COLOR_PRESET_WALL_GENERIC,
	LINE_COLOR_PRESET_WALL_SECONDARY
};

// Lookup table for presets - more efficient than switch statement
static const std::unordered_map<LineTypePreset, uint32_t> preset_colors = {
	{LINE_COLOR_PRESET_CURSOR,              64 | (0 << 8) | (255 << 16)}, // 255 hue, 127 intensity, 255 alpha
	{LINE_COLOR_PRESET_NPC_FRIENDLY,        0x00FF7FFF}, // 255 hue, 127 intensity, 255 alpha
	{LINE_COLOR_PRESET_NPC_FRIENDLY_SELECTED, 0x00FFFFFF}, // 255 hue, 255 intensity, 255 alpha
	{LINE_COLOR_PRESET_NPC_ENEMY,           0x00FF7FFF}, // 255 hue, 127 intensity, 255 alpha
	{LINE_COLOR_PRESET_WALL_GENERIC,        0x00FFBFBF}, // 191 hue, 191 intensity, 255 alpha
	{LINE_COLOR_PRESET_WALL_SECONDARY,      0x007FBFBF}  // 191 hue, 191 intensity, 127 alpha
};

// First 8 bits are hue, next are intensity, next are alpha. 8 unused.
inline uint32_t generate_line_color(LineTypePreset preset) {
	auto it = preset_colors.find(preset);
	return (it != preset_colors.end()) ? it->second : 0x00FFFFFF; // Default to all 255
}

// Generate color from given colors. Colors should be 0-1 range.
inline uint32_t generate_line_color(float hue, float intensity, float alpha) {
	uint32_t col = 0;
	col |= static_cast<uint8_t>(hue * 255.0f);           // First 8 bits are hue
	col |= static_cast<uint8_t>(intensity * 255.0f) << 8; // Next 8 bits are intensity
	col |= static_cast<uint8_t>(alpha * 255.0f) << 16;    // Next 8 bits are alpha
	return col;
}

#endif // LINE_COLOR_GEN_HPP