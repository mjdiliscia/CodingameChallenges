#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <tuple>
#include <random>
#include <assert.h>

using namespace std;

const int MAX_WIDTH = 15;
const int MAX_HEIGHT = 7;
constexpr int MAX_TILES = 15*7;
const int MAX_ACTIONS = 100;

using Coord = tuple<int, int>;
int x(Coord coord) { return get<0>(coord); }
int y(Coord coord) { return get<1>(coord); }
Coord coord(int x, int y) { return make_tuple(x, y); }

struct Tile {
    int scrapAmount;
    int owner;
    int units;
    int recycler;
    int canBuild;
    int canSpawn;
    int willBeScrapped;
};
using TileRow = array<Tile, MAX_WIDTH>;
using TileMatrix = array<TileRow, MAX_HEIGHT>;

struct RobotTile {
    Coord coord;
    int robots;
};
using RobotTiles = array<RobotTile, MAX_TILES>;

enum class ACTION {
    MOVE,
    BUILD,
    SPAWN,
    WAIT
};
array<char[8], 5> actionToString{"MOVE", "BUILD", "SPAWN", "WAIT"};
struct Action {
    ACTION kind;
    int amount;
    Coord pos;
    Coord target;
};
using Actions = array<Action, MAX_ACTIONS>;

enum class DIRECTION {
    UP, DOWN, LEFT, RIGHT
};

int boardWidth;
int boardHeight;
int currentMatter;
int opponentMatter;
TileMatrix board;
RobotTiles ownRobotsTiles;
int ownRobotTilesNum;
RobotTiles opponentRobotsTiles;
int opponentRobotTilesNum;
Actions nextActions;
int nextActionsNum;

minstd_rand randomEngine = minstd_rand(random_device()());
uniform_int_distribution directionGenerator = uniform_int_distribution(0, 3);

void init();
void updateGameStatus();
void addAction(Action action);
void calculateOrders();
void sendOrders();
void moveByRandomWalk(Tile tile, Coord coordinate);
void moveByRandomWalk(RobotTile tile);
Tile& coordTile(Coord coord);
tuple<bool, Coord> getNeighbor(Coord coord, DIRECTION direction);

int main()
{
    init();

    while (1) {
        updateGameStatus();
        calculateOrders();
        sendOrders();
    }
}

void init() {
    cin >> boardWidth >> boardHeight; cin.ignore();
}

void updateGameStatus() {
    ownRobotTilesNum = opponentRobotTilesNum = 0;
    cin >> currentMatter >> opponentMatter; cin.ignore();
    for (int i = 0; i < boardHeight; i++) {
        for (int j = 0; j < boardWidth; j++) {
            Tile& tile = board[i][j];
            cin >> tile.scrapAmount;
            cin >> tile.owner;
            cin >> tile.units;
            cin >> tile.recycler;
            cin >> tile.canBuild;
            cin >> tile.canSpawn;
            cin >> tile.willBeScrapped;
            cin.ignore();

            assert(tile.units == 0 || tile.owner != -1);
            if (tile.units > 0) {
                auto& robotTile = tile.owner == 1 ? ownRobotsTiles[ownRobotTilesNum++] : opponentRobotsTiles[opponentRobotTilesNum++];
                robotTile.coord = coord(j, i);
                robotTile.robots = tile.units;
            }
        }
    }
}

void addAction(Action action) {
    nextActions[nextActionsNum++] = action;
}

void calculateOrders() {
    for (int i=0; i < ownRobotTilesNum; i++) {
        moveByRandomWalk(ownRobotsTiles[i]);
    }
}

void sendOrders() {
    if (nextActionsNum == 0) {
        cout << "WAIT";
    }
    for (int i = 0; i < nextActionsNum; i++) {
        Action& action = nextActions[i];
        cout << actionToString[static_cast<int>(action.kind)];
        switch (action.kind) {
            case ACTION::MOVE:
            case ACTION::SPAWN:
                cout << " " << action.amount;
        }
        switch (action.kind) {
            case ACTION::MOVE:
            case ACTION::BUILD:
            case ACTION::SPAWN:
                cout << " " << x(action.pos) << " " << y(action.pos);
        }
        if (action.kind == ACTION::MOVE) {
            cout << " " << x(action.target) << " " << y(action.target);
        }
        cout << ";";
    }
    cout << endl;
    nextActionsNum = 0;
}

void moveByRandomWalk(Tile tile, Coord coordinate) {
    assert(tile.owner == 1);
    moveByRandomWalk(RobotTile{coordinate, tile.units});
}

void moveByRandomWalk(RobotTile tile) {
    int directionAmount[4]{0,0,0,0};
    for (int i = 0; i < tile.robots; i++) {
        directionAmount[directionGenerator(randomEngine)]++;
    }

    for (int i = 0; i < 4; i++) {
        if (directionAmount[i] > 0) addAction(Action{ACTION::MOVE, directionAmount[i], tile.coord, get<1>(getNeighbor(tile.coord, static_cast<DIRECTION>(i)))});
    }
}

Tile& coordTile(Coord coord) {
    return board[y(coord)][x(coord)];
}

#define BORDER_MOVE(v, c, m) if (v != c) { v += m; differentCoord = true; }
tuple<bool, Coord> getNeighbor(Coord coordinate, DIRECTION direction) {
    auto [x, y] = coordinate;
    bool differentCoord = false;
    switch (direction) {
        case DIRECTION::UP:
            BORDER_MOVE(y, 0, -1);
            break;
        case DIRECTION::DOWN:
            BORDER_MOVE(y, boardHeight-1, 1);
            break;
        case DIRECTION::LEFT:
            BORDER_MOVE(x, 0, -1);
            break;
        case DIRECTION::RIGHT:
            BORDER_MOVE(x, boardWidth-1, 1);
    }
    return make_tuple(differentCoord, coord(x, y));
}