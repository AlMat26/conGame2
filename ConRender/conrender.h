#ifndef CONRENDER_H
#define CONRENDER_H

#define NCURSES_NOMACROS

#include <ncurses.h>
#include <string>


class ConRender
{
public:
    ConRender();
    ConRender(const ConRender&) = delete;
    ConRender& operator= (const ConRender&) = delete;

    void printxy(std::string str, int x, int y, short color = 1);
    void clearScr();
    void refreshScr();

    short addColor(short foreground, short background);

    int getInput();
    void setInputTimeout(int ms);

    void setViewPosexy(int x, int y);

    int getWidth();
    int getHeight();

    ~ConRender();
private:
    void updateSize();

    int m_PoseX = 0;
    int m_PoseY = 0;

    int windowWidth = 0;
    int windowHeight = 0;

    short colorsCount = 1;
};

#endif // CONRENDER_H
