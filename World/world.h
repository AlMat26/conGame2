#ifndef WORLD_H
#define WORLD_H

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
    Sapling
};

const int tileTypesCount = 10;
const int chunkSize = 32;

struct Chunk
{
    Tile tiles[chunkSize][chunkSize];
};

class World
{
public:
    World(int widthInChunks, int heightInChunks, unsigned int seed);
    World(const World&) = delete;
    World& operator= (const World&) = delete;

    Tile getTile(int x, int y);
    void setTile(int x, int y, Tile tile);

    Tile getGroundTile(int x, int y);
    bool isInside(int x, int y);
    bool isWalkable(int x, int y);
    bool findSpawn(int& x, int& y);

    int getWidth();
    int getHeight();
    unsigned int getSeed();

    ~World();
private:
    bool isGoodSpawn(int x, int y);
    Chunk* getChunk(int chunkX, int chunkY);
    void generateChunk(Chunk* chunk, int chunkX, int chunkY);
    Tile generateTile(int x, int y);

    int widthInChunks = 0;
    int heightInChunks = 0;
    unsigned int seed = 0;

    std::vector <Chunk*> chunks;
};

#endif // WORLD_H
