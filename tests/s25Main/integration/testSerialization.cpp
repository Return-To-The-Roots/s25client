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
#include "Ware.h"
#include "addons/Addon.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "factories/GameCommandFactory.h"
#include "figures/nofHunter.h"
#include "helpers/format.hpp"
#include "network/GameMessage_Chat.h"
#include "network/PlayerGameCommands.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/MockLocalGameState.h"
#include "worldFixtures/WorldFixture.h"
#include "world/MapLoader.h"
#include "nodeObjs/noAnimal.h"
#include "nodeObjs/noFire.h"
#include "nodeObjs/noFlag.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameTypes/MapInfo.h"
#include "s25util/tmpFile.h"
#include <rttr/test/random.hpp>
#include <rttr/test/testHelpers.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(Resource)
BOOST_TEST_DONT_PRINT_LOG_VALUE(AddonId)
BOOST_TEST_DONT_PRINT_LOG_VALUE(nofBuildingWorker::State)

namespace boost { namespace test_tools { namespace tt_detail {
    template<>
    struct print_log_value<ReplayCommand>
    {
        void operator()(std::ostream& os, ReplayCommand const& rc) { os << static_cast<unsigned>(rc); }
    };
}}} // namespace boost::test_tools::tt_detail
// LCOV_EXCL_STOP

