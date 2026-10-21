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

    void printxy(std::string str, int x, int y);
    void clearScr();
    void refreshScr();

    int getInput();

    void setViewPosexy(int x, int y);

    ~ConRender();
private:
    int m_PoseX = 0;
    int m_PoseY = 0;

    int windowWidth = 0;
    int windowHeight = 0;
};

#endif // CONRENDER_H
