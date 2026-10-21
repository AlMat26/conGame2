#include "ConRender/conrender.h"
#include "Game/game.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

static const unsigned int defaultPort = 9999;

static void printUsage(const char* name)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s                    play alone\n"
            "  %s play [seed]        play alone on the given world seed\n"
            "  %s host [port]        host a game for friends\n"
            "  %s join <ip> [port]   join a friend's game\n"
            "Default port: %u\n",
            name, name, name, name, defaultPort);
}

static bool parseNumber(const char* str, unsigned long maxValue, unsigned long& number)
{
    char* end = nullptr;
    unsigned long value = strtoul(str, &end, 10);

    if(end == str || *end != '\0' || str[0] == '-' || value > maxValue)
    {
        return false;
    }
    number = value;
    return true;
}

static bool parsePort(const char* str, unsigned int& port)
{
    unsigned long value = 0;

    if(!parseNumber(str, 65535, value) || value == 0)
    {
        return false;
    }
    port = value;
    return true;
}

int main(int argc, char **argv) {

    int mode = 'p';
    std::string ip = "127.0.0.1";
    unsigned int port = defaultPort;
    unsigned int seed = time(nullptr);
    bool argsOk = true;

    if(argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        printUsage(argv[0]);
        return 0;
    } else if (argc >= 2 && argc <= 3 && strcmp(argv[1], "play") == 0)
    {
        if(argc == 3)
        {
            unsigned long value = 0;
            argsOk = parseNumber(argv[2], 0xFFFFFFFFul, value);
            seed = value;
        }
    } else if (argc >= 2 && argc <= 3 && strcmp(argv[1], "host") == 0)
    {
        mode = 's';
        if(argc == 3)
        {
            argsOk = parsePort(argv[2], port);
        }
    } else if (argc >= 3 && argc <= 4 && strcmp(argv[1], "join") == 0)
    {
        mode = 'c';
        ip = argv[2];
        if(argc == 4)
        {
            argsOk = parsePort(argv[3], port);
        }
    } else if (argc != 1)
    {
        argsOk = false;
    }

    if(!argsOk)
    {
        printUsage(argv[0]);
        return 1;
    }

    ConRender renderer;
    Game game (&renderer);
    bool started = false;

    if(mode == 'p')
    {
        started = game.startSolo(seed);
    } else if (mode == 's')
    {
        started = game.startHost(seed, port);
    } else
    {
        started = game.startJoin(ip, port);
    }

    if(started)
    {
        game.run();
    }

    std::string message = game.getMessage();
    if(!message.empty())
    {
        renderer.clearScr();
        renderer.setViewPosexy(0, 0);
        renderer.printxy(message, -(int)message.size() / 2, 0);
        renderer.printxy("Press any key", -6, -2);
        renderer.refreshScr();
        renderer.getInput();
    }

    return 0;
}
