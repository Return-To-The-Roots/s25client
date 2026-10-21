// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ReplayPlayback.h"
#include "EventManager.h"
#include "Game.h"
#include "network/PlayerGameCommands.h"
#include "variant.h"

ReplayCommandsResult ExecuteReplayCommands(Replay& replay, std::optional<unsigned>& nextGF, Game& game,
                                           const std::function<void(const Replay::ChatCommand&)>& onChat)
{
    ReplayCommandsResult result;
    const unsigned curGF = game.em_->GetCurrentGF();
    if(nextGF != curGF)
        return result;

    const AsyncChecksum checksum = AsyncChecksum::create(game);
    do
    {
        const auto cmd = replay.ReadCommand();
        visit(composeVisitor(
                [&onChat](const Replay::ChatCommand& chatCmd) {
                    if(onChat)
                        onChat(chatCmd);
                },
                [&](const Replay::GameCommand& gameCmd) {
                    result.executedGameCommands = true;
                    for(const gc::GameCommandPtr& gc : gameCmd.cmds.gcs)
                        gc->Execute(game.world_, gameCmd.player);

                    const AsyncChecksum& expected = gameCmd.cmds.checksum;
                    // A zero checksum means the recording did not store one
                    if(!result.desync && expected.randChecksum != 0 && expected != checksum)
                        result.desync = ReplayDesync{curGF, expected, checksum};
                }),
              cmd);
        nextGF = replay.ReadGF();
    } while(nextGF == curGF);
    return result;
}
