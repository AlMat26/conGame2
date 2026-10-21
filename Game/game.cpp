#include "game.h"

#include <chrono>

const int worldSizeInChunks = 8;
const int frameTimeMs = 50;
const int waterFrameMs = 250;

Game::Game(ConRender* renderer, unsigned int seed)
{
    this -> renderer = renderer;
    world = new World (worldSizeInChunks, worldSizeInChunks, seed);

    short blue = renderer -> addColor(COLOR_BLUE, COLOR_BLACK);
    short cyan = renderer -> addColor(COLOR_CYAN, COLOR_BLACK);
    short yellow = renderer -> addColor(COLOR_YELLOW, COLOR_BLACK);
    short green = renderer -> addColor(COLOR_GREEN, COLOR_BLACK);

    looks[(int)Tile::DeepWater] = { "~", blue };
    looks[(int)Tile::Water] = { "~", cyan };
    looks[(int)Tile::Sand] = { ",", yellow };
    looks[(int)Tile::Grass] = { ".", green };
    looks[(int)Tile::Tree] = { "T", green };
    looks[(int)Tile::Rock] = { "#", 1 };

    playerColor = renderer -> addColor(COLOR_BLACK, COLOR_YELLOW);
    hudColor = renderer -> addColor(COLOR_BLACK, COLOR_WHITE);

    if(!world -> findSpawn(playerX, playerY))
    {
        playerX = world -> getWidth() / 2;
        playerY = world -> getHeight() / 2;
    }
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
        draw();
    }

    renderer -> setInputTimeout(-1);
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
    case 'q':
        running = false;
        break;
    }
}

void Game::movePlayer(int dx, int dy)
{
    if(world -> isWalkable(playerX + dx, playerY + dy))
    {
        playerX += dx;
        playerY += dy;
    }
}

void Game::draw()
{
    animationTime = std::chrono::duration_cast <std::chrono::milliseconds> (
                        std::chrono::steady_clock::now().time_since_epoch()).count() / waterFrameMs;

    renderer -> clearScr();
    renderer -> setViewPosexy(-playerX, playerY);

    int halfWidth = renderer -> getWidth() / 2 + 1;
    int halfHeight = renderer -> getHeight() / 2 + 1;

    for (int y = playerY - halfHeight; y <= playerY + halfHeight; y++)
    {
        for (int x = playerX - halfWidth; x <= playerX + halfWidth; x++)
        {
            drawTile(x, y);
        }
    }

    renderer -> printxy("@", playerX, playerY, playerColor);
    drawHud();
    renderer -> refreshScr();
}

void Game::drawTile(int x, int y)
{
    Tile tile = world -> getTile(x, y);
    TileLook& look = looks[(int)tile];

    if(tile == Tile::DeepWater || tile == Tile::Water)
    {
        // Диагональные «волны» бегут по воде, небольшой сдвиг по хешу ломает ровные полосы
        unsigned int noise = ((unsigned int)x * 73856093u) ^ ((unsigned int)y * 19349663u);
        long phase = x + 2 * y + animationTime + (noise >> 7) % 3;
        if(phase % 11 == 0)
        {
            renderer -> printxy("-", x, y, look.color);
            return;
        }
    }

    renderer -> printxy(look.glyph, x, y, look.color);
}

void Game::drawHud()
{
    std::string position = " x:" + std::to_string(playerX) + " y:" + std::to_string(playerY)
                           + "  seed:" + std::to_string(world -> getSeed()) + " ";
    renderer -> printScr(position, 0, 0, hudColor);

    std::string help = " arrows/WASD - move, q - quit ";
    renderer -> printScr(help, 0, renderer -> getHeight() - 1, hudColor);
}

Game::~Game()
{
    delete world;
}
