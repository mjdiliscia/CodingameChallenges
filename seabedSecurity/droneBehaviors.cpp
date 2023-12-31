#include <optional>
#include <sstream>
#include <iostream>

#include "droneBehaviors.hpp"
#include "states.hpp"

/****** DBSurfacing ******/
DBSurfacing::DBSurfacing(DroneState &drone) : DroneBehavior(drone) {
}

unique_ptr<DroneBehavior> DBSurfacing::getCopy(DroneState &drone) const {
    DroneBehavior *newBehavior = new DBSurfacing(drone);
    return unique_ptr<DroneBehavior>(newBehavior);
}

void DBSurfacing::TryChange() {
    GameState& game = GameState::get();

    unique_ptr<DroneBehavior> buffer;
    if (drone.position.y <= 500) {
        optional<Quadrant> quadrant = findNextTargetForDrone(drone, game.own, game.visibleEnemies);
        if (quadrant.has_value()) {
            buffer = std::move(drone.currentBehavior);
            drone.currentBehavior = unique_ptr<DroneBehavior>(new DBSearching(drone, quadrant.value()));
        }
    }
}

void DBSurfacing::Process() {
    bool useLight = !isAnyCreatureInRange(GameState::get().visibleEnemies, drone.position, DARK_DISTANCE);
    drone.move(Coord{drone.position.x, 0}, useLight, "Surfacing");
}

/****** DBSearching ******/
DBSearching::DBSearching(DroneState &drone, Quadrant quadrant) : DroneBehavior(drone) {
    currentTarget = quadrant;
}

unique_ptr<DroneBehavior> DBSearching::getCopy(DroneState &drone) const {
    DroneBehavior *newBehavior = new DBSearching(drone, currentTarget);
    return unique_ptr<DroneBehavior>(newBehavior);
} 

void DBSearching::TryChange() {
    GameState& game = GameState::get();

    if (drone.currentScans.size() >= MAX_SCANS) {
        drone.currentBehavior = unique_ptr<DroneBehavior>(new DBSurfacing(drone));
        return;
    }

    optional<Quadrant> quadrant = findNextTargetForDrone(drone, game.own, game.visibleEnemies);
    if (!quadrant.has_value()) {
        drone.currentBehavior = unique_ptr<DroneBehavior>(new DBSurfacing(drone));
    } else {
        currentTarget = quadrant.value();
    }
}

void DBSearching::Process() {
    GameState& game = GameState::get();

    bool useLight = !isAnyCreatureInRange(game.visibleEnemies, drone.position, DARK_DISTANCE);
    Quadrant quadrant = static_cast<Quadrant>(currentTarget);
    Coord target = getQuadrantCenter(quadrant, drone.position);
    CreatureStateSet creaturesInQuadrant = getCreaturesInQuadrant(quadrant, drone.position, game.visibleEnemies);
    if (isAnyCreatureInRange(game.visibleEnemies, drone.position, AVOID_DISTANCE)) {
        target = cleanupDirection(drone.position, target, game.visibleEnemies);
    }
    stringstream message;
    if (abs(static_cast<float>(target.x) / target.y) < DRIFT_RATIO) {
        message << "Drifting " << getName(quadrant);
        drone.wait(useLight, message.str());
    } else {
        message << "Following " << getName(quadrant);
        drone.move(target, useLight, message.str());
    }
}

/****** Utility functions ******/
optional<Quadrant> findNextTargetForDrone(const DroneState &drone, PlayerState &state,
                                          CreatureStateSet &visibleEnemies) {
    if (state.remainingCreatures.size() == 0) {
        cerr << "No remaining creatures for " << drone.id << endl;
        return {};
    }

    int nextCreature = *state.remainingCreatures.begin();
    for (int creatureId : state.remainingCreatures) {
        if (creatureId % 2 == drone.id / 2) {
            nextCreature = creatureId;
            break;
        }
    }

    cerr << drone.id << " goes for " << nextCreature << endl;

    for (auto bleep : drone.bleeps) {
        if (bleep.second.count(nextCreature)) {
            return bleep.first;
        }
    }

    cerr << "creature " << nextCreature << " not found" << endl;
    return {};
}

optional<Quadrant> findNewTargetsByRadar(const DroneState &drone, PlayerState &state,
                                         CreatureStateSet &visibleEnemies) {
    if (isAnyCreatureInRange(visibleEnemies, drone.position, FLEE_DISTANCE))
        return {};

    Quadrant maxDensityQuadrant;
    double maxDensity = 0;
    for (auto &quadrant : drone.bleeps) {
        if (isAnyCreatureInRange(getCreaturesInQuadrant(quadrant.first, drone.position, visibleEnemies), drone.position,
                                 AVOID_DISTANCE)) {
            cerr << "Avoiding quadrant " << getName(quadrant.first) << " for " << drone.id << endl;
            continue;
        }
        IntSet remainingInQuadrant = filterSet(state.remainingCreatures, quadrant.second);
        double density = getDensity(drone.position, quadrant.first, remainingInQuadrant.size());
        cerr << "For drone " << drone.id << " density in " << getName(quadrant.first) << " is " << density << endl;
        if (maxDensity < density) {
            maxDensity = density;
            maxDensityQuadrant = quadrant.first;
        }
    }

    if (maxDensity > 0)
        return maxDensityQuadrant;
    return {};
}

Coord cleanupDirection(Coord position, Coord target, CreatureStateSet creatures) {
    Coord ret;
    bool positiveX = target.x - position.x > 0;
    bool positiveY = target.y - position.y > 0;

    Coord xDirection = position;
    xDirection.x += positiveX ? 600 : -600;
    Coord yDirection = position;
    xDirection.y += positiveY ? 600 : -600;
    int xMinDist = 50000;
    int yMinDist = 50000;
    int targetMinDist = 50000;

    for (const CreatureState &creature : creatures) {
        int xDist = getSqDistance(xDirection, creature.position + creature.velocity);
        xMinDist = min(xDist, xMinDist);
        int yDist = getSqDistance(yDirection, creature.position + creature.velocity);
        yMinDist = min(yDist, yMinDist);
        int targetDist = getSqDistance(target, creature.position + creature.velocity);
        targetMinDist = min(targetDist, targetMinDist);
    }
    if (xMinDist < FLEE_DISTANCE * FLEE_DISTANCE && yMinDist < FLEE_DISTANCE * FLEE_DISTANCE &&
        targetMinDist < FLEE_DISTANCE * FLEE_DISTANCE) {
        ret.x = position.x + (positiveX ? -600 : 600);
        ret.y = position.y + (positiveY ? -600 : 600);
        return ret;
    } else if (targetMinDist > xMinDist && targetMinDist > yMinDist) {
        return target;
    } else {
        return xMinDist > yMinDist ? xDirection : yDirection;
    }
}

