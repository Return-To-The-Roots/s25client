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
#include "GameMessage_Chat.h"
#include "GameMessage_GameCommand.h"
#include "NWFInfo.h"
#include "ReplayInfo.h"
#include "ai/AIPlayer.h"
#include "network/GameClient.h"

void GameClient::ExecuteNWF()
{
    // Geschickte Network Commands der Spieler ausführen und ggf. im Replay aufzeichnen

    AsyncChecksum checksum = AsyncChecksum::create(*game);
    const unsigned curGF = GetGFNumber();

    for(const NWFPlayerInfo& player : nwfInfo->getPlayerInfos())
    {
        const PlayerGameCommands& currentGCs = player.commands.front();

        // Command im Replay aufzeichnen (wenn nicht gerade eins schon läuft xD)
        // Nur Commands reinschreiben, KEINE PLATZHALTER (nc_count = 0)
        if(!currentGCs.gcs.empty() && replayinfo && replayinfo->replay.IsRecording())
        {
            // Set the current checksum as the GF checksum. The checksum from the command is from the last NWF!
            PlayerGameCommands replayCmds(checksum, currentGCs.gcs);
            replayinfo->replay.AddGameCommand(curGF, player.id, replayCmds);
        }

        // Das ganze Zeug soll die andere Funktion ausführen
        ExecuteAllGCs(player.id, currentGCs);
    }

    // Send GC message for this NWF
    // First for all potential AIs as we need to combine the AI cmds of the local player with our own ones
    for(AIPlayer& ai : game->aiPlayers_)
    {
        const std::vector<gc::GameCommandPtr> aiGCs = ai.FetchGameCommands();
        /// Cmds from own AI get added to our gcs
        if(ai.GetPlayerId() == GetPlayerId())
            gameCommands_.insert(gameCommands_.end(), aiGCs.begin(), aiGCs.end());
        else
            mainPlayer.sendMsgAsync(new GameMessage_GameCommand(ai.GetPlayerId(), checksum, aiGCs));
        for(auto& msg : ai.getAIInterface().FetchChatMessages())
            mainPlayer.sendMsgAsync(msg.release());
    }
    mainPlayer.sendMsgAsync(new GameMessage_GameCommand(0xFF, checksum, gameCommands_));
    gameCommands_.clear();
}
