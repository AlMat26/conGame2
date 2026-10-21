#include "game.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <thread>

const int worldSizeInChunks = 8;
const int frameTimeMs = 30;
const int waterFrameMs = 250;
const int moveCooldownMs = 30;
const int actionCooldownMs = 150;
const int joinTimeoutMs = 5000;
const int damageResetMs = 3000;
const int saplingTimeMs = 60000;
const int treeGrowTimeMs = 90000;
const int growRetryMs = 5000;
const int noteTimeMs = 2500;

const int woodFromTree = 2;
const int woodFromSupport = 1;
const int woodPerTunnel = 1;
const int supportStep = 3;
const int woodPerBuilding = 1;
const int woodPerTorch = 1;

// Обвалы: свод держится, если рядом крепь или стена, или если выработка узкая —
// в квадрате 3×3 вокруг клетки меньше collapseOpenCells выкопанных клеток
const int supportRadius = 2;
const int collapseOpenCells = 7;
const int collapseRadius = 4;
const int collapseDelayMs = 2000;
const int caveInHearRadius = 12;

// Уровни света: 0 — темно, 1 — край света (только символ плотности),
// 2 — полутень, 3 и больше — светло
const int fullLight = 3;
const int playerLightPower = 8;
const int torchLightPower = 10;

static long nowMs()
{
    return std::chrono::duration_cast <std::chrono::milliseconds> (
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Сколько ударов нужно, чтобы сломать клетку
static int getDurability(Tile tile)
{
    switch(tile)
    {
    case Tile::Tree:
        return 3;
    case Tile::Rock:
        return 5;
    case Tile::Wall:
        return 4;
    case Tile::Bridge:
        return 2;
    case Tile::Support:
        return 2;
    case Tile::Ore:
        return 6;
    case Tile::Rubble:
        return 2;
    default:
        return 1;
    }
}

static bool isWater(Tile tile)
{
    return tile == Tile::Water || tile == Tile::DeepWater;
}

Game::Game(ConRender* renderer)
{
    this -> renderer = renderer;

    // Несколько символов в строке — варианты, выбираются по координатам клетки
    setLook(Tile::DeepWater, "~", COLOR_BLUE, false);
    setLook(Tile::Water, "~", COLOR_CYAN, false);
    setLook(Tile::Sand, ",", COLOR_YELLOW, true);
    setLook(Tile::Grass, "........'", COLOR_GREEN, false);
    setLook(Tile::Tree, "TTT^", COLOR_GREEN, true);
    setLook(Tile::Rock, "#", COLOR_WHITE, false);
    setLook(Tile::RockFloor, ".", COLOR_WHITE, false);
    setLook(Tile::Wall, "#", COLOR_YELLOW, false);
    setLook(Tile::Bridge, "=", COLOR_YELLOW, false);
    setLook(Tile::Sapling, "\"", COLOR_GREEN, false);
    setLook(Tile::Support, "+", COLOR_YELLOW, false);
    setLook(Tile::Ore, "*", COLOR_MAGENTA, true);
    setLook(Tile::LadderDown, ">", COLOR_YELLOW, true);
    setLook(Tile::LadderUp, "<", COLOR_YELLOW, true);
    setLook(Tile::Rubble, "%", COLOR_WHITE, true);
    setLook(Tile::Torch, "i", COLOR_YELLOW, true);

    foamColor = renderer -> addColor(COLOR_WHITE, COLOR_BLACK, true);
    flameColor = renderer -> addColor(COLOR_RED, COLOR_BLACK, true);
    shadowColor = renderer -> addColor(COLOR_BLUE, COLOR_BLACK);
    damagedColor = renderer -> addColor(COLOR_RED, COLOR_BLACK, true);
    damagedHighlight = renderer -> addColor(COLOR_BLACK, COLOR_RED);

    playerColors[0] = renderer -> addColor(COLOR_BLACK, COLOR_YELLOW);
    playerColors[1] = renderer -> addColor(COLOR_BLACK, COLOR_CYAN);
    playerColors[2] = renderer -> addColor(COLOR_BLACK, COLOR_MAGENTA);
    playerColors[3] = renderer -> addColor(COLOR_BLACK, COLOR_RED);
    hudColor = renderer -> addColor(COLOR_BLACK, COLOR_WHITE);
}

bool Game::startSolo(unsigned int seed)
{
    createWorld(seed);
    return spawnPlayer(0);
}

bool Game::startHost(unsigned int seed, unsigned int port)
{
    server = new NetworkServer (port);
    if(!server -> start())
    {
        message = "Failed to open port " + std::to_string(port);
        return false;
    }

    hostAddress = server -> getLocalAddress() + ":" + std::to_string(port);
    createWorld(seed);
    return spawnPlayer(0);
}

bool Game::startJoin(std::string ip, unsigned int port)
{
    std::string address = ip + ":" + std::to_string(port);
    showStatus("Connecting to " + address + "...");

    client = new NetworkClient (port);
    if(!client -> connectTo(ip))
    {
        message = "Failed to connect to " + address;
        return false;
    }

    showStatus("Waiting for host...");

    long startTime = nowMs();
    while(nowMs() - startTime < joinTimeoutMs)
    {
        std::string text;
        if(client -> receive(text))
        {
            std::istringstream in (text);
            std::string type;
            int id = -1;
            unsigned int seed = 0;
            in >> type;

            if(type == "WELCOME" && in >> id >> seed && id >= 0 && id < maxPlayers)
            {
                myId = id;
                createWorld(seed);
                return true;
            } else if (type == "FULL")
            {
                message = "Server is full";
                return false;
            }
        }

        if(!client -> isConnected())
        {
            message = "Host closed the connection";
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    message = "Host did not answer";
    return false;
}

void Game::run()
{
    renderer -> setInputTimeout(frameTimeMs);

    while(running)
    {
        int key = renderer -> getInput();
        if(key != ERR)
        {
            handleInput(key);
        }

        if(client == nullptr)
        {
            updateWorld();
        }
        if(server != nullptr)
        {
            acceptPlayers();
            receiveFromClients();
        }
        if(client != nullptr)
        {
            receiveFromHost();
        }

        draw();
    }

    renderer -> setInputTimeout(-1);
}

std::string Game::getMessage()
{
    return message;
}

// Обычный вид — цветной символ на чёрном, подсветка (клетка перед игроком) — наоборот
void Game::setLook(Tile tile, std::string glyphs, short foreground, bool bold)
{
    TileLook& look = looks[(int)tile];
    look.glyphs = glyphs;
    look.color = renderer -> addColor(foreground, COLOR_BLACK, bold);
    look.highlight = renderer -> addColor(COLOR_BLACK, foreground);
}

void Game::createWorld(unsigned int seed)
{
    delete world;
    world = new World (worldSizeInChunks, worldSizeInChunks, seed);
}

// Ищет ближайшую к точке появления свободную клетку, чтобы игроки не стояли друг на друге
bool Game::spawnPlayer(int id)
{
    int spawnX = world -> getWidth() / 2;
    int spawnY = world -> getHeight() / 2;
    world -> findSpawn(spawnX, spawnY);

    for (int radius = 0; radius < 10; radius++)
    {
        for (int dy = -radius; dy <= radius; dy++)
        {
            for (int dx = -radius; dx <= radius; dx++)
            {
                if(isFree(spawnX + dx, spawnY + dy, 0))
                {
                    players[id].active = true;
                    players[id].x = spawnX + dx;
                    players[id].y = spawnY + dy;
                    players[id].z = 0;
                    return true;
                }
            }
        }
    }
    return false;
}

bool Game::isPlayerAt(int x, int y, int z)
{
    for (int i = 0; i < maxPlayers; i++)
    {
        if(players[i].active && players[i].x == x && players[i].y == y && players[i].z == z)
        {
            return true;
        }
    }
    return false;
}

bool Game::isFree(int x, int y, int z)
{
    return world -> isWalkable(x, y, z) && !isPlayerAt(x, y, z);
}

bool Game::tryMove(int id, int dx, int dy)
{
    Player& player = players[id];
    long now = nowMs();
    int x = player.x + dx;
    int y = player.y + dy;
    int z = player.z;

    if(now - player.lastMoveMs < moveCooldownMs || !isFree(x, y, z))
    {
        return false;
    }

    // Шагнул на лестницу — оказывается на парной лестнице соседнего уровня
    Tile tile = world -> getTile(x, y, z);
    if(tile == Tile::LadderDown)
    {
        z--;
    } else if (tile == Tile::LadderUp)
    {
        z++;
    }
    if(z != player.z && isPlayerAt(x, y, z))
    {
        return false;
    }

    player.x = x;
    player.y = y;
    player.z = z;
    player.lastMoveMs = now;
    return true;
}

bool Game::tryDig(int id, int dx, int dy)
{
    Player& player = players[id];
    long now = nowMs();
    int x = player.x + dx;
    int y = player.y + dy;
    int z = player.z;

    if(now - player.lastActionMs < actionCooldownMs)
    {
        return false;
    }

    Tile tile = world -> getTile(x, y, z);
    Tile result = tile;
    if(tile == Tile::Tree || tile == Tile::Sapling)
    {
        result = Tile::Grass;
    } else if (tile == Tile::Rock || tile == Tile::Ore || tile == Tile::Support || tile == Tile::Rubble)
    {
        result = Tile::RockFloor;
    } else if (tile == Tile::Wall || tile == Tile::Torch || (tile == Tile::Bridge && !isPlayerAt(x, y, z)))
    {
        result = world -> getGroundTile(x, y, z);
    } else
    {
        return false;
    }

    player.lastActionMs = now;

    int key = cellKey(x, y, z);
    Damage& damage = damages[key];
    damage.hits++;
    damage.lastHitMs = now;

    if(damage.hits < getDurability(tile))
    {
        broadcast(damageMessage(key, damage.hits));
        return true;
    }

    // На месте срубленного дерева со временем вырастет новое, срубленный росток — конец
    if(tile == Tile::Tree)
    {
        growthTimes[key] = now + saplingTimeMs;
        player.wood += woodFromTree;
    } else
    {
        growthTimes.erase(key);
    }

    if(tile == Tile::Support)
    {
        player.wood += woodFromSupport;
    } else if (tile == Tile::Wall || tile == Tile::Bridge)
    {
        player.wood += woodPerBuilding;
    } else if (tile == Tile::Torch)
    {
        player.wood += woodPerTorch;
    } else if (tile == Tile::Ore)
    {
        player.ore++;
    }

    changeTile(x, y, z, result);

    // Под землёй выкопанное игроком может обвалиться. Если есть дерево —
    // проход укрепляется крепью по бокам через каждые две клетки
    bool usedWood = false;
    bool opened = tile == Tile::Rock || tile == Tile::Ore || tile == Tile::Rubble || tile == Tile::Support;
    if(z < 0 && opened)
    {
        dugCells.insert(key);

        int along = dx != 0 ? x : y;
        if(tile != Tile::Support && along % supportStep == 0 && player.wood >= woodPerTunnel
           && placeSupports(x, y, z, dx, dy))
        {
            player.wood -= woodPerTunnel;
            usedWood = true;
        }
        checkCeiling(id, x, y, z);
    }

    if(tile == Tile::Tree || tile == Tile::Support || tile == Tile::Wall || tile == Tile::Bridge
       || tile == Tile::Torch || tile == Tile::Ore || usedWood)
    {
        sendInventory(id);
    }
    return true;
}

bool Game::tryBuild(int id, int dx, int dy)
{
    Player& player = players[id];
    long now = nowMs();
    int x = player.x + dx;
    int y = player.y + dy;
    int z = player.z;

    if(now - player.lastActionMs < actionCooldownMs || !world -> isInside(x, y, z) || isPlayerAt(x, y, z))
    {
        return false;
    }

    Tile tile = world -> getTile(x, y, z);
    Tile result = tile;
    if(isWater(tile))
    {
        result = Tile::Bridge;
    } else if (tile == Tile::Grass || tile == Tile::Sand || tile == Tile::RockFloor || tile == Tile::Sapling)
    {
        result = Tile::Wall;
    } else
    {
        return false;
    }

    player.lastActionMs = now;

    // Стены и мосты строятся из дерева, при разборе оно возвращается
    if(player.wood < woodPerBuilding)
    {
        notify(id, "Need wood");
        return false;
    }

    player.wood -= woodPerBuilding;
    changeTile(x, y, z, result);
    sendInventory(id);
    return true;
}

// Факел ставится на пол перед собой. По нему можно ходить, свод он не держит
bool Game::tryPlaceTorch(int id, int dx, int dy)
{
    Player& player = players[id];
    long now = nowMs();
    int x = player.x + dx;
    int y = player.y + dy;
    int z = player.z;

    if(now - player.lastActionMs < actionCooldownMs || !world -> isInside(x, y, z) || isPlayerAt(x, y, z))
    {
        return false;
    }

    Tile tile = world -> getTile(x, y, z);
    if(tile != Tile::RockFloor && tile != Tile::Grass && tile != Tile::Sand)
    {
        return false;
    }

    player.lastActionMs = now;

    if(player.wood < woodPerTorch)
    {
        notify(id, "Need wood");
        return false;
    }

    player.wood -= woodPerTorch;
    changeTile(x, y, z, Tile::Torch);
    sendInventory(id);
    return true;
}

// Крепь встаёт по бокам прохода — там, где осталась сплошная порода
bool Game::placeSupports(int x, int y, int z, int dx, int dy)
{
    int sideX[2] = { x + dy, x - dy };
    int sideY[2] = { y + dx, y - dx };
    bool placed = false;

    for (int i = 0; i < 2; i++)
    {
        if(world -> getTile(sideX[i], sideY[i], z) == Tile::Rock && world -> isInside(sideX[i], sideY[i], z))
        {
            changeTile(sideX[i], sideY[i], z, Tile::Support);
            placed = true;
        }
    }
    return placed;
}

bool Game::isDug(int x, int y, int z)
{
    return world -> isInside(x, y, z) && dugCells.count(cellKey(x, y, z)) > 0;
}

bool Game::isStable(int x, int y, int z)
{
    int openCells = 0;

    for (int dy = -supportRadius; dy <= supportRadius; dy++)
    {
        for (int dx = -supportRadius; dx <= supportRadius; dx++)
        {
            Tile tile = world -> getTile(x + dx, y + dy, z);
            if(tile == Tile::Support || tile == Tile::Wall)
            {
                return true;
            }
            if(std::abs(dx) <= 1 && std::abs(dy) <= 1 && isDug(x + dx, y + dy, z))
            {
                openCells++;
            }
        }
    }
    return openCells < collapseOpenCells;
}

// После копания проверяет выработку рядом: если свод не держится — через пару секунд обвал
void Game::checkCeiling(int id, int x, int y, int z)
{
    for (int dy = -collapseRadius; dy <= collapseRadius; dy++)
    {
        for (int dx = -collapseRadius; dx <= collapseRadius; dx++)
        {
            if(isDug(x + dx, y + dy, z) && !isStable(x + dx, y + dy, z))
            {
                if(collapseTimes.count(cellKey(x, y, z)) == 0)
                {
                    collapseTimes[cellKey(x, y, z)] = nowMs() + collapseDelayMs;
                    notify(id, "The ceiling is cracking!");
                }
                return;
            }
        }
    }
}

// Неустойчивые выкопанные клетки рядом засыпает завалом. Клетку с игроком — нет:
// его запирает, выкапываться придётся самому
void Game::collapse(int key)
{
    int centerX = 0;
    int centerY = 0;
    int z = 0;
    keyToCell(key, centerX, centerY, z);

    std::vector <Point> fallen;
    for (int dy = -collapseRadius; dy <= collapseRadius; dy++)
    {
        for (int dx = -collapseRadius; dx <= collapseRadius; dx++)
        {
            int x = centerX + dx;
            int y = centerY + dy;
            if(isDug(x, y, z) && !isStable(x, y, z) && !isPlayerAt(x, y, z))
            {
                Point point;
                point.x = x;
                point.y = y;
                fallen.push_back(point);
            }
        }
    }

    for (size_t i = 0; i < fallen.size(); i++)
    {
        changeTile(fallen[i].x, fallen[i].y, z, Tile::Rubble);
    }

    if(fallen.empty())
    {
        return;
    }

    for (int i = 0; i < maxPlayers; i++)
    {
        Player& player = players[i];
        if(player.active && player.z == z && std::abs(player.x - centerX) + std::abs(player.y - centerY) <= caveInHearRadius)
        {
            notify(i, "Cave-in!");
        }
    }
}

// Хост запоминает все изменения мира, чтобы отдать их тем, кто подключится позже
void Game::changeTile(int x, int y, int z, Tile tile)
{
    world -> setTile(x, y, z, tile);
    changedTiles[cellKey(x, y, z)] = tile;
    damages.erase(cellKey(x, y, z));
    // Выкопанная клетка остаётся выкопанной и под стеной или факелом, пока её не завалит
    if(tile == Tile::Rubble)
    {
        dugCells.erase(cellKey(x, y, z));
    }

    broadcast(tileMessage(x, y, z, tile));
}

// Мир живёт только у хоста (и в одиночной игре): сброс повреждений и рост деревьев
void Game::updateWorld()
{
    long now = nowMs();

    std::map <int, Damage>::iterator damage = damages.begin();
    while(damage != damages.end())
    {
        if(now - damage -> second.lastHitMs > damageResetMs)
        {
            broadcast(damageMessage(damage -> first, 0));
            damage = damages.erase(damage);
        } else
        {
            damage++;
        }
    }

    std::map <int, long>::iterator collapsing = collapseTimes.begin();
    while(collapsing != collapseTimes.end())
    {
        if(now >= collapsing -> second)
        {
            collapse(collapsing -> first);
            collapsing = collapseTimes.erase(collapsing);
        } else
        {
            collapsing++;
        }
    }

    std::map <int, long>::iterator growth = growthTimes.begin();
    while(growth != growthTimes.end())
    {
        int x = 0;
        int y = 0;
        int z = 0;
        keyToCell(growth -> first, x, y, z);
        Tile tile = world -> getTile(x, y, z);

        if(now < growth -> second)
        {
            growth++;
        } else if (tile != Tile::Grass && tile != Tile::Sapling)
        {
            growth = growthTimes.erase(growth);
        } else if (isPlayerAt(x, y, z))
        {
            growth -> second = now + growRetryMs;
            growth++;
        } else if (tile == Tile::Grass)
        {
            changeTile(x, y, z, Tile::Sapling);
            growth -> second = now + treeGrowTimeMs;
            growth++;
        } else
        {
            changeTile(x, y, z, Tile::Tree);
            growth = growthTimes.erase(growth);
        }
    }
}

int Game::cellKey(int x, int y, int z)
{
    return (-z * world -> getHeight() + y) * world -> getWidth() + x;
}

void Game::keyToCell(int key, int& x, int& y, int& z)
{
    x = key % world -> getWidth();
    y = key / world -> getWidth() % world -> getHeight();
    z = -(key / (world -> getWidth() * world -> getHeight()));
}

// Подсказка игроку внизу экрана (например, что не хватает дерева)
void Game::notify(int id, std::string text)
{
    if(id == myId && client == nullptr)
    {
        note = text;
        noteUntilMs = nowMs() + noteTimeMs;
    } else if (server != nullptr)
    {
        server -> send("NOTE " + text, players[id].connectionId);
    }
}

void Game::sendInventory(int id)
{
    if(server != nullptr && id != myId)
    {
        server -> send("INV " + std::to_string(players[id].wood) + " " + std::to_string(players[id].ore),
                       players[id].connectionId);
    }
}

void Game::handleInput(int key)
{
    switch(key)
    {
    case KEY_UP:
    case 'w':
        movePlayer(0, 1);
        break;
    case KEY_DOWN:
    case 's':
        movePlayer(0, -1);
        break;
    case KEY_LEFT:
    case 'a':
        movePlayer(-1, 0);
        break;
    case KEY_RIGHT:
    case 'd':
        movePlayer(1, 0);
        break;
    case 'e':
        doAction("DIG");
        break;
    case 'r':
        doAction("BUILD");
        break;
    case 't':
        doAction("TORCH");
        break;
    case 'q':
        running = false;
        break;
    }
}

// Клиент только сообщает о намерении, двигает игрока хост
void Game::movePlayer(int dx, int dy)
{
    facingX = dx;
    facingY = dy;

    if(client != nullptr)
    {
        client -> send("MOVE " + std::to_string(dx) + " " + std::to_string(dy));
        return;
    }

    if(tryMove(myId, dx, dy))
    {
        broadcast(positionMessage(myId));
    }
}

void Game::doAction(std::string type)
{
    if(client != nullptr)
    {
        client -> send(type + " " + std::to_string(facingX) + " " + std::to_string(facingY));
        return;
    }

    if(type == "DIG")
    {
        tryDig(myId, facingX, facingY);
    } else if (type == "BUILD")
    {
        tryBuild(myId, facingX, facingY);
    } else if (type == "TORCH")
    {
        tryPlaceTorch(myId, facingX, facingY);
    }
}

void Game::acceptPlayers()
{
    int connectionId = server -> acceptConnection();

    while(connectionId >= 0)
    {
        int id = -1;
        for (int i = 1; i < maxPlayers && id < 0; i++)
        {
            if(!players[i].active)
            {
                id = i;
            }
        }

        if(id < 0 || !spawnPlayer(id))
        {
            server -> send("FULL", connectionId);
            server -> disconnect(connectionId);
        } else
        {
            players[id].connectionId = connectionId;
            server -> send("WELCOME " + std::to_string(id) + " " + std::to_string(world -> getSeed()), connectionId);

            for (std::map <int, Tile>::iterator it = changedTiles.begin(); it != changedTiles.end(); it++)
            {
                int x = 0;
                int y = 0;
                int z = 0;
                keyToCell(it -> first, x, y, z);
                server -> send(tileMessage(x, y, z, it -> second), connectionId);
            }

            for (int i = 0; i < maxPlayers; i++)
            {
                if(players[i].active && i != id)
                {
                    server -> send(positionMessage(i), connectionId);
                }
            }
            broadcast(positionMessage(id));
        }

        connectionId = server -> acceptConnection();
    }
}

void Game::receiveFromClients()
{
    for (int id = 1; id < maxPlayers; id++)
    {
        if(!players[id].active)
        {
            continue;
        }

        std::string text;
        while(server -> receive(text, players[id].connectionId))
        {
            handleClientMessage(id, text);
        }

        if(!server -> isConnected(players[id].connectionId))
        {
            removePlayer(id);
        }
    }
}

void Game::handleClientMessage(int id, const std::string& text)
{
    std::istringstream in (text);
    std::string type;
    in >> type;

    int dx = 0;
    int dy = 0;
    if(!(in >> dx >> dy) || std::abs(dx) + std::abs(dy) != 1)
    {
        return;
    }

    if(type == "MOVE" && tryMove(id, dx, dy))
    {
        broadcast(positionMessage(id));
    } else if (type == "DIG")
    {
        tryDig(id, dx, dy);
    } else if (type == "BUILD")
    {
        tryBuild(id, dx, dy);
    } else if (type == "TORCH")
    {
        tryPlaceTorch(id, dx, dy);
    }
}

void Game::removePlayer(int id)
{
    server -> disconnect(players[id].connectionId);
    players[id] = Player();
    broadcast("LEAVE " + std::to_string(id));
}

void Game::broadcast(const std::string& text)
{
    if(server == nullptr)
    {
        return;
    }

    for (int i = 1; i < maxPlayers; i++)
    {
        if(players[i].active)
        {
            server -> send(text, players[i].connectionId);
        }
    }
}

std::string Game::positionMessage(int id)
{
    return "POS " + std::to_string(id) + " " + std::to_string(players[id].x) + " " + std::to_string(players[id].y)
           + " " + std::to_string(players[id].z);
}

std::string Game::tileMessage(int x, int y, int z, Tile tile)
{
    return "TILE " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z) + " "
           + std::to_string((int)tile);
}

std::string Game::damageMessage(int key, int hits)
{
    int x = 0;
    int y = 0;
    int z = 0;
    keyToCell(key, x, y, z);
    return "DAMAGE " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z) + " "
           + std::to_string(hits);
}

void Game::receiveFromHost()
{
    std::string text;
    while(client -> receive(text))
    {
        handleHostMessage(text);
    }

    if(!client -> isConnected())
    {
        message = "Disconnected from host";
        running = false;
    }
}

void Game::handleHostMessage(const std::string& text)
{
    std::istringstream in (text);
    std::string type;
    in >> type;

    if(type == "TILE")
    {
        int x = 0;
        int y = 0;
        int z = 0;
        int tile = -1;
        if(in >> x >> y >> z >> tile && tile >= 0 && tile < tileTypesCount && world -> isInside(x, y, z))
        {
            world -> setTile(x, y, z, (Tile)tile);
            damages.erase(cellKey(x, y, z));
        }
        return;
    }

    if(type == "DAMAGE")
    {
        int x = 0;
        int y = 0;
        int z = 0;
        int hits = 0;
        if(in >> x >> y >> z >> hits && world -> isInside(x, y, z))
        {
            if(hits > 0)
            {
                damages[cellKey(x, y, z)].hits = hits;
            } else
            {
                damages.erase(cellKey(x, y, z));
            }
        }
        return;
    }

    if(type == "INV")
    {
        int wood = 0;
        int ore = 0;
        if(in >> wood >> ore)
        {
            players[myId].wood = wood;
            players[myId].ore = ore;
        }
        return;
    }

    if(type == "NOTE")
    {
        std::getline(in >> std::ws, note);
        noteUntilMs = nowMs() + noteTimeMs;
        return;
    }

    int id = -1;
    if(!(in >> id) || id < 0 || id >= maxPlayers)
    {
        return;
    }

    if(type == "POS")
    {
        int x = 0;
        int y = 0;
        int z = 0;
        if(in >> x >> y >> z)
        {
            players[id].active = true;
            players[id].x = x;
            players[id].y = y;
            players[id].z = z;
        }
    } else if (type == "LEAVE")
    {
        players[id] = Player();
    }
}

// Источники света на уровне: игроки и факелы. Факелы ищутся в прямоугольнике
// экрана, расширенном на их силу — факел за краем экрана тоже светит на экран
void Game::collectLights(int z, int left, int bottom, int width, int height, std::vector <LightSource>& lights)
{
    for (int i = 0; i < maxPlayers; i++)
    {
        if(players[i].active && players[i].z == z)
        {
            LightSource light;
            light.x = players[i].x;
            light.y = players[i].y;
            light.power = playerLightPower;
            lights.push_back(light);
        }
    }

    for (int y = bottom - torchLightPower; y < bottom + height + torchLightPower; y++)
    {
        for (int x = left - torchLightPower; x < left + width + torchLightPower; x++)
        {
            if(world -> getTile(x, y, z) == Tile::Torch)
            {
                LightSource light;
                light.x = x;
                light.y = y;
                light.power = torchLightPower - getFlicker(x, y);
                lights.push_back(light);
            }
        }
    }
}

// Пламя факела иногда приседает: 1 — в этот момент он светит чуть слабее
int Game::getFlicker(int x, int y)
{
    unsigned int noise = ((unsigned int)x * 73856093u) ^ ((unsigned int)y * 19349663u);
    return (animationTime + (noise >> 8) % 5) % 5 == 0 ? 1 : 0;
}

// Освещённость считается для прямоугольника экрана: от каждого источника
// по прямой видимости, клетка получает самый яркий из доставших до неё
void Game::computeLight(int z, int left, int bottom, int width, int height)
{
    lightLeft = left;
    lightBottom = bottom;
    lightWidth = width;
    lightHeight = height;
    lightMap.assign(width * height, 0);

    std::vector <LightSource> lights;
    collectLights(z, left, bottom, width, height, lights);

    for (size_t i = 0; i < lights.size(); i++)
    {
        LightSource& light = lights[i];

        for (int dy = -light.power; dy <= light.power; dy++)
        {
            for (int dx = -light.power; dx <= light.power; dx++)
            {
                int column = light.x + dx - left;
                int row = light.y + dy - bottom;
                if(column < 0 || row < 0 || column >= width || row >= height)
                {
                    continue;
                }

                // Клетка терминала примерно в полтора-два раза выше, чем шире,
                // поэтому по вертикали свет «короче» — пятно выглядит круглее
                int distance = (int)std::lround(std::sqrt(dx * dx + dy * dy * 2.25));
                int level = light.power - distance;
                int& cell = lightMap[row * width + column];

                if(level > cell && isLineClear(light.x, light.y, light.x + dx, light.y + dy, z))
                {
                    cell = level;
                }
            }
        }
    }

    // Прямая до дальней клетки стены упирается в соседние клетки той же стены,
    // поэтому стена получает свет от освещённого пола рядом с ней
    std::vector <int> floorLight = lightMap;
    for (int row = 0; row < height; row++)
    {
        for (int column = 0; column < width; column++)
        {
            if(!world -> isOpaque(left + column, bottom + row, z))
            {
                continue;
            }

            int& cell = lightMap[row * width + column];
            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dx = -1; dx <= 1; dx++)
                {
                    int nearColumn = column + dx;
                    int nearRow = row + dy;
                    if(nearColumn < 0 || nearRow < 0 || nearColumn >= width || nearRow >= height
                       || world -> isOpaque(left + nearColumn, bottom + nearRow, z))
                    {
                        continue;
                    }
                    cell = std::max(cell, floorLight[nearRow * width + nearColumn]);
                }
            }
        }
    }
}

