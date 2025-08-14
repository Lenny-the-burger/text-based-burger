#pragma once

#include "json.hpp"
using json = nlohmann::json;

#include "object_utils.h"
#include "math_utils.h"
#include "npc_behaviors.hpp"

#include <vector>
#include <string>
#include <map>

// This is all the game objects. Game objects are responcible for their own
// interaction and submitting their own rendering data to the handler.

enum CollisionType {
	COLLISION_TYPE_NONE = 0, // No collision
	COLLISION_TYPE_AABB = 1, // Axis aligned bounding box collision
	COLLISION_TYPE_CIRCLE = 2 // Circle collision
};

class GameObject {
public:
	// Game objects only have once constructor because they dont need
	// Recursive construction like components do
	GameObject(json data, ObjectIO& io);

	// This is the update function. It is called every frame and should
	// return true if the object needs to be rerendered
	virtual void update(ObjectUpdateData data);

	// Render function is called every frame. You are given a pointer to
	// an array and should append yourself to it if you need to be rendered.
	// Not appending yourself will cause you to not be rendered, even if you
	// were rendered the previous frame. The entire array is cleared. For optimization
	// you should only render if you are within some distance of the camera position
	// you get from updata data. Note that you should output NDC here not world space.
	virtual int render(float* lines_list, int offset, uint32_t* colors, vec2 camera);

	// Attempt to move in the direction and distance of velocity. note that this
	// is an attempt to move, you are given the raw desired velocity and it is
	// up to you to do collision or translate it into some other paramaters if
	// you are a car for example.
	virtual void move(vec2 velocity);

	std::string targetname;

	// This is the color of the object
	uint32_t color;

	// This is the object io instance. It is used to call scripts and report errors
	ObjectIO& io;

	// Name of the (mesh?) that this object uses
	std::string mesh;

	std::string update_script_name;

	// Safe to assume all objects have a world space position
	vec2 position;

	// This is the rotation of the object in radians. This should only be used
	// for rendering, you should not have rotating collision, use a circle to
	// approximate the collision front instead.
	float rotation;

	// This is the scale of the object
	vec2 render_scale;

	CollisionType collision_type;

	// Radius if you are COLLISION_TYPE_CIRCLE 
	float radius;

	// AABB if you are COLLISION_TYPE_AABB
	vec2 aabb_from, aabb_to;

protected:
	// For now everything is publically acessible becasuse getters and setters are a waste of time a lot of the time
};

// Type selector for game objects
// This function LOVES to explode the linker. Or not this function, but VS loves
// to. There is nothing wrong, you probably just edited a contructor or
// something, and now visual studio is angry. Just clean and rebuild, it will
// fix it.
std::unique_ptr<GameObject> object_type_selector(json data, ObjectIO& io);

enum MouseState {
	MOUSE_NORMAL,
	MOUSE_CLICKING,
	MOUSE_DRAGGING
};

// Just renders the mouse cursor. This object cannot be created from JSON
// and only one exists, specially held by the ObjectsHandler
class MouseRenderer : public GameObject {
public:
	MouseRenderer(ObjectIO& io);

	virtual void update(ObjectUpdateData data) override;

	MouseState mouse_state;
};

enum CameraMode {
	// "welds" the camera to target. No sway, no rotation.
	CAMERA_MODE_WELD,

	// "follow" camera, follows the target with some random sway
	CAMERA_MODE_FOLLOW,

	// "gameplay" camera, follows the target with random sway + look ahead in
	// aiming direction.
	CAMERA_MODE_GAMEPLAY 
};

// Controls the camera. You give it a targetname and some other settings, and it
// spits out the properly orientated camera position and rotation. You could
// probably create multiple and switch between them, but its better to retarget
// one and switch modes. Keep in mind that when switching targets, the camera
// will continue to move as expected between them, so if you want it to just
// snap switch it to weld mode before retargeting. This is usually used in
// conjunction with the possesor object, although not always.
class PointViewControl : public GameObject {
public:
	PointViewControl(json data, ObjectIO& io);

	virtual void update(ObjectUpdateData data) override;

	void set_mode(CameraMode mode);

	void set_target(std::string targetname);

protected:
	std::string follow_name;
	CameraMode mode;
	vec2 prev_position;
	float rotation;
};

// Possessor is the main input method for gameplay. Attaches to a gameobject and
// translates keyboard inputs into movement of the object. Note that all this
// class does is get the inputs and call move(direction, distance) on the thing
// its possesing. The actual NPCs/vehicles are responcible for implementing
// their move and collision. 
// !! ATTENTION: !! Do not attach point_viewcontrol to the possesor, it itself
// does not move, attach it to the object it is possesing. they are seperate
// because sometimes you want to follow an object but not posses it.
class Possessor : public GameObject {
public:
	Possessor(json data, ObjectIO& io);

	virtual void update(ObjectUpdateData data) override;

	// Set the targetname of the object to possess. Set to empty string to stop
	// possessing.
	void set_target(std::string targetname);

protected:
	std::string victim_name;
};

