#include <cmath>

#include "coord.hpp"

Coord Coord::operator+(const Coord &other) const {
    return Coord{x + other.x, y + other.y};
}

Coord getQuadrantCenter(Quadrant quadrant, Coord position) {
    Coord center{0, 0};

    switch (quadrant) {
    case Quadrant::TL:
    case Quadrant::TR:
        center.y = position.y / 2;
        break;
    case Quadrant::BL:
    case Quadrant::BR:
        center.y = 5000 + position.y / 2;
        break;
    }

    switch (quadrant) {
    case Quadrant::TL:
    case Quadrant::BL:
        center.x = position.x / 2;
        break;
    case Quadrant::TR:
    case Quadrant::BR:
        center.x = 5000 + position.x / 2;
        break;
    }

    return center;
}

int getSqDistance(Coord a, Coord b) {
    return pow(a.x - b.x, 2) + pow(a.y - b.y, 2);
}

double getDensity(Coord position, Quadrant quadrant, int amount) {
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

    return static_cast<double>(amount) / (height * width);
}

bool isPositionInQuadrant(Coord position, Quadrant quadrant, Coord quadrantCenter) {
    switch (quadrant) {
    case Quadrant::TL:
        return position.x < quadrantCenter.x && position.y < quadrantCenter.y;
    case Quadrant::TR:
        return position.x > quadrantCenter.x && position.y < quadrantCenter.y;
    case Quadrant::BL:
        return position.x < quadrantCenter.x && position.y > quadrantCenter.y;
    case Quadrant::BR:
        return position.x > quadrantCenter.x && position.y > quadrantCenter.y;
    }
}

