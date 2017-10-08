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

#include "defines.h" // IWYU pragma: keep
#include "dskSplash.h"

#include "GameManager.h"
#include "Loader.h"
#include "WindowManager.h"
#include "dskMainMenu.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/oglIncludes.h"

dskSplash::dskSplash() : Desktop(LOADER.GetImageN("splash", 0))
{
    background->setFilter(GL_LINEAR);
    GAMEMANAGER.SetCursor(CURSOR_NONE);
    AddTimer(0, 5000);
}

dskSplash::~dskSplash()
{
    GAMEMANAGER.SetCursor();
}

void dskSplash::Msg_Timer(const unsigned /*ctrl_id*/)
{
    // Hauptmenü zeigen
    WINDOWMANAGER.Switch(new dskMainMenu);
}

bool dskSplash::Msg_LeftDown(const MouseCoords& /*mc*/)
{
    // Hauptmenü zeigen
    WINDOWMANAGER.Switch(new dskMainMenu, true);

    return true;
}
