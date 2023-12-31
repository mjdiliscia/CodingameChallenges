#pragma once

#include <vector>
#include <istream>

#include "config.hpp"
#include "creature.hpp"

struct DroneState;
using DroneStateVec = vector<DroneState>;

struct GameConfig {
    static GameConfig& get();
    GameConfig(bool initialize = false);
    void parseInput(istream& in);

    CreatureSet creatures;
    IntSet enemies;
};

struct PlayerState {
    void parseDrones(istream& in);
    void parseScans(istream& in);
    void parseDronesScans(istream& in);
    void parseDronesRadar(istream& in);

    DroneStateVec::iterator getDroneState(int droneId);
    void calculateRemainingCreatures();

    int score;
    IntSet totalScans;
    DroneStateVec drones;
    IntSet remainingCreatures;
};

struct GameState {
    static GameState& get();
    GameState(bool initialize = false);
    void parseInput(istream& in);
    void parseVisibleEntities(istream& in);

    PlayerState own;
    PlayerState foe;
    CreatureStateSet visibleCreatures;
    CreatureStateSet visibleEnemies;
};

