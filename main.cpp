#include "ConRender/conrender.h"
#include "Network/network.h"

int main(int argc, char **argv) {

    ConRender renderer;
    renderer.clearScr();
    renderer.printxy("A", 0, 0);
    renderer.printxy("s - server, c - client", 0, -2);
    renderer.refreshScr();

    int inp = renderer.getInput();

    if(inp == 's')
    {
        NetworkServer networkS (9999);
        renderer.printxy("Waiting for client...", 0, -3);
        renderer.refreshScr();

        if(networkS.acceptConnection())
        {
            renderer.printxy("Client connected.", 0, -4);

            std::string message;
            if(networkS.send("Hello from server", 0) && networkS.receive(message, 0))
            {
                renderer.printxy("Received: " + message, 0, -5);
            } else
            {
                renderer.printxy("Message exchange failed", 0, -5);
            }
        } else
        {
            renderer.printxy("Failed to accept connection", 0, -4);
        }
    } else if (inp == 'c')
    {
        NetworkClient networkC (9999);

        if(networkC.connectTo("127.0.0.1"))
        {
            renderer.printxy("Connected to 127.0.0.1", 0, -3);

            std::string message;
            if(networkC.receive(message) && networkC.send("Hello from client"))
            {
                renderer.printxy("Received: " + message, 0, -4);
            } else
            {
                renderer.printxy("Message exchange failed", 0, -4);
            }
        } else
        {
            renderer.printxy("Failed to connect to 127.0.0.1", 0, -3);
        }
    }

    renderer.refreshScr();

    renderer.getInput();

    return 0;
}
