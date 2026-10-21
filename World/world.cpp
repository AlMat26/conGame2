#include "world.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

static unsigned int hash2(int x, int y, unsigned int seed)
{
    unsigned int h = seed;
    h ^= (unsigned int)x * 374761393u;
    h ^= (unsigned int)y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static float latticeValue(int x, int y, unsigned int seed)
{
    return (hash2(x, y, seed) & 0xFFFF) / 65535.0f;
}

static float smoothStep(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

static float valueNoise(float x, float y, unsigned int seed)
{
    int x0 = (int)std::floor(x);
    int y0 = (int)std::floor(y);
    float tx = smoothStep(x - x0);
    float ty = smoothStep(y - y0);

    float top = latticeValue(x0, y0, seed) * (1 - tx) + latticeValue(x0 + 1, y0, seed) * tx;
    float bottom = latticeValue(x0, y0 + 1, seed) * (1 - tx) + latticeValue(x0 + 1, y0 + 1, seed) * tx;
    return top * (1 - ty) + bottom * ty;
}

// Сумма нескольких слоёв шума: крупные формы + мелкие детали, результат 0..1
static float fractalNoise(float x, float y, unsigned int seed, int octaves)
{
    float sum = 0;
    float amplitude = 1;
    float amplitudeSum = 0;

    for (int i = 0; i < octaves; i++)
    {
        sum += valueNoise(x, y, seed + i) * amplitude;
        amplitudeSum += amplitude;
        amplitude *= 0.5f;
        x *= 2;
        y *= 2;
    }
    return sum / amplitudeSum;
}

World::World(int widthInChunks, int heightInChunks, unsigned int seed)
{
    this -> widthInChunks = widthInChunks;
    this -> heightInChunks = heightInChunks;
    this -> seed = seed;

    chunks.resize(widthInChunks * heightInChunks, nullptr);
}

Tile World::getTile(int x, int y)
{
    if(x < 0 || y < 0 || x >= getWidth() || y >= getHeight())
    {
        return Tile::DeepWater;
    }

    Chunk* chunk = getChunk(x / chunkSize, y / chunkSize);
    return chunk -> tiles[y % chunkSize][x % chunkSize];
}

void World::setTile(int x, int y, Tile tile)
{
    if(x < 0 || y < 0 || x >= getWidth() || y >= getHeight())
    {
        return;
    }

    Chunk* chunk = getChunk(x / chunkSize, y / chunkSize);
    chunk -> tiles[y % chunkSize][x % chunkSize] = tile;
}

bool World::isWalkable(int x, int y)
{
    Tile tile = getTile(x, y);
    return tile != Tile::DeepWater && tile != Tile::Tree && tile != Tile::Rock;
}

bool World::findSpawn(int& x, int& y)
{
    int centerX = getWidth() / 2;
    int centerY = getHeight() / 2;
    int maxRadius = std::max(centerX, centerY);

    for (int radius = 0; radius < maxRadius; radius++)
    {
        for (int dy = -radius; dy <= radius; dy++)
        {
            for (int dx = -radius; dx <= radius; dx++)
            {
                if(std::abs(dx) != radius && std::abs(dy) != radius)
                {
                    continue;
                }
                if(isGoodSpawn(centerX + dx, centerY + dy))
                {
                    x = centerX + dx;
                    y = centerY + dy;
                    return true;
                }
            }
        }
    }
    return false;
}

// Трава, вокруг которой в радиусе 2 нет деревьев и скал, чтобы не появиться запертым в лесу
bool World::isGoodSpawn(int x, int y)
{
    if(getTile(x, y) != Tile::Grass)
    {
        return false;
    }

    for (int dy = -2; dy <= 2; dy++)
    {
        for (int dx = -2; dx <= 2; dx++)
        {
            if(!isWalkable(x + dx, y + dy))
            {
                return false;
            }
        }
    }
    return true;
}

int World::getWidth()
{
    return widthInChunks * chunkSize;
}

int World::getHeight()
{
    return heightInChunks * chunkSize;
}

unsigned int World::getSeed()
{
    return seed;
}

Chunk* World::getChunk(int chunkX, int chunkY)
{
    Chunk*& chunk = chunks[chunkY * widthInChunks + chunkX];
    if(chunk == nullptr)
    {
        chunk = new Chunk;
        generateChunk(chunk, chunkX, chunkY);
    }
    return chunk;
}

void World::generateChunk(Chunk* chunk, int chunkX, int chunkY)
{
    for (int y = 0; y < chunkSize; y++)
    {
        for (int x = 0; x < chunkSize; x++)
        {
            chunk -> tiles[y][x] = generateTile(chunkX * chunkSize + x, chunkY * chunkSize + y);
        }
    }
}

Tile World::generateTile(int x, int y)
{
    float height = fractalNoise(x / 48.0f, y / 48.0f, seed, 5);

    // Мир ограничен, поэтому делаем из него остров: к краям высота падает в океан
    float dx = (x - getWidth() / 2.0f) / (getWidth() / 2.0f);
    float dy = (y - getHeight() / 2.0f) / (getHeight() / 2.0f);
    float distance = std::sqrt(dx * dx + dy * dy);
    float falloff = std::max(0.0f, distance - 0.45f) / 0.55f;
    height -= falloff * falloff * 0.5f;

    if(height < 0.29f)
    {
        return Tile::DeepWater;
    } else if (height < 0.35f)
    {
        return Tile::Water;
    } else if (height < 0.38f)
    {
        return Tile::Sand;
    } else if (height > 0.63f)
    {
        return Tile::Rock;
    }

    float forest = fractalNoise(x / 24.0f, y / 24.0f, seed + 100, 3);
    if(forest > 0.55f && hash2(x, y, seed + 200) % 100 < 40)
    {
        return Tile::Tree;
    }
    return Tile::Grass;
}

World::~World()
{
    for (size_t i = 0; i < chunks.size(); i++)
    {
        delete chunks[i];
    }
}