// Прямая Брезенхэма от источника до клетки: свет не проходит сквозь камень.
// Сама клетка может быть непрозрачной — стену тоже видно
bool Game::isLineClear(int fromX, int fromY, int toX, int toY, int z)
{
    int dx = std::abs(toX - fromX);
    int dy = -std::abs(toY - fromY);
    int stepX = fromX < toX ? 1 : -1;
    int stepY = fromY < toY ? 1 : -1;
    int error = dx + dy;
    int x = fromX;
    int y = fromY;

    while(x != toX || y != toY)
    {
        int doubled = 2 * error;
        if(doubled >= dy)
        {
            error += dy;
            x += stepX;
        }
        if(doubled <= dx)
        {
            error += dx;
            y += stepY;
        }

        if((x != toX || y != toY) && world -> isOpaque(x, y, z))
        {
            return false;
        }
    }
    return true;
}

// На поверхности день — светло везде. Под землёй — по карте освещённости
int Game::getLight(int x, int y, int z)
{
    if(z >= 0)
    {
        return fullLight;
    }

    int column = x - lightLeft;
    int row = y - lightBottom;
    if(column < 0 || row < 0 || column >= lightWidth || row >= lightHeight)
    {
        return 0;
    }
    return lightMap[row * lightWidth + column];
}

