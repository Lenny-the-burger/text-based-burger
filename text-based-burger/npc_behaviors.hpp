#pragma once

// This file holds various npc related things. This is a header only file. Make
// sure to declare all variables as static.

#include <unordered_map>
#include <string>

#define NPC_FACTION_COUNT 5

enum NPCFaction {
	// No specific faction allegiance. Does not generate threats.
	FACTION_NONE,

	FACTION_ANOMALY, // Anomalies (the actual scps)

	FACTION_SCP, // SCP Foundation
	FACTION_CI, // Chaos Insurgency
	FACTION_GOC, // Global Occult Coalition
};

// threat to me that i know about
struct NPCThreat {
	// Go off of npc ids instead of string targetnames
	int thread_npc_id;
	bool can_see_threat;

	// If we cant see use previous known info
	vec2 prev_position;
	float prev_agression;
	double time_last_seen;
};

static const bool faction_relations[NPC_FACTION_COUNT][NPC_FACTION_COUNT] = {
	//   NONE   ANOM   SCP    CI     GOC
		{false, true , false, false, false}, // NONE
		{true , true , true , true , true }, // ANOMALY
		{false, true , false, true , false}, // SCP
		{false, true , true , false, true }, // CI
		{false, true , false, true , false}  // GOC
};

static const std::unordered_map<std::string, NPCFaction> npc_faction_map = {
	{"none", FACTION_NONE},
	{"anomaly", FACTION_ANOMALY},
	{"scp", FACTION_SCP},
	{"ci", FACTION_CI},
	{"goc", FACTION_GOC}
};