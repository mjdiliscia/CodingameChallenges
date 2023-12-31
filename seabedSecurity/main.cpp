#include <iostream>

#include "states.hpp"
#include "drone.hpp"

int main() {
    GameConfig& config = GameConfig::get();
    GameState& state = GameState::get();

    while (true) {
        state.parseInput(cin);
        state.own.calculateRemainingCreatures();
        DroneState::runAllOwnDrones();
    }
}