void Game::showStatus(std::string text)
{
    renderer -> clearScr();
    renderer -> setViewPosexy(0, 0);
    renderer -> printxy(text, -(int)text.size() / 2, 0);
    renderer -> refreshScr();
}

void Game::draw()
{
    animationTime = nowMs() / waterFrameMs;

    Player& me = players[myId];
    renderer -> clearScr();
    renderer -> setViewPosexy(-me.x, me.y);

    int halfWidth = renderer -> getWidth() / 2 + 1;
    int halfHeight = renderer -> getHeight() / 2 + 1;

    if(me.z < 0)
    {
        computeLight(me.z, me.x - halfWidth, me.y - halfHeight, 2 * halfWidth + 1, 2 * halfHeight + 1);
    }

    for (int y = me.y - halfHeight; y <= me.y + halfHeight; y++)
    {
        for (int x = me.x - halfWidth; x <= me.x + halfWidth; x++)
        {
            drawTile(x, y, me.z, false);
        }
    }

    if(me.active)
    {
        drawTile(me.x + facingX, me.y + facingY, me.z, true);
    }

    for (int i = 0; i < maxPlayers; i++)
    {
        Player& other = players[i];
        if(other.active && i != myId && other.z == me.z && getLight(other.x, other.y, other.z) > 0)
        {
            renderer -> printxy("@", other.x, other.y, playerColors[i]);
        }
    }
    if(me.active)
    {
        renderer -> printxy("@", me.x, me.y, playerColors[myId]);
    }

    drawHud();
    renderer -> refreshScr();
}

