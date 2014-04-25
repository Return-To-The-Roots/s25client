// $Id: GameClientGF_Game.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "GameClient.h"
#include "Random.h"
#include "GameMessages.h"

void GameClient::ExecuteGameFrame_Game()
{
    // Geschickte Network Commands der Spieler ausführen und ggf. im Replay aufzeichnen

    // Bei evtl. Spielerwechsel die IDs speichern, die "gewechselt" werden sollen
    unsigned char player_switch_old_id = 255, player_switch_new_id = 255;

    int checksum = Random::inst().GetCurrentRandomValue();

    for(unsigned char i = 0; i < players.getCount(); ++i)
    {
        if(players[i].ps == PS_OCCUPIED || players[i].ps == PS_KI)
        {
            GameMessage_GameCommand& msg = players[i].gc_queue.front();

            // Command im Replay aufzeichnen (wenn nicht gerade eins schon läuft xD)
            // Nur Commands reinschreiben, KEINE PLATZHALTER (nc_count = 0)
            if(msg.gcs.size() > 0 && !replay_mode)
            {
                // Aktuelle Checksumme reinschreiben
                GameMessage_GameCommand tmp(msg.player, checksum, msg.gcs);
                replayinfo.replay.AddGameCommand(framesinfo.nr, tmp.GetLength(), tmp.GetData());
            }

            // Das ganze Zeug soll die andere Funktion ausführen
            ExecuteAllGCs(msg, &player_switch_old_id, &player_switch_new_id);

            // Nachricht abwerfen :)
            players[i].gc_queue.pop_front();

        }
    }

    // Frame ausführen
    NextGF();

    //LOG.lprintf("%d = %d - %d\n", framesinfo.nr / framesinfo.nwf_length, checksum, Random::inst().GetCurrentRandomValue());

    // Stehen eigene Commands an, die gesendet werden müssen?
    send_queue.push(new GameMessage_GameCommand(playerid, checksum, gcs));

    // alles gesendet --> Liste löschen
    gcs.clear();


    // Evtl Spieler wechseln?
    if(player_switch_old_id != 255)
        ChangePlayer(player_switch_old_id, player_switch_new_id);
}
