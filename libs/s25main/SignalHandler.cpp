// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SignalHandler.h"
#include "GlobalVars.h"
#ifndef _WIN32
#    include <boost/nowide/iostream.hpp>
#    include <csignal>
#    include <cstdio>
#    include <cstdlib>
#endif // !_WIN32

/**
 *  Signal-Handler
 */
#ifdef _WIN32
BOOL WINAPI ConsoleSignalHandler(DWORD dwCtrlType)
{
    switch(dwCtrlType)
    {
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_C_EVENT:
        {
            GLOBALVARS.notdone = false;
            return TRUE;
        }
        break;
    }
    return FALSE;
}
#else
bool killme = false;
void ConsoleSignalHandler(int sig)
{
    if(sig == SIGINT)
    {
        if(!killme)
            boost::nowide::cout << "Do you really want to terminate the program (y/n) : " << std::flush;
        else
            boost::nowide::cout << "Do you really want to kill the program (y/n) : " << std::flush;

        int c = getchar();
        if(c == 'j' || c == 'y' || c == 1079565930)
        {
            if(killme)
                exit(1);

            killme = true;
            GLOBALVARS.notdone = false;
        }
    }
}
#endif // _WIN32
