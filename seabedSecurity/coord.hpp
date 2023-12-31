#pragma once

#include "config.hpp"

struct Coord {
    Coord operator+(const Coord &other) const;
        
    int x;
    int y;
};

Coord getQuadrantCenter(Quadrant quadrant, Coord dronePosition);
int getSqDistance(Coord a, Coord b);
double getDensity(Coord position, Quadrant quadrant, int amount);
bool isPositionInQuadrant(Coord position, Quadrant quadrant, Coord quadrantCenter);

