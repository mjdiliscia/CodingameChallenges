#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

using namespace std;

const int AVOID_DISTANCE = 1200;
const int FLEE_DISTANCE = 800;
const int DARK_DISTANCE = 2000;
const float DRIFT_RATIO = 0.1f;
const int MAX_SCANS = 6;

#define FORN(VAR, LIMIT) for (int VAR = 0; VAR < LIMIT; VAR++)
#define FORI(LIMIT) FORN(i, LIMIT)

using IntSet = set<int>;

enum class Color : int { ENEMY = -1, PINK, YELLOW, GREEN, BLUE };

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

struct DroneState;
struct GameState;
struct DroneBehavior {
    DroneBehavior(DroneState &drone) : drone(drone) {
    }
    virtual void TryChange(GameConfig &config, GameState &game) = 0;
    virtual void Process(GameConfig &config, GameState &game) = 0;
    virtual ~DroneBehavior() = default;
    virtual unique_ptr<DroneBehavior> getCopy(DroneState &drone) const = 0;

    DroneState &drone;
};

struct DroneState {
    DroneState() = default;
    DroneState(const DroneState &other)
        : id(other.id), position(other.position), emergency(other.emergency), battery(other.battery),
          currentScans(other.currentScans), bleeps(other.bleeps) {
        currentBehavior = other.currentBehavior->getCopy(*this);
    }
    int id;
    Coord position;
    int emergency;
    int battery;
    IntSet currentScans;
    RadarMap bleeps;
    unique_ptr<DroneBehavior> currentBehavior;
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

bool isAnyCreatureInRange(CreatureStateSet creatures, Coord position, int range);
void wait(bool useLight, string message);
void move(Coord position, bool useLight, string message);
optional<Quadrant> findNextTargetForDrone(const DroneState &drone, PlayerState &state,
                                          CreatureStateSet &visibleEnemies);
optional<Quadrant> findNewTargetsByRadar(const DroneState &drone, PlayerState &state, CreatureStateSet &visibleEnemies);
Coord getQuadrantCenter(Quadrant quadrant, Coord dronePosition);
string getName(Quadrant quadrant);
CreatureStateSet getCreaturesInQuadrant(Quadrant quadrant, const DroneState &drone, CreatureStateSet &creatures);
Coord cleanupDirection(const DroneState &drone, Coord target, CreatureStateSet creatures);

struct DBSurfacing : DroneBehavior {
    DBSurfacing(DroneState &drone);
    virtual unique_ptr<DroneBehavior> getCopy(DroneState &drone) const override;
    virtual void TryChange(GameConfig &config, GameState &game) override;
    virtual void Process(GameConfig &config, GameState &game) override;
};

struct DBSearching : DroneBehavior {
    DBSearching(DroneState &drone, Quadrant quadrant);
    virtual unique_ptr<DroneBehavior> getCopy(DroneState &drone) const override;
    virtual void TryChange(GameConfig &config, GameState &game) override;
    virtual void Process(GameConfig &config, GameState &game) override;

