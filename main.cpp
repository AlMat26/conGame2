#include "ConRender/conrender.h"
#include "Network/network.h"
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
            "  %s                    play\n"
            "  %s play [seed]        play on the given world seed\n"
            "  %s host [port]        test connection: host\n"
            "  %s join <ip> [port]   test connection: join\n"
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

static void runHostTest(ConRender& renderer, unsigned int port)
{
    NetworkServer networkS (port);
    renderer.printxy("Hosting on port " + std::to_string(port), 0, -3);
    renderer.printxy("Waiting for client...", 0, -4);
    renderer.refreshScr();

    if(networkS.acceptConnection())
    {
        renderer.printxy("Client connected.", 0, -5);

        std::string message;
        if(networkS.send("Hello from server", 0) && networkS.receive(message, 0))
        {
            renderer.printxy("Received: " + message, 0, -6);
        } else
        {
            renderer.printxy("Message exchange failed", 0, -6);
        }
    } else
    {
        renderer.printxy("Failed to open port " + std::to_string(port), 0, -5);
    }
}

static void runJoinTest(ConRender& renderer, std::string ip, unsigned int port)
{
    std::string address = ip + ":" + std::to_string(port);

    NetworkClient networkC (port);
    renderer.printxy("Connecting to " + address + "...", 0, -3);
    renderer.refreshScr();

    if(networkC.connectTo(ip))
    {
        renderer.printxy("Connected to " + address, 0, -4);

        std::string message;
        if(networkC.receive(message) && networkC.send("Hello from client"))
        {
            renderer.printxy("Received: " + message, 0, -5);
        } else
        {
            renderer.printxy("Message exchange failed", 0, -5);
        }
    } else
    {
        renderer.printxy("Failed to connect to " + address, 0, -4);
    }
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

    if(mode == 'p')
    {
        Game game (&renderer, seed);
        game.run();
        return 0;
    }

    short yellow = renderer.addColor(COLOR_YELLOW, COLOR_BLACK);
    renderer.clearScr();
    renderer.printxy("A", 0, 0, yellow);

    if(mode == 's')
    {
        runHostTest(renderer, port);
    } else
    {
        runJoinTest(renderer, ip, port);
    }

    renderer.refreshScr();

    renderer.getInput();

    return 0;
}