namespace {
using EmptyWorldFixture1P = WorldFixture<CreateEmptyWorld, 1>;
struct RandWorldFixture : public WorldFixture<CreateEmptyWorld, 4>
{
    RandWorldFixture()
    {
        RTTR_FOREACH_PT(MapPoint, world.GetSize())
        {
            MapNode& worldNode = world.GetNodeWriteable(pt);
            worldNode.altitude = rttr::test::randomValue(10, 20);
            worldNode.shadow = rttr::test::randomValue(0, 20);
            worldNode.resources = Resource(rttr::test::randomValue<uint8_t>());
            worldNode.reserved = rttr::test::randomValue(0, 1) == 0;
            worldNode.seaId = rttr::test::randomValue(0, 20);
            worldNode.harborId = rttr::test::randomValue(0, 20);
        }
        world.InitAfterLoad();
        world.GetPlayer(0).name = "Human";
        world.GetPlayer(1).ps = PlayerState::AI; //-V807
        world.GetPlayer(1).aiInfo = AI::Info(AI::Type::Default, AI::Level::Medium);
        world.GetPlayer(1).name = "PlAI";
        world.GetPlayer(2).ps = PlayerState::Locked;
        world.GetPlayer(3).ps = PlayerState::AI; //-V807
        world.GetPlayer(3).aiInfo = AI::Info(AI::Type::Default, AI::Level::Easy);
        world.GetPlayer(3).name = "PlAI2";

        ggs.speed = GameSpeed::VeryFast;
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
    replay.AddChatCommand(1, 2, ChatDestination::Enemies, "Hello");
    replay.AddChatCommand(1, 3, ChatDestination::All, "Hello2");
    replay.AddChatCommand(2, 2, ChatDestination::Allies, "Hello3");

    replay.AddGameCommand(2, 0, cmds);
    replay.AddChatCommand(2, 2, ChatDestination::Enemies, "Hello4");
    replay.UpdateLastGF(5);
}

void CheckReplayCmds(Replay& loadReplay, const PlayerGameCommands& recordedCmds)
{
    BOOST_TEST_REQUIRE(loadReplay.IsReplaying());
    unsigned gf;
    BOOST_TEST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_TEST_REQUIRE(gf == 1u);
    BOOST_TEST_REQUIRE(loadReplay.ReadRCType() == ReplayCommand::Chat);
    uint8_t player, dst;
    std::string txt;
    loadReplay.ReadChatCommand(player, dst, txt);
    BOOST_TEST_REQUIRE(player == 2);
    BOOST_TEST_REQUIRE(dst == 3);
    BOOST_TEST_REQUIRE(txt == "Hello");

    BOOST_TEST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_TEST_REQUIRE(gf == 1u);
    BOOST_TEST_REQUIRE(loadReplay.ReadRCType() == ReplayCommand::Chat);
    loadReplay.ReadChatCommand(player, dst, txt);
    BOOST_TEST_REQUIRE(player == 3);
    BOOST_TEST_REQUIRE(dst == 1);
    BOOST_TEST_REQUIRE(txt == "Hello2");

    BOOST_TEST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_TEST_REQUIRE(gf == 2u);
    BOOST_TEST_REQUIRE(loadReplay.ReadRCType() == ReplayCommand::Chat);
    loadReplay.ReadChatCommand(player, dst, txt);
    BOOST_TEST_REQUIRE(player == 2);
    BOOST_TEST_REQUIRE(dst == 2);
    BOOST_TEST_REQUIRE(txt == "Hello3");

    BOOST_TEST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_TEST_REQUIRE(gf == 2u);
    BOOST_TEST_REQUIRE(loadReplay.ReadRCType() == ReplayCommand::Game);
    PlayerGameCommands cmds;
    loadReplay.ReadGameCommand(player, cmds);
    BOOST_TEST_REQUIRE(player == 0u);
    BOOST_TEST_REQUIRE(cmds.checksum == recordedCmds.checksum);
    BOOST_TEST_REQUIRE(cmds.gcs.size() == recordedCmds.gcs.size());
    BOOST_TEST_REQUIRE(dynamic_cast<gc::SetFlag*>(cmds.gcs[0].get()));
    BOOST_TEST_REQUIRE(dynamic_cast<gc::SetCoinsAllowed*>(cmds.gcs[1].get()));

    BOOST_TEST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_TEST_REQUIRE(gf == 2u);
    BOOST_TEST_REQUIRE(loadReplay.ReadRCType() == ReplayCommand::Chat);
    loadReplay.ReadChatCommand(player, dst, txt);
    BOOST_TEST_REQUIRE(player == 2);
    BOOST_TEST_REQUIRE(dst == 3);
    BOOST_TEST_REQUIRE(txt == "Hello4");

    BOOST_TEST_REQUIRE(!loadReplay.ReadGF(&gf));
    BOOST_TEST_REQUIRE(gf == 0xFFFFFFFF);
}
} // namespace

BOOST_AUTO_TEST_SUITE(Serialization)

BOOST_AUTO_TEST_CASE(SerializeGGS)
{
    GlobalGameSettings ggs;
    ggs.speed = rttr::test::randomEnum<GameSpeed>();
    ggs.objective = rttr::test::randomEnum<GameObjective>();
    ggs.startWares = rttr::test::randomEnum<StartWares>();
    ggs.lockedTeams = rttr::test::randomBool();
    ggs.exploration = rttr::test::randomEnum<Exploration>();
    ggs.teamView = rttr::test::randomBool();
    ggs.randomStartPosition = rttr::test::randomBool();
    for(unsigned i = 0; i < ggs.getNumAddons(); i++)
    {
        const auto* addon = ggs.getAddon(i);
        BOOST_TEST_REQUIRE(addon);
        ggs.setSelection(addon->getId(), rttr::test::randomValue(0u, addon->getNumOptions() - 1));
    }
    ::Serializer ser;
    ggs.Serialize(ser);
    ::Serializer loader(ser.GetData(), ser.GetLength());
    GlobalGameSettings ggsLoaded;
    ggsLoaded.Deserialize(loader);
    BOOST_TEST(ggs.speed == ggsLoaded.speed);
    BOOST_TEST(ggs.objective == ggsLoaded.objective);
    BOOST_TEST(ggs.startWares == ggsLoaded.startWares);
    BOOST_TEST(ggs.lockedTeams == ggsLoaded.lockedTeams);
    BOOST_TEST(ggs.exploration == ggsLoaded.exploration);
    BOOST_TEST(ggs.teamView == ggsLoaded.teamView);
    BOOST_TEST(ggs.randomStartPosition == ggsLoaded.randomStartPosition);
    for(unsigned i = 0; i < ggs.getNumAddons(); i++)
    {
        const auto* addon = ggs.getAddon(i);
        const auto* addonLoaded = ggsLoaded.getAddon(i);
        BOOST_TEST_REQUIRE(addonLoaded);
        BOOST_TEST_REQUIRE(addon->getId() == addonLoaded->getId());
        BOOST_TEST(ggs.getSelection(addon->getId()) == ggsLoaded.getSelection(addon->getId()));
    }
}

BOOST_FIXTURE_TEST_CASE(BaseSaveLoad, RandWorldFixture)
{
    MockLocalGameState lgsGame;
    const std::string luaScript = helpers::format(
      "-- Hello World\n function getRequiredLuaVersion()\n return %1%\n end", LuaInterfaceGameBase::GetVersion());
    {
        MapLoader loader(world);
        TmpFile validLuaFile(".lua");
        validLuaFile.getStream() << luaScript;
        validLuaFile.close();

        BOOST_TEST_REQUIRE(loader.LoadLuaScript(*game, lgsGame, validLuaFile.filePath));
        BOOST_TEST(world.HasLua());
    }

    const MapPoint hqPos = world.GetPlayer(0).GetHQPos();
    auto* hq = world.GetSpecObj<nobBaseWarehouse>(hqPos);
    auto* hqFlag = hq->GetFlag();
    const MapPoint usualBldPos = world.MakeMapPoint(hqPos + Position(3, 0));
    auto* usualBld = static_cast<nobUsual*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Bakery, usualBldPos, 0, Nation::Vikings));
    world.BuildRoad(0, false, hqFlag->GetPos(), std::vector<Direction>(3, Direction::East));
    usualBld->is_working = true;

    // Add 3 fires with first between the others to have a mixed event order in the same GF
    for(const auto& offset : {Position(8, 0), Position(7, 0), Position(9, 0)})
    {
        const auto pt = world.MakeMapPoint(hqPos + offset);
        BOOST_TEST_REQUIRE(!world.GetNode(pt).obj);
        world.SetNO(pt, new noFire(pt, false));
    }

    for(unsigned i = 0; i < 100; i++)
        em.ExecuteNextGF();

    // Do this after running GFs to keep the state
    // Add ware to flag
    auto ware = std::make_unique<Ware>(GoodType::Flour, usualBld, hqFlag);
    ware->WaitAtFlag(hqFlag);
    ware->RecalcRoute();
    hqFlag->AddWare(std::move(ware));
    // Add a ware waiting in a warehouse. See https://github.com/Return-To-The-Roots/s25client/issues/1293
    ware = std::make_unique<Ware>(GoodType::Flour, usualBld, hq);
    hq->AddWaitingWare(std::move(ware));

    Savegame save;

    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        save.AddPlayer(world.GetPlayer(i));

    save.ggs = ggs;
    save.start_gf = em.GetCurrentGF();
    save.sgd.MakeSnapshot(*game);

    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(tmpFile.isValid());
    tmpFile.close();

    s25util::time64_t saveTime = s25util::Time::CurrentTime();
    BOOST_TEST_REQUIRE(save.Save(tmpFile.filePath, "MapTitle"));
    BOOST_TEST_REQUIRE(save.GetSaveTime() - saveTime <= 20); // 20s difference max
    const unsigned origObjNum = GameObject::GetNumObjs();
    const unsigned origObjIdNum = GameObject::GetObjIDCounter();

    for(const auto what : {SaveGameDataToLoad::Header, SaveGameDataToLoad::HeaderAndSettings, SaveGameDataToLoad::All})
    {
        Savegame loadSave;
        BOOST_TEST_REQUIRE(loadSave.Load(tmpFile.filePath, what));
        BOOST_TEST_REQUIRE(loadSave.GetSaveTime() == save.GetSaveTime());
        BOOST_TEST_REQUIRE(loadSave.GetMapName() == "MapTitle");
        BOOST_TEST_REQUIRE(loadSave.GetPlayerNames().size() == 3u);
        BOOST_TEST_REQUIRE(loadSave.GetPlayerNames()[0] == "Human");
        BOOST_TEST_REQUIRE(loadSave.GetPlayerNames()[1] == "PlAI");
        BOOST_TEST_REQUIRE(loadSave.GetPlayerNames()[2] == "PlAI2");
        BOOST_TEST_REQUIRE(loadSave.start_gf == em.GetCurrentGF());
        // Players are loaded with the settings
        if(what == SaveGameDataToLoad::Header)
        {
            BOOST_TEST_REQUIRE(loadSave.GetNumPlayers() == 0u);
        } else
        {
            BOOST_TEST_REQUIRE(loadSave.GetNumPlayers() == 4u);
            for(unsigned j = 0; j < 4; j++)
            {
                const BasePlayerInfo& loadPlayer = loadSave.GetPlayer(j);
                const BasePlayerInfo& worldPlayer = world.GetPlayer(j);
                BOOST_TEST_REQUIRE(loadPlayer.ps == worldPlayer.ps);
                if(!loadPlayer.isUsed())
                    continue;
                BOOST_TEST_REQUIRE(loadPlayer.name == worldPlayer.name);
                if(!loadPlayer.isHuman())
                {
                    BOOST_TEST_REQUIRE(loadPlayer.aiInfo.type == worldPlayer.aiInfo.type);
                    BOOST_TEST_REQUIRE(loadPlayer.aiInfo.level == worldPlayer.aiInfo.level);
                }
            }
            BOOST_TEST_REQUIRE(loadSave.ggs.speed == ggs.speed);
        }
        if(what != SaveGameDataToLoad::All)
            BOOST_TEST_REQUIRE(loadSave.sgd.GetLength() == 0u);
        else
        {
            std::vector<PlayerInfo> players;
            for(unsigned j = 0; j < 4; j++)
                players.push_back(PlayerInfo(loadSave.GetPlayer(j)));
            Game game(save.ggs, loadSave.start_gf, players);
            const World& newWorld = game.world_;
            MockLocalGameState localGameState;
            save.sgd.ReadSnapshot(game, localGameState);
            auto& newEm = static_cast<TestEventManager&>(game.world_.GetEvMgr());

            BOOST_TEST_REQUIRE(newWorld.GetSize() == world.GetSize());
            BOOST_TEST_REQUIRE(newEm.GetCurrentGF() == em.GetCurrentGF());
            BOOST_TEST_REQUIRE(GameObject::GetNumObjs() == origObjNum);
            BOOST_TEST_REQUIRE(GameObject::GetObjIDCounter() == origObjIdNum);
            std::vector<const GameEvent*> worldEvs = em.GetEvents();
            std::vector<const GameEvent*> loadEvs = newEm.GetEvents();
            BOOST_TEST_REQUIRE(worldEvs.size() == loadEvs.size());
            for(unsigned j = 0; j < worldEvs.size(); ++j)
            {
                BOOST_TEST_REQUIRE(worldEvs[j]->GetInstanceId() == loadEvs[j]->GetInstanceId());
                BOOST_TEST_REQUIRE(worldEvs[j]->startGF == loadEvs[j]->startGF);
                BOOST_TEST_REQUIRE(worldEvs[j]->length == loadEvs[j]->length);
                BOOST_TEST_REQUIRE(worldEvs[j]->id == loadEvs[j]->id);
            }
            RTTR_FOREACH_PT(MapPoint, world.GetSize())
            {
                const MapNode& worldNode = world.GetNode(pt);
                const MapNode& loadNode = newWorld.GetNode(pt);
                BOOST_TEST_REQUIRE(loadNode.roads == worldNode.roads, boost::test_tools::per_element());
                BOOST_TEST_REQUIRE(loadNode.altitude == worldNode.altitude);
                BOOST_TEST_REQUIRE(loadNode.shadow == worldNode.shadow);
                BOOST_TEST_REQUIRE(loadNode.t1 == worldNode.t1);
                BOOST_TEST_REQUIRE(loadNode.t2 == worldNode.t2);
                BOOST_TEST_REQUIRE(loadNode.resources == worldNode.resources);
                BOOST_TEST_REQUIRE(loadNode.reserved == worldNode.reserved);
                BOOST_TEST_REQUIRE(loadNode.owner == worldNode.owner);
                BOOST_TEST_REQUIRE(loadNode.bq == worldNode.bq);
                BOOST_TEST_REQUIRE(loadNode.seaId == worldNode.seaId);
                BOOST_TEST_REQUIRE(loadNode.harborId == worldNode.harborId);
                BOOST_TEST_REQUIRE((loadNode.obj != nullptr) == (worldNode.obj != nullptr));
            }
            const auto* newUsual = newWorld.GetSpecObj<nobUsual>(usualBldPos);
            BOOST_TEST_REQUIRE(newUsual);
            BOOST_TEST_REQUIRE(newUsual->is_working == usualBld->is_working);
            BOOST_TEST_REQUIRE(newUsual->HasWorker() == usualBld->HasWorker());
            BOOST_TEST_REQUIRE(newUsual->GetProductivity() == usualBld->GetProductivity());

            hq = world.GetSpecObj<nobBaseWarehouse>(hqPos);
            BOOST_TEST_REQUIRE(hq);
            hqFlag = hq->GetFlag();
            BOOST_TEST_REQUIRE(hqFlag);
            BOOST_TEST(hqFlag->GetNumWares() == 1u);

            BOOST_TEST_REQUIRE(world.HasLua());
            BOOST_TEST(world.GetLua().getScript() == luaScript);

            SerializedGameData loadedSgd;
            loadedSgd.MakeSnapshot(game);
            BOOST_REQUIRE_EQUAL_COLLECTIONS(loadedSgd.GetData(), loadedSgd.GetData() + loadedSgd.GetLength(),
                                            save.sgd.GetData(), save.sgd.GetData() + save.sgd.GetLength());
        }
    }
}

