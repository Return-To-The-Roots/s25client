// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h" // IWYU pragma: keep
#include "ctrlTimer.h"

#include "drivers/VideoDriverWrapper.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

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
    timer = VIDEODRIVER.GetTickCount();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  stoppt den Timer
 *
 *  @author FloSoft
 */
void ctrlTimer::Stop()
{
    timer = 0;
}

void ctrlTimer::Msg_PaintBefore()
{
    // timer ist deaktiviert, nix tun
    if(timer == 0)
        return;

    // Bei Timeout weiterschalten
    if(VIDEODRIVER.GetTickCount() - timer > timeout)
    {
        parent_->Msg_Timer(GetID());

        if (timer != 0)
        {
            timer = VIDEODRIVER.GetTickCount();
        }
    }
}
