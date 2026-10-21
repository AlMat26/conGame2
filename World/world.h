#ifndef WORLD_H
#define WORLD_H

#include <random>
#include <vector>

enum class Tile
{
    DeepWater,
    Water,
    Sand,
    Grass,
    Tree,
    Rock,
    RockFloor,
    Wall,
    Bridge,
    Sapling,
    Support,
    Ore,
    LadderDown,
    LadderUp,
    Rubble,
    Torch
};

const int tileTypesCount = 16;
const int chunkSize = 32;
const int mineDepth = 8;

struct Chunk
{
    Tile tiles[chunkSize][chunkSize];
};

struct Point
{
    int x = 0;
    int y = 0;
};

struct MineRoom
{
    int left = 0;
    int bottom = 0;
    int right = 0;
    int top = 0;
};

// Уровень шахты: комнаты и штольни заброшенной шахты, лестницы.
// laddersDown нулевого уровня — входы в шахты на поверхности
struct MineLevel
{
    std::vector <MineRoom> rooms;
    std::vector <Point> laddersUp;
    std::vector <Point> laddersDown;
};

class World
{
public:
    World(int widthInChunks, int heightInChunks, unsigned int seed);
    World(const World&) = delete;
    World& operator= (const World&) = delete;

    Tile getTile(int x, int y, int z);
    void setTile(int x, int y, int z, Tile tile);

    Tile getGroundTile(int x, int y, int z);
    bool isInside(int x, int y, int z);
    bool isWalkable(int x, int y, int z);
    bool isOpaque(int x, int y, int z);
    bool findSpawn(int& x, int& y);

    int getWidth();
    int getHeight();
    unsigned int getSeed();

    ~World();
private:
    bool isGoodSpawn(int x, int y);

    void generateMines();
    bool isNearRock(Point point);
    void addRoom(MineLevel& level, int left, int bottom, int right, int top);
    void addCorridor(MineLevel& level, Point from, Point to);

    Chunk* getChunk(int chunkX, int chunkY, int z);
    void generateChunk(Chunk* chunk, int chunkX, int chunkY, int z);
    Tile generateTile(int x, int y, int z);
    Tile generateSurfaceTile(int x, int y);
    Tile generateUndergroundTile(int x, int y, int z);

    int widthInChunks = 0;
    int heightInChunks = 0;
    unsigned int seed = 0;

    std::vector <Chunk*> chunks;
    std::vector <MineLevel> levels;
};

#endif // WORLD_H