struct ReplayMapFixture
{
    MapInfo map;
    std::vector<PlayerInfo> players;

    ReplayMapFixture() : players(4)
    {
        map.type = MapType::OldMap;
        map.title = "MapTitle";
        map.filepath = "Map.swd";
        map.luaFilepath = "Map.lua";
        map.mapData.data = std::vector<char>(rttr::test::randomValue(30, 60), rttr::test::randomValue<int8_t>());
        map.mapData.uncompressedLength = rttr::test::randomValue(20, 50);
        map.luaData.data = std::vector<char>(rttr::test::randomValue(30, 60), rttr::test::randomValue<int8_t>());
        map.luaData.uncompressedLength = rttr::test::randomValue(20, 50);

        players[0].ps = PlayerState::Occupied;
        players[0].name = "Human";
        players[1].ps = PlayerState::AI;
        players[1].aiInfo = AI::Info(rttr::test::randomEnum<AI::Type>(), rttr::test::randomEnum<AI::Level>());
        players[1].name = "PlAI";
        players[2].ps = PlayerState::Locked;
        players[3].ps = PlayerState::AI;
        players[3].aiInfo = AI::Info(rttr::test::randomEnum<AI::Type>(), rttr::test::randomEnum<AI::Level>());
        players[3].name = "PlAI2";
    }
};

