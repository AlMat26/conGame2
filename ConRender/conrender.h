#ifndef CONRENDER_H
#define CONRENDER_H

#define NCURSES_NOMACROS

#include <ncurses.h>
#include <string>
#include <vector>


class ConRender
{
public:
    ConRender();
    ConRender(const ConRender&) = delete;
    ConRender& operator= (const ConRender&) = delete;

    void printxy(std::string str, int x, int y, short color = 1);
    void printScr(std::string str, int col, int row, short color = 1);
    void clearScr();
    void refreshScr();

    short addColor(short foreground, short background, bool bold = false);

    int getInput();
    void setInputTimeout(int ms);

    void setViewPosexy(int x, int y);

    int getWidth();
    int getHeight();

    ~ConRender();
private:
    void printClipped(std::string str, int row, int col, short color);
    void updateSize();

    int m_PoseX = 0;
    int m_PoseY = 0;

    int windowWidth = 0;
    int windowHeight = 0;

    short colorsCount = 1;
    std::vector <bool> boldColors;
};

#endif // CONRENDER_H
