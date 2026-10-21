#include "world.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

const int mineEntrances = 4;
const int laddersPerLevel = 3;

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

static bool isFarFrom(const std::vector <Point>& points, Point point, int distance)
{
    for (size_t i = 0; i < points.size(); i++)
    {
        if(std::abs(points[i].x - point.x) + std::abs(points[i].y - point.y) < distance)
        {
            return false;
        }
    }
    return true;
}

static Point findNearest(const std::vector <Point>& points, Point point)
{
    Point nearest = points[0];
    for (size_t i = 1; i < points.size(); i++)
    {
        if(std::abs(points[i].x - point.x) + std::abs(points[i].y - point.y)
           < std::abs(nearest.x - point.x) + std::abs(nearest.y - point.y))
        {
            nearest = points[i];
        }
    }
    return nearest;
}

static bool isPointIn(const std::vector <Point>& points, int x, int y)
{
    for (size_t i = 0; i < points.size(); i++)
    {
        if(points[i].x == x && points[i].y == y)
        {
            return true;
        }
    }
    return false;
}

World::World(int widthInChunks, int heightInChunks, unsigned int seed)
{
    this -> widthInChunks = widthInChunks;
    this -> heightInChunks = heightInChunks;
    this -> seed = seed;

    chunks.resize(widthInChunks * heightInChunks * (mineDepth + 1), nullptr);
    generateMines();
}

Tile World::getTile(int x, int y, int z)
{
    if(!isInside(x, y, z))
    {
        return z < 0 ? Tile::Rock : Tile::DeepWater;
    }

    Chunk* chunk = getChunk(x / chunkSize, y / chunkSize, z);
    return chunk -> tiles[y % chunkSize][x % chunkSize];
}

void World::setTile(int x, int y, int z, Tile tile)
{
    if(!isInside(x, y, z))
    {
        return;
    }

    Chunk* chunk = getChunk(x / chunkSize, y / chunkSize, z);
    chunk -> tiles[y % chunkSize][x % chunkSize] = tile;
}

// Что лежит под постройкой или под выкопанной скалой — берётся из генератора,
// поэтому хранить это отдельно не нужно
Tile World::getGroundTile(int x, int y, int z)
{
    Tile tile = generateTile(x, y, z);

    if(tile == Tile::Rock || tile == Tile::Ore || tile == Tile::Support)
    {
        return Tile::RockFloor;
    } else if (tile == Tile::Tree)
    {
        return Tile::Grass;
    }
    return tile;
}

bool World::isInside(int x, int y, int z)
{
    return x >= 0 && y >= 0 && x < getWidth() && y < getHeight() && z <= 0 && z >= -mineDepth;
}

bool World::isWalkable(int x, int y, int z)
{
    Tile tile = getTile(x, y, z);
    return tile != Tile::DeepWater && tile != Tile::Tree && tile != Tile::Rock && tile != Tile::Wall
           && tile != Tile::Support && tile != Tile::Ore && tile != Tile::Rubble;
}

