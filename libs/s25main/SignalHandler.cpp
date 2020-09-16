// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