BOOST_FIXTURE_TEST_CASE(ReplayWithMap, ReplayMapFixture)
{
    Replay replay;
    BOOST_TEST_REQUIRE(!replay.IsValid());
    BOOST_TEST_REQUIRE(!replay.IsRecording());
    BOOST_TEST_REQUIRE(!replay.IsReplaying());
    for(const BasePlayerInfo& player : players)
        replay.AddPlayer(player);
    replay.ggs.speed = GameSpeed::VeryFast;
    replay.random_init = 815;

    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(tmpFile.isValid());
    tmpFile.close();
    // No overwrite
    BOOST_TEST_REQUIRE(!replay.StartRecording(tmpFile.filePath, map));
    BOOST_TEST_REQUIRE(!replay.IsValid());
    BOOST_TEST_REQUIRE(!replay.IsRecording());
    BOOST_TEST_REQUIRE(!replay.IsReplaying());

    bfs::remove(tmpFile.filePath);
    s25util::time64_t saveTime = s25util::Time::CurrentTime();
    BOOST_TEST_REQUIRE(replay.StartRecording(tmpFile.filePath, map));
    BOOST_TEST(replay.GetSaveTime() - saveTime <= 20); // 20s difference max
    BOOST_TEST_REQUIRE(replay.IsValid());
    BOOST_TEST_REQUIRE(replay.IsRecording());
    BOOST_TEST_REQUIRE(!replay.IsReplaying());

    GlobalGameSettings ggs = replay.ggs;
    Game game(ggs, 0u, players);
    PlayerGameCommands cmds = GetTestCommands().create(game).result;
    AddReplayCmds(replay, cmds);
    BOOST_TEST(replay.GetLastGF() == 5u);
    BOOST_TEST_REQUIRE(replay.StopRecording());
    BOOST_TEST_REQUIRE(!replay.IsValid());
    BOOST_TEST_REQUIRE(!replay.IsRecording());

    for(const bool loadSettings : {false, true})
    {
        Replay loadReplay;
        BOOST_TEST_REQUIRE(loadReplay.LoadHeader(tmpFile.filePath));
        BOOST_TEST(loadReplay.GetSaveTime() == replay.GetSaveTime());
        BOOST_TEST(loadReplay.GetMapName() == "MapTitle");
        BOOST_TEST_REQUIRE(loadReplay.GetPlayerNames().size() == 3u);
        BOOST_TEST(loadReplay.GetPlayerNames()[0] == "Human");
        BOOST_TEST(loadReplay.GetPlayerNames()[1] == "PlAI");
        BOOST_TEST(loadReplay.GetPlayerNames()[2] == "PlAI2");
        BOOST_TEST_REQUIRE(loadReplay.GetLastGF() == 5u);
        if(!loadSettings)
        {
            // Not loaded
            BOOST_TEST(loadReplay.GetNumPlayers() == 0u);
            continue;
        }
        MapInfo newMap;
        BOOST_TEST_REQUIRE(loadReplay.LoadGameData(newMap));
        BOOST_TEST_REQUIRE(loadReplay.GetNumPlayers() == 4u);
        for(unsigned j = 0; j < 4; j++)
        {
            const BasePlayerInfo& loadPlayer = loadReplay.GetPlayer(j);
            const BasePlayerInfo& worldPlayer = players[j];
            BOOST_TEST_REQUIRE(loadPlayer.ps == worldPlayer.ps);
            if(!loadPlayer.isUsed())
                continue;
            BOOST_TEST_REQUIRE(loadPlayer.name == worldPlayer.name);
            if(!loadPlayer.isHuman())
            {
                BOOST_TEST_REQUIRE(loadPlayer.aiInfo.type == worldPlayer.aiInfo.type);
                BOOST_TEST_REQUIRE(loadPlayer.aiInfo.level == worldPlayer.aiInfo.level);
            }
        }
        BOOST_TEST(loadReplay.ggs.speed == replay.ggs.speed);
        BOOST_TEST(loadReplay.random_init == replay.random_init);
        BOOST_TEST(newMap.type == map.type);
        BOOST_TEST(newMap.title == map.title);
        BOOST_TEST(newMap.filepath == map.filepath);
        BOOST_TEST(newMap.mapData.data == map.mapData.data, boost::test_tools::per_element());
        BOOST_TEST(newMap.luaData.data == map.luaData.data, boost::test_tools::per_element());
        BOOST_TEST_REQUIRE(loadReplay.IsReplaying());

        CheckReplayCmds(loadReplay, cmds);
    }
}

