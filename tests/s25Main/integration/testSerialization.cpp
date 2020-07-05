// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "GameCommands.h"
#include "GameEvent.h"
#include "GamePlayer.h"
#include "PointOutput.h"
#include "Replay.h"
#include "RttrForeachPt.h"
#include "Savegame.h"
#include "SerializedGameData.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "factories/GameCommandFactory.h"
#include "network/PlayerGameCommands.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "nodeObjs/noFire.h"
#include "gameTypes/MapInfo.h"
#include "s25util/tmpFile.h"
#include <rttr/test/testHelpers.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>

// LCOV_EXCL_START
template<class T>
std::ostream& operator<<(std::ostream& os, const DescIdx<T>& d)
{
    return os << d.value;
}
namespace boost { namespace test_tools { namespace tt_detail {
    template<>
    struct print_log_value<ReplayCommand>
    {
        void operator()(std::ostream& os, ReplayCommand const& rc) { os << static_cast<unsigned>(rc); }
    };
}}} // namespace boost::test_tools::tt_detail
// LCOV_EXCL_STOP

namespace {
struct RandWorldFixture : public WorldFixture<CreateEmptyWorld, 4>
{
    RandWorldFixture()
    {
        RTTR_FOREACH_PT(MapPoint, world.GetSize())
        {
            MapNode& worldNode = world.GetNodeWriteable(pt);
            worldNode.altitude = rand() % 10 + 0xA;
            worldNode.shadow = rand() % 20;
            worldNode.resources = Resource(rand() % 0xFF);
            worldNode.reserved = rand() % 2 == 0;
            worldNode.seaId = rand() % 20;
            worldNode.harborId = rand() % 20;
        }
        world.InitAfterLoad();
        world.GetPlayer(0).name = "Human";
        world.GetPlayer(1).ps = PS_AI; //-V807
        world.GetPlayer(1).aiInfo = AI::Info(AI::DEFAULT, AI::MEDIUM);
        world.GetPlayer(1).name = "PlAI";
        world.GetPlayer(2).ps = PS_LOCKED;
        world.GetPlayer(3).ps = PS_AI; //-V807
        world.GetPlayer(3).aiInfo = AI::Info(AI::DEFAULT, AI::EASY);
        world.GetPlayer(3).name = "PlAI2";

        ggs.speed = GS_VERYFAST;
    }
};

struct GetTestCommands : public GameCommandFactory
{
    PlayerGameCommands result;

    GetTestCommands& create(const Game& game)
    {
        result.checksum = AsyncChecksum::create(game);
        SetFlag(MapPoint(4, 5));
        SetCoinsAllowed(MapPoint(42, 24), false);
        return *this;
    }

protected:
    bool AddGC(gc::GameCommandPtr gc) override
    {
        result.gcs.push_back(gc);
        return true;
    }
};

void AddReplayCmds(Replay& replay, const PlayerGameCommands& cmds)
{
    replay.UpdateLastGF(1);
    replay.AddChatCommand(1, 2, 3, "Hello");
    replay.AddChatCommand(1, 3, 1, "Hello2");
    replay.AddChatCommand(2, 2, 2, "Hello3");

    replay.AddGameCommand(2, 0, cmds);
    replay.AddChatCommand(2, 2, 3, "Hello4");
    replay.UpdateLastGF(5);
}

void CheckReplayCmds(Replay& loadReplay, const PlayerGameCommands& recordedCmds)
{
    BOOST_REQUIRE(loadReplay.IsReplaying());
    unsigned gf;
    BOOST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_REQUIRE_EQUAL(gf, 1u);
    BOOST_REQUIRE_EQUAL(loadReplay.ReadRCType(), ReplayCommand::Chat);
    uint8_t player, dst;
    std::string txt;
    loadReplay.ReadChatCommand(player, dst, txt);
    BOOST_REQUIRE_EQUAL(player, 2);
    BOOST_REQUIRE_EQUAL(dst, 3);
    BOOST_REQUIRE_EQUAL(txt, "Hello");

    BOOST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_REQUIRE_EQUAL(gf, 1u);
    BOOST_REQUIRE_EQUAL(loadReplay.ReadRCType(), ReplayCommand::Chat);
    loadReplay.ReadChatCommand(player, dst, txt);
    BOOST_REQUIRE_EQUAL(player, 3);
    BOOST_REQUIRE_EQUAL(dst, 1);
    BOOST_REQUIRE_EQUAL(txt, "Hello2");

    BOOST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_REQUIRE_EQUAL(gf, 2u);
    BOOST_REQUIRE_EQUAL(loadReplay.ReadRCType(), ReplayCommand::Chat);
    loadReplay.ReadChatCommand(player, dst, txt);
    BOOST_REQUIRE_EQUAL(player, 2);
    BOOST_REQUIRE_EQUAL(dst, 2);
    BOOST_REQUIRE_EQUAL(txt, "Hello3");

    BOOST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_REQUIRE_EQUAL(gf, 2u);
    BOOST_REQUIRE_EQUAL(loadReplay.ReadRCType(), ReplayCommand::Game);
    PlayerGameCommands cmds;
    loadReplay.ReadGameCommand(player, cmds);
    BOOST_REQUIRE_EQUAL(player, 0u);
    BOOST_REQUIRE(cmds.checksum == recordedCmds.checksum);
    BOOST_REQUIRE_EQUAL(cmds.gcs.size(), recordedCmds.gcs.size());
    BOOST_REQUIRE(dynamic_cast<gc::SetFlag*>(cmds.gcs[0].get()));
    BOOST_REQUIRE(dynamic_cast<gc::SetCoinsAllowed*>(cmds.gcs[1].get()));

    BOOST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_REQUIRE_EQUAL(gf, 2u);
    BOOST_REQUIRE_EQUAL(loadReplay.ReadRCType(), ReplayCommand::Chat);
    loadReplay.ReadChatCommand(player, dst, txt);
    BOOST_REQUIRE_EQUAL(player, 2);
    BOOST_REQUIRE_EQUAL(dst, 3);
    BOOST_REQUIRE_EQUAL(txt, "Hello4");

    BOOST_REQUIRE(!loadReplay.ReadGF(&gf));
    BOOST_REQUIRE_EQUAL(gf, 0xFFFFFFFF);
}
} // namespace

