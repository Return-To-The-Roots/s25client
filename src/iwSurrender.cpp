// $Id: iwSurrender.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "iwSurrender.h"

#include "Loader.h"
#include "GameManager.h"

#include "dskMainMenu.h"
#include "iwSave.h"
#include "WindowManager.h"
#include "GameClient.h"
#include "GameCommands.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p iwSurrender.
 *
 *  @author jh
 */
iwSurrender::iwSurrender(void)
    : IngameWindow(CGI_ENDGAME, 0xFFFF, 0xFFFF, 240, 100, _("Surrender game?"), LOADER.GetImageN("resource", 41))
{
    // Ok
    AddImageButton(0,  85, 24, 68, 57, TC_GREEN2, LOADER.GetImageN("io", 32), _("Surrender"));
    // Ok + Abbrennen
    AddImageButton(2,  16, 24, 68, 57, TC_GREEN2, LOADER.GetImageN("io", 23), _("Destroy all buildings and surrender"));
    // Abbrechen
    AddImageButton(1,  158, 24, 68, 57, TC_RED1, LOADER.GetImageN("io", 40), _("Don't surrender"));
}


void iwSurrender::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 0: // OK
        {
            GameClient::inst().AddGC(new gc::Surrender);
            Close();
        } break;
        case 1: // Abbrechen
        {
            Close();
        } break;
        case 2: // OK + Alles abbrennen
        {
            GameClient::inst().AddGC(new gc::DestroyAll);
            Close();
        } break;
    }
}
