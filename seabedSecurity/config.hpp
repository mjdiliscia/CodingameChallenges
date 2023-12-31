#pragma once

#include <set>
#include <map>
#include <string>

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

string getName(Quadrant quadrant);
Quadrant getQuadrant(string str);
IntSet filterSet(const IntSet &filter, const IntSet &set);