BOOST_AUTO_TEST_SUITE(Serialization)

BOOST_AUTO_TEST_CASE(Serializer)
{
    SerializedGameData sgd;
    // Test corner cases of var size
    sgd.PushVarSize(0);
    sgd.PushVarSize(0x7F);
    sgd.PushVarSize(0x80);
    sgd.PushVarSize(0x3FFF);
    sgd.PushVarSize(0x4000);
    sgd.PushVarSize(0x1FFFFF);
    sgd.PushVarSize(0xFFFFFFF);
    sgd.PushVarSize(0x10000000);
    sgd.PushVarSize(0xFFFFFFFF);
    BOOST_REQUIRE_EQUAL(sgd.PopVarSize(), 0u);
    BOOST_REQUIRE_EQUAL(sgd.PopVarSize(), 0x7Fu);
    BOOST_REQUIRE_EQUAL(sgd.PopVarSize(), 0x80u);
    BOOST_REQUIRE_EQUAL(sgd.PopVarSize(), 0x3FFFu);
    BOOST_REQUIRE_EQUAL(sgd.PopVarSize(), 0x4000u);
    BOOST_REQUIRE_EQUAL(sgd.PopVarSize(), 0x1FFFFFu);
    BOOST_REQUIRE_EQUAL(sgd.PopVarSize(), 0xFFFFFFFu);
    BOOST_REQUIRE_EQUAL(sgd.PopVarSize(), 0x10000000u);
    BOOST_REQUIRE_EQUAL(sgd.PopVarSize(), 0xFFFFFFFFu);
}