BOOST_FIXTURE_TEST_CASE(BrokenReplayWithMap, ReplayMapFixture)
{
    GlobalGameSettings ggs;
    ggs.speed = GameSpeed::VeryFast;
    Game game(ggs, 0u, players);
    const PlayerGameCommands cmds = GetTestCommands().create(game).result;
    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(tmpFile.isValid());
    tmpFile.close();
    bfs::remove(tmpFile.filePath);

    const auto randomInit = rttr::test::randomValue<unsigned>();
    s25util::time64_t saveTime;
    {
        Replay replay;
        for(const BasePlayerInfo& player : players)
            replay.AddPlayer(player);
        replay.ggs.speed = GameSpeed::VeryFast;
        replay.random_init = randomInit;

        BOOST_TEST_REQUIRE(replay.StartRecording(tmpFile.filePath, map));
        saveTime = replay.GetSaveTime();
        BOOST_TEST_REQUIRE(replay.IsRecording());
        AddReplayCmds(replay, cmds);
        BOOST_TEST(replay.GetLastGF() == 5u);
        // Assume an exception/crash here so Replay is simply destroyed without Close or StopRecording
    }

    Replay loadReplay;
    BOOST_TEST_REQUIRE(loadReplay.LoadHeader(tmpFile.filePath));
    BOOST_TEST(loadReplay.GetSaveTime() == saveTime);
    BOOST_TEST(loadReplay.GetMapName() == map.title);
    BOOST_TEST(loadReplay.GetPlayerNames().size() == 3u);
    BOOST_TEST(loadReplay.GetPlayerNames()[0] == "Human");
    BOOST_TEST(loadReplay.GetPlayerNames()[1] == "PlAI");
    BOOST_TEST(loadReplay.GetPlayerNames()[2] == "PlAI2");
    BOOST_TEST(loadReplay.GetLastGF() == 5u);
    MapInfo newMap;
    BOOST_TEST_REQUIRE(loadReplay.LoadGameData(newMap));
    BOOST_TEST_REQUIRE(loadReplay.GetNumPlayers() == 4u);
    for(unsigned j = 0; j < 4; j++)
    {
        const BasePlayerInfo& loadPlayer = loadReplay.GetPlayer(j);
        const BasePlayerInfo& worldPlayer = players[j];
        BOOST_TEST_REQUIRE(loadPlayer.ps == worldPlayer.ps);
        if(loadPlayer.isUsed())
            BOOST_TEST_REQUIRE(loadPlayer.name == worldPlayer.name);
    }
    BOOST_TEST(loadReplay.ggs.speed == ggs.speed);
    BOOST_TEST(loadReplay.random_init == randomInit);
    BOOST_TEST(newMap.type == map.type);
    BOOST_TEST(newMap.title == map.title);
    BOOST_TEST(newMap.filepath == map.filepath);
    BOOST_TEST(newMap.mapData.data == map.mapData.data, boost::test_tools::per_element());
    BOOST_TEST(newMap.luaData.data == map.luaData.data, boost::test_tools::per_element());
    BOOST_TEST(loadReplay.IsReplaying());

    CheckReplayCmds(loadReplay, cmds);
}

