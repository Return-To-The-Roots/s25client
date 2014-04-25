// $Id: dskMainMenu.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "dskMainMenu.h"

#include "WindowManager.h"
#include "Loader.h"
#include "GlobalVars.h"
#include "controls.h"

#include "Settings.h"

#include "dskAboutRTTR.h"
#include "dskSinglePlayer.h"
#include "dskMultiPlayer.h"
#include "dskOptions.h"
#include "dskIntro.h"
#include "dskCredits.h"
#include "iwMsgbox.h"

#include "ListDir.h"
#include "iwTextfile.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/** @class dskMainMenu
 *
 *  Klasse des Hauptmen¸ Desktops.
 *
 *  @author OLiver
 */

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p dskMainMenu.
 *
 *  @author OLiver
 *  @author FloSoft
 */
dskMainMenu::dskMainMenu(void) : Desktop(LOADER.GetImageN("menu", 0))
{
    // Version
    AddVarText(0, 0, 600, _("Return To The Roots - v%s-%s"), COLOR_YELLOW, 0 | glArchivItem_Font::DF_BOTTOM, NormalFont, 2, GetWindowVersion(), GetWindowRevision());
    // URL
    AddText(1, 400, 600, _("http://www.siedler25.org"), COLOR_GREEN, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, NormalFont);
    // Copyright
    AddVarText(2, 800, 600, _("© 2005 - %s Settlers Freaks"), COLOR_YELLOW, glArchivItem_Font::DF_RIGHT | glArchivItem_Font::DF_BOTTOM, NormalFont, 1, GetCurrentYear());

    // "Einzelspieler"
    AddTextButton(4, 115, 180, 220, 22, TC_GREEN2, _("Singleplayer"), NormalFont);
    // "Mehrspieler"
    AddTextButton(5, 115, 210, 220, 22, TC_GREEN2, _("Multiplayer"), NormalFont);
    // "Optionen"
    AddTextButton(6, 115, 250, 220, 22, TC_GREEN2, _("Options"), NormalFont);
    // "Intro"
    AddTextButton(7, 115, 280, 220, 22, TC_GREEN2, _("Intro"), NormalFont);
    // "ReadMe"
    AddTextButton(10, 115, 310, 220, 22, TC_GREEN2, _("Readme"), NormalFont);
    // "Credits"
    AddTextButton(8, 115, 340, 220, 22, TC_GREEN2, _("Credits"), NormalFont);
    // "Programm verlassen"
    AddTextButton(9, 115, 390, 220, 22, TC_RED1, _("Quit program"), NormalFont);

    AddImage(11, 20, 20, LOADER.GetImageN("logo", 0));

    if (SETTINGS.global.submit_debug_data == 0)
    {
        AddTimer(20, 250);
    }

    /*AddText(20, 50, 450, _("Font Test"), COLOR_YELLOW, glArchivItem_Font::DF_LEFT, SmallFont);
    AddText(21, 50, 470, _("Font Test"), COLOR_YELLOW, glArchivItem_Font::DF_LEFT, NormalFont);
    AddText(22, 50, 490, _("Font Test"), COLOR_YELLOW, glArchivItem_Font::DF_LEFT, LargeFont);*/
    //  !\"#$%&'()*+,-./0123456789:;<=>?@abcdefghijklmnopqrstuvwxyz\\_ABCDEFGHIJKLMNOPQRSTUVWXYZ«¸È‚‰‡ÂÁÍÎËÔÓÏ©ƒ≈ÙˆÚ˚˘÷‹·ÌÛ˙Ò
}

void dskMainMenu::Msg_Timer(const unsigned int ctrl_id)
{
    GetCtrl<ctrlTimer>(ctrl_id)->Stop();
    WindowManager::inst().Show( new iwMsgbox(_("Submit debug data?"), _("RttR now supports sending debug data. Would you like to help us improving this game by sending debug data?"), this, MSB_YESNO, MSB_QUESTIONRED, 100) );
}

void dskMainMenu::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    // Sollen alle Replays gel<F6>scht werden?
    if (msgbox_id == 100)
    {
        if (mbr == MSR_YES)
        {
            SETTINGS.global.submit_debug_data = 1;
        }
        else
        {
            SETTINGS.global.submit_debug_data = 2;
        }

        SETTINGS.Save();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 *  @author FloSoft
 */
void dskMainMenu::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 4: // "Single Player"
        {
            WindowManager::inst().Switch(new dskSinglePlayer);
        } break;
        case 5: // "Multiplayer"
        {
            WindowManager::inst().Switch(new dskMultiPlayer);
        } break;
        case 6: // "Options"
        {
            WindowManager::inst().Switch(new dskOptions);
        } break;
        case 7: // "Intro"
        {
            WindowManager::inst().Switch(new dskIntro);
        } break;
        case 8: // "Credits"
        {
            WindowManager::inst().Switch(new dskCredits);
        } break;
        case 9: // "Quit"
        {
            GLOBALVARS.notdone = false;
        } break;
        case 10: // "Readme"
        {
            WindowManager::inst().Show(new iwTextfile("readme.txt", _("Readme!")));
        } break;

    }
}

void dskMainMenu::Msg_PaintAfter()
{

    /*for(unsigned i = 0;i<100;++i)
    {
        glDisable(GL_TEXTURE_2D);
        glColor4f(1.0f,0.0f,0.0f,1.0f);
        glBegin(GL_QUADS);
        glVertex3f(0.0f, 0.0f,float(i));
        glVertex3f(0.0f, 600.0f,float(i));
        glVertex3f(700.0f, 600.0f,float(i));
        glVertex3f(700.0f, 0.0f,float(i));
        glEnd();
        glEnable(GL_TEXTURE_2D);
    }*/
}