BOOST_FIXTURE_TEST_CASE(BaseSaveLoad, RandWorldFixture)
{
    MapPoint hqPos = world.GetPlayer(0).GetHQPos();
    MapPoint usualBldPos = world.MakeMapPoint(hqPos + Position(3, 0));
    auto* usualBld = static_cast<nobUsual*>(BuildingFactory::CreateBuilding(world, BLD_WOODCUTTER, usualBldPos, 0, NAT_VIKINGS));
    world.BuildRoad(0, false, world.GetNeighbour(hqPos, Direction::SOUTHEAST), std::vector<Direction>(3, Direction::EAST));
    usualBld->is_working = true;

    // Add 3 fires with first between the others to have a mixed event order in the same GF
    std::vector<MapPoint> firePositions;
    firePositions.push_back(world.MakeMapPoint(hqPos + Position(8, 0)));
    firePositions.push_back(world.MakeMapPoint(hqPos + Position(7, 0)));
    firePositions.push_back(world.MakeMapPoint(hqPos + Position(9, 0)));
    for(const MapPoint& pt : firePositions)
    {
        BOOST_REQUIRE(!world.GetNode(pt).obj);
        world.SetNO(pt, new noFire(pt, false));
    }

    for(unsigned i = 0; i < 100; i++)
        em.ExecuteNextGF();

    Savegame save;

    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        save.AddPlayer(world.GetPlayer(i));

    save.ggs = ggs;
    save.start_gf = em.GetCurrentGF();
    save.sgd.MakeSnapshot(game);

    TmpFile tmpFile;
    BOOST_REQUIRE(tmpFile.isValid());
    tmpFile.close();

    s25util::time64_t saveTime = s25util::Time::CurrentTime();
    BOOST_REQUIRE(save.Save(tmpFile.filePath, "MapTitle"));
    BOOST_REQUIRE_LE(save.GetSaveTime() - saveTime, 20); // 20s difference max
    const unsigned origObjNum = GameObject::GetNumObjs();
    const unsigned origObjIdNum = GameObject::GetObjIDCounter();

    for(int i = 0; i < 3; i++)
    {
        Savegame loadSave;
        BOOST_REQUIRE(loadSave.Load(tmpFile.filePath, i > 0, i > 1));
        BOOST_REQUIRE_EQUAL(loadSave.GetSaveTime(), save.GetSaveTime());
        BOOST_REQUIRE_EQUAL(loadSave.GetMapName(), "MapTitle");
        BOOST_REQUIRE_EQUAL(loadSave.GetPlayerNames().size(), 3u);
        BOOST_REQUIRE_EQUAL(loadSave.GetPlayerNames()[0], "Human");
        BOOST_REQUIRE_EQUAL(loadSave.GetPlayerNames()[1], "PlAI");
        BOOST_REQUIRE_EQUAL(loadSave.GetPlayerNames()[2], "PlAI2");
        BOOST_REQUIRE_EQUAL(loadSave.start_gf, em.GetCurrentGF());
        if(i == 0)
        {
            // Not loaded
            BOOST_REQUIRE_EQUAL(loadSave.GetNumPlayers(), 0u);
        } else
        {
            BOOST_REQUIRE_EQUAL(loadSave.GetNumPlayers(), 4u);
            for(unsigned j = 0; j < 4; j++)
            {
                const BasePlayerInfo& loadPlayer = loadSave.GetPlayer(j);
                const BasePlayerInfo& worldPlayer = world.GetPlayer(j);
                BOOST_REQUIRE_EQUAL(loadPlayer.ps, worldPlayer.ps);
                if(!loadPlayer.isUsed())
                    continue;
                BOOST_REQUIRE_EQUAL(loadPlayer.name, worldPlayer.name);
                if(!loadPlayer.isHuman())
                {
                    BOOST_REQUIRE_EQUAL(loadPlayer.aiInfo.type, worldPlayer.aiInfo.type);
                    BOOST_REQUIRE_EQUAL(loadPlayer.aiInfo.level, worldPlayer.aiInfo.level);
                }
            }
            BOOST_REQUIRE_EQUAL(loadSave.ggs.speed, ggs.speed);
        }
        if(i < 2)
            BOOST_REQUIRE_EQUAL(loadSave.sgd.GetLength(), 0u);
        else
        {
            std::vector<PlayerInfo> players;
            for(unsigned j = 0; j < 4; j++)
                players.push_back(PlayerInfo(loadSave.GetPlayer(j)));
            std::shared_ptr<Game> sharedGame(new Game(save.ggs, loadSave.start_gf, players));
            GameWorld& newWorld = sharedGame->world_;
            save.sgd.ReadSnapshot(sharedGame);
            auto& newEm = static_cast<TestEventManager&>(sharedGame->world_.GetEvMgr());

            BOOST_REQUIRE_EQUAL(newWorld.GetSize(), world.GetSize());
            BOOST_REQUIRE_EQUAL(newEm.GetCurrentGF(), em.GetCurrentGF());
            BOOST_REQUIRE_EQUAL(GameObject::GetNumObjs(), origObjNum);
            BOOST_REQUIRE_EQUAL(GameObject::GetObjIDCounter(), origObjIdNum);
            std::vector<const GameEvent*> worldEvs = em.GetEvents();
            std::vector<const GameEvent*> loadEvs = newEm.GetEvents();
            BOOST_REQUIRE_EQUAL(worldEvs.size(), loadEvs.size());
            for(unsigned j = 0; j < worldEvs.size(); ++j)
            {
                BOOST_REQUIRE_EQUAL(worldEvs[j]->GetInstanceId(), loadEvs[j]->GetInstanceId());
                BOOST_REQUIRE_EQUAL(worldEvs[j]->startGF, loadEvs[j]->startGF);
                BOOST_REQUIRE_EQUAL(worldEvs[j]->length, loadEvs[j]->length);
                BOOST_REQUIRE_EQUAL(worldEvs[j]->id, loadEvs[j]->id);
            }
            RTTR_FOREACH_PT(MapPoint, world.GetSize())
            {
                const MapNode& worldNode = world.GetNode(pt);
                const MapNode& loadNode = newWorld.GetNode(pt);
                RTTR_REQUIRE_EQUAL_COLLECTIONS(loadNode.roads, worldNode.roads);
                BOOST_REQUIRE_EQUAL(loadNode.altitude, worldNode.altitude);
                BOOST_REQUIRE_EQUAL(loadNode.shadow, worldNode.shadow);
                BOOST_REQUIRE_EQUAL(loadNode.t1, worldNode.t1);
                BOOST_REQUIRE_EQUAL(loadNode.t2, worldNode.t2);
                BOOST_REQUIRE(loadNode.resources == worldNode.resources);
                BOOST_REQUIRE_EQUAL(loadNode.reserved, worldNode.reserved);
                BOOST_REQUIRE_EQUAL(loadNode.owner, worldNode.owner);
                BOOST_REQUIRE_EQUAL(loadNode.bq, worldNode.bq);
                BOOST_REQUIRE_EQUAL(loadNode.seaId, worldNode.seaId);
                BOOST_REQUIRE_EQUAL(loadNode.harborId, worldNode.harborId);
                BOOST_REQUIRE_EQUAL(loadNode.obj != nullptr, worldNode.obj != nullptr);
            }
            const nobUsual* newUsual = newWorld.GetSpecObj<nobUsual>(usualBldPos);
            BOOST_REQUIRE(newUsual);
            BOOST_REQUIRE_EQUAL(newUsual->is_working, usualBld->is_working);
            BOOST_REQUIRE_EQUAL(newUsual->HasWorker(), usualBld->HasWorker());
            BOOST_REQUIRE_EQUAL(newUsual->GetProductivity(), usualBld->GetProductivity());

            SerializedGameData loadedSgd;
            loadedSgd.MakeSnapshot(sharedGame);
            BOOST_REQUIRE_EQUAL_COLLECTIONS(loadedSgd.GetData(), loadedSgd.GetData() + loadedSgd.GetLength(), save.sgd.GetData(),
                                            save.sgd.GetData() + save.sgd.GetLength());
        }
    }
}

