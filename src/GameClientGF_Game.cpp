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

#include "defines.h" // IWYU pragma: keep
#include "GameClient.h"
#include "GamePlayer.h"
#include "Random.h"
#include "GameMessage_GameCommand.h"
#include "libutil/src/Log.h"
#include "libutil/src/Serializer.h"

void GameClient::ExecuteNWF()
{
    // Geschickte Network Commands der Spieler ausführen und ggf. im Replay aufzeichnen

    AsyncChecksum checksum(RANDOM.GetCurrentRandomValue());
    const unsigned curGF = GetGFNumber();

    for(unsigned i = 0; i < GetPlayerCount(); ++i)
    {
        GamePlayer& player = GetPlayer(i);
        if(player.isUsed())
        {
            GameMessage_GameCommand& msg = player.gc_queue.front();

            // Command im Replay aufzeichnen (wenn nicht gerade eins schon läuft xD)
            // Nur Commands reinschreiben, KEINE PLATZHALTER (nc_count = 0)
            if(!msg.gcs.empty() && !replay_mode)
            {
                // Aktuelle Checksumme reinschreiben
                msg.checksum = checksum;
                Serializer ser;
                msg.Serialize(ser);
                replayinfo.replay.AddGameCommand(curGF, ser.GetLength(), ser.GetData());
            }

            // Das ganze Zeug soll die andere Funktion ausführen
            ExecuteAllGCs(msg);

            // Nachricht abwerfen :)
            player.gc_queue.pop();
        }
    }

    // Send GC message for this NWF
    send_queue.push(new GameMessage_GameCommand(playerId_, checksum, gameCommands_));
    //LOG.write("CLIENT >>> GC %u\n", playerId_);

    // alles gesendet --> Liste löschen
    gameCommands_.clear();
}
