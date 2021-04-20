// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
