#pragma once

#include <istream>
#include <memory>

#include "config.hpp"
#include "states.hpp"

struct GameConfig;
struct GameState;
struct DroneState;

struct DroneBehavior {
    DroneBehavior(DroneState &drone);
    virtual void TryChange() = 0;
    virtual void Process() = 0;
    virtual ~DroneBehavior() = default;
    virtual unique_ptr<DroneBehavior> getCopy(DroneState &drone) const = 0;

    DroneState &drone;
};

struct DroneState {
    DroneState(int droneId);
    DroneState(const DroneState &other);
    void parseInput(istream& in);
    void wait(bool useLight, string message);
    void move(Coord position, bool useLight, string message);
    static void runAllOwnDrones();

    int id;
    Coord position;
    int emergency;
    int battery;
    IntSet currentScans;
    RadarMap bleeps;
    unique_ptr<DroneBehavior> currentBehavior;
};
using DroneStateVec = vector<DroneState>;


