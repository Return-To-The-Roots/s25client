// $Id: dskMainMenu.cpp 7084 2011-03-26 21:31:12Z OLiver $
//
// Copyright (c) 2005 - 2010 Settlers Freaks (sf-team at siedler25.org)
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
#include "stdafx.h"
#include "main.h"
#include "dskMainMenu.h"

#include "WindowManager.h"
#include "Loader.h"
#include "GlobalVars.h"
#include "controls.h"

#include "dskAboutRTTR.h"
#include "dskSinglePlayer.h"
#include "dskMultiPlayer.h"
#include "dskOptions.h"
#include "dskIntro.h"
#include "dskCredits.h"

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
 *  Klasse des Hauptmenü Desktops.
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
	AddText(1, 400, 600, "http://www.siedler25.org", COLOR_GREEN, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, NormalFont);
	// Copyright
	AddText(2, 800, 600, "° 2005 - 2010 Settlers Freaks", COLOR_YELLOW, glArchivItem_Font::DF_RIGHT | glArchivItem_Font::DF_BOTTOM, NormalFont);

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
	case 4: // "Einzelspieler"
		{
			WindowManager::inst().Switch(new dskSinglePlayer);
		} break;
	case 5: // "Mehrspieler"
		{
			WindowManager::inst().Switch(new dskMultiPlayer);
		} break;
	case 6: // "Optionen"
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
	case 9: // "Programm verlassen"
		{
			GLOBALVARS.notdone = false;
		} break;
        case 10: // "Readme"
                {
			WindowManager::inst().Show(new iwTextfile("readme.txt",_("Readme!")));
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
	}*/
}