    Quadrant currentTarget;
};

DBSurfacing::DBSurfacing(DroneState &drone) : DroneBehavior(drone) {
}

unique_ptr<DroneBehavior> DBSurfacing::getCopy(DroneState &drone) const {
    DroneBehavior *newBehavior = new DBSurfacing(drone);
    return unique_ptr<DroneBehavior>(newBehavior);
}

void DBSurfacing::TryChange(GameConfig &config, GameState &game) {
    unique_ptr<DroneBehavior> buffer;
    if (drone.position.y <= 500) {
        optional<Quadrant> quadrant = findNextTargetForDrone(drone, game.own, game.visibleEnemies);
        if (quadrant.has_value()) {
            buffer = std::move(drone.currentBehavior);
            drone.currentBehavior = unique_ptr<DroneBehavior>(new DBSearching(drone, quadrant.value()));
        }
    }
}

void DBSurfacing::Process(GameConfig &config, GameState &game) {
    bool useLight = !isAnyCreatureInRange(game.visibleEnemies, drone.position, DARK_DISTANCE);
    move(Coord{drone.position.x, 0}, useLight, "Surfacing");
}

DBSearching::DBSearching(DroneState &drone, Quadrant quadrant) : DroneBehavior(drone) {
    currentTarget = quadrant;
}

unique_ptr<DroneBehavior> DBSearching::getCopy(DroneState &drone) const {
    DroneBehavior *newBehavior = new DBSearching(drone, currentTarget);
    return unique_ptr<DroneBehavior>(newBehavior);
}

void DBSearching::TryChange(GameConfig &config, GameState &game) {
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

void DBSearching::Process(GameConfig &config, GameState &game) {
    bool useLight = !isAnyCreatureInRange(game.visibleEnemies, drone.position, DARK_DISTANCE);
    Quadrant quadrant = static_cast<Quadrant>(currentTarget);
    Coord target = getQuadrantCenter(quadrant, drone.position);
    CreatureStateSet creaturesInQuadrant = getCreaturesInQuadrant(quadrant, drone, game.visibleEnemies);
    if (isAnyCreatureInRange(game.visibleEnemies, drone.position, AVOID_DISTANCE)) {
        target = cleanupDirection(drone, target, game.visibleEnemies);
    }
    stringstream message;
    if (abs(static_cast<float>(target.x) / target.y) < DRIFT_RATIO) {
        message << "Drifting " << getName(quadrant);
        wait(useLight, message.str());
    } else {
        message << "Following " << getName(quadrant);
        move(target, useLight, message.str());
    }
}

GameConfig parseConfig() {
    GameConfig config;

    int creatureCount;
    cin >> creatureCount;
    cin.ignore();
    FORI(creatureCount) {
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
    FORI(scanCount) {
        int id;
        cin >> id;
        cin.ignore();
        scans.insert(id);
    }

    return scans;
}

void parseDrones(DroneStateVec &drones) {
    int droneCount;
    cin >> droneCount;
    cin.ignore();
    FORI(droneCount) {
        DroneState drone;
        cin >> drone.id >> drone.position.x >> drone.position.y >> drone.emergency >> drone.battery;
        cin.ignore();
        auto droneIter = find_if(drones.begin(), drones.end(), [&drone](DroneState &d) { return drone.id == d.id; });
        if (droneIter != drones.end()) {
            droneIter->position = drone.position;
            droneIter->emergency = drone.emergency;
            droneIter->battery = drone.battery;
            droneIter->currentScans.clear();
            droneIter->bleeps.clear();
        } else {
            drone.currentBehavior = unique_ptr<DroneBehavior>(new DBSurfacing(drone));
            drones.push_back(drone);
        }
    }
}

void fillDroneScans(DroneStateVec &drones) {
    int scanCount;
    cin >> scanCount;
    cin.ignore();
    FORI(scanCount) {
        int droneId, creatureId;
        cin >> droneId >> creatureId;
        cin.ignore();

        auto droneIter =
            find_if(drones.begin(), drones.end(), [droneId](DroneState &drone) { return drone.id == droneId; });
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
    FORI(visibleCreatureCount) {
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

void fillDroneRadars(DroneStateVec &drones) {
    int blipCount;
    cin >> blipCount;
    cin.ignore();
    FORI(blipCount) {
        int droneId;
        int creatureId;
        string radar;
        cin >> droneId >> creatureId >> radar;
        cin.ignore();
        auto droneIter =
            find_if(drones.begin(), drones.end(), [droneId](DroneState &drone) { return drone.id == droneId; });
        droneIter->bleeps[getQuadrant(radar)].insert(creatureId);
    }
}

void parseGameState(GameState &state, GameConfig &config) {
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
    fillDroneRadars(state.own.drones);
}

void calculateRemainingCreatures(PlayerState &state, GameConfig &config) {
    IntSet presentCreaturesIds;
    for (auto &drone : state.drones)
        for (auto &qPairs : drone.bleeps)
            for (int creatureId : qPairs.second)
                if (!config.enemies.count(creatureId))
                    presentCreaturesIds.insert(creatureId);

    IntSet knownCreatureIds;
    for (auto &drone : state.drones) {
        for (int creatureId : drone.currentScans)
            knownCreatureIds.insert(creatureId);
    }
    for (int creatureId : state.totalScans)
        knownCreatureIds.insert(creatureId);

    state.remainingCreatures.clear();
    for (auto &creature : presentCreaturesIds)
        if (!knownCreatureIds.count(creature))
            state.remainingCreatures.insert(creature);
}

int getSqDistance(Coord a, Coord b) {
    return pow(a.x - b.x, 2) + pow(a.y - b.y, 2);
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

bool isPositionInQuadrant(Coord position, Quadrant quadrant, const DroneState &drone) {
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

CreatureStateSet getCreaturesInQuadrant(Quadrant quadrant, const DroneState &drone, CreatureStateSet &creatures) {
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

IntSet getFilteredCreaturesInCuadrant(const IntSet &filter, const IntSet &creatures) {
    IntSet filteredCreatures;
    for (int creatureId : creatures) {
        if (find(filter.begin(), filter.end(), creatureId) != filter.end())
            filteredCreatures.insert(creatureId);
    }

    return filteredCreatures;
}

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
        if (isAnyCreatureInRange(getCreaturesInQuadrant(quadrant.first, drone, visibleEnemies), drone.position,
                                 AVOID_DISTANCE)) {
            cerr << "Avoiding quadrant " << getName(quadrant.first) << " for " << drone.id << endl;
            continue;
        }
        IntSet remainingInQuadrant = getFilteredCreaturesInCuadrant(state.remainingCreatures, quadrant.second);
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

Coord cleanupDirection(const DroneState &drone, Coord target, CreatureStateSet creatures) {
    Coord ret;
    bool positiveX = target.x - drone.position.x > 0;
    bool positiveY = target.y - drone.position.y > 0;

    Coord xDirection = drone.position;
    xDirection.x += positiveX ? 600 : -600;
    Coord yDirection = drone.position;
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
        ret.x = drone.position.x + (positiveX ? -600 : 600);
        ret.y = drone.position.y + (positiveY ? -600 : 600);
        return ret;
    } else if (targetMinDist > xMinDist && targetMinDist > yMinDist) {
        return target;
    } else {
        return xMinDist > yMinDist ? xDirection : yDirection;
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

int main() {
    GameConfig config = parseConfig();
    GameState state;

    while (true) {
        parseGameState(state, config);
        calculateRemainingCreatures(state.own, config);

        for (auto &drone : state.own.drones) {
            drone.currentBehavior->TryChange(config, state);
            drone.currentBehavior->Process(config, state);
        }
    }
}
