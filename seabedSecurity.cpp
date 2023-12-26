#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <sstream>

using namespace std;

const int AVOID_DISTANCE = 1200;
const int FLEE_DISTANCE = 800;
const int DARK_DISTANCE = 2000;
const float DRIFT_RATIO = 0.1f;

#define forn(VAR, LIMIT) for (int VAR = 0; VAR < LIMIT; VAR++)
#define fori(LIMIT) forn(i, LIMIT)

/**
 * Score points by scanning valuable fish faster than your opponent.
 **/
// using IntVec = vector<int>;
using IntSet = set<int>;

enum class Color : int { ENEMY = -1, PINK, YELLOW, GREEN, BLLUE };

enum class Type : int { ENEMY = -1, OCTUPUS, FISH, CRAB };

enum class TargetType { NONE, CREATURE, QUADRANT };

enum class Quadrant : int { TL, TR, BL, BR };
using RadarMap = map<Quadrant, IntSet>;

struct Creature {
    bool operator<(const Creature &other) const {
        return id < other.id;
    }

    int id;
    Color color;
    Type type;
};
using CreatureSet = set<Creature>;

struct GameConfig {
    CreatureSet creatures;
    IntSet enemies;
};

struct Coord {
    Coord operator+(const Coord &other) const {
        return Coord{x + other.x, y + other.y};
    }

    int x;
    int y;
};

struct DroneState {
    int id;
    Coord position;
    int emergency;
    int battery;
    IntSet currentScans;
    RadarMap bleeps;
    int currentTarget;
    TargetType currentTargetType;
};
using DroneStateVec = vector<DroneState>;

struct PlayerState {
    int score;
    IntSet totalScans;
    DroneStateVec drones;
    IntSet remainingCreatures;
};

struct CreatureState {
    bool operator<(const CreatureState &other) const {
        return id < other.id;
    }
    int id;
    Coord position;
    Coord velocity;
};
using CreatureStateSet = set<CreatureState>;

struct GameState {
    PlayerState own;
    PlayerState foe;
    CreatureStateSet visibleCreatures;
    CreatureStateSet visibleEnemies;
};

GameConfig parseConfig() {
    GameConfig config;

    int creatureCount;
    cin >> creatureCount;
    cin.ignore();
    fori(creatureCount) {
        Creature creature;
        cin >> creature.id >> reinterpret_cast<int &>(creature.color) >> reinterpret_cast<int &>(creature.type);
        cin.ignore();
        if (creature.type != Type::ENEMY)
            config.creatures.insert(creature);
        else
            config.enemies.insert(creature.id);
    }

    return config;
}

IntSet parseScans() {
    IntSet scans;

    int scanCount;
    cin >> scanCount;
    cin.ignore();
    fori(scanCount) {
        int id;
        cin >> id;
        cin.ignore();
        scans.insert(id);
    }

    return scans;
}

DroneStateVec parseDrones(DroneStateVec &drones) {
    int droneCount;
    cin >> droneCount;
    cin.ignore();
    fori(droneCount) {
        DroneState drone;
        cin >> drone.id >> drone.position.x >> drone.position.y >> drone.emergency >> drone.battery;
        cin.ignore();
        auto droneIter = find_if(drones.begin(), drones.end(), [drone](DroneState d) { return drone.id == d.id; });
        if (droneIter != drones.end()) {
            droneIter->position = drone.position;
            droneIter->emergency = drone.emergency;
            droneIter->battery = drone.battery;
            droneIter->currentScans.clear();
            droneIter->bleeps.clear();
        } else {
            drones.push_back(drone);
        }
    }

    return drones;
}

void fillDroneScans(DroneStateVec &drones) {
    int scanCount;
    cin >> scanCount;
    cin.ignore();
    fori(scanCount) {
        int droneId, creatureId;
        cin >> droneId >> creatureId;
        cin.ignore();

        auto droneIter =
            find_if(drones.begin(), drones.end(), [droneId](DroneState drone) { return drone.id == droneId; });
        if (droneIter != drones.end())
            droneIter->currentScans.insert(creatureId);
    }
}

void fillVisibleCreatures(GameState &state, GameConfig &config) {
    state.visibleCreatures.clear();
    state.visibleEnemies.clear();

    int visibleCreatureCount;
    cin >> visibleCreatureCount;
    cin.ignore();
    fori(visibleCreatureCount) {
        CreatureState creatureState;
        cin >> creatureState.id;
        cin >> creatureState.position.x >> creatureState.position.y;
        cin >> creatureState.velocity.x >> creatureState.velocity.y;
        cin.ignore();
        if (config.enemies.count(creatureState.id))
            state.visibleEnemies.insert(creatureState);
        else
            state.visibleCreatures.insert(creatureState);
    }
}

