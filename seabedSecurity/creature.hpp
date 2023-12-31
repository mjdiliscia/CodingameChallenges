#pragma once

#include "config.hpp"
#include "coord.hpp"

struct Creature {
    bool operator<(const Creature &other) const {
        return id < other.id;
    }

    int id;
    Color color;
    Type type;
};
using CreatureSet = set<Creature>;

struct CreatureState {
    bool operator<(const CreatureState &other) const {
        return id < other.id;
    }
    int id;
    Coord position;
    Coord velocity;
};
using CreatureStateSet = set<CreatureState>;

CreatureStateSet getCreaturesInQuadrant(Quadrant quadrant, Coord quadrantCenter, CreatureStateSet &creatures);
bool isAnyCreatureInRange(CreatureStateSet creatures, Coord position, int range);

