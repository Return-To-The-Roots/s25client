// $Id: dskUnlimitedPlay.cpp 6958 2011-01-02 11:39:33Z FloSoft $
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
#include "dskUnlimitedPlay.h"
#include "dskGameLoader.h"
#include "dskSelectMap.h"
#include "dskSinglePlayer.h"

#include "WindowManager.h"
#include "Loader.h"
#include "GameClient.h"
#include "GameServer.h"
#include "controls.h"
#include "LobbyClient.h"

#include "iwMsgbox.h"
#include "iwAddons.h"
#include "iwPleaseWait.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
	#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p dskHostGame.
 *
 *  @author Devil
 *  @author FloSoft
 */
dskUnlimitedPlay::dskUnlimitedPlay() :
	Desktop(LOADER.GetImageN("setup015", 0)), temppunkte(0), has_countdown(false)
{
	// Kartenname
	AddText(0, 400, 5,GAMECLIENT.GetGameName().c_str(), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, LargeFont);

	// "Spielername"
	AddText(10, 95, 40, _("Player Name"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
	// "Einstufung"
	AddText(11, 205, 40, _("Classification"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
	// "Volk"
	AddText(12, 285, 40, _("Race"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
	// "Farbe"
	AddText(13, 355, 40, _("Color"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
	// "Team"
	AddText(14, 405, 40, _("Team"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
	// "Verschieben" (nur bei Savegames!)
	if(GAMECLIENT.IsSavegame())
		AddText(17, 645, 40, _("Past player"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);

	// "Spiel starten"
	AddTextButton(2, 600, 560, 180, 22, TC_GREEN2, (GAMECLIENT.IsHost() ? _("Start game") : _("Ready")), NormalFont);

	// "Zurück"
	AddTextButton(3, 400, 560, 180, 22, TC_RED1, _("Return"), NormalFont);

	// "Teams sperren"
	AddCheckBox(20, 400, 460, 180, 26, TC_GREY, _("Lock teams:"), NormalFont, !GAMECLIENT.IsHost()||GAMECLIENT.IsSavegame());
	// "Gemeinsame Team-Sicht"
	AddCheckBox(19, 600, 460, 180, 26, TC_GREY, _("Shared team view"), NormalFont, !GAMECLIENT.IsHost()||GAMECLIENT.IsSavegame());

	// "Enhancements"
	AddText(21, 400, 499, _("Addons:"), COLOR_YELLOW, 0, NormalFont);
	AddTextButton(22, 600, 495, 180, 22, TC_GREEN1, (GAMECLIENT.IsHost() ? _("Change Settings...") : _("View Settings...")),NormalFont);

	ctrlComboBox *combo;

	// umgedrehte Reihenfolge, damit die Listen nicht dahinter sind

	// "Aufklärung"
	AddText(30, 400, 405, _("Exploration:"), COLOR_YELLOW, 0, NormalFont);
	combo = AddComboBox(40, 600, 400, 180, 20, TC_GREY, NormalFont, 100, !GAMECLIENT.IsHost()||GAMECLIENT.IsSavegame());
	combo->AddString(_("Off (all visible)"));
	combo->AddString(_("Classic (Settlers 2)"));
	combo->AddString(_("Fog of War"));
	combo->AddString(_("FoW - all explored"));

	// "Waren zu Beginn"
	AddText(31, 400, 375, _("Goods at start:"), COLOR_YELLOW, 0, NormalFont);
	combo = AddComboBox(41, 600, 370, 180, 20, TC_GREY, NormalFont, 100, !GAMECLIENT.IsHost()||GAMECLIENT.IsSavegame());
	combo->AddString(_("Very Low"));
	combo->AddString(_("Low"));
	combo->AddString(_("Normal"));
	combo->AddString(_("A lot"));

	// "Spielziel"
	AddText(32, 400, 345, _("Goals:"), COLOR_YELLOW, 0, NormalFont);
	combo = AddComboBox(42, 600, 340, 180, 20, TC_GREY, NormalFont, 100, !GAMECLIENT.IsHost()||GAMECLIENT.IsSavegame());
	combo->AddString(_("None")); // Kein Spielziel
	combo->AddString(_("Conquer 3/4 of map")); // Besitz 3/4 des Landes
	combo->AddString(_("Total domination")); // Alleinherrschaft

	// "Geschwindigkeit"
	AddText(33, 400, 315, _("Speed:"), COLOR_YELLOW, 0, NormalFont);
	combo = AddComboBox(43, 600, 310, 180, 20, TC_GREY, NormalFont, 100, !GAMECLIENT.IsHost());
	combo->AddString(_("Very slow")); // Sehr Langsam
	combo->AddString(_("Slow")); // Langsam
	combo->AddString(_("Normal")); // Normal
	combo->AddString(_("Fast")); // Schnell
	combo->AddString(_("Very fast")); // Sehr Schnell

	// Karte laden, um Kartenvorschau anzuzeigen
	if(GameClient::inst().GetMapType() == MAPTYPE_OLDMAP)
	{
		// Map laden
		libsiedler2::ArchivInfo ai;
		// Karteninformationen laden
		if(libsiedler2::loader::LoadMAP(GameClient::inst().GetMapPath().c_str(), &ai) == 0)
		{
			glArchivItem_Map *map = static_cast<glArchivItem_Map*>(ai.get(0));
			ctrlPreviewMinimap * preview = AddPreviewMinimap(70,560,40,220,220,map);

			// Titel der Karte, Y-Position relativ je nach Höhe der Minimap festlegen, daher nochmals danach
			// verschieben, da diese Position sonst skaliert wird!
			ctrlText * text = AddText(71,670,0,_("Map: ") +  GameClient::inst().GetMapTitle(),COLOR_YELLOW,glArchivItem_Font::DF_CENTER,NormalFont);
			text->Move(text->GetX(false),preview->GetY(false)+preview->GetBottom()+10);
		}
	}

	// Alle Spielercontrols erstellen
	for(unsigned char i = GAMECLIENT.GetPlayerCount(); i; --i)
		UpdatePlayerRow(i-1);

	// GGS aktualisieren, zum ersten Mal
	this->CI_GGSChanged(GameClient::inst().GetGGS());

	GAMECLIENT.SetInterface(this);

	// Setze initial auf KI
	for (unsigned char i = 1; i < GAMECLIENT.GetPlayerCount(); i++)
		GAMESERVER.TogglePlayerState(i);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Größe ändern-Reaktionen die nicht vom Skaling-Mechanismus erfasst werden.
 *
 *  @author Divan
 */
void dskUnlimitedPlay::Resize_(unsigned short width, unsigned short height)
{
	// Text unter der PreviewMinimap verschieben, dessen Höhe von der Höhe der 
	// PreviewMinimap abhängt, welche sich gerade geändert hat.
	ctrlPreviewMinimap *preview = GetCtrl<ctrlPreviewMinimap>(70);
	ctrlText *text = GetCtrl<ctrlText>(71);
	assert(preview);
	assert(text);
	text->Move(text->GetX(false),preview->GetY(false)+preview->GetBottom()+10);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::UpdatePlayerRow(const unsigned row)
{
	GameClientPlayer *player = GAMECLIENT.GetPlayer(row);
	assert(player);

	unsigned cy = 80 + row * 30;
	TextureColor tc = (row&1 ? TC_GREY : TC_GREEN2);

	// Alle Controls erstmal zerstören (die ganze Gruppe)
	DeleteCtrl(58 - row);
	// und neu erzeugen
	ctrlGroup *group = AddGroup(58 - row, scale);

	std::string name;
	// Name
	switch(player->ps)
	{
	default:
		name = "";
		break;
	case PS_OCCUPIED:
	case PS_KI:
		{
			name = player->name;
			//if (player->aiType == AI_DUMMY)
			//	GAMESERVER.TogglePlayerState(player->getPlayerID());
		} break;
	case PS_FREE:
	case PS_RESERVED:
		{
			// Offen
			name = _("Open");
		} break;
	case PS_LOCKED:
		{
			// Geschlossen
			name = _("Closed");
		} break;
	}
	
	if(GetCtrl<ctrlPreviewMinimap>(70))
	{
		if(player->ps == PS_OCCUPIED || player->ps == PS_KI)
		   // Nur KIs und richtige Spieler haben eine Farbe auf der Karte
			GetCtrl<ctrlPreviewMinimap>(70)->SetPlayerColor(row, COLORS[player->color]);
		else
			// Keine richtigen Spieler --> Startposition auf der Karte ausblenden
			GetCtrl<ctrlPreviewMinimap>(70)->SetPlayerColor(row, 0);
	}

	// Spielername, beim Hosts Spielerbuttons, aber nich beim ihm selber, er kann sich ja nich selber kicken!
	ctrlBaseText * text;
	if(GAMECLIENT.IsHost() && !player->is_host)
		text = group->AddTextButton(1, 20, cy, 150, 22, tc, name.c_str(), NormalFont);
	else
		text = group->AddDeepening(1, 20, cy, 150, 22, tc, name.c_str(), NormalFont, COLOR_YELLOW);

	// Is das der Host? Dann farblich markieren
	if(player->is_host == true)
		text->SetColor(0xFF00FF00);

	// Bei geschlossenem nicht sichtbar
	if(player->ps == PS_OCCUPIED || player->ps == PS_KI)
	{
		/// Einstufung nur bei Lobbyspielen anzeigen @todo Einstufung ( "%d" )
		group->AddVarDeepening(2, 180, cy, 50, 22, tc, (LOBBYCLIENT.LoggedIn() || player->ps == PS_KI ? _("%d") : _("n/a")), NormalFont, COLOR_YELLOW, 1, &player->rating);

		// Host kann nur das Zeug von der KI noch mit einstellen
		if(((GAMECLIENT.IsHost() && player->ps == PS_KI) || GAMECLIENT.GetPlayerID() == row) && !GAMECLIENT.IsSavegame())
		{
			// Volk
			group->AddTextButton( 3, 240, cy, 90, 22, tc, _("Africans"), NormalFont);
			// Farbe
			group->AddColorButton( 4, 340, cy, 30, 22, tc, 0);
			// Team
			group->AddTextButton( 5, 380, cy, 50, 22, tc, _("-"), NormalFont);
		}
		else
		{
			// Volk
			group->AddDeepening( 3, 240, cy, 90, 22, tc, _("Africans"), NormalFont, COLOR_YELLOW);
			// Farbe
			group->AddColorDeepening( 4, 340, cy, 30, 22, tc, 0);
			// Team
			group->AddDeepening( 5, 380, cy, 50, 22, tc, _("-"), NormalFont, COLOR_YELLOW);
		}

		// Bereit (nicht bei KIs und Host)
		if(player->ps == PS_OCCUPIED && !player->is_host)
			group->AddCheckBox(6, 450, cy, 22, 22, tc, EMPTY_STRING, NULL, (GAMECLIENT.GetPlayerID() != row) );

		// Ping ( "%d" )
		ctrlVarDeepening *ping = group->AddVarDeepening(7, 490, cy, 50, 22, tc, _("%d"), NormalFont, COLOR_YELLOW, 1, &player->ping);

		// Verschieben (nur bei Savegames und beim Host!)
		if(GAMECLIENT.IsSavegame() && player->ps == PS_OCCUPIED)
		{
			ctrlComboBox *combo = group->AddComboBox(8, 570, cy, 150, 22, tc, NormalFont, 150, !GAMECLIENT.IsHost());

			// Mit den alten Namen füllen
			for(unsigned i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
			{
				if(GAMECLIENT.GetPlayer(i)->origin_name.length())
				{
					combo->AddString(GAMECLIENT.GetPlayer(i)->origin_name.c_str());
					if(i == row)
						combo->SetSelection(combo->GetCount() - 1);
				}
			}
		}

		// Ping bei KI und Host ausblenden
		if(player->ps == PS_KI || player->is_host)
			ping->SetVisible(false);

		// Felder ausfüllen
		ChangeNation(row,player->nation);
		ChangeTeam(row,player->team);
		ChangePing(row);
		ChangeReady(row,player->ready);
		ChangeColor(row,player->color);
	}
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Methode vor dem Zeichnen
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::Msg_PaintBefore()
{
	// Chatfenster Fokus geben
	//GetCtrl<ctrlEdit>(4)->SetFocus();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::Msg_Group_ButtonClick(const unsigned int group_id, const unsigned int ctrl_id)
{
	unsigned player_id = 8 - (group_id - 50);

	switch(ctrl_id)
	{
	// Klick auf Spielername
	case 1:
		{
			if(GAMECLIENT.IsHost())
			{
				
				GAMESERVER.TogglePlayerState(player_id);
			}
		} break;
	// Volk
	case 3:
		{
			TogglePlayerReady(player_id, false);

			if(GAMECLIENT.IsHost())
				GAMESERVER.TogglePlayerNation(player_id);
			if(player_id == GAMECLIENT.GetPlayerID())
			{
					GAMECLIENT.Command_ToggleNation();
					GAMECLIENT.GetLocalPlayer()->nation = Nation((unsigned(GAMECLIENT.GetLocalPlayer()->nation) + 1) % 4);
					ChangeNation(GAMECLIENT.GetPlayerID(),GAMECLIENT.GetLocalPlayer()->nation);
				}
			} break;

	// Farbe
	case 4:
	{
		TogglePlayerReady(player_id, false);

		if(GAMECLIENT.IsHost())
			GAMESERVER.TogglePlayerColor(player_id);
		if(player_id == GAMECLIENT.GetPlayerID())
		{
			GAMECLIENT.Command_ToggleColor();

			GameClientPlayer *player = GAMECLIENT.GetLocalPlayer();
			bool reserved_colors[PLAYER_COLORS_COUNT];
			memset(reserved_colors, 0, sizeof(bool) * PLAYER_COLORS_COUNT);

			for(unsigned char cl = 0; cl < GAMECLIENT.GetPlayerCount(); ++cl)
			{
				GameClientPlayer *others = GAMECLIENT.GetPlayer(cl);

				if( (player != others) && ( (others->ps == PS_OCCUPIED) || (others->ps == PS_KI) ) )
					reserved_colors[others->color] = true;
			}
			do {
				player->color = (player->color + 1) % PLAYER_COLORS_COUNT;
			} while(reserved_colors[player->color]);
			ChangeColor(GAMECLIENT.GetPlayerID(), player->color);
		}

		// Start-Farbe der Minimap ändern
	} break;

	// Team
	case 5:
		{
			TogglePlayerReady(player_id, false);

			if(GAMECLIENT.IsHost())
				GAMESERVER.TogglePlayerTeam(player_id);
			if(player_id == GAMECLIENT.GetPlayerID())
			{
				GAMECLIENT.Command_ToggleTeam();
				GAMECLIENT.GetLocalPlayer()->team = Team((GAMECLIENT.GetLocalPlayer()->team + 1) % TEAM_COUNT);
				ChangeTeam(GAMECLIENT.GetPlayerID(),GAMECLIENT.GetLocalPlayer()->team);
			}
		} break;
	}
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::Msg_Group_CheckboxChange(const unsigned int group_id, const unsigned int ctrl_id, const bool checked)
{
	unsigned player_id = 8 - (group_id - 50);

	// Bereit
	if(player_id < 8)
		TogglePlayerReady(player_id, checked);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::Msg_Group_ComboSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short selection)
{
	unsigned player_id = 8 - (group_id - 50);

	// Spieler wurden vertauscht

	// 2. Player herausfinden (Strings vergleichen
	unsigned player2;
	for(player2 = 0;player2<GAMECLIENT.GetPlayerCount();++player2)
	{
		if(GAMECLIENT.GetPlayer(player2)->origin_name == GetCtrl<ctrlGroup>(group_id)->
			GetCtrl<ctrlComboBox>(8)->GetText(selection))
			break;
	}

	// Keinen Namen gefunden?
	if(player2 == GAMECLIENT.GetPlayerCount())
	{
		LOG.lprintf("dskHostGame: ERROR: No Origin Name found, stop swapping!\n");
		return;
	}

	GameServer::inst().SwapPlayer(player_id,player2);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::Msg_ButtonClick(const unsigned int ctrl_id)
{
	switch(ctrl_id)
	{
	case 3: // Zurück
		{
			if(GAMECLIENT.IsHost())
				GAMESERVER.Stop();

			GAMECLIENT.Stop();

			WindowManager::inst().Switch(new dskSinglePlayer);

		} break;
	
	case 2: // Starten
		{
			ctrlTextButton *ready = GetCtrl<ctrlTextButton>(2);
			if(GAMECLIENT.IsHost())
			{
				if(ready->GetText() == _("Start game"))
				{
					if(GAMESERVER.StartCountdown())
					{
						ready->SetText(_("Cancel start"));
					}
					else
						WindowManager::inst().Show(new iwMsgbox(_("Error"), _("Game can only be started as soon as everybody has a unique color,everyone is ready and all free slots are closed."), this, MSB_OK, MSB_EXCLAMATIONRED, 10));
				}
				else
					GAMESERVER.CancelCountdown();
			}
			else
			{
				if(ready->GetText() == _("Ready"))
					TogglePlayerReady(GAMECLIENT.GetPlayerID(), true);
				else
					TogglePlayerReady(GAMECLIENT.GetPlayerID(), false);
			}
		} break;
	case 22: // Addons
		{
			iwAddons *w = new iwAddons(GAMECLIENT.IsHost() ? iwAddons::HOSTGAME : iwAddons::READONLY);
			w->SetParent(this);
			WindowManager::inst().Show(w);
		} break;
	}
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::Msg_EditEnter(const unsigned int ctrl_id)
{
	GAMECLIENT.Command_Chat(GetCtrl<ctrlEdit>(4)->GetText(),CD_ALL);
	GetCtrl<ctrlEdit>(4)->SetText("");
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author FloSoft
 */
void dskUnlimitedPlay::CI_Countdown(int countdown)
{

}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author FloSoft
 */
void dskUnlimitedPlay::CI_CancelCountdown()
{
	GetCtrl<ctrlChat>(1)->AddMessage("", "", 0xFFCC2222, _("Start aborted"), 0xFFFFCC00);
	
	has_countdown = false;

	if(GAMECLIENT.IsHost())
		TogglePlayerReady(GAMECLIENT.GetPlayerID(), false);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author FloSoft
 */
void dskUnlimitedPlay::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
	switch(msgbox_id)
	{ 
	case 0: // Verbindung zu Server verloren?
		{
			GAMECLIENT.Stop();

			WindowManager::inst().Switch(new dskSinglePlayer);
		} break;
	case CGI_ADDONS: // addon-window applied settings?
		{
			if(mbr == MSR_YES)
				UpdateGGS();
		} break;
	}
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::Msg_ComboSelectItem(const unsigned int ctrl_id, const unsigned short selection)
{
	switch(ctrl_id)
	{
	default:
		break;

	case 43: // Geschwindigkeit
	case 42: // Ziel
	case 41: // Waren
	case 40: // Aufklärung
		{
			// GameSettings wurden verändert, resetten
			UpdateGGS();
		} break;
	}
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::Msg_CheckboxChange(const unsigned int ctrl_id, const bool checked)
{

	switch(ctrl_id)
	{
	default:
		break;
	case 19: // Team-Sicht
	case 20: // Teams
		{
			// GameSettings wurden verändert, resetten
			UpdateGGS();
		} break;
	}
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::UpdateGGS()
{
	// Geschwindigkeit
	ggs.game_speed = static_cast<GlobalGameSettings::GameSpeed>(GetCtrl<ctrlComboBox>(43)->GetSelection());
	// Spielziel
	ggs.game_objective = static_cast<GlobalGameSettings::GameObjective>(GetCtrl<ctrlComboBox>(42)->GetSelection());
	// Waren zu Beginn
	ggs.start_wares = static_cast<GlobalGameSettings::StartWares>(GetCtrl<ctrlComboBox>(41)->GetSelection());
	// Aufklärung
	ggs.exploration = static_cast<GlobalGameSettings::Exploration>(GetCtrl<ctrlComboBox>(40)->GetSelection());
	// Teams gesperrt
	ggs.lock_teams = GetCtrl<ctrlCheck>(20)->GetCheck();
	// Team sicht
	ggs.team_view = GetCtrl<ctrlCheck>(19)->GetCheck();

	// An Server übermitteln
	GameServer::inst().ChangeGlobalGameSettings(ggs);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::ChangeTeam(const unsigned i, const unsigned char nr)
{
	const std::string teams[6] =
	{ "-","?", "1","2","3","4", };

	GetCtrl<ctrlGroup>(58-i)->GetCtrl<ctrlBaseText>(5)->SetText(teams[nr]);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::ChangeReady(const unsigned int player, const bool ready)
{
	ctrlCheck *check;
	if( (check = GetCtrl<ctrlGroup>(58 - player)->GetCtrl<ctrlCheck>(6)))
		check->SetCheck(ready);

	ctrlTextButton *start;
	if(player == GAMECLIENT.GetPlayerID() && (start = GetCtrl<ctrlTextButton>(2)))
	{
		if(GAMECLIENT.IsHost())
			start->SetText(has_countdown ? _("Cancel start") : _("Start game"));
		else
			start->SetText(ready ? _("Not Ready") : _("Ready"));
	}
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::ChangeNation(const unsigned i, const Nation nation)
{
	const std::string nations[4] =
	{ _("Africans"), _("Japaneses"), _("Romans"), _("Vikings") };
	GetCtrl<ctrlGroup>(58-i)->GetCtrl<ctrlBaseText>(3)->SetText(nations[nation]);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::ChangePing(const unsigned i)
{
	unsigned int color = COLOR_RED;

	// Farbe bestimmen
	if(GAMECLIENT.GetPlayer(id)->ping < 300)
		color = COLOR_GREEN;
	else if(GAMECLIENT.GetPlayer(id)->ping < 800 )
		color = COLOR_YELLOW;

	// und setzen
	GetCtrl<ctrlGroup>(58-i)->GetCtrl<ctrlVarDeepening>(7)->SetColor(color);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::ChangeColor(const unsigned i, const unsigned char color)
{
	GetCtrl<ctrlGroup>(58-i)->GetCtrl<ColorControlInterface>(4)->SetColor(COLORS[color]);

	// Minimap-Startfarbe ändern
	if(GetCtrl<ctrlPreviewMinimap>(70))
		GetCtrl<ctrlPreviewMinimap>(70)->SetPlayerColor(i,COLORS[color]);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::TogglePlayerReady(unsigned char player, bool ready)
{
	if(player == GAMECLIENT.GetPlayerID())
	{
		GAMECLIENT.GetLocalPlayer()->ready = (GAMECLIENT.IsHost() ? true : ready);
		GAMECLIENT.Command_ToggleReady();
		ChangeReady(GAMECLIENT.GetPlayerID(), GAMECLIENT.GetLocalPlayer()->ready);
	}
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_NewPlayer(const unsigned player_id)
{
	// Spielername setzen
	UpdatePlayerRow(player_id);

	// Rankinginfo abrufen
	if(LOBBYCLIENT.LoggedIn())
	{
		for(unsigned char i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
		{
			GameClientPlayer *player = GAMECLIENT.GetPlayer(i);
			if(player->ps == PS_OCCUPIED)
				LOBBYCLIENT.SendRankingInfoRequest(player->name);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_PlayerLeft(const unsigned player_id)
{
	UpdatePlayerRow(player_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_GameStarted(GameWorldViewer *gwv)
{
	// Desktop wechseln
	WindowManager::inst().Switch(new dskGameLoader(gwv));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_PSChanged(const unsigned player_id, const PlayerState ps)
{
	if (ps == PS_FREE)
		GAMESERVER.TogglePlayerState(player_id);
	UpdatePlayerRow(player_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_NationChanged(const unsigned player_id, const Nation nation)
{
	ChangeNation(player_id,nation);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_TeamChanged(const unsigned player_id, const unsigned char team)
{
	ChangeTeam(player_id,team);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_ColorChanged(const unsigned player_id, const unsigned char color)
{
	ChangeColor(player_id, color);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_PingChanged(const unsigned player_id, const unsigned short ping)
{
	ChangePing(player_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_ReadyChanged(const unsigned player_id, const bool ready)
{
	ChangeReady(player_id,ready);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_PlayersSwapped(const unsigned player1, const unsigned player2)
{
	// Spieler wurden vertauscht, beide Reihen updaten
	UpdatePlayerRow(player1);
	UpdatePlayerRow(player2);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_GGSChanged(const GlobalGameSettings& ggs)
{
	this->ggs = ggs;
	
	// Geschwindigkeit
	GetCtrl<ctrlComboBox>(43)->SetSelection(static_cast<unsigned short>(ggs.game_speed));
	// Ziel
	GetCtrl<ctrlComboBox>(42)->SetSelection(static_cast<unsigned short>(ggs.game_objective));
	// Waren
	GetCtrl<ctrlComboBox>(41)->SetSelection(static_cast<unsigned short>(ggs.start_wares));
	// Aufklärung
	GetCtrl<ctrlComboBox>(40)->SetSelection(static_cast<unsigned short>(ggs.exploration));
	// Teams
	GetCtrl<ctrlCheck>(20)->SetCheck(ggs.lock_teams);
	// Team-Sicht
	GetCtrl<ctrlCheck>(19)->SetCheck(ggs.team_view);

	TogglePlayerReady(GAMECLIENT.GetPlayerID(), false);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_Chat(const unsigned player_id, const ChatDestination cd, const std::string& msg)
{

}

///////////////////////////////////////////////////////////////////////////////
/**
 *  
 *
 *  @author OLiver
 */
void dskUnlimitedPlay::CI_Error(const ClientError ce)
{
	switch(ce)
	{
	case CE_INCOMPLETEMESSAGE:
	case CE_CONNECTIONLOST:
		{
			// Verbindung zu Server abgebrochen
			WindowManager::inst().Show(new iwMsgbox(_("Error"), _("Lost connection to server!"), this, MSB_OK, MSB_EXCLAMATIONRED, 0));
		} break;
	default: break;
	}
}