Quadrant getQuadrant(string str) {
    if (str.compare("TL") == 0)
        return Quadrant::TL;
    if (str.compare("TR") == 0)
        return Quadrant::TR;
    if (str.compare("BL") == 0)
        return Quadrant::BL;
    if (str.compare("BR") == 0)
        return Quadrant::BR;

    assert(false);
}

string getName(Quadrant quadrant) {
    switch (quadrant) {
    case Quadrant::TL:
        return "TL";
    case Quadrant::TR:
        return "TR";
    case Quadrant::BL:
        return "BL";
    case Quadrant::BR:
        return "BR";
    }
}

void fillDroneRadars(GameState &state) {
    int blipCount;
    cin >> blipCount;
    cin.ignore();
    fori(blipCount) {
        int droneId;
        int creatureId;
        string radar;
        cin >> droneId >> creatureId >> radar;
        cin.ignore();
        auto droneIter = find_if(state.own.drones.begin(), state.own.drones.end(),
                                 [droneId](DroneState drone) { return drone.id == droneId; });
        droneIter->bleeps[getQuadrant(radar)].insert(creatureId);
    }
}

GameState parseGameState(GameState &state, GameConfig &config) {
    cin >> state.own.score;
    cin.ignore();
    cin >> state.foe.score;
    cin.ignore();
    state.own.totalScans = parseScans();
    state.foe.totalScans = parseScans();
    parseDrones(state.own.drones);
    parseDrones(state.foe.drones);
    fillDroneScans(state.own.drones);
    fillVisibleCreatures(state, config);
    fillDroneRadars(state);

    return state;
}

void calculateRemainingCreatures(PlayerState &state, GameConfig &config) {
    IntSet knownCreatureIds;
    for (auto &drone : state.drones)
        for (int creatureId : drone.currentScans)
            knownCreatureIds.insert(creatureId);
    for (int creatureId : state.totalScans)
        knownCreatureIds.insert(creatureId);

    for (auto &creature : config.creatures)
        if (knownCreatureIds.count(creature.id))
            state.remainingCreatures.insert(creature.id);
}

void resetDroneTargets(GameState &state) {
    for (auto &drone : state.own.drones)
        if (drone.currentTargetType == TargetType::CREATURE)
            if (state.own.totalScans.count(drone.currentTarget) || drone.currentScans.count(drone.currentTarget))
                drone.currentTargetType = TargetType::NONE;
}

int getSqDistance(Coord a, Coord b) {
    return pow(a.x - b.x, 2) + pow(a.y - b.y, 2);
}

void findNewTargetsByVisibility(GameState &state) {
    CreatureStateSet remainingCreatures;
    for (auto &creature : state.visibleCreatures)
        if (!state.own.totalScans.count(creature.id))
            remainingCreatures.insert(creature);

    for (auto &drone : state.own.drones) {
        if (drone.currentTarget == -1) {
            int minSqDist = 50000;
            int closestCreatureId = -1;
            for (auto creatureState : remainingCreatures) {
                int distance = getSqDistance(drone.position, creatureState.position);
                if (distance < minSqDist) {
                    minSqDist = distance;
                    closestCreatureId = creatureState.id;
                }
            }
            drone.currentTarget = closestCreatureId;
            drone.currentTargetType = TargetType::CREATURE;
        }
    }
}

double getDensity(Coord position, Quadrant quadrant, int creatureCount) {
    int height;
    switch (quadrant) {
    case Quadrant::TL:
    case Quadrant::TR:
        height = position.y;
        break;
    case Quadrant::BL:
    case Quadrant::BR:
        height = 10000 - position.y;
        break;
    }

    int width;
    switch (quadrant) {
    case Quadrant::TL:
    case Quadrant::BL:
        width = position.x;
        break;
    case Quadrant::TR:
    case Quadrant::BR:
        width = 10000 - position.y;
        break;
    }

    return static_cast<double>(creatureCount) / (height * width);
}

bool isPositionInQuadrant(Coord position, Quadrant quadrant, DroneState &drone) {
    switch (quadrant) {
    case Quadrant::TL:
        return position.x < drone.position.x && position.y < drone.position.y;
    case Quadrant::TR:
        return position.x > drone.position.x && position.y < drone.position.y;
    case Quadrant::BL:
        return position.x < drone.position.x && position.y > drone.position.y;
    case Quadrant::BR:
        return position.x > drone.position.x && position.y > drone.position.y;
    }
}

