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
#include "defines.h"
#include "GameClient.h"

#include "drivers/VideoDriverWrapper.h"
#include "nodeObjs/noFlag.h"
#include "GameWorld.h"
#include "GameClientPlayer.h"

#include "GameServer.h"
#include "buildings/nobUsual.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobBaseWarehouse.h"
#include "desktops/dskGameInterface.h"
#include "ClientInterface.h"
#include "GameMessages.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void GameClient::Command_SetFlag2(const MapPoint pt, unsigned char player)
{
    gw->SetFlag(pt, player);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Chatbefehl, hängt eine Textnachricht in die Sende-Queue.
 *
 *  @param[in] text        Der Text
 *  @param[in] destination Ziel der Nachricht
 *
 *  @author FloSoft
 */
void GameClient::Command_Chat(const std::string& text, const ChatDestination cd)
{
    // Replaymodus oder kein Text --> nichts senden
    if(GAMECLIENT.IsReplayModeOn() || text.length() == 0)
        return;

    send_queue.push(new GameMessage_Server_Chat(playerId_, cd, text));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
void GameClient::Command_ToggleNation()
{
    send_queue.push(new GameMessage_Player_Toggle_Nation
                    (0xff, Nation((this->GetLocalPlayer()->nation + 1) % NAT_COUNT)));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
void GameClient::Command_ToggleTeam(Team newteam)
{
    send_queue.push(new GameMessage_Player_Toggle_Team(0xff, newteam));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  sendet den "Bereit"-Status.
 *
 *  @author FloSoft
 */
void GameClient::Command_ToggleReady()
{
    send_queue.push(new GameMessage_Player_Ready(0xFF, GetLocalPlayer()->ready ));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 */
void GameClient::Command_ToggleColor()
{
    send_queue.push(new GameMessage_Player_Toggle_Color(0xFF, 0xFF));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  wechselt einen Spieler.
 *
 *  @param[in] old_id Alte Spieler-ID
 *  @param[in] new_id Neue Spieler-ID
 *
 *  @author OLiver
 *  @author FloSoft
 */
void GameClient::ChangePlayer(const unsigned char old_id, const unsigned char new_id)
{
	LOG.lprintf("GameClient::ChangePlayer %i - %i \n",old_id, new_id); 
    // ID auch innerhalb der Spielerzahl?
    if(new_id >= players.getCount())
        return;

    // Gleiche ID - wäre unsinnig zu wechseln
    if(old_id == new_id)
        return;

    // old_id muss richtiger Spieler, new_id KI sein, ansonsten geht das natürlich nicht
    if( !(players[old_id].ps == PS_OCCUPIED && players[new_id].ps == PS_KI) )
        return;

    players[old_id].ps = PS_KI;
    players[new_id].ps = PS_OCCUPIED;

    // Wenn wir betroffen waren, unsere ID neu setzen und BQ neu berechnen
    if(playerId_ == old_id)
    {
        playerId_ = new_id;

        // BQ überall neu berechnen
        for(unsigned y = 0; y < gw->GetHeight(); ++y)
        {
            for(unsigned x = 0; x < gw->GetWidth(); ++x)
                gw->SetBQ(MapPoint(x, y), new_id);
        }

        // Visuelle Einstellungen vom Spieler wieder holen
        GetVisualSettings();

        //// zum HQ hinscrollen
        //gw->MoveToMapObject(player->hqPos.x,player->hqy);
        //GameClientPlayer *player = players[playerid]; // wegen GCC-Fehlermeldung auskommentiert
    }
	//swap command que
    std::swap(players[old_id].gc_queue, players[new_id].gc_queue);

    // GUI Bescheid sagen (um z.B. Schatten neu zu berechnen)
    if(ci)
        ci->CI_PlayersSwapped(old_id, new_id);
}

void GameClient::ChangeReplayPlayer(const unsigned new_id)
{
    unsigned old_id = playerId_;

    if(old_id == new_id)
        // Unsinn auf den selben Spieler zu wechseln
        return;
    // Auch innerhalb der gültigen Spieler?
    if(new_id >= GAMECLIENT.GetPlayerCount())
        return;
    // Und ein richtiger ehemaliger Spieler?
    if(GAMECLIENT.GetPlayer(new_id)->ps != PS_KI &&
            GAMECLIENT.GetPlayer(new_id)->ps != PS_OCCUPIED)
        return;


    playerId_ = new_id;

    // BQ überall neu berechnen
    for(unsigned y = 0; y < gw->GetHeight(); ++y)
    {
        for(unsigned x = 0; x < gw->GetWidth(); ++x)
            gw->SetBQ(MapPoint(x, y), new_id);
    }

    // GUI Bescheid sagen (um z.B. Schatten neu zu berechnen)
    if(ci)
        ci->CI_PlayersSwapped(old_id, new_id);
}

