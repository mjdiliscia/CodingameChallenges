#include "creature.hpp"

CreatureStateSet getCreaturesInQuadrant(Quadrant quadrant, Coord quadrantCenter, CreatureStateSet &creatures) {
    CreatureStateSet enemies;
    for (auto &enemy : creatures)
        if (isPositionInQuadrant(enemy.position, quadrant, quadrantCenter))
            enemies.insert(enemy);

    return enemies;
}

bool isAnyCreatureInRange(CreatureStateSet creatures, Coord position, int range) {
    for (auto &creature : creatures)
        if (getSqDistance(creature.position + creature.velocity, position) < range * range)
            return true;

    return false;
}

