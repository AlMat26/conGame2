#include "ConRender/conrender.h"
#include "Network/network.h"

int main(int argc, char **argv) {

    ConRender renderer;
    renderer.clearScr();
    renderer.printxy("A", 0, 0);
    renderer.refreshScr();

    Network networkC (9999, false);
    Network networkS (9999, true);

    char inp = renderer.getInput();

    if(inp == 's')
    {
        networkS.acceptConnection();
        renderer.printxy("Client connected.", 1, 0);
    } else if (inp == 'c')
    {
        networkC.connectTo("127.0.0.1");
        renderer.printxy("Connected to 127.0.0.1", 1, 0);
    }

    renderer.refreshScr();

    renderer.getInput();

    return 0;
}
