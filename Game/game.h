#ifndef GAME_H
#define GAME_H

#include "ConRender/conrender.h"
#include "Network/network.h"
#include "World/world.h"

#include <map>
#include <string>

const int maxPlayers = 4;

struct TileLook
{
    std::string glyphs;
    short color = 1;
    short highlight = 1;
};

struct Damage
{
    int hits = 0;
    long lastHitMs = 0;
};

struct Player
{
    bool active = false;
    int x = 0;
    int y = 0;
    int connectionId = -1;
    long lastMoveMs = 0;
    long lastActionMs = 0;
};

class Game
{
public:
    Game(ConRender* renderer);
    Game(const Game&) = delete;
    Game& operator= (const Game&) = delete;

    bool startSolo(unsigned int seed);
    bool startHost(unsigned int seed, unsigned int port);
    bool startJoin(std::string ip, unsigned int port);
    void run();

    std::string getMessage();

    ~Game();
private:
    void setLook(Tile tile, std::string glyphs, short foreground, bool bold);
    void createWorld(unsigned int seed);
    bool spawnPlayer(int id);
    bool isPlayerAt(int x, int y);
    bool isFree(int x, int y);
    bool tryMove(int id, int dx, int dy);
    bool tryDig(int id, int dx, int dy);
    bool tryBuild(int id, int dx, int dy);
    void changeTile(int x, int y, Tile tile);
    void updateWorld();
    int cellKey(int x, int y);

    void handleInput(int key);
    void movePlayer(int dx, int dy);
    void doAction(bool dig);

    void acceptPlayers();
    void receiveFromClients();
    void handleClientMessage(int id, const std::string& text);
    void removePlayer(int id);
    void broadcast(const std::string& text);
    std::string positionMessage(int id);
    std::string tileMessage(int x, int y, Tile tile);
    std::string damageMessage(int key, int hits);

    void receiveFromHost();
    void handleHostMessage(const std::string& text);

    void showStatus(std::string text);
    void draw();
    bool isCoast(int x, int y);
    void drawTile(int x, int y, bool highlight);
    void drawHud();

    ConRender* renderer = nullptr;
    World* world = nullptr;
    NetworkServer* server = nullptr;
    NetworkClient* client = nullptr;

    Player players[maxPlayers];
    int myId = 0;
    int facingX = 0;
    int facingY = 1;
    bool running = true;
    long animationTime = 0;

    std::string message;
    std::string hostAddress;

    std::map <int, Tile> changedTiles;
    std::map <int, Damage> damages;
    std::map <int, long> growthTimes;

    TileLook looks[tileTypesCount];
    short playerColors[maxPlayers];
    short hudColor = 1;
    short foamColor = 1;
    short damagedColor = 1;
    short damagedHighlight = 1;
};

#endif // GAME_H
