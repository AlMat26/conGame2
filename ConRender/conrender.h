#ifndef CONRENDER_H
#define CONRENDER_H

#include <ncurses.h>
#include <string>


class ConRender
{
public:
    ConRender();
    void printxy(std::string str, int x, int y);
    void clearScr();
    void refreshScr();

    char getInput();

    void setViewPosexy(int x, int y);

    ~ConRender();
private:
    int m_PoseX = 0;
    int m_PoseY = 0;

    unsigned int windowWidth = 0;
    unsigned int windowHeight = 0;
};

#endif // CONRENDER_H
