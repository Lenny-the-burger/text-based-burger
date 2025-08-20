# Text-Based Burger Mesh Editor Documentation

## Overview

The Text-Based Burger Mesh Editor is a 2D line-based drawing tool designed for creating and editing "meshes" for the Text-Based Burger game engine. These meshes consist of collections of line segments that can be used to create visual elements, collision boundaries, or other geometric structures within the game.

### Key Characteristics
- **2D Only**: Meshes are strictly two-dimensional, consisting of line segments in 2D space
- **Non-indexed Lines**: Line segments are independent and do not need to be connected
- **Arbitrary Order**: Lines can be drawn and stored in any order
- **JSON Storage**: Meshes are saved in a simple JSON format for easy editing and version control

## Features

### Core Drawing Tools
- **Draw Tool**: Create new line segments by clicking and dragging
- **Select Tool**: Select individual lines or use box selection for multiple lines
- **Edit Tool**: Modify existing lines by dragging vertices

### Selection and Editing
- **Single Line Selection**: Click on any line to select it
- **Multi-Line Selection**: Use box selection (click and drag) to select multiple lines
- **Multi-Line Operations**: Move, delete, or modify multiple selected lines simultaneously
- **Vertex Editing**: Drag individual line endpoints to reshape lines

### Snapping Features
- **Vertex Snapping**: Snap new lines to existing vertex points for precise alignment
- **Grid Snapping**: Snap to a regular grid for consistent spacing and alignment
- **Configurable Tolerance**: Adjustable snap radius and grid size

### Undo/Redo System
- **History Stack**: Maintains up to 50 previous states for undo/redo operations
- **State Preservation**: Automatically saves states after significant operations
- **Selection Recovery**: Handles selection state changes during undo/redo

### File Operations
- **JSON Import/Export**: Load and save meshes in JSON format
- **Mesh Storage**: Files stored in `gamedata/meshes/` directory
- **Format Compatibility**: Compatible with game engine mesh loading system

### Visual Customization
- **Line Colors**: Adjustable hue, intensity, and alpha properties
- **Line Thickness**: Configurable line thickness for visual distinction
- **Z-Height**: Support for layered rendering with z-height values

## Architecture and Design

### Core Components

#### LineCanvas Class
The heart of the mesh editor is the `LineCanvas` class, which inherits from `GameObject` and provides all drawing and editing functionality.

**Location**: `game_object.h` and `game_object.cpp`

**Key Responsibilities**:
- Mouse input handling and tool behavior
- Line storage and management
- Selection system implementation
- Undo/redo state management
- File import/export operations

#### UI System Integration
The editor uses a JSON-based UI system that defines the interface layout and button interactions.

**UI Definition**: `gamedata/ui/mesh_editor_ui.json`
**Script Integration**: `scripts.cpp` contains functions that bridge UI interactions to LineCanvas operations

### Data Structures

#### Line Storage
```cpp
std::vector<vec2> canvas_lines;        // Line endpoints (pairs of vec2)
std::vector<int> canvas_colors;        // Color indices for each line
std::vector<float> canvas_z_height;    // Z-height values for layering
```

#### Selection System
```cpp
int selected_line = -1;                           // Currently selected line
std::vector<int> selected_lines;                 // Multi-selection list
bool is_box_selecting = false;                   // Box selection state
vec2 box_selection_start, box_selection_end;     // Box selection coordinates
```

#### History Management
```cpp
struct HistoryState {
    std::vector<vec2> lines;
    std::vector<int> colors;
    std::vector<float> z_heights;
};
std::vector<HistoryState> history_stack;
size_t current_history_index = 0;
static const size_t max_history_size = 50;
```

### Tool System

#### Canvas Tools
```cpp
enum CanvasTool {
    CANVAS_TOOL_SELECT,     // Selection and multi-selection
    CANVAS_TOOL_DRAW_LINE,  // Line drawing
    CANVAS_TOOL_EDIT        // Vertex editing
};
```

Each tool implements different mouse interaction behaviors:
- **Draw Tool**: Creates new lines on click-and-drag
- **Select Tool**: Handles single/multi-selection and line movement
- **Edit Tool**: Allows vertex manipulation of existing lines

## User Interface

### Layout Structure

The mesh editor UI consists of two main regions:

#### Top Menu Bar
- **File**: Save mesh functionality
- **Edit**: Load mesh functionality  
- **Exit**: Close editor and return to main application

