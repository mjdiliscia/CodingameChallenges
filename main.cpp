#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

using namespace std;

const int MAX_WIDTH = 15;
const int MAX_HEIGHT = 7;
constexpr int MAX_TILES = 15*7;

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

int boardWidth;
int boardHeight;
int currentMatter;
int opponentMatter;
TileMatrix board;
RobotTiles ownRobotsTiles;
RobotTiles opponentRobotsTiles;

inline void init();
inline void updateGameStatus();

int main()
{
    init();

    while (1) {
        updateGameStatus();
        
    }
}

inline void init() {
    cin >> boardWidth >> boardHeight; cin.ignore();
}

inline void updateGameStatus() {
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

            if (tile.units)
        }
    }
}