BOOST_AUTO_TEST_CASE(ReplayWithMap)
{
    MapInfo map;
    map.type = MAPTYPE_OLDMAP;
    map.title = "MapTitle";
    map.filepath = "Map.swd";
    map.luaFilepath = "Map.lua";
    map.mapData.data = std::vector<char>(42, 0x42);
    map.mapData.length = 50;
    map.luaData.data = std::vector<char>(21, 0x21);
    map.luaData.length = 40;
    std::vector<PlayerInfo> players(4);
    players[0].ps = PS_OCCUPIED;
    players[0].name = "Human";
    players[1].ps = PS_AI;
    players[1].aiInfo = AI::Info(AI::DEFAULT, AI::MEDIUM);
    players[1].name = "PlAI";
    players[2].ps = PS_LOCKED;
    players[3].ps = PS_AI;
    players[3].aiInfo = AI::Info(AI::DEFAULT, AI::EASY);
    players[3].name = "PlAI2";

    Replay replay;
    BOOST_REQUIRE(!replay.IsValid());
    BOOST_REQUIRE(!replay.IsRecording());
    BOOST_REQUIRE(!replay.IsReplaying());
    for(const BasePlayerInfo& player : players)
        replay.AddPlayer(player);
    replay.ggs.speed = GS_VERYFAST;
    replay.random_init = 815;

    TmpFile tmpFile;
    BOOST_REQUIRE(tmpFile.isValid());
    tmpFile.close();
    // No overwrite
    BOOST_REQUIRE(!replay.StartRecording(tmpFile.filePath, map));
    BOOST_REQUIRE(!replay.IsValid());
    BOOST_REQUIRE(!replay.IsRecording());
    BOOST_REQUIRE(!replay.IsReplaying());

    bfs::remove(tmpFile.filePath);
    s25util::time64_t saveTime = s25util::Time::CurrentTime();
    BOOST_REQUIRE(replay.StartRecording(tmpFile.filePath, map));
    BOOST_REQUIRE_LE(replay.GetSaveTime() - saveTime, 20); // 20s difference max
    BOOST_REQUIRE(replay.IsValid());
    BOOST_REQUIRE(replay.IsRecording());
    BOOST_REQUIRE(!replay.IsReplaying());

    GlobalGameSettings ggs;
    Game game(ggs, 0u, players);
    PlayerGameCommands cmds = GetTestCommands().create(game).result;
    AddReplayCmds(replay, cmds);
    replay.StopRecording();
    BOOST_REQUIRE(!replay.IsValid());
    BOOST_REQUIRE(!replay.IsRecording());

    for(int i = 0; i < 2; i++)
    {
        Replay loadReplay;
        BOOST_REQUIRE(loadReplay.LoadHeader(tmpFile.filePath, i > 0));
        BOOST_REQUIRE_EQUAL(loadReplay.GetSaveTime(), replay.GetSaveTime());
        BOOST_REQUIRE_EQUAL(loadReplay.GetMapName(), "MapTitle");
        BOOST_REQUIRE_EQUAL(loadReplay.GetPlayerNames().size(), 3u);
        BOOST_REQUIRE_EQUAL(loadReplay.GetPlayerNames()[0], "Human");
        BOOST_REQUIRE_EQUAL(loadReplay.GetPlayerNames()[1], "PlAI");
        BOOST_REQUIRE_EQUAL(loadReplay.GetPlayerNames()[2], "PlAI2");
        BOOST_REQUIRE_EQUAL(loadReplay.GetLastGF(), 5u);
        if(i == 0)
        {
            // Not loaded
            BOOST_REQUIRE_EQUAL(loadReplay.GetNumPlayers(), 0u);
            continue;
        }
        BOOST_REQUIRE_EQUAL(loadReplay.GetNumPlayers(), 4u);
        for(unsigned j = 0; j < 4; j++)
        {
            const BasePlayerInfo& loadPlayer = loadReplay.GetPlayer(j);
            const BasePlayerInfo& worldPlayer = players[j];
            BOOST_REQUIRE_EQUAL(loadPlayer.ps, worldPlayer.ps);
            if(!loadPlayer.isUsed())
                continue;
            BOOST_REQUIRE_EQUAL(loadPlayer.name, worldPlayer.name);
            if(!loadPlayer.isHuman())
            {
                BOOST_REQUIRE_EQUAL(loadPlayer.aiInfo.type, worldPlayer.aiInfo.type);
                BOOST_REQUIRE_EQUAL(loadPlayer.aiInfo.level, worldPlayer.aiInfo.level);
            }
        }
        BOOST_REQUIRE_EQUAL(loadReplay.ggs.speed, replay.ggs.speed);
        MapInfo newMap;
        BOOST_REQUIRE(loadReplay.LoadGameData(newMap));
        BOOST_REQUIRE_EQUAL(loadReplay.random_init, replay.random_init);
        BOOST_REQUIRE_EQUAL(newMap.type, map.type);
        BOOST_REQUIRE_EQUAL(newMap.title, map.title);
        BOOST_REQUIRE_EQUAL(newMap.filepath, map.filepath);
        RTTR_REQUIRE_EQUAL_COLLECTIONS(newMap.mapData.data, map.mapData.data);
        RTTR_REQUIRE_EQUAL_COLLECTIONS(newMap.luaData.data, map.luaData.data);
        BOOST_REQUIRE(loadReplay.IsReplaying());

        CheckReplayCmds(loadReplay, cmds);
    }
}