#### Left Toolbar
- **Draw**: Activate line drawing tool
- **Select**: Activate selection tool
- **Edit**: Activate vertex editing tool
- **Snap: On/Off**: Toggle vertex snapping
- **Grid: On/Off**: Toggle grid snapping

### UI Configuration

The interface is defined in `gamedata/ui/mesh_editor_ui.json`:

```json
{
  "stencil_regions": [ [80, 0], [960, 520] ],
  "stencil_state": 0,
  "root": [
    {
      "targetname": "toolbar",
      "type": "container",
      "position": {"x": 0, "y": 0},
      "bg_empty": 70,
      "bbox": [ [0, 0], [119, 0] ],
      "children": [
        // Top menu buttons (File, Edit, Exit)
      ]
    },
    {
      "targetname": "canvas tool panel", 
      "type": "container",
      "position": {"x": 0, "y": 0},
      "bg_empty": 70,
      "bbox": [ [0, 1], [9, 33] ],
      "children": [
        // Tool buttons (Draw, Select, Edit, Snap controls)
      ]
    }
  ]
}
```

## File Format Specification

### Mesh JSON Format

Meshes are stored in JSON format with the following structure:

```json
{
  "mesh_name": [x1, y1, x2, y2, x3, y3, x4, y4, ...],
  "another_mesh": [x1, y1, x2, y2, x3, y3, ...]
}
```

#### Format Details
- **Keys**: Arbitrary string identifiers for each mesh (used to reference the mesh in the game engine)
- **Values**: Arrays of floating-point numbers representing coordinates in sequence [x1, y1, x2, y2, ...] where each pair of coordinates represents a line endpoint
- **Line Storage**: All lines within a mesh are stored in a single flat array, with every two coordinate pairs forming one line segment
- **Coordinates**: 2D coordinates in the mesh coordinate space
- **Global Scope**: Mesh names are globally scoped across all mesh files

#### Example Mesh File
```json
{
  "rectangle_mesh": [
    260.0, 420.0, 260.0, 140.0,
    260.0, 420.0, 640.0, 420.0, 
    640.0, 420.0, 640.0, 140.0,
    260.0, 140.0, 640.0, 140.0
  ]
}
```

This example creates a mesh named "rectangle_mesh" containing four line segments that form a rectangle with corners at (260,140), (640,140), (640,420), and (260,420). Each set of four numbers represents one line: start_x, start_y, end_x, end_y.

### Storage Location
- **Directory**: `gamedata/meshes/`
- **File Extension**: `.json`
- **Default Save**: `saved_mesh.json`

## Implementation Details

### Key Methods

#### LineCanvas Core Methods

**Tool Management**:
```cpp
void set_active_tool(CanvasTool tool);
CanvasTool get_active_tool() const;
```

**Drawing Operations**:
```cpp
void on_press(ObjectUpdateData data);    // Handle mouse press
void on_drag(ObjectUpdateData data);     // Handle mouse drag
void on_click(ObjectUpdateData data);    // Handle mouse click
void on_release(ObjectUpdateData data);  // Handle mouse release
```

**Selection System**:
```cpp
int find_line_at_position(vec2 pos, float tolerance = 10.0f);
void find_lines_in_box(vec2 box_min, vec2 box_max, std::vector<int>& lines_in_box);
void clear_selection();
void add_line_to_selection(int line_index);
bool is_line_selected(int line_index) const;
```

**Snapping Functions**:
```cpp
vec2 find_snap_position(vec2 pos, bool exclude_current_line = false);
vec2 find_nearest_vertex(vec2 pos, bool exclude_current_line = false);
vec2 snap_to_grid(vec2 pos);
void toggle_snapping();
void toggle_grid_snapping();
```

**History Management**:
```cpp
void save_state_to_history();
void undo();
void redo();
bool can_undo() const;
bool can_redo() const;
```

**File Operations**:
```cpp
bool save_mesh_to_file(const std::string& filename);
bool load_mesh_from_file(const std::string& filename);
json export_mesh_to_json() const;
bool import_mesh_from_json(const json& mesh_data);
```

#### Script Integration

The UI buttons trigger script functions that interact with the LineCanvas:

**Tool Selection**:
```cpp
void set_canvas_tool(json data, ScriptHandles handles);
```

**Snapping Controls**:
```cpp
void toggle_snapping(json data, ScriptHandles handles);
void toggle_grid_snapping(json data, ScriptHandles handles);
```