// How the agression mechanic works:
// Certian weapons and actions impact your agression level. For example, even
// walking around with an M249 will very quickly increase your agression level.
// It goes from 0 to 1, where 0 is no agression and 1 is maximum agression.
// Higher agression means npcs are less likely to fight back, even if able to,
// and affects things like threat responce, like what do police get deployed.
//
// There arent really any things that can activly lower your agression, apart
// from like holstering your weapon. If a civvie sees you weilding a machine
// gun, and then see you with a pistol they will still remember the machine gun
// because obviously.
// There are many things that can increase agression, firing weapons for
// example. If you fire a pistol, it goes up a little bit, but if you fire a
// scarier weapon it goes up more, proportional to the weapons inherent
// agression. Other specific actions, like shooting at civillians or setting off
// explosions also increase agression. Shooting at armed people increases your
// agression, but by less than shooting at unarmed people. Weapons can also be
// holstered, which makes them not visible and so do not affect agression, but
// this depends on the weapon size. Anything over 6 is too big to be holstered,
// so will not be able to be hidden completly.

// Loadouts:
// NPCs have outfits and a set of weapons. Outfits are basically just a thing to
// attach passive stats too, like an overt military outfit will give off higher
// agression than plainclothes. NPCs have 10 weapon size slots, and each weapon
// have a size. For example, a rifle is 4, a pistol is 2 and a grenade is 1.
// That means as a loadout you could do: RRRRP PGGGG, where R is rifle, P is
// pistol and G is grenade. This means that you could carry a rifle, a pistol,
// and up to 4 grenades. Bigger weapons have bigger sizes, like an M249 would be
// 6, so you could have at most the M249 and a rifle. This is meant to emulate
// size and weight restrictions so you couldnt go into battle with a machine
// gun, rocket, launcher, rifle all at once. It does mean you can go into battle
// with 10 grenades. That is stupid, but believable.
// A rocket launcher may need physical ammo as opposed to just having infinite
// bullets like guns, and they may take up space aswell, for balance. An RPG
// would probably kill everything you pointed it at, So only having 4 shots is a
// good idea. Example: RRRRR RAAPP, where R is rpg, A is ammo, and p is pistol.
// Here, each A is worth two rockets, so an rpg with four shots and a pistol.
// Seems like a fine setup to blow stuff up

// Base npc class
class NPC : public GameObject {
public:
	NPC(json data, ObjectIO& io);

	virtual void update(ObjectUpdateData data) override;
	virtual void move(vec2 velocity) override;

	virtual void aim(vec2 direction);
	virtual void attack();

	void give_weapon(std::string weapon_name);
	void steal_weapon(std::string weapon_name);

	// Remember to set this to false or hes gonna be braindead
	void set_possessed(bool possessed) {
		is_possessed = possessed;
	}

protected:
	std::vector<std::string> weapons;
	NPCFaction faction;

	// Dont try to move if your possessed (unless your into that)
	bool is_possessed = false;

	// How likely are we going to try to fight back. 0 being always flee 1 being
	// juggernaut.
	float bravery;

	// if false then we are activly holding a gun. Any weapon size <5 can be
	// fully concealed. No sneaking around with stingers sorry boys.
	bool weapon_holstered;

	vec2 aim_direction;
	vec2 move_velocity;

	// Depending on bravery and enemy's agression we either flee this guy as
	// much as we can or seek him out. Mortal enemies only happen if the enemy
	// does soemthing bad to you, like shoots and hits you, shoots and hits one
	// of your friends, sets of big gun or explosion next to you etc. Mortal
	// enemies only get unset if they die, or a new one is set.
	std::string mortal_enemy;

	std::string outfit;
};

enum CanvasTool {
	CANVAS_TOOL_SELECT,
	CANVAS_TOOL_DRAW_LINE,
	CANVAS_TOOL_EDIT
};

// Line canvas is a special object that lets you draw lines on the screen. Used for mesh and map editors.
class LineCanvas : public GameObject {
public:
	LineCanvas(json data, ObjectIO& io);

	virtual void update(ObjectUpdateData data) override;

	// Canvas renders in a special way
	virtual int render(float* lines_list, int offset, uint32_t* colors, vec2 camera);

	void set_active_tool(CanvasTool new_tool) {
		tool = new_tool;
	}

protected:
	void on_press(ObjectUpdateData data);
	void on_click(ObjectUpdateData data);
	void on_release(ObjectUpdateData data);

	bool are_drawing = false;

	// This is mostly used when editing for holding what line we are editing
	int active_line = 0;
	int active_color = 0;
	
	CanvasTool tool = CANVAS_TOOL_DRAW_LINE;

	std::vector<vec2> canvas_lines;
	std::vector<int> canvas_colors;
	std::vector<float> canvas_z_height;

	bool do_parrallax = false;

	int canvas_line_count = 0;

	// Click tracking
	bool is_clicking = false;
	bool is_click_start_inside = false;
	bool have_already_fired = false;
};