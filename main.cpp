#include "ConRender/conrender.h"
#include "Network/network.h"

int main(int argc, char **argv) {

    ConRender renderer;
    renderer.clearScr();
    renderer.printxy("A", 0, 0);
    renderer.refreshScr();

    //Network networkC (9999, false);
    //Network networkS (9999, true);

    //networkS.acceptConnection();

    renderer.getInput();
    return 0;
}
