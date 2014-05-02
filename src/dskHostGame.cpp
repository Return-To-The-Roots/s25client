// $Id: dskHostGame.cpp 9388 2014-05-02 07:37:20Z FloSoft $
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
#include "dskHostGame.h"
#include "dskGameLoader.h"

#include "WindowManager.h"
#include "Loader.h"
#include "GameClient.h"
#include "GameServer.h"
#include "controls.h"
#include "LobbyClient.h"

#include "dskDirectIP.h"
#include "dskLobby.h"
#include "dskSinglePlayer.h"
#include "iwMsgbox.h"
#include "iwAddons.h"
#include "Random.h"

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
dskHostGame::dskHostGame(bool single_player) :
    Desktop(LOADER.GetImageN("setup015", 0)), temppunkte(0), has_countdown(false), single_player(single_player)
{
    // Kartenname
    AddText(0, 400, 5, GAMECLIENT.GetGameName().c_str(), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, LargeFont);

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

    if (!single_player)
    {
        // "Bereit"
        AddText(15, 465, 40, _("Ready?"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
        // "Ping"
        AddText(16, 515, 40, _("Ping"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
    }
    // "Swap"
    if(GAMECLIENT.IsHost() && !GAMECLIENT.IsSavegame())
        AddText(24, 10, 40, _("Swap"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
    // "Verschieben" (nur bei Savegames!)
    if(GAMECLIENT.IsSavegame())
        AddText(17, 645, 40, _("Past player"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);

    if (!single_player)
    {
        // Chatfenster
        AddChatCtrl(1, 20, 310, 360, 218, TC_GREY, NormalFont);
        // Edit für Chatfenster
        AddEdit(4, 20, 530, 360, 22, TC_GREY, NormalFont);
    }

    // "Spiel starten"
    AddTextButton(2, 600, 560, 180, 22, TC_GREEN2, (GAMECLIENT.IsHost() ? _("Start game") : _("Ready")), NormalFont);

    // "Zurück"
    AddTextButton(3, 400, 560, 180, 22, TC_RED1, _("Return"), NormalFont);

    // "Teams sperren"
    AddCheckBox(20, 400, 460, 180, 26, TC_GREY, _("Lock teams:"), NormalFont, !GAMECLIENT.IsHost() || GAMECLIENT.IsSavegame());
    // "Gemeinsame Team-Sicht"
    AddCheckBox(19, 600, 460, 180, 26, TC_GREY, _("Shared team view"), NormalFont, !GAMECLIENT.IsHost() || GAMECLIENT.IsSavegame());
    // "Random Start Locations"
    AddCheckBox(23, 600, 430, 180, 26, TC_GREY, _("Random start locations"), NormalFont, !GAMECLIENT.IsHost() || GAMECLIENT.IsSavegame());

    // "Enhancements"
    AddText(21, 400, 499, _("Addons:"), COLOR_YELLOW, 0, NormalFont);
    AddTextButton(22, 600, 495, 180, 22, TC_GREEN2, (GAMECLIENT.IsHost() ? _("Change Settings...") : _("View Settings...")), NormalFont);

    ctrlComboBox* combo;

    // umgedrehte Reihenfolge, damit die Listen nicht dahinter sind

    // "Aufklärung"
    AddText(30, 400, 405, _("Exploration:"), COLOR_YELLOW, 0, NormalFont);
    combo = AddComboBox(40, 600, 400, 180, 20, TC_GREY, NormalFont, 100, !GAMECLIENT.IsHost() || GAMECLIENT.IsSavegame());
    combo->AddString(_("Off (all visible)"));
    combo->AddString(_("Classic (Settlers 2)"));
    combo->AddString(_("Fog of War"));
    combo->AddString(_("FoW - all explored"));

    // "Waren zu Beginn"
    AddText(31, 400, 375, _("Goods at start:"), COLOR_YELLOW, 0, NormalFont);
    combo = AddComboBox(41, 600, 370, 180, 20, TC_GREY, NormalFont, 100, !GAMECLIENT.IsHost() || GAMECLIENT.IsSavegame());
    combo->AddString(_("Very Low"));
    combo->AddString(_("Low"));
    combo->AddString(_("Normal"));
    combo->AddString(_("A lot"));

    // "Spielziel"
    AddText(32, 400, 345, _("Goals:"), COLOR_YELLOW, 0, NormalFont);
    combo = AddComboBox(42, 600, 340, 180, 20, TC_GREY, NormalFont, 100, !GAMECLIENT.IsHost() || GAMECLIENT.IsSavegame());
    combo->AddString(_("None")); // Kein Spielziel
    combo->AddString(_("Conquer 3/4 of map")); // Besitz 3/4 des Landes
    combo->AddString(_("Total domination")); // Alleinherrschaft
    // Lobby game?
    if(LOBBYCLIENT.LoggedIn())
    {
        // Then add tournament modes as possible "objectives"
        for(unsigned i = 0; i < TOURNAMENT_MODES_COUNT; ++i)
        {
            char str[512];
            sprintf (str, _("Tournament: %u minutes"), TOURNAMENT_MODES_DURATION[i]);
            combo->AddString(str);
        }
    }

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
            glArchivItem_Map* map = static_cast<glArchivItem_Map*>(ai.get(0));
            ctrlPreviewMinimap* preview = AddPreviewMinimap(70, 560, 40, 220, 220, map);

            // Titel der Karte, Y-Position relativ je nach Höhe der Minimap festlegen, daher nochmals danach
            // verschieben, da diese Position sonst skaliert wird!
            ctrlText* text = AddText(71, 670, 0, _("Map: ") +  GameClient::inst().GetMapTitle(), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, NormalFont);
            text->Move(text->GetX(false), preview->GetY(false) + preview->GetBottom() + 10);
        }
    }

    if (single_player && !GAMECLIENT.IsSavegame())
    {
        // Setze initial auf KI
        for (unsigned char i = 0; i < GAMECLIENT.GetPlayerCount(); i++)
        {
            if (!GAMECLIENT.GetPlayer(i)->is_host)
                GAMESERVER.TogglePlayerState(i);
        }
    }

    // Alle Spielercontrols erstellen
    for(unsigned char i = GAMECLIENT.GetPlayerCount(); i; --i)
        UpdatePlayerRow(i - 1);
    //swap buttons erstellen
    if(GAMECLIENT.IsHost() && !GAMECLIENT.IsSavegame())
    {
        for(unsigned char i = GAMECLIENT.GetPlayerCount(); i; --i)
            AddTextButton(80 + i, 5, 80 + (i - 1) * 30, 10, 22, TC_RED1, _("-"), NormalFont);;
    }
    // GGS aktualisieren, zum ersten Mal
    GameClient::inst().LoadGGS();
    this->CI_GGSChanged(GameClient::inst().GetGGS());

    LOBBYCLIENT.SetInterface(this);
    if(LOBBYCLIENT.LoggedIn())
    {
        LOBBYCLIENT.SendServerJoinRequest();
        LOBBYCLIENT.SendRankingInfoRequest(GAMECLIENT.GetPlayer(GAMECLIENT.GetPlayerID())->name);
        for(unsigned char i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
        {
            GameClientPlayer* player = GAMECLIENT.GetPlayer(i);
            if(player->ps == PS_OCCUPIED)
                LOBBYCLIENT.SendRankingInfoRequest(player->name);
        }
    }

    GAMECLIENT.SetInterface(this);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Größe ändern-Reaktionen die nicht vom Skaling-Mechanismus erfasst werden.
 *
 *  @author Divan
 */
void dskHostGame::Resize_(unsigned short width, unsigned short height)
{
    // Text unter der PreviewMinimap verschieben, dessen Höhe von der Höhe der
    // PreviewMinimap abhängt, welche sich gerade geändert hat.
    ctrlPreviewMinimap* preview = GetCtrl<ctrlPreviewMinimap>(70);
    ctrlText* text = GetCtrl<ctrlText>(71);
    assert(preview);
    assert(text);
    text->Move(text->GetX(false), preview->GetY(false) + preview->GetBottom() + 10);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::UpdatePlayerRow(const unsigned row)
{
    GameClientPlayer* player = GAMECLIENT.GetPlayer(row);
    assert(player);

    unsigned cy = 80 + row * 30;
    TextureColor tc = (row & 1 ? TC_GREY : TC_GREEN2);

    // Alle Controls erstmal zerstören (die ganze Gruppe)
    DeleteCtrl(58 - row);
    // und neu erzeugen
    ctrlGroup* group = AddGroup(58 - row, scale);

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
    ctrlBaseText* text;
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
        ctrlVarDeepening* ping = group->AddVarDeepening(7, 490, cy, 50, 22, tc, _("%d"), NormalFont, COLOR_YELLOW, 1, &player->ping);

        // Verschieben (nur bei Savegames und beim Host!)
        if(GAMECLIENT.IsSavegame() && player->ps == PS_OCCUPIED)
        {
            ctrlComboBox* combo = group->AddComboBox(8, 570, cy, 150, 22, tc, NormalFont, 150, !GAMECLIENT.IsHost());

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
        ChangeNation(row, player->nation);
        ChangeTeam(row, player->team);
        ChangePing(row);
        ChangeReady(row, player->ready);
        ChangeColor(row, player->color);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Methode vor dem Zeichnen
 *
 *  @author OLiver
 */
void dskHostGame::Msg_PaintBefore()
{
    // Chatfenster Fokus geben
    if (!single_player)
    {
        GetCtrl<ctrlEdit>(4)->SetFocus();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::Msg_Group_ButtonClick(const unsigned int group_id, const unsigned int ctrl_id)
{
    unsigned player_id = 8 - (group_id - 50);

    switch(ctrl_id)
    {
            // Klick auf Spielername
        case 1:
        {
            if(GAMECLIENT.IsHost())
                GAMESERVER.TogglePlayerState(player_id);
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
                GAMECLIENT.GetLocalPlayer()->nation = Nation((unsigned(GAMECLIENT.GetLocalPlayer()->nation) + 1) % NATION_COUNT);
                ChangeNation(GAMECLIENT.GetPlayerID(), GAMECLIENT.GetLocalPlayer()->nation);
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

                GameClientPlayer* player = GAMECLIENT.GetLocalPlayer();
                bool reserved_colors[PLAYER_COLORS_COUNT];
                memset(reserved_colors, 0, sizeof(bool) * PLAYER_COLORS_COUNT);

                for(unsigned char cl = 0; cl < GAMECLIENT.GetPlayerCount(); ++cl)
                {
                    GameClientPlayer* others = GAMECLIENT.GetPlayer(cl);

                    if( (player != others) && ( (others->ps == PS_OCCUPIED) || (others->ps == PS_KI) ) )
                        reserved_colors[others->color] = true;
                }
                do
                {
                    player->color = (player->color + 1) % PLAYER_COLORS_COUNT;
                }
                while(reserved_colors[player->color]);
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
                if(GAMECLIENT.GetLocalPlayer()->team > TM_RANDOMTEAM && GAMECLIENT.GetLocalPlayer()->team < TEAM_COUNT) // team: 1->2->3->4->0
                {
                    GAMECLIENT.GetLocalPlayer()->team = Team((GAMECLIENT.GetLocalPlayer()->team + 1) % TEAM_COUNT);
                }
                else
                {
                    if(GAMECLIENT.GetLocalPlayer()->team == TM_NOTEAM) // 0(noteam)->randomteam(1-4)
                    {
                        int rnd = RANDOM.Rand(__FILE__, __LINE__, 0, 4);
                        if(!rnd)
                            GAMECLIENT.GetLocalPlayer()->team = TM_RANDOMTEAM;
                        else
                            GAMECLIENT.GetLocalPlayer()->team = Team(rnd + 5);
                    }
                    else //any randomteam -> team 1
                    {
                        GAMECLIENT.GetLocalPlayer()->team = TM_TEAM1;
                    }
                }
                GAMECLIENT.Command_ToggleTeam(GAMECLIENT.GetLocalPlayer()->team);
                ChangeTeam(GAMECLIENT.GetPlayerID(), GAMECLIENT.GetLocalPlayer()->team);
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
void dskHostGame::Msg_Group_CheckboxChange(const unsigned int group_id, const unsigned int ctrl_id, const bool checked)
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
void dskHostGame::Msg_Group_ComboSelectItem(const unsigned int group_id, const unsigned int ctrl_id, const unsigned short selection)
{
    unsigned player_id = 8 - (group_id - 50);

    // Spieler wurden vertauscht

    // 2. Player herausfinden (Strings vergleichen
    unsigned player2;
    for(player2 = 0; player2 < GAMECLIENT.GetPlayerCount(); ++player2)
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

    GameServer::inst().SwapPlayer(player_id, player2);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
        case 81:
        case 82:
        case 83:
        case 84:
        case 85:
        case 86:
        case 87:
        case 88:
        case 80: //swap
        {
            LOG.lprintf("dskHostGame: swap button pressed\n");
            unsigned char p = 0;
            while (true)
            {
                if (GAMECLIENT.GetPlayer(p)->is_host)
                {
                    LOG.lprintf("dskHostGame: host detected\n");
                    break;
                }
                if(p > MAX_PLAYERS)
                {
                    LOG.lprintf("dskHostGame: could not find host\n");
                    break;
                }
                else
                    p++;
            }

            if (p < MAX_PLAYERS)
            {
                GameServer::inst().SwapPlayer(p, ctrl_id - 81);
                CI_PlayersSwapped(p, ctrl_id - 81);
            }
        } break;
        case 3: // Zurück
        {
            if(GAMECLIENT.IsHost())
                GAMESERVER.Stop();

            GAMECLIENT.Stop();

            if (single_player)
            {
                WindowManager::inst().Switch(new dskSinglePlayer);
            }
            else if(LOBBYCLIENT.LoggedIn())
                WindowManager::inst().Switch(new dskLobby);
            else
                // Hauptmenü zeigen
                WindowManager::inst().Switch(new dskDirectIP);

        } break;

        case 2: // Starten
        {
            ctrlTextButton* ready = GetCtrl<ctrlTextButton>(2);
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
            iwAddons* w = new iwAddons(&ggs, GAMECLIENT.IsHost() ? iwAddons::HOSTGAME : iwAddons::READONLY);
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
void dskHostGame::Msg_EditEnter(const unsigned int ctrl_id)
{
    GAMECLIENT.Command_Chat(GetCtrl<ctrlEdit>(4)->GetText(), CD_ALL);
    GetCtrl<ctrlEdit>(4)->SetText("");
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
void dskHostGame::CI_Countdown(int countdown)
{
    has_countdown = true;

    if (single_player)
    {
        return;
    }

    std::stringstream message;

    if (countdown == 10)
    {
        GetCtrl<ctrlChat>(1)->AddMessage("", "", 0, _("You have 10 seconds until game starts"), COLOR_RED);
        GetCtrl<ctrlChat>(1)->AddMessage("", "", 0, _("Don't forget to check the addon configuration!"), 0xFFFFDD00);
        GetCtrl<ctrlChat>(1)->AddMessage("", "", 0, "", 0xFFFFCC00);
    }

    if(countdown > 0)
        message << " " << countdown;
    else
        message << _("Starting game, please wait");

    GetCtrl<ctrlChat>(1)->AddMessage("", "", 0, message.str(), 0xFFFFBB00);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
void dskHostGame::CI_CancelCountdown()
{
    if (single_player)
    {
        return;
    }

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
void dskHostGame::Msg_MsgBoxResult(const unsigned msgbox_id, const MsgboxResult mbr)
{
    switch(msgbox_id)
    {
        case 0: // Verbindung zu Server verloren?
        {
            GAMECLIENT.Stop();

            if (single_player)
            {
                WindowManager::inst().Switch(new dskSinglePlayer);
            }
            else if(LOBBYCLIENT.LoggedIn())   // steht die Lobbyverbindung noch?
                WindowManager::inst().Switch(new dskLobby);
            else
                WindowManager::inst().Switch(new dskDirectIP);
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
void dskHostGame::Msg_ComboSelectItem(const unsigned int ctrl_id, const unsigned short selection)
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
void dskHostGame::Msg_CheckboxChange(const unsigned int ctrl_id, const bool checked)
{

    switch(ctrl_id)
    {
        default:
            break;
        case 19: // Team-Sicht
        case 20: // Teams
        case 23: //random startlocation
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
void dskHostGame::UpdateGGS()
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
    //random locations
    ggs.random_location = GetCtrl<ctrlCheck>(23)->GetCheck();

    // An Server übermitteln
    GameServer::inst().ChangeGlobalGameSettings(ggs);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::ChangeTeam(const unsigned i, const unsigned char nr)
{
    const std::string teams[9] =
    { "-", "?", "1", "2", "3", "4", "?", "?", "?"};

    GetCtrl<ctrlGroup>(58 - i)->GetCtrl<ctrlBaseText>(5)->SetText(teams[nr]);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::ChangeReady(const unsigned int player, const bool ready)
{
    ctrlCheck* check;
    if( (check = GetCtrl<ctrlGroup>(58 - player)->GetCtrl<ctrlCheck>(6)))
        check->SetCheck(ready);

    ctrlTextButton* start;
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
void dskHostGame::ChangeNation(const unsigned i, const Nation nation)
{
    const std::string nations[NATION_COUNT] =
    { _("Africans"), _("Japaneses"), _("Romans"), _("Vikings"), _("Babylonians") };
    GetCtrl<ctrlGroup>(58 - i)->GetCtrl<ctrlBaseText>(3)->SetText(nations[nation]);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::ChangePing(const unsigned i)
{
    unsigned int color = COLOR_RED;

    // Farbe bestimmen
    if(GAMECLIENT.GetPlayer(id)->ping < 300)
        color = COLOR_GREEN;
    else if(GAMECLIENT.GetPlayer(id)->ping < 800 )
        color = COLOR_YELLOW;

    // und setzen
    GetCtrl<ctrlGroup>(58 - i)->GetCtrl<ctrlVarDeepening>(7)->SetColor(color);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::ChangeColor(const unsigned i, const unsigned char color)
{
    GetCtrl<ctrlGroup>(58 - i)->GetCtrl<ColorControlInterface>(4)->SetColor(COLORS[color]);

    // Minimap-Startfarbe ändern
    if(GetCtrl<ctrlPreviewMinimap>(70))
        GetCtrl<ctrlPreviewMinimap>(70)->SetPlayerColor(i, COLORS[color]);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::TogglePlayerReady(unsigned char player, bool ready)
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
void dskHostGame::CI_NewPlayer(const unsigned player_id)
{
    // Spielername setzen
    UpdatePlayerRow(player_id);

    // Rankinginfo abrufen
    if(LOBBYCLIENT.LoggedIn())
    {
        for(unsigned char i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
        {
            GameClientPlayer* player = GAMECLIENT.GetPlayer(i);
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
void dskHostGame::CI_PlayerLeft(const unsigned player_id)
{
    UpdatePlayerRow(player_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::CI_GameStarted(GameWorldViewer* gwv)
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
void dskHostGame::CI_PSChanged(const unsigned player_id, const PlayerState ps)
{
    if ((single_player) && (ps == PS_FREE))
        GAMESERVER.TogglePlayerState(player_id);

    UpdatePlayerRow(player_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::CI_NationChanged(const unsigned player_id, const Nation nation)
{
    ChangeNation(player_id, nation);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::CI_TeamChanged(const unsigned player_id, const unsigned char team)
{
    ChangeTeam(player_id, team);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::CI_ColorChanged(const unsigned player_id, const unsigned char color)
{
    ChangeColor(player_id, color);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::CI_PingChanged(const unsigned player_id, const unsigned short ping)
{
    ChangePing(player_id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::CI_ReadyChanged(const unsigned player_id, const bool ready)
{
    ChangeReady(player_id, ready);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::CI_PlayersSwapped(const unsigned player1, const unsigned player2)
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
void dskHostGame::CI_GGSChanged(const GlobalGameSettings& ggs)
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
    //random location
    GetCtrl<ctrlCheck>(23)->SetCheck(ggs.random_location);

    TogglePlayerReady(GAMECLIENT.GetPlayerID(), false);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::CI_Chat(const unsigned player_id, const ChatDestination cd, const std::string& msg)
{
    if ((player_id != 0xFFFFFFFF) && !single_player) // Keine Lobby-Nachrichten anzeigen
    {
        // Zeit holen
        char time_string[64];
        TIME.FormatTime(time_string, "(%H:%i:%s)", NULL);

        GetCtrl<ctrlChat>(1)->AddMessage(time_string, GAMECLIENT.GetPlayer(player_id)->name.c_str(),
                                         COLORS[GAMECLIENT.GetPlayer(player_id)->color], msg.c_str(), 0xFFFFFF00);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void dskHostGame::CI_Error(const ClientError ce)
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

///////////////////////////////////////////////////////////////////////////////
/**
 *  (Lobby-)RankingInfo: Rankinginfo eines bestimmten Benutzers empfangen
 *
 *  @author FloSoft
 */
void dskHostGame::LC_RankingInfo(const LobbyPlayerInfo& player)
{
    for(unsigned int i = 0; i < GAMECLIENT.GetPlayerCount(); ++i)
    {
        if(GAMECLIENT.GetPlayer(i)->name == player.getName())
            GAMECLIENT.GetPlayer(i)->rating = player.getPunkte();
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  (Lobby-)Status: Benutzerdefinierter Fehler (kann auch Conn-Loss o.ä sein)
 *
 *  @author FloSoft
 */
void dskHostGame::LC_Status_Error(const std::string& error)
{
    WindowManager::inst().Show(new iwMsgbox(_("Error"), error, this, MSB_OK, MSB_EXCLAMATIONRED, 0));
}
