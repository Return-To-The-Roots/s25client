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

#include "rttrDefines.h" // IWYU pragma: keep
#include "ClientInterface.h"
#include "GameClient.h"
#include "GameMessages.h"
#include "GamePlayer.h"

/**
 *  Chatbefehl, hängt eine Textnachricht in die Sende-Queue.
 *
 *  @param[in] text        Der Text
 *  @param[in] destination Ziel der Nachricht
 */
void GameClient::Command_Chat(const std::string& text, const ChatDestination cd)
{
    // Replaymodus oder kein Text --> nichts senden
    if(GAMECLIENT.IsReplayModeOn() || text.length() == 0)
        return;

    send_queue.push(new GameMessage_Server_Chat(playerId_, cd, text));
}

void GameClient::Command_SetNation(Nation newNation)
{
    send_queue.push(new GameMessage_Player_Set_Nation(0xff, newNation));
}

void GameClient::Command_SetTeam(Team newTeam)
{
    send_queue.push(new GameMessage_Player_Set_Team(0xff, newTeam));
}

/**
 *  sendet den "Bereit"-Status.
 */
void GameClient::Command_SetReady(bool isReady)
{
    send_queue.push(new GameMessage_Player_Ready(0xFF, isReady));
}

void GameClient::Command_SetColor(unsigned newColor)
{
    send_queue.push(new GameMessage_Player_Set_Color(0xFF, newColor));
}

/**
 *  wechselt einen Spieler.
 *
 *  @param[in] old_id Alte Spieler-ID
 *  @param[in] new_id Neue Spieler-ID
 */
void GameClient::ChangePlayerIngame(const unsigned char player1, const unsigned char player2)
{
    RTTR_Assert(state == CS_GAME); // Must be ingame

    LOG.write("GameClient::ChangePlayer %i - %i \n") % player1 % player2;
    // Gleiche ID - wäre unsinnig zu wechseln
    if(player1 == player2)
        return;

    // ID auch innerhalb der Spielerzahl?
    if(player2 >= GetPlayerCount() || player1 >= GetPlayerCount())
        return;

    if(IsReplayModeOn())
    {
        RTTR_Assert(player1 == playerId_);
        // There must be someone at this slot
        if(!GetPlayer(player2).isUsed())
            return;

        // In replay mode we don't touch the player
    } else
    {
        // old_id must be a player
        if(GetPlayer(player1).ps != PS_OCCUPIED)
            return;
        // new_id must be an AI
        if(GetPlayer(player2).ps != PS_AI)
            return;

        GetPlayer(player1).ps = PS_AI;
        GetPlayer(player2).ps = PS_OCCUPIED;
    }

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
    swap(GetPlayer(player1).gc_queue, GetPlayer(player2).gc_queue);

    if(ci)
        ci->CI_PlayersSwapped(player1, player2);
}
