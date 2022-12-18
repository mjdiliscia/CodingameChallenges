#include <algorithm>
#include <array>
#include <assert.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <random>
#include <string>
#include <tuple>

using namespace std;

#define PROFILE_START(ID) auto _ID_ = chrono::steady_clock::now()
#define PROFILE_STOP(ID, MESSAGE) fprintf(stderr, MESSAGE, chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - _ID_).count())

const int MAX_WIDTH = 15;
const int MAX_HEIGHT = 7;
constexpr int MAX_TILES = MAX_WIDTH * MAX_HEIGHT;
const int MAX_ACTIONS = 100;
const int BUILD_COST = 10;

using Coord = tuple<int, int>;
int x(Coord coord) { return get<0>(coord); }
int y(Coord coord) { return get<1>(coord); }
Coord coord(int x, int y) { return make_tuple(x, y); }

struct Tile {
    Tile() { neighbors.reserve(4); }

    int scrapAmount;
    int owner;
    int units;
    int recycler;
    int canBuild;
    int canSpawn;
    int willBeScrapped;
    Coord coord;
    vector<vector<Tile>::iterator> neighbors;
};
using TileRow = vector<Tile>;
using TileMatrix = vector<TileRow>;
using TileIterator = TileRow::iterator;
using TileIterators = vector<TileRow::iterator>;
#define GET_TILE_ITERATOR(ROW, COLUMN) (board[ROW].begin() + (COLUMN))

struct RobotTile {
    RobotTile(TileIterator tile, int robots) : tile(tile), robots(robots) {}
    TileIterator tile;
    int robots;
};
using RobotTiles = vector<RobotTile>;

enum class ACTION {
    MOVE,
    BUILD,
    SPAWN,
    WAIT,
    MESSAGE,
    Count
};
using ActionStringArray = array<char[8], static_cast<size_t>(ACTION::Count)>;
ActionStringArray actionToString{"MOVE", "BUILD", "SPAWN", "WAIT", "MESSAGE"};

struct Action {
    static Action move(int amount, Coord pos, Coord target) { return Action{ACTION::MOVE, amount, pos, target}; }
    static Action build(Coord pos) { return Action{ACTION::BUILD, 0, pos}; }
    static Action spawn(int amount, Coord pos) { return Action{ACTION::SPAWN, amount, pos}; }
    static Action wait() { return Action{ACTION::WAIT}; }

    ACTION kind;
    int amount;
    Coord pos;
    Coord target;
};
using Actions = vector<Action>;

enum class DIRECTION {
    UP, DOWN, LEFT, RIGHT, Count
};

int boardWidth;
int boardHeight;
int currentMatter;
int opponentMatter;
TileMatrix board;
RobotTiles ownRobotsTiles;
RobotTiles opponentRobotsTiles;
TileIterators ownTiles;
TileIterators ownRecyclerTiles;
Actions nextActions;

minstd_rand randomEngine = minstd_rand(random_device()());
uniform_int_distribution<int> directionGenerator[4] = {
    uniform_int_distribution(0, 0),
    uniform_int_distribution(0, 1),
    uniform_int_distribution(0, 2),
    uniform_int_distribution(0, 3)
};
uniform_int_distribution bigGenerator;

void init();
void updateGameStatus();
void calculateOrders();
void sendOrders();
void buildStuff();
bool tryBuildRecycler();
Tile* getBestTileForRecycler();
bool spawnRobotSomewhere();
void moveByRandomWalk(const Tile& tile);
void moveByRandomWalk(const RobotTile& tile);
TileIterator coordTile(const Coord& coord);
tuple<bool, Coord> getNeighbor(const Coord& coord, DIRECTION direction);
tuple<int, int, int> getTileReachableScrap(Tile& tile);

int main()
{
    cin.peek();
    PROFILE_START(init);
    init();
    PROFILE_STOP(init, "Init time: %ldµs\n");

    while (1) {
        cin.peek();
        PROFILE_START(turn);
        updateGameStatus();
        calculateOrders();
        sendOrders();
        PROFILE_STOP(turn, "Turn time: %ldµs\n");
    }
}

void init() {
    cin >> boardWidth >> boardHeight; cin.ignore();
    board.reserve(boardHeight);
    board.resize(boardHeight);
    for (auto& row : board) {
        row.reserve(boardWidth);
        row.resize(boardWidth);
    }
    ownRobotsTiles.reserve(MAX_TILES);
    opponentRobotsTiles.reserve(MAX_TILES);
    ownTiles.reserve(MAX_TILES);
    nextActions.reserve(MAX_ACTIONS);
}

void updateGameStatus() {
    ownRobotsTiles.clear();
    opponentRobotsTiles.clear();
    ownTiles.clear();
    ownRecyclerTiles.clear();
    cin >> currentMatter >> opponentMatter; cin.ignore();
    for (int i = 0; i < board.size(); i++) {
        for (int j = 0; j < board[i].size(); j++) {
            Tile& tile = board[i][j];
            cin >> tile.scrapAmount;
            cin >> tile.owner;
            cin >> tile.units;
            cin >> tile.recycler;
            cin >> tile.canBuild;
            cin >> tile.canSpawn;
            cin >> tile.willBeScrapped;
            cin.ignore();

            tile.coord = coord(j, i);
            assert(tile.units == 0 || tile.owner != -1);
            if (tile.units > 0) {
                RobotTile robotTile(GET_TILE_ITERATOR(i, j), tile.units);
                if (tile.owner == 1) ownRobotsTiles.push_back(robotTile);
                else opponentRobotsTiles.push_back(robotTile);
            }
            if (tile.owner == 1) {
                ownTiles.push_back(GET_TILE_ITERATOR(i, j));
                if (tile.recycler == 1) ownRecyclerTiles.push_back(GET_TILE_ITERATOR(i, j));
            }
        }
    }

    for (int i = 0; i < board.size(); i++) {
        for (int j = 0; j < board[i].size(); j++) {
            Tile& tile = board[i][j];
            tile.neighbors.clear();

            for (int d = 0; d < static_cast<int>(DIRECTION::Count); d++) {
                DIRECTION direction = static_cast<DIRECTION>(d);
                auto [valid, coord] = getNeighbor(tile.coord, direction);
                if (valid && coordTile(coord)->recycler == 0) {
                    tile.neighbors.push_back(coordTile(coord));
                }
            }
        }
    }
}

