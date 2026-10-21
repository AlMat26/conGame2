#include "conrender.h"

ConRender::ConRender()
{
    initscr ();
    cbreak ();
    start_color ();
    curs_set ( 0 );
    noecho ();
    keypad ( stdscr, TRUE );

    init_pair ( 1, COLOR_WHITE, COLOR_BLACK );
    attrset ( COLOR_PAIR ( 1 ) );

    getmaxyx ( stdscr, windowHeight, windowWidth );
}

void ConRender::printxy ( std::string str, int x, int y )
{
    mvprintw( -y + m_PoseY + windowHeight/2,
              x + m_PoseX + windowWidth/2,
              "%s",
              str.c_str() );
}

void ConRender::clearScr()
{
    clear();
    getmaxyx ( stdscr, windowHeight, windowWidth );
}

void ConRender::refreshScr()
{
    refresh ();
}

int ConRender::getInput()
{
    return getch();
}

void ConRender::setViewPosexy ( int x, int y )
{
    m_PoseX = x;
    m_PoseY = y;
}

ConRender::~ConRender()
{
    endwin();
}
