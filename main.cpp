#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <assert.h>

using namespace std;

const int MAX_WIDTH = 15;
const int MAX_HEIGHT = 7;
constexpr int MAX_TILES = 15*7;
const int MAX_ACTIONS = 100;

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
    int xCoord, yCoord;
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
    int x, y, targetX, targetY;
    int amount;
};
using Actions = array<Action, MAX_ACTIONS>;

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

void init();
void updateGameStatus();
void calculateOrders();
void sendOrders();

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

            assert(tile.units == 0 || tile.owner == -1);
            if (tile.units > 0) {
                auto& robotTile = tile.owner == 1 ? ownRobotsTiles[ownRobotTilesNum++] : opponentRobotsTiles[opponentRobotTilesNum++];
                robotTile.xCoord = j;
                robotTile.yCoord = i;
                robotTile.robots = tile.units;
            }
        }
    }
}

void calculateOrders() {

}

void sendOrders() {
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
                cout << " " << action.x << " " << action.y;
        }
        if (action.kind == ACTION::MOVE) {
            cout << " " << action.targetX << " " << action.targetY;
        }
        cout << ";"
    }
    cout << endl;
}