CreatureStateSet getCreaturesInQuadrant(Quadrant quadrant, DroneState &drone, CreatureStateSet &creatures) {
    CreatureStateSet enemies;
    for (auto &enemy : creatures)
        if (isPositionInQuadrant(enemy.position, quadrant, drone))
            enemies.insert(enemy);

    return enemies;
}

bool isAnyCreatureInRange(CreatureStateSet creatures, Coord position, int range) {
    for (auto &creature : creatures)
        if (getSqDistance(creature.position + creature.velocity, position) < range * range)
            return true;

    return false;
}

IntSet getFilteredCreaturesInCuadrant(IntSet &filter, IntSet &creatures) {
    IntSet filteredCreatures;
    for (int creatureId : creatures)
        if (find(filter.begin(), filter.end(), creatureId) != filter.end())
            filteredCreatures.insert(creatureId);

    return filteredCreatures;
}

void findNewTargetsByRadar(GameState &state) {
    for (auto &drone : state.own.drones) {
        if (drone.currentTargetType == TargetType::NONE && drone.position.y > 500)
            continue;

        drone.currentTargetType = TargetType::NONE;
        double maxDensity = 0;
        for (auto &quadrant : drone.bleeps) {
            if (isAnyCreatureInRange(getCreaturesInQuadrant(quadrant.first, drone, state.visibleEnemies),
                                     drone.position, AVOID_DISTANCE)) {
                cerr << "Avoiding quadrant " << getName(quadrant.first) << " for " << drone.id << endl;
                continue;
            }

            IntSet remainingInQuadrant = getFilteredCreaturesInCuadrant(state.own.remainingCreatures, quadrant.second);
            double density = getDensity(drone.position, quadrant.first, remainingInQuadrant.size());
            if (maxDensity < density) {
                maxDensity = density;
                drone.currentTargetType = TargetType::QUADRANT;
                drone.currentTarget = static_cast<int>(quadrant.first);
            }
        }

        if (isAnyCreatureInRange(state.visibleEnemies, drone.position, FLEE_DISTANCE))
            drone.currentTargetType = TargetType::NONE;
    }
}

Coord getQuadrantCenter(Quadrant quadrant, Coord dronePosition) {
    Coord center{0, 0};

    switch (quadrant) {
    case Quadrant::TL:
    case Quadrant::TR:
        center.y = dronePosition.y / 2;
        break;
    case Quadrant::BL:
    case Quadrant::BR:
        center.y = 5000 + dronePosition.y / 2;
        break;
    }

    switch (quadrant) {
    case Quadrant::TL:
    case Quadrant::BL:
        center.x = dronePosition.x / 2;
        break;
    case Quadrant::TR:
    case Quadrant::BR:
        center.x = 5000 + dronePosition.x / 2;
        break;
    }

    return center;
}

void wait(bool useLight, string message) {
    cout << "WAIT " << (useLight ? "1" : "0") << " " << message << endl;
}

void move(Coord position, bool useLight, string message) {
    cout << "MOVE " << position.x << " " << position.y << " " << (useLight ? "1" : "0") << " " << message << endl;
}

void sendDroneCommands(GameState &state) {
    for (auto &drone : state.own.drones) {
        bool useLight = !isAnyCreatureInRange(state.visibleEnemies, drone.position, DARK_DISTANCE);
        switch (drone.currentTargetType) {
        case TargetType::NONE:
            move(Coord{drone.position.x, 0}, useLight, "Surfacing");
            break;

        case TargetType::CREATURE: {
            auto creatureStateIter = find_if(state.visibleCreatures.begin(), state.visibleCreatures.end(),
                                             [&drone](auto creature) { return creature.id == drone.currentTarget; });
            move(creatureStateIter->position, useLight, "Hunting");
        } break;

        case TargetType::QUADRANT: {
            Quadrant quadrant = static_cast<Quadrant>(drone.currentTarget);
            Coord quadrantCenter = getQuadrantCenter(quadrant, drone.position);
            stringstream message;
            if (abs(static_cast<float>(quadrantCenter.x) / quadrantCenter.y) < DRIFT_RATIO) {
                message << "Drifting " << getName(quadrant);
                wait(useLight, message.str());
            } else {
                message << "Following " << getName(quadrant);
                move(quadrantCenter, useLight, message.str());
            }
        } break;
        }
    }
}

int main() {
    GameConfig config = parseConfig();
    GameState state;

    while (true) {
        parseGameState(state, config);
        calculateRemainingCreatures(state.own, config);

        resetDroneTargets(state);
        findNewTargetsByRadar(state);
        sendDroneCommands(state);
    }
}
