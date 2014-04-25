// $Id: ctrlTimer.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "ctrlTimer.h"

#include "VideoDriverWrapper.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/** @var ctrlTimer::timer
 *
 *  Der Timer zum Abgleichen der Zeit.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/** @var ctrlTimer::timeout
 *
 *  Die Zeit nach der der Timer zünden soll.
 *
 *  @author FloSoft
 */

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p ctrlTimer.
 *
 *  @author FloSoft
 */
ctrlTimer::ctrlTimer(Window* parent,
                     unsigned int id,
                     unsigned int timeout)
    : Window(0, 0, id, parent)
{
    Start(timeout);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  startet den Timer.
 *
 *  @author FloSoft
 */
void ctrlTimer::Start(unsigned int timeout)
{
    this->timeout = timeout;

    // timer initialisieren
    timer = VideoDriverWrapper::inst().GetTickCount();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  stoppt den Timer
 *
 *  @author FloSoft
 */
void ctrlTimer::Stop(void)
{
    timer = 0;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
void ctrlTimer::Msg_PaintBefore()
{
    // timer ist deaktiviert, nix tun
    if(timer == 0)
        return;

    // Bei Timeout weiterschalten
    if(VideoDriverWrapper::inst().GetTickCount() - timer > timeout)
    {
        parent->Msg_Timer(GetID());

        if (timer != 0)
        {
            timer = VideoDriverWrapper::inst().GetTickCount();
        }
    }
}