BOOST_FIXTURE_TEST_CASE(ReplayWithSavegame, RandWorldFixture)
{
    MapInfo map;
    map.type = MapType::Savegame;
    map.title = "MapTitle";
    map.filepath = "Map.swd";
    map.luaFilepath = "Map.lua";
    map.savegame = std::make_unique<Savegame>();
    for(unsigned i = 0; i < world.GetNumPlayers(); i++)
        map.savegame->AddPlayer(world.GetPlayer(i));
    // We can change players
    std::vector<BasePlayerInfo> players(4);
    players[0].ps = PlayerState::AI;
    players[0].aiInfo = AI::Info(AI::Type::Default, AI::Level::Medium);
    players[0].name = "PlAI";
    players[1].ps = PlayerState::Occupied;
    players[1].name = "Human";
    players[2].ps = PlayerState::Locked;
    players[3].ps = PlayerState::AI;
    players[3].aiInfo = AI::Info(AI::Type::Default, AI::Level::Easy);
    players[3].name = "PlAI2";

    map.savegame->ggs = ggs;
    map.savegame->start_gf = em.GetCurrentGF();
    map.savegame->sgd.MakeSnapshot(*game);

    Replay replay;
    BOOST_TEST_REQUIRE(!replay.IsValid());
    BOOST_TEST_REQUIRE(!replay.IsRecording());
    BOOST_TEST_REQUIRE(!replay.IsReplaying());
    for(const BasePlayerInfo& player : players)
        replay.AddPlayer(player);
    replay.ggs.speed = GameSpeed::VeryFast;
    replay.random_init = 815;

    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(tmpFile.isValid());
    tmpFile.close();
    bfs::remove(tmpFile.filePath);
    s25util::time64_t saveTime = s25util::Time::CurrentTime();
    BOOST_TEST_REQUIRE(replay.StartRecording(tmpFile.filePath, map));
    BOOST_TEST_REQUIRE(replay.GetSaveTime() - saveTime <= 20); // 20s difference max
    BOOST_TEST_REQUIRE(replay.IsValid());
    BOOST_TEST_REQUIRE(replay.IsRecording());
    BOOST_TEST_REQUIRE(!replay.IsReplaying());

    PlayerGameCommands cmds = GetTestCommands().create(*game).result;
    AddReplayCmds(replay, cmds);
    BOOST_TEST(replay.GetLastGF() == 5u);
    BOOST_TEST_REQUIRE(replay.StopRecording());
    BOOST_TEST_REQUIRE(!replay.IsValid());
    BOOST_TEST_REQUIRE(!replay.IsRecording());

    for(const bool loadSettings : {false, true})
    {
        Replay loadReplay;
        BOOST_TEST_REQUIRE(loadReplay.LoadHeader(tmpFile.filePath));
        BOOST_TEST(loadReplay.GetSaveTime() == replay.GetSaveTime());
        BOOST_TEST(loadReplay.GetMapName() == "MapTitle");
        BOOST_TEST_REQUIRE(loadReplay.GetPlayerNames().size() == 3u);
        BOOST_TEST(loadReplay.GetPlayerNames()[0] == "PlAI");
        BOOST_TEST(loadReplay.GetPlayerNames()[1] == "Human");
        BOOST_TEST(loadReplay.GetPlayerNames()[2] == "PlAI2");
        BOOST_TEST(loadReplay.GetLastGF() == 5u);
        if(!loadSettings)
        {
            // Not loaded
            BOOST_TEST(loadReplay.GetNumPlayers() == 0u);
            continue;
        }
        MapInfo newMap;
        BOOST_TEST_REQUIRE(loadReplay.LoadGameData(newMap));
        BOOST_TEST_REQUIRE(loadReplay.GetNumPlayers() == 4u);
        for(unsigned j = 0; j < 4; j++)
        {
            const BasePlayerInfo& loadPlayer = loadReplay.GetPlayer(j);
            const BasePlayerInfo& worldPlayer = players[j];
            BOOST_TEST_REQUIRE(loadPlayer.ps == worldPlayer.ps);
            if(!loadPlayer.isUsed())
                continue;
            BOOST_TEST_REQUIRE(loadPlayer.name == worldPlayer.name);
            if(!loadPlayer.isHuman())
            {
                BOOST_TEST_REQUIRE(loadPlayer.aiInfo.type == worldPlayer.aiInfo.type);
                BOOST_TEST_REQUIRE(loadPlayer.aiInfo.level == worldPlayer.aiInfo.level);
            }
        }
        BOOST_TEST(loadReplay.ggs.speed == replay.ggs.speed);
        BOOST_TEST(loadReplay.random_init == replay.random_init);
        BOOST_TEST(newMap.type == map.type);
        BOOST_TEST(newMap.title == map.title);
        BOOST_TEST(newMap.filepath == map.filepath);

        BOOST_REQUIRE_EQUAL_COLLECTIONS(newMap.savegame->sgd.GetData(),                                    //-V807
                                        newMap.savegame->sgd.GetData() + newMap.savegame->sgd.GetLength(), //-V807
                                        map.savegame->sgd.GetData(),
                                        map.savegame->sgd.GetData() + map.savegame->sgd.GetLength());

        CheckReplayCmds(loadReplay, cmds);
    }
}

