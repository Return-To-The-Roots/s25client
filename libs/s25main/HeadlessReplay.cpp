// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "HeadlessReplay.h"
#include "EventManager.h"
#include "Game.h"
#include "GameSetup.h"
#include "PlayerInfo.h"
#include "Savegame.h"
#include "ai/random.h"
#include "network/PlayerGameCommands.h"
#include "random/Random.h"
#include "variant.h"
#include "world/GameWorld.h"
#include <stdexcept>

HeadlessReplay::HeadlessReplay(const boost::filesystem::path& replayPath)
{
    if(!replay_.LoadHeader(replayPath) || !replay_.LoadGameData(mapInfo_))
    {
        const std::string reason =
          replay_.GetLastErrorMsg().empty() ? std::string("Unknown error") : replay_.GetLastErrorMsg();
        throw std::runtime_error("Could not load replay " + replayPath.string() + ": " + reason);
    }

    // Recorded on another machine, so use the map data embedded in the replay instead
    mapInfo_.filepath.clear();

    std::vector<PlayerInfo> players;
    players.reserve(replay_.GetNumPlayers());
    for(unsigned i = 0; i < replay_.GetNumPlayers(); ++i)
        players.emplace_back(replay_.GetPlayer(i));

    startGF_ = mapInfo_.savegame ? mapInfo_.savegame->start_gf : 0;
    RANDOM.Init(replay_.getSeed());
    AI::getRandomGenerator().seed(replay_.getSeed());

    game_ = std::make_unique<Game>(replay_.ggs, startGF_, players);
    SetupGameWorld(*game_, mapInfo_, localGameState_, &replay_);

    // A script writing to stdout would corrupt whatever the caller prints
    if(game_->world_.HasLua())
        game_->world_.GetLua().setSuppressStdout(true);

    game_->Start(isFromSavegame());
    nextGF_ = replay_.ReadGF();
}

HeadlessReplay::~HeadlessReplay() = default;

const GameWorld& HeadlessReplay::getWorld() const
{
    return game_->world_;
}

unsigned HeadlessReplay::getCurrentGF() const
{
    return game_->em_->GetCurrentGF();
}

bool HeadlessReplay::RunGF()
{
    if(!nextGF_ || desync_)
        return false;

    const unsigned curGF = getCurrentGF();
    AsyncChecksum checksum;
    if(*nextGF_ == curGF)
        checksum = AsyncChecksum::create(*game_);

    while(nextGF_ && *nextGF_ == curGF)
    {
        const auto cmd = replay_.ReadCommand();
        visit(composeVisitor([](const Replay::ChatCommand&) {},
                             [&](const Replay::GameCommand& gcmd) {
                                 for(const gc::GameCommandPtr& gc : gcmd.cmds.gcs)
                                     gc->Execute(game_->world_, gcmd.player);

                                 const AsyncChecksum& expected = gcmd.cmds.checksum;
                                 // A zero checksum means the recording did not store one
                                 if(!desync_ && expected.randChecksum != 0 && expected != checksum)
                                     desync_ = ReplayDesync{curGF, expected, checksum};
                             }),
              cmd);
        nextGF_ = replay_.ReadGF();
    }

    // Stop at a desync so the world and the RNG log still show the state that produced it
    if(desync_)
        return false;
    game_->RunGF();
    return nextGF_.has_value();
}

bool HeadlessReplay::Run(const std::function<void(const HeadlessReplay&)>& onGameFrame)
{
    for(bool hasMoreFrames = true; hasMoreFrames;)
    {
        hasMoreFrames = RunGF();
        if(onGameFrame)
            onGameFrame(*this);
    }
    return !desync_;
}
