// $Id: GameClientGF_Replay.cpp 9524 2014-12-01 14:06:14Z marcus $
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
                if(msg.checksum != 0 && msg.checksum != (unsigned)randcheckinfo.rand)
                {
                    if(replayinfo.async == 0)
                    {
                        // Meldung mit GF erzeugen
                        char text[256];
                        sprintf(text, _("Warning: The played replay is not in sync with the original match. (GF: %u)"), framesinfo.nr);

                        // Messenger im Game (prints to console too)
                        if(ci && GLOBALVARS.ingame)
                            ci->CI_ReplayAsync(text);

                        // pausieren
                        framesinfo.pause = true;
                    }

                    replayinfo.async++;
                }

                // resync random generator, so replay "can't" be async.
                // (of course it can, since we resynchronize only after each command, the command itself could be use multiple rand values)
                //RANDOM.ReplaySet(msg.checksum);

                delete[] data;
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
        char text[256];
        sprintf(text, _("Notice: The played replay has ended. (GF: %u, %dh %dmin %ds, TF: %u, AVG_FPS: %u)"), framesinfo.nr, GameManager::inst().GetRuntime() / 3600, ((GameManager::inst().GetRuntime()) % 3600) / 60, (GameManager::inst().GetRuntime()) % 3600 % 60, GameManager::inst().GetFrameCount(), GameManager::inst().GetAverageFPS());

        // Messenger im Game
        if(ci && GLOBALVARS.ingame)
            ci->CI_ReplayEndReached(text);

        if(replayinfo.async != 0)
        {
            char text[256];
            sprintf(text, _("Notice: Overall asynchronous frame count: %u"), replayinfo.async);
            // Messenger im Game
            if(ci && GLOBALVARS.ingame)
                ci->CI_ReplayEndReached(text);
        }

        replayinfo.end = true;

        // pausieren
        framesinfo.pause = true;
    }
}