BOOST_FIXTURE_TEST_CASE(SerializeHunter, EmptyWorldFixture1P)
{
    SerializedGameData sgd;
    const auto hunterPos1 = world.MakeMapPoint(world.GetPlayer(0).GetHQPos() + rttr::test::randomPoint<Position>(2, 4));
    const auto hunterPos2 = world.MakeMapPoint(hunterPos1 + Position(2, 0));
    {
        auto* hunterBld1 = static_cast<nobUsual*>(BuildingFactory::CreateBuilding(
          world, BuildingType::Hunter, hunterPos1, 0, rttr::test::randomEnum<Nation>()));
        auto* hunterBld2 = static_cast<nobUsual*>(BuildingFactory::CreateBuilding(
          world, BuildingType::Hunter, hunterPos2, 0, rttr::test::randomEnum<Nation>()));
        world.AddFigure(hunterPos1,
                        std::make_unique<nofHunter>(hunterPos1, rttr::test::randomValue(0u, 10u), hunterBld1));
        auto& hunter2 = world.AddFigure(
          hunterPos2, std::make_unique<nofHunter>(hunterPos2, rttr::test::randomValue(0u, 10u), hunterBld2));
        world.AddFigure(hunterPos2, std::make_unique<noAnimal>(Species::Deer, hunterPos2));
        hunter2.TryStartHunting();
        sgd.MakeSnapshot(*game);
    }
    MockLocalGameState lgs;
    em.Clear();
    world.Unload();
    sgd.ReadSnapshot(*game, lgs);
    BOOST_TEST_REQUIRE(world.GetFigures(hunterPos1).size() == 1u);
    BOOST_TEST_REQUIRE(world.GetFigures(hunterPos2).size() == 2u);
    const auto* deserializedHunter1 = dynamic_cast<const nofHunter*>(&world.GetFigures(hunterPos1).front());
    const auto* deserializedHunter2 = dynamic_cast<const nofHunter*>(&world.GetFigures(hunterPos2).front());
    BOOST_TEST_REQUIRE(deserializedHunter1);
    BOOST_TEST_REQUIRE(deserializedHunter2);
    BOOST_TEST(deserializedHunter1->GetState() != deserializedHunter2->GetState());

    // Serialize again and compare data
    SerializedGameData sgd2;
    sgd2.MakeSnapshot(*game);
    BOOST_CHECK_EQUAL_COLLECTIONS(sgd.GetData(), sgd.GetData() + sgd.GetLength(), sgd2.GetData(),
                                  sgd2.GetData() + sgd2.GetLength());
}

BOOST_AUTO_TEST_CASE(SerializeGameMessageChat)
{
    GameMessage_Chat msg(rttr::test::randomValue(0u, 10u), rttr::test::randomEnum<ChatDestination>(), "Hello");
    Serializer ser;
    msg.Serialize(ser);
    std::unique_ptr<Message> newMsg(GameMessage::create_game(msg.getId()));
    BOOST_TEST_REQUIRE(!!newMsg);
    newMsg->Deserialize(ser);
    const auto* newMsgChat = dynamic_cast<GameMessage_Chat*>(newMsg.get());
    BOOST_TEST_REQUIRE(newMsgChat);
    BOOST_TEST(newMsgChat->player == msg.player);
    BOOST_TEST(newMsgChat->destination == msg.destination);
    BOOST_TEST(newMsgChat->text == msg.text);
}

BOOST_AUTO_TEST_SUITE_END()
