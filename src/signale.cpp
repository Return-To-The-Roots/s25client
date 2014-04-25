// $Id: signale.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "signal.h"

#include "GlobalVars.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Signal-Handler
 *
 *  @author FloSoft
 */
#ifdef _WIN32
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
    switch(dwCtrlType)
    {
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_C_EVENT:
        {
            GLOBALVARS.notdone = false;
            return TRUE;
        } break;
    }
    return FALSE;
}
#else
bool killme = false;
void HandlerRoutine(int sig)
{

    int c;
    if(sig != SIGINT)
        return;
    else
    {
        if(!killme)
            LOG.lprintf("Wollen Sie das Programm beenden (j/n) : ");
        else
            LOG.lprintf("Wollen Sie das Programm killen (j/n) : ");

        c = getchar();
        if(c == 'j' || c == 1079565930)
        {
            if(killme == true)
                exit(1);

            killme = true;
            GLOBALVARS.notdone = false;
        }
        else
            return;
    }
}
#endif // _WIN32
