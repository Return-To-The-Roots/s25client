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

#include "Game.h"
#include "GamePlayer.h"
#include "ai/AIPlayer.h"
#include "network/ClientInterface.h"
#include "network/GameClient.h"
#include "network/GameMessages.h"

/**
 *  Chatbefehl, hängt eine Textnachricht in die Sende-Queue.
 *
 *  @param[in] text        Der Text
 *  @param[in] destination Ziel der Nachricht
 */
void GameClient::Command_Chat(const std::string& text, const ChatDestination cd)
{
    // Replaymodus oder kein Text --> nichts senden
    if(IsReplayModeOn() || text.empty())
        return;

    mainPlayer.sendMsgAsync(new GameMessage_Chat(0xff, cd, text));
}

void GameClient::Command_SetNation(Nation newNation)
{
    mainPlayer.sendMsgAsync(new GameMessage_Player_Nation(0xff, newNation));
}

void GameClient::Command_SetTeam(Team newTeam)
{
    mainPlayer.sendMsgAsync(new GameMessage_Player_Team(0xff, newTeam));
}

/**
 *  sendet den "Bereit"-Status.
 */
void GameClient::Command_SetReady(bool isReady)
{
    mainPlayer.sendMsgAsync(new GameMessage_Player_Ready(0xFF, isReady));
}

void GameClient::Command_SetColor(unsigned newColor)
{
    mainPlayer.sendMsgAsync(new GameMessage_Player_Color(0xFF, newColor));
}

/**
 *  wechselt einen Spieler.
 *
 *  @param[in] old_id Alte Spieler-ID
 *  @param[in] new_id Neue Spieler-ID
 */
void GameClient::ChangePlayerIngame(const unsigned char playerId1, const unsigned char playerId2)
{
    RTTR_Assert(state == CS_GAME); // Must be ingame

    LOG.write("GameClient::ChangePlayer %i - %i \n") % static_cast<unsigned>(playerId1)
      % static_cast<unsigned>(playerId2);
    // Gleiche ID - wäre unsinnig zu wechseln
    if(playerId1 == playerId2)
        return;

    // ID auch innerhalb der Spielerzahl?
    if(playerId2 >= GetNumPlayers() || playerId1 >= GetNumPlayers())
        return;

    if(IsReplayModeOn())
    {
        RTTR_Assert(playerId1 == GetPlayerId());
        // There must be someone at this slot
        if(!GetPlayer(playerId2).isUsed())
            return;

        // In replay mode we don't touch the player
    } else
    {
        // old_id must be a player
        GamePlayer& player1 = GetPlayer(playerId1);
        if(player1.ps != PS_OCCUPIED)
            return;
        // new_id must be an AI
        GamePlayer& player2 = GetPlayer(playerId2);
        if(player2.ps != PS_AI)
            return;

        std::swap(player1.ps, player2.ps);
        std::swap(player1.aiInfo, player2.aiInfo);
        if(IsHost())
        {
            // Switch AIs
            game->aiPlayers_.erase_if([playerId2](const auto& player) { return player.GetPlayerId() == playerId2; });
            game->AddAIPlayer(CreateAIPlayer(playerId1, AI::Info(AI::DUMMY)));
        }
        GetPlayer(playerId1).ps = PS_AI;
        GetPlayer(playerId2).ps = PS_OCCUPIED;
    }

    // Wenn wir betroffen waren, unsere ID neu setzen
    if(mainPlayer.playerId == playerId1)
    {
        mainPlayer.playerId = playerId2;

        if(!IsReplayModeOn())
        {
            // Our currently accumulated gamecommands are invalid after the change, as they would modify the old player
            gameCommands_.clear();
        }
    }

    if(ci)
        ci->CI_PlayersSwapped(playerId1, playerId2);
}
