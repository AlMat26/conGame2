#include "ConRender/conrender.h"

int main(int argc, char **argv) {

    ConRender renderer;
    renderer.clearScr();
    renderer.printxy("A", 0, 0);
    renderer.refreshScr();
    renderer.getInput();
    return 0;
}