bool Game::isCoast(int x, int y)
{
    return !isWater(world -> getTile(x + 1, y, 0)) || !isWater(world -> getTile(x - 1, y, 0))
           || !isWater(world -> getTile(x, y + 1, 0)) || !isWater(world -> getTile(x, y - 1, 0));
}

void Game::drawTile(int x, int y, int z, bool highlight)
{
    int light = getLight(x, y, z);
    if(light <= 0)
    {
        return;
    }

    Tile tile = world -> getTile(x, y, z);
    TileLook& look = looks[(int)tile];

    unsigned int noise = ((unsigned int)x * 73856093u) ^ ((unsigned int)y * 19349663u);
    std::string glyph = look.glyphs.substr((noise >> 5) % look.glyphs.size(), 1);
    short color = highlight ? look.highlight : look.color;

    if(tile == Tile::DeepWater)
    {
        // Диагональные «волны» бегут по глубокой воде, сдвиг по хешу ломает ровные полосы
        long phase = x + 2 * y + animationTime + (noise >> 7) % 3;
        if(phase % 11 == 0)
        {
            glyph = "-";
        }
    } else if (tile == Tile::Water)
    {
        // У берега вспыхивает пена прибоя, на мелководье изредка блики
        long phase = animationTime + (noise >> 9) % 64;
        if(isCoast(x, y) && phase % 8 == 0 && !highlight)
        {
            color = foamColor;
        } else if (phase % 19 == 0)
        {
            glyph = "-";
        }
    }

    if(tile == Tile::Torch && getFlicker(x, y) == 1 && !highlight)
    {
        color = flameColor;
    }

    if(world -> isInside(x, y, z) && damages.count(cellKey(x, y, z)) > 0)
    {
        color = highlight ? damagedHighlight : damagedColor;
    }

    // Край света — только символ плотности, полутень — синим
    if(light == 1)
    {
        glyph = world -> isOpaque(x, y, z) ? ":" : ".";
        color = shadowColor;
    } else if (light == 2 && !highlight)
    {
        color = shadowColor;
    }

    renderer -> printxy(glyph, x, y, color);
}

