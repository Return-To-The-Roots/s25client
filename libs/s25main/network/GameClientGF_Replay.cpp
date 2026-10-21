// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameManager.h"
#include "ReplayInfo.h"
#include "ReplayPlayback.h"
#include "helpers/format.hpp"
#include "network/ClientInterface.h"
#include "network/GameClient.h"
#include "s25util/Log.h"

void GameClient::ExecuteGameFrame_Replay()
{
    const unsigned curGF = GetGFNumber();
    RTTR_Assert(replayinfo->nextGF.value_or(curGF) >= curGF || curGF > replayinfo->replay.GetLastGF()); //-V807

    const ReplayCommandsResult result =
      ExecuteReplayCommands(replayinfo->replay, replayinfo->nextGF, *game, [this](const Replay::ChatCommand& cmd) {
          if(ci)
              ci->CI_Chat(cmd.player, cmd.dest, cmd.msg);
      });

    // Only the first one is worth reporting: every NWF after it is async too.
    if(result.desync && !replayinfo->desyncGF)
    {
        const ReplayDesync& desync = *result.desync;
        replayinfo->desyncGF = curGF;

        if(ci)
        {
            ci->CI_ReplayAsync(
              helpers::format(_("Warning: The played replay is not in sync with the original match. (GF: %u)"), curGF));
        }

        LOG.write("Async at GF %u: Checksum %i:%i ObjCt %u:%u ObjIdCt %u:%u\n") % curGF % desync.expected.randChecksum
          % desync.actual.randChecksum % desync.expected.objCt % desync.actual.objCt % desync.expected.objIdCt
          % desync.actual.objIdCt;

        // and pause the game for further investigation
        framesinfo.isPaused = true;
        if(skiptogf)
            skiptogf = 0;
    }

    // Run game simulation
    NextGF(result.executedGameCommands);

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

        if(replayinfo->desyncGF && ci)
        {
            // in-game messenger
            ci->CI_ReplayEndReached(
              helpers::format(_("Notice: The replay was out of sync from GF %u on."), *replayinfo->desyncGF));
        }
        replayinfo->nextGF.reset();
        framesinfo.isPaused = true;
        if(skiptogf)
            skiptogf = GetGFNumber();
    }
}
