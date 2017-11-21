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
#include "ClientPlayers.h"
#include "GameMessage_GameCommand.h"
#include "GamePlayer.h"
#include "ReplayInfo.h"
#include "network/GameClient.h"
#include "random/Random.h"
#include "libutil/Log.h"
#include <boost/foreach.hpp>

void GameClient::ExecuteNWF()
{
    // Geschickte Network Commands der Spieler ausführen und ggf. im Replay aufzeichnen

    AsyncChecksum checksum = AsyncChecksum::create(*game);
    const unsigned curGF = GetGFNumber();

    BOOST_FOREACH(ClientPlayer& player, clientPlayers->players)
    {
        PlayerGameCommands& currentGCs = player.gcsToExecute.front();

        // Command im Replay aufzeichnen (wenn nicht gerade eins schon läuft xD)
        // Nur Commands reinschreiben, KEINE PLATZHALTER (nc_count = 0)
        if(!currentGCs.gcs.empty() && !replayMode)
        {
            // Aktuelle Checksumme reinschreiben
            currentGCs.checksum = checksum;
            replayinfo->replay.AddGameCommand(curGF, player.id, currentGCs);
        }

        // Das ganze Zeug soll die andere Funktion ausführen
        ExecuteAllGCs(player.id, currentGCs);

        // Nachricht abwerfen :)
        player.gcsToExecute.pop();
    }

    // Send GC message for this NWF
    mainPlayer.sendMsgAsync(new GameMessage_GameCommand(0xFF, checksum, gameCommands_));
    // LOG.writeToFile("CLIENT >>> GC %u\n") % playerId_;

    // alles gesendet --> Liste löschen
    gameCommands_.clear();
}