void Game::drawHud()
{
    Player& me = players[myId];

    int playersCount = 0;
    for (int i = 0; i < maxPlayers; i++)
    {
        if(players[i].active)
        {
            playersCount++;
        }
    }

    std::string info = " x:" + std::to_string(me.x) + " y:" + std::to_string(me.y);
    if(me.z < 0)
    {
        info += " depth:" + std::to_string(-me.z);
    }
    info += "  seed:" + std::to_string(world -> getSeed());
    if(server != nullptr)
    {
        info += "  host " + hostAddress;
    }
    if(server != nullptr || client != nullptr)
    {
        info += "  players:" + std::to_string(playersCount) + "/" + std::to_string(maxPlayers);
    }
    info += " ";
    renderer -> printScr(info, 0, 0, hudColor);
    renderer -> printScr("@", info.size() + 1, 0, playerColors[myId]);

    if(!note.empty() && nowMs() < noteUntilMs)
    {
        renderer -> printScr(" " + note + " ", 0, renderer -> getHeight() - 2, damagedHighlight);
    }

    std::string bottom = " wood:" + std::to_string(me.wood) + " ore:" + std::to_string(me.ore)
                         + "  WASD - move, e - dig, r - build, t - torch, q - quit ";
    renderer -> printScr(bottom, 0, renderer -> getHeight() - 1, hudColor);
}

Game::~Game()
{
    delete client;
    delete server;
    delete world;
}
