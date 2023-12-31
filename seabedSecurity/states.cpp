#include <cassert>
#include <iostream>
#include <algorithm>

#include "states.hpp"
#include "drone.hpp"

/****** GameConfig ******/
GameConfig& GameConfig::get() {
    static GameConfig gameConfig(true);
    return gameConfig;
}

GameConfig::GameConfig(bool initialize) {
    if (initialize)
        parseInput(cin);
}

void GameConfig::parseInput(istream& in) {
    int creatureCount;
    in >> creatureCount;
    in.ignore();

    creatures.clear();
    enemies.clear();
    FORI(creatureCount) {
        Creature creature;
        in >> creature.id >> reinterpret_cast<int &>(creature.color) >> reinterpret_cast<int &>(creature.type);
        in.ignore();
        if (creature.type != Type::ENEMY)
            creatures.insert(creature);
        else
            enemies.insert(creature.id);
    }
}

/****** PlayerState ******/
void PlayerState::parseDrones(istream& in) {
    int droneCount;
    in >> droneCount;
    in.ignore();

    FORI(droneCount) {
        int droneId;
        in >> droneId;
        auto droneIter = getDroneState(droneId);
        if (droneIter != drones.end()) {
            droneIter->parseInput(in);
        } else {
            DroneState drone(droneId);
            drone.parseInput(in);
            drones.push_back(drone);
        }
    }
}

void PlayerState::parseScans(istream& in) {
    int scanCount;
    in >> scanCount;
    in.ignore();

    FORI(scanCount) {
        int id;
        in >> id;
        in.ignore();
        totalScans.insert(id);
    }
}

void PlayerState::parseDronesScans(istream& in) {
    int scanCount;
    in >> scanCount;
    in.ignore();

    FORI(scanCount) {
        int droneId, creatureId;
        in >> droneId >> creatureId;
        in.ignore();

        auto droneIter = getDroneState(droneId);
        assert (droneIter != drones.end());
        droneIter->currentScans.insert(creatureId);
    }
}

void PlayerState::parseDronesRadar(istream& in) {
    int blipCount;
    in >> blipCount;
    in.ignore();

    FORI(blipCount) {
        int droneId;
        int creatureId;
        string radar;
        in >> droneId >> creatureId >> radar;
        in.ignore();

        auto droneIter = getDroneState(droneId);
        droneIter->bleeps[getQuadrant(radar)].insert(creatureId);
    }
}

DroneStateVec::iterator PlayerState::getDroneState(int droneId) {
    return find_if(drones.begin(), drones.end(), [droneId](DroneState& ds) { return ds.id == droneId; });
}

void PlayerState::calculateRemainingCreatures() {
    GameConfig& config = GameConfig::get();

    IntSet presentCreaturesIds;
    for (auto &drone : drones)
        for (auto &qPairs : drone.bleeps)
            for (int creatureId : qPairs.second)
                if (!config.enemies.count(creatureId))
                    presentCreaturesIds.insert(creatureId);

    IntSet knownCreatureIds;
    for (auto &drone : drones) {
        for (int creatureId : drone.currentScans)
            knownCreatureIds.insert(creatureId);
    }
    for (int creatureId : totalScans)
        knownCreatureIds.insert(creatureId);

    remainingCreatures.clear();
    for (auto &creature : presentCreaturesIds)
        if (!knownCreatureIds.count(creature))
            remainingCreatures.insert(creature);
}

/****** GameState ******/
GameState& GameState::get() {
    static GameState gameState(false);
    return gameState;
}

GameState::GameState(bool initialize) {
    if (initialize)
        parseInput(cin);
}

void GameState::parseInput(istream& in) {
    in >> own.score;
    in.ignore();
    in >> foe.score;
    in.ignore();

    own.parseScans(in);
    foe.parseScans(in);
    own.parseDrones(in);
    foe.parseDrones(in);
    own.parseDronesScans(in);
    parseVisibleEntities(in);
    own.parseDronesRadar(in);
}

void GameState::parseVisibleEntities(istream& in) {
    visibleCreatures.clear();
    visibleEnemies.clear();

    int visibleCreatureCount;
    cin >> visibleCreatureCount;
    cin.ignore();

    FORI(visibleCreatureCount) {
        CreatureState creatureState;
        cin >> creatureState.id;
        cin >> creatureState.position.x >> creatureState.position.y;
        cin >> creatureState.velocity.x >> creatureState.velocity.y;
        cin.ignore();

        if (GameConfig::get().enemies.count(creatureState.id))
            visibleEnemies.insert(creatureState);
        else
            visibleCreatures.insert(creatureState);
    }
}