BOOST_FIXTURE_TEST_CASE(ReplayWithSavegame, RandWorldFixture)
{
    MapInfo map;
    map.type = MAPTYPE_SAVEGAME;
    map.title = "MapTitle";
    map.filepath = "Map.swd";
    map.luaFilepath = "Map.lua";
    map.savegame = std::make_unique<Savegame>();
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        map.savegame->AddPlayer(world.GetPlayer(i));
    // We can change players
    std::vector<BasePlayerInfo> players(4);
    players[0].ps = PS_AI;
    players[0].aiInfo = AI::Info(AI::DEFAULT, AI::MEDIUM);
    players[0].name = "PlAI";
    players[1].ps = PS_OCCUPIED;
    players[1].name = "Human";
    players[2].ps = PS_LOCKED;
    players[3].ps = PS_AI;
    players[3].aiInfo = AI::Info(AI::DEFAULT, AI::EASY);
    players[3].name = "PlAI2";

    map.savegame->ggs = ggs;
    map.savegame->start_gf = em.GetCurrentGF();
    map.savegame->sgd.MakeSnapshot(game);

    Replay replay;
    BOOST_REQUIRE(!replay.IsValid());
    BOOST_REQUIRE(!replay.IsRecording());
    BOOST_REQUIRE(!replay.IsReplaying());
    for(const BasePlayerInfo& player : players)
        replay.AddPlayer(player);
    replay.ggs.speed = GS_VERYFAST;
    replay.random_init = 815;

    TmpFile tmpFile;
    BOOST_REQUIRE(tmpFile.isValid());
    tmpFile.close();
    bfs::remove(tmpFile.filePath);
    s25util::time64_t saveTime = s25util::Time::CurrentTime();
    BOOST_REQUIRE(replay.StartRecording(tmpFile.filePath, map));
    BOOST_REQUIRE_LE(replay.GetSaveTime() - saveTime, 20); // 20s difference max
    BOOST_REQUIRE(replay.IsValid());
    BOOST_REQUIRE(replay.IsRecording());
    BOOST_REQUIRE(!replay.IsReplaying());

    PlayerGameCommands cmds = GetTestCommands().create(*game).result;
    AddReplayCmds(replay, cmds);
    replay.StopRecording();
    BOOST_REQUIRE(!replay.IsValid());
    BOOST_REQUIRE(!replay.IsRecording());

    for(int i = 0; i < 2; i++)
    {
        Replay loadReplay;
        BOOST_REQUIRE(loadReplay.LoadHeader(tmpFile.filePath, i > 0));
        BOOST_REQUIRE_EQUAL(loadReplay.GetSaveTime(), replay.GetSaveTime());
        BOOST_REQUIRE_EQUAL(loadReplay.GetMapName(), "MapTitle");
        BOOST_REQUIRE_EQUAL(loadReplay.GetPlayerNames().size(), 3u);
        BOOST_REQUIRE_EQUAL(loadReplay.GetPlayerNames()[0], "PlAI");
        BOOST_REQUIRE_EQUAL(loadReplay.GetPlayerNames()[1], "Human");
        BOOST_REQUIRE_EQUAL(loadReplay.GetPlayerNames()[2], "PlAI2");
        BOOST_REQUIRE_EQUAL(loadReplay.GetLastGF(), 5u);
        if(i == 0)
        {
            // Not loaded
            BOOST_REQUIRE_EQUAL(loadReplay.GetNumPlayers(), 0u);
            continue;
        }
        BOOST_REQUIRE_EQUAL(loadReplay.GetNumPlayers(), 4u);
        for(unsigned j = 0; j < 4; j++)
        {
            const BasePlayerInfo& loadPlayer = loadReplay.GetPlayer(j);
            const BasePlayerInfo& worldPlayer = players[j];
            BOOST_REQUIRE_EQUAL(loadPlayer.ps, worldPlayer.ps);
            if(!loadPlayer.isUsed())
                continue;
            BOOST_REQUIRE_EQUAL(loadPlayer.name, worldPlayer.name);
            if(!loadPlayer.isHuman())
            {
                BOOST_REQUIRE_EQUAL(loadPlayer.aiInfo.type, worldPlayer.aiInfo.type);
                BOOST_REQUIRE_EQUAL(loadPlayer.aiInfo.level, worldPlayer.aiInfo.level);
            }
        }
        BOOST_REQUIRE_EQUAL(loadReplay.ggs.speed, replay.ggs.speed);
        MapInfo newMap;
        BOOST_REQUIRE(loadReplay.LoadGameData(newMap));
        BOOST_REQUIRE_EQUAL(loadReplay.random_init, replay.random_init);
        BOOST_REQUIRE_EQUAL(newMap.type, map.type);
        BOOST_REQUIRE_EQUAL(newMap.title, map.title);
        BOOST_REQUIRE_EQUAL(newMap.filepath, map.filepath);

        BOOST_REQUIRE_EQUAL_COLLECTIONS(newMap.savegame->sgd.GetData(),                                    //-V807
                                        newMap.savegame->sgd.GetData() + newMap.savegame->sgd.GetLength(), //-V807
                                        map.savegame->sgd.GetData(), map.savegame->sgd.GetData() + map.savegame->sgd.GetLength());

        CheckReplayCmds(loadReplay, cmds);
    }
}

BOOST_AUTO_TEST_SUITE_END()