void calculateOrders() {
    for (auto robotsTile : ownRobotsTiles) {
        moveByRandomWalk(robotsTile);
    }
    buildStuff();
}

void sendOrders() {
    if (nextActions.size() == 0) cout << "WAIT";
    for (auto& action : nextActions) {
        cout << actionToString[static_cast<int>(action.kind)];
        switch (action.kind) {
            case ACTION::MOVE:
            case ACTION::SPAWN:
                cout << " " << action.amount;
            default: ;
        }
        switch (action.kind) {
            case ACTION::MOVE:
            case ACTION::BUILD:
            case ACTION::SPAWN:
                cout << " " << x(action.pos) << " " << y(action.pos);
            default: ;
        }
        if (action.kind == ACTION::MOVE) {
            cout << " " << x(action.target) << " " << y(action.target);
        }
        cout << ";";
    }
    cout << endl;
    nextActions.clear();
}

void buildStuff() {
    int remainingMatter = currentMatter;

    while (remainingMatter > BUILD_COST) {
        bool RecyclerBuilt = tryBuildRecycler();
        bool robotBuilt = RecyclerBuilt ? false : spawnRobotSomewhere();

        remainingMatter -= RecyclerBuilt || robotBuilt ? BUILD_COST : 0;
    }
}

bool tryBuildRecycler() {
    if (ownRecyclerTiles.size() >= ownTiles.size() / 20) return false;

    if (Tile* bestTileForRecycler = getBestTileForRecycler()) {
        nextActions.push_back(Action::build(bestTileForRecycler->coord));
        return true;
    }
    
    return false;
}

Tile* getBestTileForRecycler() {
    Tile* bestTile = nullptr;

    int bestTileValue = 0;
    for (auto tile : ownTiles) {
        if (tile->units == 0) {
            auto [own, opponent, free] = getTileReachableScrap(*tile);
            if (opponent+free > bestTileValue) {
                bestTileValue = opponent+free;
                bestTile = &*tile;
            }
        }
    }

    return bestTile;
}

bool spawnRobotSomewhere() {
    if (ownRecyclerTiles.size() == ownTiles.size()) return false;

    Coord randomCoord;
    bool freeTile;
    do {
        auto tile = ownTiles[bigGenerator(randomEngine) % ownTiles.size()];
        if ((freeTile = tile->recycler == 0))
            randomCoord = tile->coord;
    }while(!freeTile);

    nextActions.push_back(Action::spawn(1, randomCoord));
    return true;
}

void moveByRandomWalk(const Tile& tile) {
    assert(tile.owner == 1 && tile.units > 0);

    int neighborsCount = tile.neighbors.size();
    vector<tuple<int, Coord>> moves;
    moves.resize(neighborsCount);
    for (int i = 0; i < tile.units; i++) {
        int neighborNum = directionGenerator[neighborsCount-1](randomEngine);
        auto& move = moves[neighborNum];
        get<0>(move)++;
        get<1>(move) = tile.neighbors[neighborNum]->coord;
    }

    for (auto move : moves) {
        if (get<0>(move) > 0) {
            nextActions.push_back(Action::move(get<0>(move), tile.coord, get<1>(move)));
        }
    }
}

void moveByRandomWalk(const RobotTile& tile) {
    moveByRandomWalk(*tile.tile);
}

TileIterator coordTile(const Coord& coord) {
    return GET_TILE_ITERATOR(y(coord), x(coord));
}

#define BORDER_MOVE(v, c, m) if (v != c) { v += m; differentCoord = true; }
tuple<bool, Coord> getNeighbor(const Coord& coordinate, DIRECTION direction) {
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
        default: ;
    }
    return make_tuple(differentCoord, coord(x, y));
}

tuple<int, int, int> getTileReachableScrap(Tile& tile) {
    int own = 0;
    int opponent = 0;
    int free = 0;

    switch(tile.owner) {
        case 1:
            own += tile.scrapAmount;
            break;
        case 0:
            free += tile.scrapAmount;
            break;
        case -1:
            opponent += tile.scrapAmount;
            break;
        default: ;
    }

    for (int d=0; d < static_cast<int>(DIRECTION::Count); d++) {
        DIRECTION direction = static_cast<DIRECTION>(d);
        auto neighborCoord = getNeighbor(tile.coord, direction);
        if (get<0>(neighborCoord)) {
            TileIterator tileIter = coordTile(get<1>(neighborCoord));
            switch(tileIter->owner) {
                case 1:
                    own += tileIter->scrapAmount;
                    break;
                case 0:
                    free += tileIter->scrapAmount;
                    break;
                case -1:
                    opponent += tileIter->scrapAmount;
                    break;
                default: ;
            }
        }        
    }

    return make_tuple(own, opponent, free);
}