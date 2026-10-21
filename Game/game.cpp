#include "game.h"

#include <chrono>
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

    foamColor = renderer -> addColor(COLOR_WHITE, COLOR_BLACK, true);
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
                if(isFree(spawnX + dx, spawnY + dy))
                {
                    players[id].active = true;
                    players[id].x = spawnX + dx;
                    players[id].y = spawnY + dy;
                    return true;
                }
            }
        }
    }
    return false;
}

bool Game::isPlayerAt(int x, int y)
{
    for (int i = 0; i < maxPlayers; i++)
    {
        if(players[i].active && players[i].x == x && players[i].y == y)
        {
            return true;
        }
    }
    return false;
}

bool Game::isFree(int x, int y)
{
    return world -> isWalkable(x, y) && !isPlayerAt(x, y);
}

bool Game::tryMove(int id, int dx, int dy)
{
    Player& player = players[id];
    long now = nowMs();

    if(now - player.lastMoveMs < moveCooldownMs || !isFree(player.x + dx, player.y + dy))
    {
        return false;
    }

    player.x += dx;
    player.y += dy;
    player.lastMoveMs = now;
    return true;
}

bool Game::tryDig(int id, int dx, int dy)
{
    Player& player = players[id];
    long now = nowMs();
    int x = player.x + dx;
    int y = player.y + dy;

    if(now - player.lastActionMs < actionCooldownMs)
    {
        return false;
    }

    Tile tile = world -> getTile(x, y);
    Tile result = tile;
    if(tile == Tile::Tree || tile == Tile::Sapling)
    {
        result = Tile::Grass;
    } else if (tile == Tile::Rock)
    {
        result = Tile::RockFloor;
    } else if (tile == Tile::Wall || (tile == Tile::Bridge && !isPlayerAt(x, y)))
    {
        result = world -> getGroundTile(x, y);
    } else
    {
        return false;
    }

    player.lastActionMs = now;

    int key = cellKey(x, y);
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
    } else
    {
        growthTimes.erase(key);
    }

    changeTile(x, y, result);
    return true;
}

bool Game::tryBuild(int id, int dx, int dy)
{
    Player& player = players[id];
    long now = nowMs();
    int x = player.x + dx;
    int y = player.y + dy;

    if(now - player.lastActionMs < actionCooldownMs || !world -> isInside(x, y) || isPlayerAt(x, y))
    {
        return false;
    }

    Tile tile = world -> getTile(x, y);
    if(isWater(tile))
    {
        changeTile(x, y, Tile::Bridge);
    } else if (tile == Tile::Grass || tile == Tile::Sand || tile == Tile::RockFloor || tile == Tile::Sapling)
    {
        changeTile(x, y, Tile::Wall);
    } else
    {
        return false;
    }

    player.lastActionMs = now;
    return true;
}

// Хост запоминает все изменения мира, чтобы отдать их тем, кто подключится позже
void Game::changeTile(int x, int y, Tile tile)
{
    world -> setTile(x, y, tile);
    changedTiles[cellKey(x, y)] = tile;
    damages.erase(cellKey(x, y));

    broadcast(tileMessage(x, y, tile));
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

    std::map <int, long>::iterator growth = growthTimes.begin();
    while(growth != growthTimes.end())
    {
        int x = growth -> first % world -> getWidth();
        int y = growth -> first / world -> getWidth();
        Tile tile = world -> getTile(x, y);

        if(now < growth -> second)
        {
            growth++;
        } else if (tile != Tile::Grass && tile != Tile::Sapling)
        {
            growth = growthTimes.erase(growth);
        } else if (isPlayerAt(x, y))
        {
            growth -> second = now + growRetryMs;
            growth++;
        } else if (tile == Tile::Grass)
        {
            changeTile(x, y, Tile::Sapling);
            growth -> second = now + treeGrowTimeMs;
            growth++;
        } else
        {
            changeTile(x, y, Tile::Tree);
            growth = growthTimes.erase(growth);
        }
    }
}