// Сквозь что не проходит свет
bool World::isOpaque(int x, int y, int z)
{
    Tile tile = getTile(x, y, z);
    return tile == Tile::Tree || tile == Tile::Rock || tile == Tile::Wall
           || tile == Tile::Support || tile == Tile::Ore || tile == Tile::Rubble;
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
    if(getTile(x, y, 0) != Tile::Grass)
    {
        return false;
    }

    for (int dy = -2; dy <= 2; dy++)
    {
        for (int dx = -2; dx <= 2; dx++)
        {
            if(!isWalkable(x + dx, y + dy, 0) || getTile(x + dx, y + dy, 0) == Tile::LadderDown)
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

// Шахты расставляются один раз при создании мира: входы, лестницы, штольни.
// Генератор случайных чисел с тем же seed — у всех игроков шахты одинаковые
void World::generateMines()
{
    std::mt19937 random (seed);
    levels.resize(mineDepth + 1);

    // Входы — на траве у подножия скал, далеко друг от друга
    for (int attempt = 0; attempt < 5000 && (int)levels[0].laddersDown.size() < mineEntrances; attempt++)
    {
        Point point;
        point.x = 32 + random() % (getWidth() - 64);
        point.y = 32 + random() % (getHeight() - 64);
        bool anywhere = attempt > 4000;

        if(generateSurfaceTile(point.x, point.y) == Tile::Grass && (anywhere || isNearRock(point))
           && isFarFrom(levels[0].laddersDown, point, 40))
        {
            levels[0].laddersDown.push_back(point);
        }
    }

    for (int depth = 1; depth <= mineDepth; depth++)
    {
        MineLevel& level = levels[depth];
        level.laddersUp = levels[depth - 1].laddersDown;

        for (int attempt = 0; attempt < 1000 && depth < mineDepth && (int)level.laddersDown.size() < laddersPerLevel; attempt++)
        {
            Point point;
            point.x = 24 + random() % (getWidth() - 48);
            point.y = 24 + random() % (getHeight() - 48);

            if(isFarFrom(level.laddersUp, point, 12) && isFarFrom(level.laddersDown, point, 30))
            {
                level.laddersDown.push_back(point);
            }
        }

        for (size_t i = 0; i < level.laddersUp.size(); i++)
        {
            Point up = level.laddersUp[i];
            addRoom(level, up.x - 2, up.y - 1, up.x + 2, up.y + 1);
            if(!level.laddersDown.empty())
            {
                addCorridor(level, up, findNearest(level.laddersDown, up));
            }
        }

        for (size_t i = 0; i < level.laddersDown.size(); i++)
        {
            Point down = level.laddersDown[i];
            addRoom(level, down.x - 2, down.y - 1, down.x + 2, down.y + 1);
            if(!level.laddersUp.empty())
            {
                addCorridor(level, findNearest(level.laddersUp, down), down);
            }
        }
    }
}

bool World::isNearRock(Point point)
{
    for (int dy = -4; dy <= 4; dy++)
    {
        for (int dx = -4; dx <= 4; dx++)
        {
            if(generateSurfaceTile(point.x + dx, point.y + dy) == Tile::Rock)
            {
                return true;
            }
        }
    }
    return false;
}

void World::addRoom(MineLevel& level, int left, int bottom, int right, int top)
{
    MineRoom room;
    room.left = std::min(left, right);
    room.right = std::max(left, right);
    room.bottom = std::min(bottom, top);
    room.top = std::max(bottom, top);
    level.rooms.push_back(room);
}

// Штольня буквой «Г»: сначала по горизонтали, потом по вертикали
void World::addCorridor(MineLevel& level, Point from, Point to)
{
    addRoom(level, from.x, from.y, to.x, from.y);
    addRoom(level, to.x, from.y, to.x, to.y);
}

Chunk* World::getChunk(int chunkX, int chunkY, int z)
{
    Chunk*& chunk = chunks[(-z * heightInChunks + chunkY) * widthInChunks + chunkX];
    if(chunk == nullptr)
    {
        chunk = new Chunk;
        generateChunk(chunk, chunkX, chunkY, z);
    }
    return chunk;
}

void World::generateChunk(Chunk* chunk, int chunkX, int chunkY, int z)
{
    for (int y = 0; y < chunkSize; y++)
    {
        for (int x = 0; x < chunkSize; x++)
        {
            chunk -> tiles[y][x] = generateTile(chunkX * chunkSize + x, chunkY * chunkSize + y, z);
        }
    }
}

Tile World::generateTile(int x, int y, int z)
{
    if(z < 0)
    {
        return generateUndergroundTile(x, y, z);
    }

    if(isPointIn(levels[0].laddersDown, x, y))
    {
        return Tile::LadderDown;
    }
    return generateSurfaceTile(x, y);
}

Tile World::generateSurfaceTile(int x, int y)
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

Tile World::generateUndergroundTile(int x, int y, int z)
{
    int depth = -z;
    MineLevel& level = levels[depth];

    if(isPointIn(level.laddersUp, x, y))
    {
        return Tile::LadderUp;
    } else if (isPointIn(level.laddersDown, x, y))
    {
        return Tile::LadderDown;
    }

    bool nearRoom = false;
    for (size_t i = 0; i < level.rooms.size(); i++)
    {
        MineRoom& room = level.rooms[i];
        if(x >= room.left && x <= room.right && y >= room.bottom && y <= room.top)
        {
            return Tile::RockFloor;
        }
        if(x >= room.left - 1 && x <= room.right + 1 && y >= room.bottom - 1 && y <= room.top + 1)
        {
            nearRoom = true;
        }
    }

    float cave = fractalNoise(x / 14.0f, y / 14.0f, seed + 1000 + depth * 31, 3);
    if(cave > 0.64f)
    {
        return Tile::RockFloor;
    }

    // Стены штолен заброшенной шахты — крепь через клетку (в пещерах стен нет)
    if(nearRoom)
    {
        return (x + y) % 2 == 0 ? Tile::Support : Tile::Rock;
    }

    float vein = fractalNoise(x / 5.0f, y / 5.0f, seed + 2000 + depth * 31, 2);
    if(vein > 0.78f - depth * 0.01f)
    {
        return Tile::Ore;
    }
    return Tile::Rock;
}

World::~World()
{
    for (size_t i = 0; i < chunks.size(); i++)
    {
        delete chunks[i];
    }
}
