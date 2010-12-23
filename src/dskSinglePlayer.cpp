// $Id: dskSinglePlayer.cpp 6932 2010-12-23 20:27:55Z OLiver $
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
#include "main.h"
#include "dskSinglePlayer.h"

#include "WindowManager.h"
#include "Loader.h"

#include "dskMainMenu.h"
#include "iwPlayReplay.h"
#include "iwSave.h"
#include "iwMsgbox.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/** @class dskSinglePlayer
 *
 *  Klasse des Einzelspieler Desktops.
 *
 *  @author OLiver
 */

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p dskSinglePlayer.
 *
 *  @author OLiver
 *  @author FloSoft
 */
dskSinglePlayer::dskSinglePlayer(void) : Desktop(LOADER.GetImageN("menu", 0))
{
	// Version
	AddVarText(0, 0, 600, _("Return To The Roots - v%s-%s"), COLOR_YELLOW, 0 | glArchivItem_Font::DF_BOTTOM, NormalFont, 2, GetWindowVersion(), GetWindowRevision());
	// URL
	AddText(1, 400, 600, _("http://www.siedler25.org"), COLOR_GREEN, glArchivItem_Font::DF_CENTER | glArchivItem_Font::DF_BOTTOM, NormalFont);
	// Copyright
	AddText(2, 800, 600, "° 2005 - 2010 Settlers Freaks", COLOR_YELLOW, glArchivItem_Font::DF_RIGHT | glArchivItem_Font::DF_BOTTOM, NormalFont);

	// "Letztes Spiel fortsetzen"
	AddTextButton(3, 115, 180, 220, 22, TC_GREEN2, _("Resume last game"),NormalFont);
	// "Replay abspielen"
	AddTextButton(4, 115, 220, 220, 22, TC_GREEN2, _("Play Replay"),NormalFont);
	// "Spiel starten"
	AddTextButton(5, 115, 260, 220, 22, TC_GREEN2, _("Start game"),NormalFont);
	// "Spiel laden"
	AddTextButton(6, 115, 290, 220, 22, TC_GREEN2, _("Load game"),NormalFont);
	// "Zurück"
	AddTextButton(7, 115, 330, 220, 22, TC_RED1, _("Back"),NormalFont);

	AddImage(11, 20, 20, LOADER.GetImageN("logo", 0));
}



void dskSinglePlayer::Msg_ButtonClick(const unsigned int ctrl_id)
{
	switch(ctrl_id)
	{
	case 3: // "Letztes Spiel fortsetzen"
		{
		} break;
	case 4: // "Replay abspielen"
		{
			WindowManager::inst().Show(new iwPlayReplay);
		} break;
	case 5: // "Spiel starten"
		{
			/// @todo Hier dann Auswahl zwischen Kampagne(n) und "Freies Spiel"
			WindowManager::inst().Show(new iwMsgbox(_("Not available"),_("Please use the Multiplayer mode and \'Direct IP\' to create a Singleplayer game."),this,MSB_OK,MSB_EXCLAMATIONGREEN));
		} break;
	case 6: // "Spiel laden"
		{
		} break;
	case 7: // "Zurück"
		{
			WindowManager::inst().Switch(new dskMainMenu);
		} break;
	}
}

