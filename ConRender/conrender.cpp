#include "conrender.h"

ConRender::ConRender()
{
    initscr ();
    cbreak ();
    curs_set ( 0 );
    noecho ();
    keypad ( stdscr, TRUE );

    if ( has_colors () )
    {
        start_color ();
        init_pair ( 1, COLOR_WHITE, COLOR_BLACK );
        attrset ( COLOR_PAIR ( 1 ) );
    }

    updateSize ();
}

void ConRender::printxy ( std::string str, int x, int y, short color )
{
    printClipped ( str,
                   -y + m_PoseY + windowHeight/2,
                   x + m_PoseX + windowWidth/2,
                   color );
}

void ConRender::printScr ( std::string str, int col, int row, short color )
{
    printClipped ( str, row, col, color );
}

void ConRender::printClipped ( std::string str, int row, int col, short color )
{
    if ( row < 0 || row >= windowHeight )
    {
        return;
    }

    // ncurses переносит строку на следующую, если она вылезает за правый край,
    // поэтому всё, что за границами экрана, отрезаем сами
    int skip = 0;
    if ( col < 0 )
    {
        skip = -col;
        col = 0;
    }

    int length = (int)str.size() - skip;
    if ( length > windowWidth - col )
    {
        length = windowWidth - col;
    }
    if ( length <= 0 )
    {
        return;
    }

    if ( has_colors () )
    {
        attrset ( COLOR_PAIR ( color ) );
    }
    mvaddnstr ( row, col, str.c_str() + skip, length );
}

void ConRender::clearScr()
{
    erase ();
    updateSize ();
}

void ConRender::refreshScr()
{
    refresh ();
}

short ConRender::addColor ( short foreground, short background )
{
    if ( !has_colors () || colorsCount + 1 >= COLOR_PAIRS )
    {
        return 1;
    }

    colorsCount++;
    init_pair ( colorsCount, foreground, background );
    return colorsCount;
}

int ConRender::getInput()
{
    int key = getch ();
    if ( key == KEY_RESIZE )
    {
        updateSize ();
    }
    return key;
}

void ConRender::setInputTimeout ( int ms )
{
    timeout ( ms );
}

void ConRender::setViewPosexy ( int x, int y )
{
    m_PoseX = x;
    m_PoseY = y;
}

int ConRender::getWidth()
{
    return windowWidth;
}

int ConRender::getHeight()
{
    return windowHeight;
}

void ConRender::updateSize()
{
    getmaxyx ( stdscr, windowHeight, windowWidth );
}

ConRender::~ConRender()
{
    endwin();
}
