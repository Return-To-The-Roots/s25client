// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "ClientInterface.h"
#include "GameClient.h"
#include "GameManager.h"
#include "GlobalVars.h"
#include "PlayerGameCommands.h"
#include "ReplayInfo.h"
#include "random/Random.h"
#include "libutil/Log.h"
#include "libutil/Serializer.h"

void GameClient::ExecuteGameFrame_Replay()
{
    AsyncChecksum checksum(RANDOM.GetChecksum());

    const unsigned curGF = GetGFNumber();
    RTTR_Assert(replayinfo->next_gf >= curGF || curGF > replayinfo->replay.GetLastGF());

    // Execute all commands from the replay for the current GF
    while(replayinfo->next_gf == curGF)
    {
        // What type of command follows?
        Replay::ReplayCommand rc = replayinfo->replay.ReadRCType();

        if(rc == Replay::RC_CHAT)
        {
            uint8_t player, dest;
            std::string message;
            replayinfo->replay.ReadChatCommand(player, dest, message);

            if(ci)
                ci->CI_Chat(player, ChatDestination(dest), message);
        } else if(rc == Replay::RC_GAME)
        {
            std::vector<unsigned char> gcData = replayinfo->replay.ReadGameCommand();
            Serializer ser(&gcData.front(), gcData.size());
            PlayerGameCommands msg;
            uint8_t gcPlayer = ser.PopUnsignedChar();
            msg.Deserialize(ser);

            // Execute them
            ExecuteAllGCs(gcPlayer, msg);
            AsyncChecksum& msgChecksum = msg.checksum;

            // Check for async if checksum data is valid
            if(msgChecksum.randChecksum != 0 && msgChecksum != checksum)
            {
                // Show message if this is the first async GF
                if(replayinfo->async == 0)
                {
                    char text[256];
                    sprintf(text, _("Warning: The played replay is not in sync with the original match. (GF: %u)"), curGF);

                    if(ci)
                        ci->CI_ReplayAsync(text);

                    LOG.write("Async at GF %u: Checksum %i:%i ObjCt %u:%u ObjIdCt %u:%u\n") % curGF % msgChecksum.randChecksum
                      % checksum.randChecksum % msgChecksum.objCt % checksum.objCt % msgChecksum.objIdCt % checksum.objIdCt;

                    // and pause the game for further investigation
                    framesinfo.isPaused = true;
                }

                replayinfo->async++;
            }
        }
        // Read GF of next command
        replayinfo->replay.ReadGF(&replayinfo->next_gf);
    }

    // Run game simulation
    NextGF();

    // Check for game end
    if(curGF == replayinfo->replay.GetLastGF())
    {
        char text[256];
        sprintf(text, _("Notice: The played replay has ended. (GF: %u, %dh %dmin %ds, TF: %u, AVG_FPS: %u)"), curGF,
                GAMEMANAGER.GetRuntime() / 3600, ((GAMEMANAGER.GetRuntime()) % 3600) / 60, (GameManager::inst().GetRuntime()) % 3600 % 60,
                GameManager::inst().GetFrameCount(), GameManager::inst().GetAverageFPS());

        if(ci)
            ci->CI_ReplayEndReached(text);

        if(replayinfo->async != 0)
        {
            char text[256];
            sprintf(text, _("Notice: Overall asynchronous frame count: %u"), replayinfo->async);
            // Messenger im Game
            if(ci)
                ci->CI_ReplayEndReached(text);
        }

        replayinfo->end = true;
        framesinfo.isPaused = true;
    }
}
