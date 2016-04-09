// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h" // IWYU pragma: keep
#include "GameClient.h"

#include "GlobalVars.h"
#include "Random.h"
#include "GameManager.h"
#include "ClientInterface.h"
#include "libutil/src/Log.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

void GameClient::ExecuteGameFrame_Replay()
{
    randcheckinfo.rand = RANDOM.GetCurrentRandomValue();
    AsyncChecksum checksum(randcheckinfo.rand);

    RTTR_Assert(replayinfo.next_gf >= framesinfo.gf_nr || framesinfo.gf_nr > replayinfo.replay.lastGF_);

    // Commands alle aus der Datei lesen
    while(replayinfo.next_gf == framesinfo.gf_nr) // Schon an der Zeit?
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

            if(ci)
                ci->CI_Chat(player, ChatDestination(dest), message);
        }
        // Game Command?
        else if(rc == Replay::RC_GAME)
        {
            std::vector<unsigned char> gcData = replayinfo.replay.ReadGameCommand();
            Serializer ser(&gcData.front(), gcData.size());
            GameMessage_GameCommand msg;
            msg.Deserialize(ser);
            // Nächsten Zeitpunkt lesen
            replayinfo.replay.ReadGF(&replayinfo.next_gf);

            // NCs ausführen (4 Bytes Checksumme und 1 Byte Player-ID überspringen)
            ExecuteAllGCs(msg);

            // Replay ist NSYNC äh ASYNC!
            if(msg.checksum.randState != 0 &&  msg.checksum != checksum)
            {
                if(replayinfo.async == 0)
                {
                    // Meldung mit GF erzeugen
                    char text[256];
                    sprintf(text, _("Warning: The played replay is not in sync with the original match. (GF: %u)"), framesinfo.gf_nr);

                    // Messenger im Game (prints to console too)
                    if(ci)
                        ci->CI_ReplayAsync(text);

                    LOG.lprintf("Async at GF %u: Checksum %i:%i ObjCt %u:%u ObjIdCt %u:%u\n", framesinfo.gf_nr,
                        msg.checksum.randState, checksum.randState,
                        msg.checksum.objCt, checksum.objCt,
                        msg.checksum.objIdCt, checksum.objIdCt);

                    // pausieren
                    framesinfo.isPaused = true;
                }

                replayinfo.async++;
            }

            // resync random generator, so replay "can't" be async.
            // (of course it can, since we resynchronize only after each command, the command itself could be use multiple rand values)
            //RANDOM.ReplaySet(msg.checksum);
        }
    }

    // Frame ausführen
    NextGF();

    // Replay zu Ende?
    if(framesinfo.gf_nr == replayinfo.replay.lastGF_)
    {
        // Replay zu Ende

        // Meldung erzeugen
        char text[256];
        sprintf(text, _("Notice: The played replay has ended. (GF: %u, %dh %dmin %ds, TF: %u, AVG_FPS: %u)"), framesinfo.gf_nr, GAMEMANAGER.GetRuntime() / 3600, ((GAMEMANAGER.GetRuntime()) % 3600) / 60, (GameManager::inst().GetRuntime()) % 3600 % 60, GameManager::inst().GetFrameCount(), GameManager::inst().GetAverageFPS());

        // Messenger im Game
        if(ci)
            ci->CI_ReplayEndReached(text);

        if(replayinfo.async != 0)
        {
            char text[256];
            sprintf(text, _("Notice: Overall asynchronous frame count: %u"), replayinfo.async);
            // Messenger im Game
            if(ci)
                ci->CI_ReplayEndReached(text);
        }

        replayinfo.end = true;

        // pausieren
        framesinfo.isPaused = true;
    }
}
