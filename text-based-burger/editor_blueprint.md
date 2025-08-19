# Blueprint for the Text-Based Burger Mesh Editor
This file contains a blueprint for the text-based burger mesh editor, which is a tool designed to create and edit "meshes" for the engine. These "meshes" are actually only two dimentional, not full 3d meshes, and consist of a list of lines. The lines are non indexed abnd arbitrary, meaning that they can be drawn in any order and do not need to be connected to each other. 

In the text based burger engine there are two primary types of graphical elements: lines and text. Text is rendered as a bitmap font, and lines are rendered similarly to gl lines (but internally uses a custom renderer).

Meshes are strictly 2 dimentional. In the future the there will be a map editor that builds on top of the regular mesh editor, but that is later.

Meshes are stored in gamedata/meshes/ in a plaintext json format. They are stored in the format `{"name": [x1, y1, x2, y2]}` with each pair of coordinates representing a line segment. The name is used to identify the mesh in the game engine. Note that meshes are global scoped, so even though you can have multiple files with meshes that you load dynamically, beware of name collisions. Meshes are stored in a flat map, so cannoot have "folders" or subdirectories.

## Mesh Editor UI
The mesh editor has a simple ui with a pretty minial design. There are two main region: the left toolbar area, and a "dropdown" bar at the very top for like file/edit/view etc. Actual dropdown funcionality is not implemented, for now its just regular buttons. See `mesh_editor_ui.json`.

## Mesh editor implementation
The core of the editor is a `LineCanvas` object. It handles most of the interaction. Buttons in the text UI use script calls to interact with it, such as setting the current tool and toggling snapping. `LineCanvas` is a game object, and so resides in `game_object.cpp/h` files.

## Mesh Editor Features
Some features are already implemented, those are marked with a checkmark "[x]". Features that are not yet implemented are marked with an empty box "[ ]".

## Mesh Creation
- [x] Draw lines individually by clicking and dragging the mouse.
- [x] Togglable snapping to other vertices (note that the snapping modes affect all tools, not just the line drawing tool).
- [x] Togglable snapping to a grid.

## Mesh Editing
- [x] Select lines and move vertices by clicking and dragging.
- [x] Delete lines by selecting them and pressing the delete key.
- [x] Undo/redo functionality for mesh edits. This will require a history stack to be implemented in the editor.

## Select tool
- [x] Select lines by clicking on them.
- [x] Box selection functionality to select multiple lines at once.
- [x] Once multiple lines are selected they should be movable by clicking and dragging (translation), and should be deletable by pressing the delete key like single lines.

## Mesh Saving/Loading
- [ ] Save the current mesh to a file. This can probably just use the default file picker dialogue, editor is not part of the game.
- [ ] Load a mesh from a file. This can also use the default file picker dialogue.
Both are partially implemented, file io needs to be fixed and finished.