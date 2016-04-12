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
#include "GameClient.h"

#include "GameClientPlayer.h"

#include "ClientInterface.h"
#include "GameMessages.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep




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

void GameClient::Command_ToggleNation()
{
    send_queue.push(new GameMessage_Player_Set_Nation
                    (0xff, Nation((this->GetLocalPlayer().nation + 1) % NAT_COUNT)));
}

void GameClient::Command_ToggleTeam(Team newteam)
{
    send_queue.push(new GameMessage_Player_Set_Team(0xff, newteam));
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  sendet den "Bereit"-Status.
 *
 *  @author FloSoft
 */
void GameClient::Command_ToggleReady()
{
    send_queue.push(new GameMessage_Player_Ready(0xFF, GetLocalPlayer().ready));
}

void GameClient::Command_SetColor()
{
    send_queue.push(new GameMessage_Player_Set_Color(0xFF, GetLocalPlayer().color));
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
void GameClient::ChangePlayerIngame(const unsigned char player1, const unsigned char player2)
{
	LOG.lprintf("GameClient::ChangePlayer %i - %i \n",player1, player2); 
    // Gleiche ID - wäre unsinnig zu wechseln
    if(player1 == player2)
        return;

    // ID auch innerhalb der Spielerzahl?
    if(player2 >= players.getCount() || player1 >= players.getCount())
        return;

    // old_id must be a player unless its a replay
    if(players[player1].ps != PS_OCCUPIED && !IsReplayModeOn())
        return;
    // new_id must be an AI. For replays it can also be another player
    if(players[player2].ps != PS_KI && (!IsReplayModeOn() || players[player2].ps != PS_OCCUPIED))
        return;

    // In replay mode we don't touch the player
    if(!IsReplayModeOn())
    {
        players[player1].ps = PS_KI;
        players[player2].ps = PS_OCCUPIED;
    }else
        RTTR_Assert(player1 == playerId_);

    // Wenn wir betroffen waren, unsere ID neu setzen
    if(playerId_ == player1)
    {
        playerId_ = player2;

        if(!IsReplayModeOn())
        {
            // Our currently accumulated gamecommands are invalid after the change, as they would modify the old player
            gameCommands_.clear();
        }
    }

    using std::swap;
    swap(players[player1].gc_queue, players[player2].gc_queue);

    if(ci)
        ci->CI_PlayersSwapped(player1, player2);
}

