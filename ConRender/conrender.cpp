#include "conrender.h"

ConRender::ConRender()
{
    initscr ();
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
              str.c_str() );
}

void ConRender::clear()
{
    clear();
    getmaxyx ( stdscr, windowHeight, windowWidth );
}

void ConRender::refresh()
{
    refresh ();
}

char ConRender::getInput()
{
    return getchar();
}

ConRender::~ConRender()
{
    endwin();
}
