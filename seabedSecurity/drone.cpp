#include <memory>
#include <iostream>

#include "drone.hpp"
#include "droneBehaviors.hpp"
#include "states.hpp"

DroneBehavior::DroneBehavior(DroneState &drone) : drone(drone) {
}

DroneState::DroneState(int droneId) : id(droneId) {
    currentBehavior = make_unique<DBSurfacing>(*this);
}

DroneState::DroneState(const DroneState &other)
    : id(other.id), position(other.position), emergency(other.emergency), battery(other.battery),
      currentScans(other.currentScans), bleeps(other.bleeps) {
    currentBehavior = other.currentBehavior->getCopy(*this);
}

void DroneState::parseInput(istream &in) {
    in >> position.x >> position.y >> emergency >> battery;
    in.ignore();

    currentScans.clear();
    bleeps.clear();
}

void DroneState::wait(bool useLight, string message) {
    cout << "WAIT " << (useLight ? "1" : "0") << " " << message << endl;
}

void DroneState::move(Coord position, bool useLight, string message) {
    cout << "MOVE " << position.x << " " << position.y << " " << (useLight ? "1" : "0") << " " << message << endl;
}

void DroneState::runAllOwnDrones() {
    GameState &state = GameState::get();
    for (auto &drone : state.own.drones) {
        drone.currentBehavior->TryChange();
        drone.currentBehavior->Process();
    }
}