int Game::cellKey(int x, int y)
{
    return y * world -> getWidth() + x;
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
        doAction(true);
        break;
    case 'r':
        doAction(false);
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

    if(tryMove(myId, dx, dy) && server != nullptr)
    {
        broadcast(positionMessage(myId));
    }
}

void Game::doAction(bool dig)
{
    if(client != nullptr)
    {
        std::string type = dig ? "DIG " : "BUILD ";
        client -> send(type + std::to_string(facingX) + " " + std::to_string(facingY));
        return;
    }

    if(dig)
    {
        tryDig(myId, facingX, facingY);
    } else
    {
        tryBuild(myId, facingX, facingY);
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
                int x = it -> first % world -> getWidth();
                int y = it -> first / world -> getWidth();
                server -> send(tileMessage(x, y, it -> second), connectionId);
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
    return "POS " + std::to_string(id) + " " + std::to_string(players[id].x) + " " + std::to_string(players[id].y);
}

std::string Game::tileMessage(int x, int y, Tile tile)
{
    return "TILE " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string((int)tile);
}

std::string Game::damageMessage(int key, int hits)
{
    int x = key % world -> getWidth();
    int y = key / world -> getWidth();
    return "DAMAGE " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(hits);
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
        int tile = -1;
        if(in >> x >> y >> tile && tile >= 0 && tile < tileTypesCount && world -> isInside(x, y))
        {
            world -> setTile(x, y, (Tile)tile);
            damages.erase(cellKey(x, y));
        }
        return;
    }

    if(type == "DAMAGE")
    {
        int x = 0;
        int y = 0;
        int hits = 0;
        if(in >> x >> y >> hits && world -> isInside(x, y))
        {
            if(hits > 0)
            {
                damages[cellKey(x, y)].hits = hits;
            } else
            {
                damages.erase(cellKey(x, y));
            }
        }
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
        if(in >> x >> y)
        {
            players[id].active = true;
            players[id].x = x;
            players[id].y = y;
        }
    } else if (type == "LEAVE")
    {
        players[id] = Player();
    }
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

    for (int y = me.y - halfHeight; y <= me.y + halfHeight; y++)
    {
        for (int x = me.x - halfWidth; x <= me.x + halfWidth; x++)
        {
            drawTile(x, y, false);
        }
    }

    if(me.active)
    {
        drawTile(me.x + facingX, me.y + facingY, true);
    }

    for (int i = 0; i < maxPlayers; i++)
    {
        if(players[i].active && i != myId)
        {
            renderer -> printxy("@", players[i].x, players[i].y, playerColors[i]);
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
    return !isWater(world -> getTile(x + 1, y)) || !isWater(world -> getTile(x - 1, y))
           || !isWater(world -> getTile(x, y + 1)) || !isWater(world -> getTile(x, y - 1));
}

void Game::drawTile(int x, int y, bool highlight)
{
    Tile tile = world -> getTile(x, y);
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

    if(world -> isInside(x, y) && damages.count(cellKey(x, y)) > 0)
    {
        color = highlight ? damagedHighlight : damagedColor;
    }

    renderer -> printxy(glyph, x, y, color);
}

void Game::drawHud()
{
    int playersCount = 0;
    for (int i = 0; i < maxPlayers; i++)
    {
        if(players[i].active)
        {
            playersCount++;
        }
    }

    std::string info = " x:" + std::to_string(players[myId].x) + " y:" + std::to_string(players[myId].y)
                       + "  seed:" + std::to_string(world -> getSeed());
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

    std::string help = " arrows/WASD - move, e - dig, r - build, q - quit ";
    renderer -> printScr(help, 0, renderer -> getHeight() - 1, hudColor);
}

Game::~Game()
{
    delete client;
    delete server;
    delete world;
}