**File Operations**:
```cpp
void save_mesh_file(json data, ScriptHandles handles);
void load_mesh_file(json data, ScriptHandles handles);
```

**Property Updates**:
```cpp
void update_canvas_color_from_ui(json data, ScriptHandles handles);
```

### Configuration Properties

#### Snapping Configuration
```cpp
bool snapping_enabled = false;           // Vertex snapping state
bool grid_snapping_enabled = false;      // Grid snapping state  
float snap_radius = 15.0f;               // Snapping tolerance in pixels
float grid_size = 20.0f;                 // Grid spacing in units
```

#### Visual Properties
```cpp
float current_hue = 0.75f;               // Line hue (0.0-1.0)
float current_intensity = 0.75f;         // Line intensity (0.0-1.0)
float current_alpha = 1.0f;              // Line alpha (0.0-1.0)
float current_thickness = 0.0f;          // Line thickness
```

## Usage Instructions

### Basic Drawing Workflow

1. **Select Draw Tool**: Click the "Draw" button in the left toolbar
2. **Enable Snapping** (optional): Toggle "Snap" or "Grid" buttons as needed
3. **Draw Lines**: Click and drag to create line segments
4. **Switch Tools**: Use "Select" or "Edit" tools to modify existing lines

### Selection and Editing

1. **Select Tool**: Click "Select" button to activate selection mode
2. **Single Selection**: Click on any line to select it
3. **Multi-Selection**: Click and drag to create a selection box around multiple lines
4. **Move Lines**: Drag selected lines to move them
5. **Delete Lines**: Press Delete key while lines are selected

### Vertex Editing

1. **Edit Tool**: Click "Edit" button to activate vertex editing mode
2. **Select Line**: Click on a line to select it for editing
3. **Drag Vertices**: Click and drag line endpoints to reshape the line
4. **Precision**: Use snapping features for precise vertex placement

### File Management

1. **Save Mesh**: Click "File" button to save current mesh to `gamedata/meshes/saved_mesh.json`
2. **Load Mesh**: Click "Edit" button to load mesh from `gamedata/meshes/saved_mesh.json`
3. **Clear Canvas**: Load an empty mesh or manually delete all lines

### Snapping Features

#### Vertex Snapping
- **Purpose**: Snap new line endpoints to existing vertices
- **Activation**: Click "Snap: Off" to toggle to "Snap: On"
- **Behavior**: When drawing near existing vertices (within snap_radius), new points will snap to exact vertex positions

#### Grid Snapping  
- **Purpose**: Snap to regular grid intersections
- **Activation**: Click "Grid: Off" to toggle to "Grid: On"
- **Behavior**: All drawing and editing operations will snap to grid points spaced by grid_size units

### Undo/Redo Operations

- **Undo**: Use standard undo keyboard shortcut (implementation-dependent)
- **Redo**: Use standard redo keyboard shortcut (implementation-dependent)  
- **History Limit**: Up to 50 previous states are maintained
- **Auto-Save**: States are automatically saved after drawing, editing, or deleting operations

## Technical Notes

### Coordinate System
- Origin (0,0) is typically at the top-left of the canvas
- Positive X extends right, positive Y extends down
- Coordinates are floating-point values for sub-pixel precision

### Performance Considerations
- Line storage uses efficient vector containers
- Selection operations use spatial queries with configurable tolerance
- History system limits memory usage to 50 states maximum

### Integration with Game Engine
- Meshes can be loaded by the game engine for collision detection
- Line rendering uses the engine's custom line renderer
- Coordinate space matches game world coordinates

### Future Extensions
- The mesh editor serves as foundation for a planned map editor
- Additional tools and features may be added in future versions
- File format is designed to be extensible while maintaining backward compatibility

## Limitations

### Current Implementation Status
- **File Dialogs**: Save/load operations use fixed filenames instead of file picker dialogs
- **Limited File Management**: No support for multiple mesh files or mesh organization
- **Basic UI**: Simple button-based interface without advanced UI features

### Design Constraints  
- **2D Only**: No support for 3D coordinates or operations
- **Line Segments Only**: Cannot create curves, circles, or other geometric primitives
- **Global Namespace**: All mesh names share global scope across files
- **No Grouping**: Lines cannot be grouped or organized into hierarchical structures

This documentation provides a comprehensive overview of the Text-Based Burger Mesh Editor's features, architecture, and usage. For additional implementation details, refer to the source code in `game_object.h`, `game_object.cpp`, and related files.