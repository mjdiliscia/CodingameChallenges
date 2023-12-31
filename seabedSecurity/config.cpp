#include <algorithm>
#include <cassert>
#include <vector>

#include "config.hpp"

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

IntSet filterSet(const IntSet &filter, const IntSet &set) {
    vector<int> inVector(set.begin(), set.end());
    vector<int> filteredVector;
    copy_if(inVector.begin(), inVector.end(), filteredVector.begin(), [&filter](auto& elem) { return filter.count(elem) != 1; });
    return IntSet(filteredVector.begin(), filteredVector.end());
}
