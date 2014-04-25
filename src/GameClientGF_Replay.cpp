// $Id: GameClientGF_Replay.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Siedler II.5 RTTR.
//
// Siedler II.5 RTTR is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Siedler II.5 RTTR is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Siedler II.5 RTTR. If not, see <http://www.gnu.org/licenses/>.

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "GameClient.h"

#include "GlobalVars.h"
#include "Loader.h"
#include "Random.h"
#include "GameManager.h"
#include "dskGameInterface.h"
#include "ClientInterface.h"
#include "GameMessages.h"

void GameClient::ExecuteGameFrame_Replay()
{
    randcheckinfo.rand = RANDOM.GetCurrentRandomValue();

    // Commands alle aus der Datei lesen
    while(true)
    {
        // Schon an der Zeit?
        if(replayinfo.next_gf == framesinfo.nr)
        {
            // RC-Type auslesen
            Replay::ReplayCommand rc = replayinfo.replay.ReadRCType();

            // Chat Command?
            if(rc == Replay::RC_CHAT)
            {
                unsigned char player, dest;
                std::string message;
                replayinfo.replay.ReadChatCommand(&player, &dest, message);

                // Nächsten Zeitpunkt lesen
                replayinfo.replay.ReadGF(&replayinfo.next_gf);

                /*      char from[256];
                        snprintf(from, 256, _("<%s> "), players[player]->name.GetStr());*/

                if(ci)
                    ci->CI_Chat(player, ChatDestination(dest), message);
            }
            // Game Command?
            else if(rc == Replay::RC_GAME)
            {
                unsigned char* data;
                unsigned short length;

                replayinfo.replay.ReadGameCommand(&length, &data);
                // Nächsten Zeitpunkt lesen
                replayinfo.replay.ReadGF(&replayinfo.next_gf);
                GameMessage_GameCommand msg(data, length);

                // NCs ausführen (4 Bytes Checksumme und 1 Byte Player-ID überspringen)
                ExecuteAllGCs(msg, 0, 0);

                // Replay ist NSYNC äh ASYNC!
                if(!replayinfo.async && msg.checksum != 0 && msg.checksum != (unsigned)randcheckinfo.rand)
                {
                    // Meldung mit GF erzeugen
                    char msg[256];
                    sprintf(msg, _("Warning: The played replay is not in sync with the original match. (GF: %u)"), framesinfo.nr);
                    // Messenger im Game
                    if(ci && GLOBALVARS.ingame)
                        ci->CI_ReplayAsync(msg);

                    // Konsole
                    LOG.lprintf("%s\n", msg);

                    replayinfo.async = true;

                    //// pausieren
                    //framesinfo.pause = true;
                }


                delete [] data;
            }




        }
        else
        {
            // noch nichtan der Reihe, dann wieder raus
            break;
        }
    }



    // Frame ausführen
    NextGF();

    // Replay zu Ende?
    if(framesinfo.nr == replayinfo.replay.last_gf)
    {
        // Replay zu Ende

        // Meldung erzeugen
        char msg[256];
        sprintf(msg, _("Notice: The played replay has ended. (GF: %u, %dh %dmin %ds, TF: %u, AVG_FPS: %u)"), framesinfo.nr, GameManager::inst().GetRuntime() / 3600, ((GameManager::inst().GetRuntime()) % 3600) / 60, (GameManager::inst().GetRuntime()) % 3600 % 60, GameManager::inst().GetFrameCount(), GameManager::inst().GetAverageFPS());
        // Messenger im Game
        if(ci && GLOBALVARS.ingame)
            ci->CI_ReplayEndReached(msg);

        // Konsole
        LOG.lprintf("%s\n", msg);

        replayinfo.end = true;

        // pausieren
        framesinfo.pause = true;
    }
}
