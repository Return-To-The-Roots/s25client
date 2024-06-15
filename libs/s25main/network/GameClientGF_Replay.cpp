// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameManager.h"
#include "PlayerGameCommands.h"
#include "ReplayInfo.h"
#include "helpers/format.hpp"
#include "network/ClientInterface.h"
#include "network/GameClient.h"
#include "variant.h"
#include "s25util/Log.h"

void GameClient::ExecuteGameFrame_Replay()
{
    AsyncChecksum checksum = AsyncChecksum::create(*game);

    const unsigned curGF = GetGFNumber();
    RTTR_Assert(replayinfo->next_gf.value_or(curGF) >= curGF || curGF > replayinfo->replay.GetLastGF()); //-V807

    bool cmdsExecuted = false;
    auto& replay = replayinfo->replay;
    // Execute all commands from the replay for the current GF
    while(replayinfo->next_gf && replayinfo->next_gf == curGF)
    {
        const auto cmd = replay.ReadCommand();
        visit(
          composeVisitor(
            [this](const Replay::ChatCommand& cmd) {
                if(ci)
                    ci->CI_Chat(cmd.player, cmd.dest, cmd.msg);
            },
            [this, &cmdsExecuted, &checksum, curGF](const Replay::GameCommand& cmd) {
                cmdsExecuted = true;

                ExecuteAllGCs(cmd.player, cmd.cmds);
                const AsyncChecksum& msgChecksum = cmd.cmds.checksum;

                // Check for async if checksum data is valid
                if(msgChecksum.randChecksum != 0 && msgChecksum != checksum)
                {
                    // Show message if this is the first async GF
                    if(replayinfo->async == 0)
                    {
                        if(ci)
                        {
                            ci->CI_ReplayAsync(helpers::format(
                              _("Warning: The played replay is not in sync with the original match. (GF: %u)"), curGF));
                        }

                        LOG.write("Async at GF %u: Checksum %i:%i ObjCt %u:%u ObjIdCt %u:%u\n") % curGF
                          % msgChecksum.randChecksum % checksum.randChecksum % msgChecksum.objCt % checksum.objCt
                          % msgChecksum.objIdCt % checksum.objIdCt;

                        // and pause the game for further investigation
                        framesinfo.isPaused = true;
                        if(skiptogf)
                            skiptogf = 0;
                    }

                    replayinfo->async++;
                }
            }),
          cmd);
        // Read GF of next command
        replayinfo->next_gf = replayinfo->replay.ReadGF();
    }

    // Run game simulation
    NextGF(cmdsExecuted);

    // Check for game end
    if(curGF == replayinfo->replay.GetLastGF())
    {
        if(ci)
        {
            using std::chrono::duration_cast;
            std::chrono::seconds runtime = duration_cast<std::chrono::seconds>(GAMEMANAGER.GetRuntime());
            std::chrono::hours hours = duration_cast<std::chrono::hours>(runtime);
            std::chrono::minutes mins = duration_cast<std::chrono::minutes>(runtime - hours);
            std::chrono::seconds secs = duration_cast<std::chrono::seconds>(runtime - hours - mins);
            const std::string text =
              (boost::format(_("Notice: The played replay has ended. (GF: %u, %dh %dmin %ds, TF: %u, AVG_FPS: %u)"))
               % curGF % hours.count() % mins.count() % secs.count() % GAMEMANAGER.GetNumFrames()
               % GAMEMANAGER.GetAverageGFPS())
                .str();

            ci->CI_ReplayEndReached(text);
        }

        if(replayinfo->async != 0)
        {
            // in-game messenger
            if(ci)
            {
                ci->CI_ReplayEndReached(
                  helpers::format(_("Notice: Overall asynchronous frame count: %u"), replayinfo->async));
            }
        }
        replayinfo->next_gf.reset();
        framesinfo.isPaused = true;
        if(skiptogf)
            skiptogf = GetGFNumber();
    }
}
