#ifndef GAME_H
#define GAME_H

#include "ConRender/conrender.h"
#include "World/world.h"

#include <string>

struct TileLook
{
    std::string glyph;
    short color = 1;
};

class Game
{
public:
    Game(ConRender* renderer, unsigned int seed);
    Game(const Game&) = delete;
    Game& operator= (const Game&) = delete;

    void run();

    ~Game();
private:
    void handleInput(int key);
    void movePlayer(int dx, int dy);

    void draw();
    void drawTile(int x, int y);
    void drawHud();

    ConRender* renderer = nullptr;
    World* world = nullptr;

    int playerX = 0;
    int playerY = 0;
    bool running = true;
    long animationTime = 0;

    TileLook looks[6];
    short playerColor = 1;
    short hudColor = 1;
};

#endif // GAME_H
