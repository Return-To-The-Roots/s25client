// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "iwPleaseWait.h"
#include "Loader.h"
#include "WindowManager.h"
#include "ogl/FontStyle.h"
#include "gameData/const_gui_ids.h"

/**
 *  Konstruktor von @p iwPleaseWait.
 *
 *  Fenster wird modal geöffnet, damit man ggf. einen "Weiter"-Button nicht
 *  mehrfach betätigen kann.
 */
iwPleaseWait::iwPleaseWait()
    : IngameWindow(CGI_PLEASEWAIT, IngameWindow::posLastOrCenter, Extent(300, 60), _("Please wait..."),
                   LOADER.GetImageN("resource", 41), true, false)
{
    WINDOWMANAGER.SetCursor(Cursor::Moon);
    AddText(0, GetSize() / 2, _("Please wait..."), COLOR_YELLOW, FontStyle::CENTER | FontStyle::VCENTER, NormalFont);
}

iwPleaseWait::~iwPleaseWait()
{
    WINDOWMANAGER.SetCursor();
}
