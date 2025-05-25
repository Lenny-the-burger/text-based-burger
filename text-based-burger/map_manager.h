#pragma once

// This class handles the loading and rendering of a map. It also handles
// things like fetching relevant geometry for other system for things like
// collision detection, etc.

// Maps are encoded in a binary format (for once not json can you imagine?)
// There will probably be a header eventually but i dont know what to put in
// it right now. Map files only contain the geometry in a .vdg file, (vector
// display geometry) and the rest of the data is in a .json file, like the
// entities, brushes, etc.
// 
// You can just load a raw vdg file but it will be missing everything that
// makes it an actual map. See dev documentation for more details on this.
// 
// Format is per line that gets rendered, (parralax lines dont count as they
// are generated dynamically).
// 
// ---- format ----
// 
// 32 bit uint: x component of first vertex
// 32 bit uint: y component of first vertex
// 32 bit uint: x component of second vertex
// 32 bit uint: y component of second vertex
// 
// Lines only have a flat height, and shouldnt reach very high. Keep in mind
//     that this is a purely cosmetic effect and has no effect on gameplay.
// 8 bit uint: z component of entire line
//
// We only have 8 bit monochrome color.
// 8 bit uint: color of line
// 
// Lines have several basic types:
// 0: normal line (collision, parralax)
// 1: cosmetic line (no collision, no parralax)
// 
// More advanced types should be handled by brush entities, this is mostly
// intended for general purpouse brushless lines.
// 8 bit uint: line type
// 
// 255 misc flags for whatever might be needed, like maybe dashed line?
// 8 bit uint: misc flags
// 
// 32 bit uint: Id of brush it is part of. Brushes are groups of lines that can
//     have special properties, like being a door or a window. They are a type
//	   of entity and are stored in the json file. Id of 2^32 means no brush.
//
// ---- end ----
//
// Right now lines only take up 192 bits, and 64 at the end are tacked on as
// "unused" space. This is just for future proofing, if you are doing something
// that requires more flags then use these (if misc flags isnt enough).
//
// Lines are packed like this and one map may have several thousand lines, 
// especially the more expansive ones.