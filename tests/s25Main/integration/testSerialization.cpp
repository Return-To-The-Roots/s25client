// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
#include "ai/random.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "factories/GameCommandFactory.h"
#include "figures/nofHunter.h"
#include "helpers/OptionalIO.h"
#include "helpers/Range.h"
#include "helpers/format.hpp"
#include "helpers/serializeRNG.h"
#include "network/GameMessage_Chat.h"
#include "network/PlayerGameCommands.h"
#include "random/Random.h"
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
#include <array>
#include <memory>

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(Resource)
BOOST_TEST_DONT_PRINT_LOG_VALUE(AddonId)
BOOST_TEST_DONT_PRINT_LOG_VALUE(nofBuildingWorker::State)
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
            worldNode.seaId = SeaId(rttr::test::randomValue(0, 20));
            worldNode.harborId = HarborId(rttr::test::randomValue(0, 20));
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

    auto gf = loadReplay.ReadGF();
    BOOST_TEST(gf == 1u);
    {
        const auto cmd = get<Replay::ChatCommand>(loadReplay.ReadCommand());
        BOOST_TEST(cmd.player == 2);
        BOOST_TEST(cmd.dest == ChatDestination::Enemies);
        BOOST_TEST(cmd.msg == "Hello");
    }

    gf = loadReplay.ReadGF();
    BOOST_TEST(gf == 1u);
    {
        const auto cmd = get<Replay::ChatCommand>(loadReplay.ReadCommand());
        BOOST_TEST(cmd.player == 3);
        BOOST_TEST(cmd.dest == ChatDestination::All);
        BOOST_TEST(cmd.msg == "Hello2");
    }

    gf = loadReplay.ReadGF();
    BOOST_TEST(gf == 2u);
    {
        const auto cmd = get<Replay::ChatCommand>(loadReplay.ReadCommand());
        BOOST_TEST(cmd.player == 2);
        BOOST_TEST(cmd.dest == ChatDestination::Allies);
        BOOST_TEST(cmd.msg == "Hello3");
    }

    gf = loadReplay.ReadGF();
    BOOST_TEST(gf == 2u);
    {
        const auto cmd = get<Replay::GameCommand>(loadReplay.ReadCommand());
        BOOST_TEST(cmd.player == 0);
        BOOST_TEST(cmd.cmds.checksum == recordedCmds.checksum);
        BOOST_TEST(cmd.cmds.gcs.size() == recordedCmds.gcs.size());
        BOOST_TEST(dynamic_cast<gc::SetFlag*>(cmd.cmds.gcs[0].get()));
        BOOST_TEST(dynamic_cast<gc::SetCoinsAllowed*>(cmd.cmds.gcs[1].get()));
    }

    gf = loadReplay.ReadGF();
    BOOST_TEST(gf == 2u);
    {
        const auto cmd = get<Replay::ChatCommand>(loadReplay.ReadCommand());
        BOOST_TEST(cmd.player == 2);
        BOOST_TEST(cmd.dest == ChatDestination::Enemies);
        BOOST_TEST(cmd.msg == "Hello4");
    }

    gf = loadReplay.ReadGF();
    BOOST_TEST(!gf);
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
    auto& hq = ensureNonNull(world.GetSpecObj<nobBaseWarehouse>(hqPos));
    auto& hqFlag = ensureNonNull(hq.GetFlag());
    const MapPoint usualBldPos = world.MakeMapPoint(hqPos + Position(3, 0));
    auto* usualBld = static_cast<nobUsual*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Bakery, usualBldPos, 0, Nation::Vikings));
    world.RecalcBQAroundPointBig(usualBldPos);
    world.BuildRoad(0, false, hqFlag.GetPos(), std::vector<Direction>(3, Direction::East));
    usualBld->is_working = true;

    // Add 3 fires with first between the others to have a mixed event order in the same GF
    for(const auto& offset : {Position(8, 0), Position(7, 0), Position(9, 0)})
    {
        const auto pt = world.MakeMapPoint(hqPos + offset);
        BOOST_TEST_REQUIRE(!world.GetNode(pt).obj);
        world.SetNO(pt, new noFire(pt, false));
        world.RecalcBQAroundPoint(pt);
    }

    for(unsigned i = 0; i < 100; i++)
        em.ExecuteNextGF();

    // Do this after running GFs to keep the state
    // Add ware to flag
    auto ware = std::make_unique<Ware>(GoodType::Flour, usualBld, &hqFlag);
    ware->WaitAtFlag(hqFlag);
    ware->RecalcRoute();
    hqFlag.AddWare(std::move(ware));
    // Add a ware waiting in a warehouse. See https://github.com/Return-To-The-Roots/s25client/issues/1293
    ware = std::make_unique<Ware>(GoodType::Flour, usualBld, &hq);
    hq.AddWaitingWare(std::move(ware));

    Savegame save;

    for(const auto& player : world.getPlayers())
        save.AddPlayer(player);

    save.ggs = ggs;
    save.start_gf = em.GetCurrentGF();
    save.sgd.MakeSnapshot(*game);

    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(tmpFile.isValid());
    tmpFile.close();

    s25util::time64_t saveTime = s25util::Time::CurrentTime();
    BOOST_TEST_REQUIRE(save.Save(tmpFile.filePath, "MapTitle"));
    BOOST_TEST(save.GetSaveTime() - saveTime <= 20); // 20s difference max
    const unsigned origObjNum = GameObject::GetNumObjs();
    const unsigned origObjIdNum = GameObject::GetObjIDCounter();

    for(const auto what : {SaveGameDataToLoad::Header, SaveGameDataToLoad::HeaderAndSettings, SaveGameDataToLoad::All})
    {
        Savegame loadSave;
        BOOST_TEST_REQUIRE(loadSave.Load(tmpFile.filePath, what));
        BOOST_TEST(loadSave.GetSaveTime() == save.GetSaveTime());
        BOOST_TEST(loadSave.GetMapName() == "MapTitle");
        BOOST_TEST(loadSave.GetPlayerNames().size() == 3u);
        BOOST_TEST(loadSave.GetPlayerNames()[0] == "Human");
        BOOST_TEST(loadSave.GetPlayerNames()[1] == "PlAI");
        BOOST_TEST(loadSave.GetPlayerNames()[2] == "PlAI2");
        BOOST_TEST(loadSave.start_gf == em.GetCurrentGF());
        // Players are loaded with the settings
        if(what == SaveGameDataToLoad::Header)
        {
            BOOST_TEST(loadSave.GetNumPlayers() == 0u);
        } else
        {
            BOOST_TEST_REQUIRE(loadSave.GetNumPlayers() == 4u);
            for(unsigned j = 0; j < 4; j++)
            {
                const BasePlayerInfo& loadPlayer = loadSave.GetPlayer(j);
                const BasePlayerInfo& worldPlayer = world.GetPlayer(j);
                BOOST_TEST(loadPlayer.ps == worldPlayer.ps);
                if(!loadPlayer.isUsed())
                    continue;
                BOOST_TEST(loadPlayer.name == worldPlayer.name);
                if(!loadPlayer.isHuman())
                {
                    BOOST_TEST(loadPlayer.aiInfo.type == worldPlayer.aiInfo.type);
                    BOOST_TEST(loadPlayer.aiInfo.level == worldPlayer.aiInfo.level);
                }
            }
            BOOST_TEST(loadSave.ggs.speed == ggs.speed);
        }
        if(what != SaveGameDataToLoad::All)
            BOOST_TEST(loadSave.sgd.GetLength() == 0u);
        else
        {
            std::vector<PlayerInfo> players;
            for(unsigned j = 0; j < 4; j++)
                players.push_back(PlayerInfo(loadSave.GetPlayer(j)));
            Game game(save.ggs, loadSave.start_gf, players);
            MockLocalGameState localGameState;
            save.sgd.ReadSnapshot(game, localGameState);
            game.world_.InitAfterLoad();
            const World& newWorld = game.world_;
            auto& newEm = static_cast<TestEventManager&>(game.world_.GetEvMgr());

            BOOST_TEST(newWorld.GetSize() == world.GetSize());
            BOOST_TEST(newEm.GetCurrentGF() == em.GetCurrentGF());
            BOOST_TEST(GameObject::GetNumObjs() == origObjNum);
            BOOST_TEST(GameObject::GetObjIDCounter() == origObjIdNum);
            std::vector<const GameEvent*> worldEvs = em.GetEvents();
            std::vector<const GameEvent*> loadEvs = newEm.GetEvents();
            BOOST_TEST(worldEvs.size() == loadEvs.size());
            for(unsigned j = 0; j < worldEvs.size(); ++j)
            {
                BOOST_TEST(worldEvs[j]->GetInstanceId() == loadEvs[j]->GetInstanceId());
                BOOST_TEST(worldEvs[j]->startGF == loadEvs[j]->startGF);
                BOOST_TEST(worldEvs[j]->length == loadEvs[j]->length);
                BOOST_TEST(worldEvs[j]->id == loadEvs[j]->id);
            }
            RTTR_FOREACH_PT(MapPoint, world.GetSize())
                BOOST_TEST_CONTEXT("Point " << pt)
                {
                    const MapNode& worldNode = world.GetNode(pt);
                    const MapNode& loadNode = newWorld.GetNode(pt);
                    BOOST_TEST(loadNode.roads == worldNode.roads, boost::test_tools::per_element());
                    BOOST_TEST(loadNode.altitude == worldNode.altitude);
                    BOOST_TEST(loadNode.shadow == worldNode.shadow);
                    BOOST_TEST(loadNode.t1 == worldNode.t1);
                    BOOST_TEST(loadNode.t2 == worldNode.t2);
                    BOOST_TEST(loadNode.resources == worldNode.resources);
                    BOOST_TEST(loadNode.reserved == worldNode.reserved);
                    BOOST_TEST(loadNode.owner == worldNode.owner);
                    BOOST_TEST(loadNode.bq == worldNode.bq);
                    BOOST_TEST(loadNode.seaId == worldNode.seaId);
                    BOOST_TEST(loadNode.harborId == worldNode.harborId);
                    BOOST_TEST((loadNode.obj != nullptr) == (worldNode.obj != nullptr));
                }
            const auto* newUsual = newWorld.GetSpecObj<nobUsual>(usualBldPos);
            BOOST_TEST_REQUIRE(newUsual);
            BOOST_TEST(newUsual->is_working == usualBld->is_working);
            BOOST_TEST(newUsual->HasWorker() == usualBld->HasWorker());
            BOOST_TEST(newUsual->GetProductivity() == usualBld->GetProductivity());

            auto* hqLoaded = world.GetSpecObj<nobBaseWarehouse>(hqPos);
            BOOST_TEST_REQUIRE(hqLoaded);
            auto* hqFlagLoaded = hqLoaded->GetFlag();
            BOOST_TEST_REQUIRE(hqFlagLoaded);
            BOOST_TEST(hqFlagLoaded->GetNumWares() == 1u);

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

BOOST_FIXTURE_TEST_CASE(EmptyReplayRecordingIsRemoved, ReplayMapFixture)
{
    Replay replay;
    for(const BasePlayerInfo& player : players)
        replay.AddPlayer(player);
    replay.ggs.speed = GameSpeed::VeryFast;

    TmpFile tmpFile(".rpl");
    BOOST_TEST_REQUIRE(tmpFile.isValid());
    tmpFile.close();
    bfs::remove(tmpFile.filePath);

    BOOST_TEST_REQUIRE(replay.StartRecording(tmpFile.filePath, map, 42));
    BOOST_TEST_REQUIRE(replay.IsRecording());
    BOOST_TEST(replay.GetLastGF() == 0u);

    BOOST_TEST_REQUIRE(replay.StopRecording());
    BOOST_TEST_REQUIRE(!replay.IsRecording());
    BOOST_TEST(!bfs::exists(tmpFile.filePath));
}
BOOST_FIXTURE_TEST_CASE(ReplayWithMap, ReplayMapFixture)
{
    Replay replay;
    BOOST_TEST_REQUIRE(!replay.IsRecording());
    BOOST_TEST_REQUIRE(!replay.IsReplaying());
    for(const BasePlayerInfo& player : players)
        replay.AddPlayer(player);
    replay.ggs.speed = GameSpeed::VeryFast;

    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(tmpFile.isValid());
    tmpFile.close();
    // No overwrite
    BOOST_TEST_REQUIRE(!replay.StartRecording(tmpFile.filePath, map, 815));
    BOOST_TEST_REQUIRE(!replay.IsRecording());
    BOOST_TEST(!replay.IsReplaying());

    bfs::remove(tmpFile.filePath);
    s25util::time64_t saveTime = s25util::Time::CurrentTime();
    BOOST_TEST_REQUIRE(replay.StartRecording(tmpFile.filePath, map, 42));
    BOOST_TEST(replay.GetSaveTime() - saveTime <= 20); // 20s difference max
    BOOST_TEST_REQUIRE(replay.IsRecording());
    BOOST_TEST(!replay.IsReplaying());

    GlobalGameSettings ggs = replay.ggs;
    Game game(ggs, 0u, players);
    PlayerGameCommands cmds = GetTestCommands().create(game).result;
    AddReplayCmds(replay, cmds);
    BOOST_TEST(replay.GetLastGF() == 5u);
    BOOST_TEST_REQUIRE(replay.StopRecording());
    BOOST_TEST_REQUIRE(!replay.IsRecording());
    BOOST_TEST(!replay.IsReplaying());

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
            BOOST_TEST(loadPlayer.ps == worldPlayer.ps);
            if(!loadPlayer.isUsed())
                continue;
            BOOST_TEST(loadPlayer.name == worldPlayer.name);
            if(!loadPlayer.isHuman())
            {
                BOOST_TEST(loadPlayer.aiInfo.type == worldPlayer.aiInfo.type);
                BOOST_TEST(loadPlayer.aiInfo.level == worldPlayer.aiInfo.level);
            }
        }
        BOOST_TEST(loadReplay.ggs.speed == replay.ggs.speed);
        BOOST_TEST(loadReplay.getSeed() == replay.getSeed());
        BOOST_TEST(newMap.type == map.type);
        BOOST_TEST(newMap.title == map.title);
        BOOST_TEST(newMap.filepath == map.filepath);
        BOOST_TEST(newMap.mapData.data == map.mapData.data, boost::test_tools::per_element());
        BOOST_TEST(newMap.luaData.data == map.luaData.data, boost::test_tools::per_element());

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

        BOOST_TEST_REQUIRE(replay.StartRecording(tmpFile.filePath, map, randomInit));
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
    BOOST_TEST(loadReplay.getSeed() == randomInit);
    BOOST_TEST(newMap.type == map.type);
    BOOST_TEST(newMap.title == map.title);
    BOOST_TEST(newMap.filepath == map.filepath);
    BOOST_TEST(newMap.mapData.data == map.mapData.data, boost::test_tools::per_element());
    BOOST_TEST(newMap.luaData.data == map.luaData.data, boost::test_tools::per_element());

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
    for(const auto& player : world.getPlayers())
        map.savegame->AddPlayer(player);
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
    BOOST_TEST_REQUIRE(!replay.IsRecording());
    BOOST_TEST_REQUIRE(!replay.IsReplaying());
    for(const BasePlayerInfo& player : players)
        replay.AddPlayer(player);
    replay.ggs.speed = GameSpeed::VeryFast;

    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(tmpFile.isValid());
    tmpFile.close();
    bfs::remove(tmpFile.filePath);
    s25util::time64_t saveTime = s25util::Time::CurrentTime();
    BOOST_TEST_REQUIRE(replay.StartRecording(tmpFile.filePath, map, 815));
    BOOST_TEST_REQUIRE(replay.GetSaveTime() - saveTime <= 20); // 20s difference max
    BOOST_TEST_REQUIRE(replay.IsRecording());
    BOOST_TEST(!replay.IsReplaying());

    PlayerGameCommands cmds = GetTestCommands().create(*game).result;
    AddReplayCmds(replay, cmds);
    BOOST_TEST(replay.GetLastGF() == 5u);
    BOOST_TEST_REQUIRE(replay.StopRecording());
    BOOST_TEST(!replay.IsRecording());
    BOOST_TEST_REQUIRE(!replay.IsReplaying());

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
        BOOST_TEST(loadReplay.getSeed() == replay.getSeed());
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
    const GameMessage_Chat msg(rttr::test::randomValue(0u, 10u), rttr::test::randomEnum<ChatDestination>(), "Hello");
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

BOOST_AUTO_TEST_CASE(SerializeRNGs)
{
    struct Expected
    {
        struct States
        {
            std::string afterSeed, afterUse;
        };
        unsigned seed;
        States local;  // helpers::getRandomGenerator
        States global; // RANDOM
        States AI;     // AI::getRandomGenerator
    };
    // We need states to be identical across compilers, so hardcode expected states here
    std::array expectedStates = {
      // clang-format off
        Expected{1234u,
          {"1234 3159640283 4062961311 3954462607 3112783424 2849714703 731821095 2232873578 1251953424 3917199038 231030171 268845362 2344188614 1849447265 2383103214 2227731755 1714639421 1491439037 462325566 5081481 737186849 236395034 2597008984 1186503577 323074832 2742705513 2905742929 4282737114 279690169 2839941402 3748828822 1960715752 1397597709 97709789 4061626963 1748187571 838075742 431451195 669402733 3800374824 2463558175 2497634970 1432010274 864206586 4221510350 3741544462 1605184079 2065133301 323705460 1498334197 2929517430 30068983 697933991 2032582936 1565495315 3975903569 4234264466 1259089262 841438213 1033753396 2075600832 1439692898 3048143437 2116421738 2121241975 2000428495 4014801032 1182308890 4073340907 2773901005 1601777393 226358519 2710230203 3836472902 3661295235 677550283 2218257251 1923991186 2950234957 4002190714 2076424717 3105673997 2394639421 896257070 3238931578 62082322 494537328 2043416199 787575094 2605813927 2115591795 11224405 2902448101 2261503360 3052447400 3620256625 3332372058 1080087166 2354185085 3971176574 2223492789 3526720920 1653571981 1237715107 2855148114 1215640057 1271033666 1075167450 2529642963 1406662882 983280893 3857294656 2272530143 818487970 760306780 3255479487 1305133728 1905128186 1290154109 707231075 4116935559 3786028941 2361715072 624210629 3772314933 1296868555 1022565936 979540335 3540774987 2571392233 2144617529 2227757723 2729723873 1724766228 2087484879 2126700749 2193874180 4223166951 363383678 2565912385 1876961531 2956195119 49270863 2235594426 4043263784 71647112 3674855738 2897294608 47092398 314637627 2118482909 867605091 475980967 2925434236 2814601552 1018392565 1701333317 2184596593 159036669 389008752 2564619728 465798779 679010857 601369552 3328940468 1169659096 1577376579 2282673073 3943256391 2684777597 1516039621 2453910271 2422866813 1647007944 656185595 3473125046 3353725721 1561332723 3109501228 3309645273 315277750 4288861059 3337417270 1150086304 3668084029 136233007 3192126021 1035124158 1463913138 3472524380 1816531257 1331938263 1493420590 2163928140 2308372872 518082101 1093744813 745760929 351105355 274454110 512090590 2244209503 1254859131 1578565613 4252779752 2304111748 1718562732 874276112 1153003552 1460790998 4068090533 21267025 3858482121 178346375 4047671833 3324612377 1303404570 247553664 3814508890 1758138872 2391496985 2741410692 694162108 2417099019 1290222189 1047648637 1052357683 2451072002 1831040228 3603868734 3412171255 2660323883 3748420629 4020378519 2093160014 2417038614 4270917072 941242412 1134278218 2701791110 132236292 4195646085 2169507280 3568883405 2676162106 2621588749 1417889505 148428631 539723083 1150139024 716780847 1087447942 1798784063 3859094387 2076361774 1075678858 2797510871 253391370 1924946164 3667736236 3037936399 3565896230 3428443807 3659888787 708682200 852849473 3685434031 3019070183 1391094373 1109884545 2982635918 3455881547 3344751224 904595096 1516827402 516278122 665151462 136713427 770712405 1979281056 4194553757 49182831 3851545573 622471385 1293762489 1907072181 3900516386 3099688740 4083974430 3736237714 3352518999 247946055 4151378727 1614840409 798787038 3715538877 1635497246 2979335524 969138280 2687971891 3405383553 4013394295 3416543218 573736004 3566375940 1329407732 1772911835 1641278773 1861839544 965942578 1443383280 56319052 3447985972 2617940204 3532109088 1735340810 1518143379 1767841751 2708149932 86315493 4151745945 1666816259 743010572 3989636863 193240752 453954741 2371264943 2272157576 1416793274 1537120784 1753015039 1806794113 3887072460 279817720 3899087910 572139752 1921928408 4281562094 1607570131 2432651309 3689728735 3640641057 1088380608 622636156 67267140 2007913005 1358166198 2170294414 3523310744 909571460 867074674 1892055641 774856216 1152579801 3484403354 4224728256 3070317907 1801847642 1921517389 1051102819 2191403383 2865297810 2177402682 3607253635 1402015212 4129903854 2335987695 2639759088 3009265898 378619129 2483583151 950481588 654661240 1034507981 3470652759 2334637979 4117133781 1165727975 3303041592 2432728514 3904724540 1373562712 1982785691 254947393 2556875813 1764463588 2006575067 3686933637 550379618 2069183023 1141150636 1318115784 1144094421 1278377773 539933158 3509940041 1705608126 26948328 4107353878 652419544 4053857224 3313816488 4032497417 4092863109 1601525362 1437837300 3900072511 283828035 3170497031 189016722 2930437940 3052292073 1726751827 2430089207 3203609159 4156343256 2168879623 708694682 2051965540 1904509052 4050950133 2550963123 2854365051 2263373156 3978774758 818237698 3846244724 3069865118 4247803704 157155060 3895602162 1424380100 293227625 3137558558 3409756350 3007852868 1053026386 2363264015 3931191255 3753344091 4216793200 1831698968 2286073495 1343932292 2147083829 1966450753 2398462718 2684758827 3920034285 3357735591 3093997686 1634110343 2798240674 2695548901 4025984745 2104366617 552177984 4087451657 3172544444 2343470529 1841636027 963788591 1546393177 2982275719 1839576393 1163661113 2885614314 204941147 3444521915 2839546221 559489441 2995810140 1915174894 2654741028 2466894040 4053406173 3932871794 1597535090 1932728637 3427668619 1148197256 2091083246 999104557 3793799332 3001125575 765603486 3124819004 336039005 61024153 1623097926 2731697901 2057405750 2860842911 2895044574 2033127354 1360611766 3211658787 2123830774 2585647973 1006809238 967439138 2971148383 3865152423 431152555 266271087 1014878660 2671880270 2718700535 1294270117 1650080945 14951278 3506035301 691913278 3389182583 3874037190 3343527356 653804895 917256064 1076193158 467278154 2644153914 2749972001 4097523929 3473621517 1814859666 2003784204 1815041583 3339558965 4237761886 2413845698 1583831506 110976338 4165252462 2855136022 2490856698 1401020399 4204116222 3355206378 4158699783 3452445871 3695871480 1492594980 1456878263 245328365 4128219553 890796299 4273710713 4051040837 3328521666 1408860234 3441738429 384468253 1804471193 3831950625 91275924 1068104335 1125082007 2994180699 4075642955 3794110103 1809796756 305840122 1505790420 327966780 3347453408 3604094916 106498745 2129477172 425126433 3581741630 679129419 508958674 1232390678 3051031632 3879846040 356577894 3441572478 2503539346 4152487186 3583484664 3286803019 2576912557 124239441 3506336060 1270245155 2786452659 719123743 4029894278 2061904325 2105827489 2169487726 2364415467 626163773 500711266 2481616380 915897737 3601980001 850894079 611215857 131647340 523467764 810646941 1470797387 885057421 3705721341 1527105939 489007864 4112723511 3188134116 1058980639 3345454237 3535286969 2431094726 3047661241 2586883373 2927011058 363037976 3479526625 4091582868 1357867518 2284276487 3888728166 3236601671 3064478787",
           "231030171 268845362 2344188614 1849447265 2383103214 2227731755 1714639421 1491439037 462325566 5081481 737186849 236395034 2597008984 1186503577 323074832 2742705513 2905742929 4282737114 279690169 2839941402 3748828822 1960715752 1397597709 97709789 4061626963 1748187571 838075742 431451195 669402733 3800374824 2463558175 2497634970 1432010274 864206586 4221510350 3741544462 1605184079 2065133301 323705460 1498334197 2929517430 30068983 697933991 2032582936 1565495315 3975903569 4234264466 1259089262 841438213 1033753396 2075600832 1439692898 3048143437 2116421738 2121241975 2000428495 4014801032 1182308890 4073340907 2773901005 1601777393 226358519 2710230203 3836472902 3661295235 677550283 2218257251 1923991186 2950234957 4002190714 2076424717 3105673997 2394639421 896257070 3238931578 62082322 494537328 2043416199 787575094 2605813927 2115591795 11224405 2902448101 2261503360 3052447400 3620256625 3332372058 1080087166 2354185085 3971176574 2223492789 3526720920 1653571981 1237715107 2855148114 1215640057 1271033666 1075167450 2529642963 1406662882 983280893 3857294656 2272530143 818487970 760306780 3255479487 1305133728 1905128186 1290154109 707231075 4116935559 3786028941 2361715072 624210629 3772314933 1296868555 1022565936 979540335 3540774987 2571392233 2144617529 2227757723 2729723873 1724766228 2087484879 2126700749 2193874180 4223166951 363383678 2565912385 1876961531 2956195119 49270863 2235594426 4043263784 71647112 3674855738 2897294608 47092398 314637627 2118482909 867605091 475980967 2925434236 2814601552 1018392565 1701333317 2184596593 159036669 389008752 2564619728 465798779 679010857 601369552 3328940468 1169659096 1577376579 2282673073 3943256391 2684777597 1516039621 2453910271 2422866813 1647007944 656185595 3473125046 3353725721 1561332723 3109501228 3309645273 315277750 4288861059 3337417270 1150086304 3668084029 136233007 3192126021 1035124158 1463913138 3472524380 1816531257 1331938263 1493420590 2163928140 2308372872 518082101 1093744813 745760929 351105355 274454110 512090590 2244209503 1254859131 1578565613 4252779752 2304111748 1718562732 874276112 1153003552 1460790998 4068090533 21267025 3858482121 178346375 4047671833 3324612377 1303404570 247553664 3814508890 1758138872 2391496985 2741410692 694162108 2417099019 1290222189 1047648637 1052357683 2451072002 1831040228 3603868734 3412171255 2660323883 3748420629 4020378519 2093160014 2417038614 4270917072 941242412 1134278218 2701791110 132236292 4195646085 2169507280 3568883405 2676162106 2621588749 1417889505 148428631 539723083 1150139024 716780847 1087447942 1798784063 3859094387 2076361774 1075678858 2797510871 253391370 1924946164 3667736236 3037936399 3565896230 3428443807 3659888787 708682200 852849473 3685434031 3019070183 1391094373 1109884545 2982635918 3455881547 3344751224 904595096 1516827402 516278122 665151462 136713427 770712405 1979281056 4194553757 49182831 3851545573 622471385 1293762489 1907072181 3900516386 3099688740 4083974430 3736237714 3352518999 247946055 4151378727 1614840409 798787038 3715538877 1635497246 2979335524 969138280 2687971891 3405383553 4013394295 3416543218 573736004 3566375940 1329407732 1772911835 1641278773 1861839544 965942578 1443383280 56319052 3447985972 2617940204 3532109088 1735340810 1518143379 1767841751 2708149932 86315493 4151745945 1666816259 743010572 3989636863 193240752 453954741 2371264943 2272157576 1416793274 1537120784 1753015039 1806794113 3887072460 279817720 3899087910 572139752 1921928408 4281562094 1607570131 2432651309 3689728735 3640641057 1088380608 622636156 67267140 2007913005 1358166198 2170294414 3523310744 909571460 867074674 1892055641 774856216 1152579801 3484403354 4224728256 3070317907 1801847642 1921517389 1051102819 2191403383 2865297810 2177402682 3607253635 1402015212 4129903854 2335987695 2639759088 3009265898 378619129 2483583151 950481588 654661240 1034507981 3470652759 2334637979 4117133781 1165727975 3303041592 2432728514 3904724540 1373562712 1982785691 254947393 2556875813 1764463588 2006575067 3686933637 550379618 2069183023 1141150636 1318115784 1144094421 1278377773 539933158 3509940041 1705608126 26948328 4107353878 652419544 4053857224 3313816488 4032497417 4092863109 1601525362 1437837300 3900072511 283828035 3170497031 189016722 2930437940 3052292073 1726751827 2430089207 3203609159 4156343256 2168879623 708694682 2051965540 1904509052 4050950133 2550963123 2854365051 2263373156 3978774758 818237698 3846244724 3069865118 4247803704 157155060 3895602162 1424380100 293227625 3137558558 3409756350 3007852868 1053026386 2363264015 3931191255 3753344091 4216793200 1831698968 2286073495 1343932292 2147083829 1966450753 2398462718 2684758827 3920034285 3357735591 3093997686 1634110343 2798240674 2695548901 4025984745 2104366617 552177984 4087451657 3172544444 2343470529 1841636027 963788591 1546393177 2982275719 1839576393 1163661113 2885614314 204941147 3444521915 2839546221 559489441 2995810140 1915174894 2654741028 2466894040 4053406173 3932871794 1597535090 1932728637 3427668619 1148197256 2091083246 999104557 3793799332 3001125575 765603486 3124819004 336039005 61024153 1623097926 2731697901 2057405750 2860842911 2895044574 2033127354 1360611766 3211658787 2123830774 2585647973 1006809238 967439138 2971148383 3865152423 431152555 266271087 1014878660 2671880270 2718700535 1294270117 1650080945 14951278 3506035301 691913278 3389182583 3874037190 3343527356 653804895 917256064 1076193158 467278154 2644153914 2749972001 4097523929 3473621517 1814859666 2003784204 1815041583 3339558965 4237761886 2413845698 1583831506 110976338 4165252462 2855136022 2490856698 1401020399 4204116222 3355206378 4158699783 3452445871 3695871480 1492594980 1456878263 245328365 4128219553 890796299 4273710713 4051040837 3328521666 1408860234 3441738429 384468253 1804471193 3831950625 91275924 1068104335 1125082007 2994180699 4075642955 3794110103 1809796756 305840122 1505790420 327966780 3347453408 3604094916 106498745 2129477172 425126433 3581741630 679129419 508958674 1232390678 3051031632 3879846040 356577894 3441572478 2503539346 4152487186 3583484664 3286803019 2576912557 124239441 3506336060 1270245155 2786452659 719123743 4029894278 2061904325 2105827489 2169487726 2364415467 626163773 500711266 2481616380 915897737 3601980001 850894079 611215857 131647340 523467764 810646941 1470797387 885057421 3705721341 1527105939 489007864 4112723511 3188134116 1058980639 3345454237 3535286969 2431094726 3047661241 2586883373 2927011058 363037976 3479526625 4091582868 1357867518 2284276487 3888728166 3236601671 3064478787 2260313690 348938374 3392255680 2909033704 140638832 1016917445 4051655600 976942074 1628339371 932989997"},
          {"1234", "109916815137328668"},
          {"1234", "1061641155"},
        },
        Expected{322371650,
          {"322371650 3718540299 3286689578 2045584432 2890170457 2251577068 3182275052 3539364077 1608565742 1276195924 2183013011 2488941632 1336350742 896651296 791166126 874188469 3430853241 2559216691 3573975143 1968001927 2044635634 1735464948 1495503039 1353697549 2001059284 1844306466 2681467625 1141610962 1185586011 1709733023 1936756852 401251144 2611006344 1008936851 2735818785 1382101234 1580161795 3580951023 281404994 2853906993 1370616135 2520557255 1720727779 2340774741 3643457663 574901529 3118713867 2672775356 2847050022 1859149669 117319334 3682473393 3258503534 3195598134 2237501882 1290663375 1393506430 162939220 2536094302 3770008199 3999949392 3000776188 196623220 3349973251 3680701760 822220464 1388782514 3096502498 4288714916 1822333224 917003891 3290287782 2514211937 1461241432 2539315815 650534692 1059704192 3908539341 1516801684 1637940760 724476205 1642179602 463736529 1197388232 2113910177 91572341 1111438207 2247081741 2990434371 4264618750 2683902763 1995417736 2508050281 512759956 1208805058 3890898510 945066433 3819227782 2785547227 3938317824 1135520403 1865379327 1770345372 2158232152 2389514218 439195377 2254079615 3709531324 3990210247 1544703937 1311192878 675933434 1741834514 2190934000 1814658284 1186209524 2119455773 353119105 1838953307 898723065 4236263349 3333130567 2667482190 3650487927 1053859904 3486611901 2482878068 4009239309 4027208454 2611179130 2195770842 2982268603 2351337601 639770420 412427274 3076952697 624467471 2723505012 1042572312 3667154435 4169920140 3418924536 886689685 4078478168 4087424631 1498693717 2264290742 1526277271 2163848642 2097706325 2154577594 4014815023 3459338484 680141068 3783423574 3793693988 2508697599 664917102 1955180036 2257933720 1391472738 1499271856 1066408311 398179222 1032734930 64560511 401251009 2745653196 1839522542 2282254324 116970168 1274518339 2442433462 3969227441 3115141864 3269183745 3595985018 2030293358 1470040189 950842783 3568222063 961958481 1028838315 1923155502 1696569411 3485492419 2920448122 3075794963 1227335793 4040778989 2269786276 1893206077 2873580652 2996852263 1046399579 1110280618 1649464379 1359791527 1062850884 2018047131 669313930 3894659131 3586914018 750637200 2943375772 1922755107 3713569336 2647672086 196015540 3849340117 3288051520 300679490 1066848734 836934507 1468044557 2027043475 1581727346 2343317304 46836924 2576007943 1195351765 4024770177 4031159336 4271612118 4266357225 2300587059 850307383 1274557078 3793732983 2371877801 708431453 2026451096 848367173 331386658 427797332 2170195215 3583024141 3522367603 2613144862 3262646523 3572515528 236322312 2782364186 1900883051 4011671750 3751213998 3774603575 2807082619 25335733 4255692898 1424205887 344756081 3964604817 2660700567 3239423431 937915219 374139839 4240128604 2494816125 2651530526 621125904 3319405653 3599840500 56825210 1793836074 3693897984 3301771833 519312365 1550889613 3117899337 3413858213 3072027533 793376635 3858528152 2178556985 3658538842 3080568881 2726109236 3138690148 631130965 2031564449 818163257 2310127255 2217799396 3882964954 3936996026 159972635 1648119750 720239011 3790649712 2371663489 2765430226 2780369972 3276791923 816807254 3567876629 936178134 2518741143 1272031475 3552520869 180923050 119951423 415785225 1983584956 3184243393 190636832 3876699346 4257372328 2949480363 520343522 1307506528 1364296572 150062729 610379334 2959872216 3718074685 4013234 2659333495 170767719 3824005602 3927939077 4143722911 2471497678 1973315775 851378746 3307400231 472018810 194266729 604830133 1521705650 4286364905 1874183069 2175314392 124169807 4143189369 3755318385 4280138826 2189253406 3740987230 178533892 1036874984 1744259293 680623138 2579639489 283444551 1394546012 1060975627 3293342898 4247297073 4193936151 3294284866 283664388 1462503156 2160778506 3414850954 2777733488 3768700254 273140758 3039120404 3389574165 1196051478 2880001404 3607177760 1576492090 3700422579 371845597 3451619999 577711355 869012343 1647271524 1190612043 2775233701 3260524119 2430833817 1754109341 3962602499 3251115384 30166528 760792954 1394409117 351662344 3661455781 2802015740 1804797109 795947140 1312475541 1176354022 841230246 1057324034 143982671 1981573041 3800177143 2732839628 3679742671 2970177542 3030038303 1255392509 2363200761 205069205 4284325976 2705193079 4003243194 2231566223 2378975540 222349794 3284638143 2468955330 106094935 3064690667 1748042118 1406655197 1682773607 2016439770 3135409156 1417311740 3546280816 3930626303 3345430797 1621361192 1090243792 699050265 4035058050 2709592715 1424083636 964876305 3401013598 3333687387 400974947 1012111291 2471162228 2583915324 2926542629 1796868883 380699083 1582848457 1432319387 4090417526 4108534238 1156793063 1058045045 2001647841 3240022553 1716441084 4132533644 1358293543 4200446139 2614909526 1385339107 2379564266 3646708041 3594376692 2879888182 2577076552 1198812663 1233258964 3093553104 3907826082 1473789774 1901806325 1417805839 3518203218 2674062274 2159799438 3817816331 2559177208 381764467 3279759665 1405813901 3831216400 1103114580 9237855 1887807314 2190176407 274082210 162799812 3472216879 2335366712 2945759423 2780105071 2155024096 9032458 4120288979 2535062002 2576420243 3245602073 211853095 3197872969 3575822718 2053248313 4204091649 278671796 1881469423 160289490 109351111 1832427377 12179743 3260891691 1179288505 3588977034 1281695232 2130338137 3762848173 1036208540 3014953091 2865620445 4032367092 4080476013 3096659553 1400553995 663480815 3018667081 4288595606 469571529 4011062606 843120227 1417435666 1539880579 1969853775 3501238220 3391684018 1304875485 2730723797 1422553821 3210415831 2108676373 286808817 4024294435 3795122863 62977004 4179462701 1188832312 3452685712 3849373715 419397733 3074864879 591790744 2485805072 1520405299 147697364 3280039871 3305157448 2756108724 2998629612 1879736837 299200948 1325085221 636227158 4129796113 3277855550 4264126006 2507846159 4245435976 2129432767 3388787487 1462902838 1334126302 3312365351 442589025 203197555 1771173518 3581005723 3945218345 1216491972 3227718124 1027079295 2561644368 4034893712 1400470070 2094377195 3994476683 2967953378 3166156187 1205373081 363806261 1709238823 4262771005 2382448054 2360989253 3371076421 1167026913 2649164964 1383519683 3516213712 2885709190 3311590236 3674170820 3500910797 3065108881 949978187 4140324580 1742531185 3702419583 3462971196 1281699116 2169694739 4151342088 2646912427 2334586626 2173597014 4283056251 3625167280 3984048376 4049762401 2431906565 1451139871 51604275 225220733 1356730544 3747762485 3216634543 3321255331 3311561347 1951565540 1526908862 1865877441 2317078823 3018412801 1636785560 1372967111 4085208713 3427991774 1245748894 1253748265 99754039",
           "2183013011 2488941632 1336350742 896651296 791166126 874188469 3430853241 2559216691 3573975143 1968001927 2044635634 1735464948 1495503039 1353697549 2001059284 1844306466 2681467625 1141610962 1185586011 1709733023 1936756852 401251144 2611006344 1008936851 2735818785 1382101234 1580161795 3580951023 281404994 2853906993 1370616135 2520557255 1720727779 2340774741 3643457663 574901529 3118713867 2672775356 2847050022 1859149669 117319334 3682473393 3258503534 3195598134 2237501882 1290663375 1393506430 162939220 2536094302 3770008199 3999949392 3000776188 196623220 3349973251 3680701760 822220464 1388782514 3096502498 4288714916 1822333224 917003891 3290287782 2514211937 1461241432 2539315815 650534692 1059704192 3908539341 1516801684 1637940760 724476205 1642179602 463736529 1197388232 2113910177 91572341 1111438207 2247081741 2990434371 4264618750 2683902763 1995417736 2508050281 512759956 1208805058 3890898510 945066433 3819227782 2785547227 3938317824 1135520403 1865379327 1770345372 2158232152 2389514218 439195377 2254079615 3709531324 3990210247 1544703937 1311192878 675933434 1741834514 2190934000 1814658284 1186209524 2119455773 353119105 1838953307 898723065 4236263349 3333130567 2667482190 3650487927 1053859904 3486611901 2482878068 4009239309 4027208454 2611179130 2195770842 2982268603 2351337601 639770420 412427274 3076952697 624467471 2723505012 1042572312 3667154435 4169920140 3418924536 886689685 4078478168 4087424631 1498693717 2264290742 1526277271 2163848642 2097706325 2154577594 4014815023 3459338484 680141068 3783423574 3793693988 2508697599 664917102 1955180036 2257933720 1391472738 1499271856 1066408311 398179222 1032734930 64560511 401251009 2745653196 1839522542 2282254324 116970168 1274518339 2442433462 3969227441 3115141864 3269183745 3595985018 2030293358 1470040189 950842783 3568222063 961958481 1028838315 1923155502 1696569411 3485492419 2920448122 3075794963 1227335793 4040778989 2269786276 1893206077 2873580652 2996852263 1046399579 1110280618 1649464379 1359791527 1062850884 2018047131 669313930 3894659131 3586914018 750637200 2943375772 1922755107 3713569336 2647672086 196015540 3849340117 3288051520 300679490 1066848734 836934507 1468044557 2027043475 1581727346 2343317304 46836924 2576007943 1195351765 4024770177 4031159336 4271612118 4266357225 2300587059 850307383 1274557078 3793732983 2371877801 708431453 2026451096 848367173 331386658 427797332 2170195215 3583024141 3522367603 2613144862 3262646523 3572515528 236322312 2782364186 1900883051 4011671750 3751213998 3774603575 2807082619 25335733 4255692898 1424205887 344756081 3964604817 2660700567 3239423431 937915219 374139839 4240128604 2494816125 2651530526 621125904 3319405653 3599840500 56825210 1793836074 3693897984 3301771833 519312365 1550889613 3117899337 3413858213 3072027533 793376635 3858528152 2178556985 3658538842 3080568881 2726109236 3138690148 631130965 2031564449 818163257 2310127255 2217799396 3882964954 3936996026 159972635 1648119750 720239011 3790649712 2371663489 2765430226 2780369972 3276791923 816807254 3567876629 936178134 2518741143 1272031475 3552520869 180923050 119951423 415785225 1983584956 3184243393 190636832 3876699346 4257372328 2949480363 520343522 1307506528 1364296572 150062729 610379334 2959872216 3718074685 4013234 2659333495 170767719 3824005602 3927939077 4143722911 2471497678 1973315775 851378746 3307400231 472018810 194266729 604830133 1521705650 4286364905 1874183069 2175314392 124169807 4143189369 3755318385 4280138826 2189253406 3740987230 178533892 1036874984 1744259293 680623138 2579639489 283444551 1394546012 1060975627 3293342898 4247297073 4193936151 3294284866 283664388 1462503156 2160778506 3414850954 2777733488 3768700254 273140758 3039120404 3389574165 1196051478 2880001404 3607177760 1576492090 3700422579 371845597 3451619999 577711355 869012343 1647271524 1190612043 2775233701 3260524119 2430833817 1754109341 3962602499 3251115384 30166528 760792954 1394409117 351662344 3661455781 2802015740 1804797109 795947140 1312475541 1176354022 841230246 1057324034 143982671 1981573041 3800177143 2732839628 3679742671 2970177542 3030038303 1255392509 2363200761 205069205 4284325976 2705193079 4003243194 2231566223 2378975540 222349794 3284638143 2468955330 106094935 3064690667 1748042118 1406655197 1682773607 2016439770 3135409156 1417311740 3546280816 3930626303 3345430797 1621361192 1090243792 699050265 4035058050 2709592715 1424083636 964876305 3401013598 3333687387 400974947 1012111291 2471162228 2583915324 2926542629 1796868883 380699083 1582848457 1432319387 4090417526 4108534238 1156793063 1058045045 2001647841 3240022553 1716441084 4132533644 1358293543 4200446139 2614909526 1385339107 2379564266 3646708041 3594376692 2879888182 2577076552 1198812663 1233258964 3093553104 3907826082 1473789774 1901806325 1417805839 3518203218 2674062274 2159799438 3817816331 2559177208 381764467 3279759665 1405813901 3831216400 1103114580 9237855 1887807314 2190176407 274082210 162799812 3472216879 2335366712 2945759423 2780105071 2155024096 9032458 4120288979 2535062002 2576420243 3245602073 211853095 3197872969 3575822718 2053248313 4204091649 278671796 1881469423 160289490 109351111 1832427377 12179743 3260891691 1179288505 3588977034 1281695232 2130338137 3762848173 1036208540 3014953091 2865620445 4032367092 4080476013 3096659553 1400553995 663480815 3018667081 4288595606 469571529 4011062606 843120227 1417435666 1539880579 1969853775 3501238220 3391684018 1304875485 2730723797 1422553821 3210415831 2108676373 286808817 4024294435 3795122863 62977004 4179462701 1188832312 3452685712 3849373715 419397733 3074864879 591790744 2485805072 1520405299 147697364 3280039871 3305157448 2756108724 2998629612 1879736837 299200948 1325085221 636227158 4129796113 3277855550 4264126006 2507846159 4245435976 2129432767 3388787487 1462902838 1334126302 3312365351 442589025 203197555 1771173518 3581005723 3945218345 1216491972 3227718124 1027079295 2561644368 4034893712 1400470070 2094377195 3994476683 2967953378 3166156187 1205373081 363806261 1709238823 4262771005 2382448054 2360989253 3371076421 1167026913 2649164964 1383519683 3516213712 2885709190 3311590236 3674170820 3500910797 3065108881 949978187 4140324580 1742531185 3702419583 3462971196 1281699116 2169694739 4151342088 2646912427 2334586626 2173597014 4283056251 3625167280 3984048376 4049762401 2431906565 1451139871 51604275 225220733 1356730544 3747762485 3216634543 3321255331 3311561347 1951565540 1526908862 1865877441 2317078823 3018412801 1636785560 1372967111 4085208713 3427991774 1245748894 1253748265 99754039 989925923 1841981440 2209034816 773261444 2911277004 3688115065 2109722013 1655744533 3855549333 187622740"},
           {"322371650", "7960522923621777100"},
           {"322371650", "1707330405"},
        },
      // clang-format on
    };
    for(const auto& [seed, excpectedLocal, expectedGlobal, expectedAI] : expectedStates)
    {
        BOOST_TEST_INFO_SCOPE("seed=" << seed);
        {
            auto rng = helpers::getRandomGenerator();
            rng.seed(seed);
            BOOST_TEST(helpers::serializeRng(rng) == excpectedLocal.afterSeed);
            Serializer ser;
            helpers::pushRng(ser, rng);
            auto rng2 = helpers::popRng<decltype(rng)>(ser);
            for([[maybe_unused]] const auto i : helpers::range(10))
                BOOST_TEST(rng() == rng2());
            // BOOST_TEST(helpers::randomValue<int>(rng) == helpers::randomValue<int>(rng2));

            BOOST_TEST(helpers::serializeRng(rng) == helpers::serializeRng(rng2));
            BOOST_TEST(helpers::serializeRng(rng) == excpectedLocal.afterUse);
        }
        {
            RANDOM.Init(seed);
            BOOST_TEST(helpers::serializeRng(RANDOM.GetCurrentState()) == expectedGlobal.afterSeed);
            const auto GetObjId = []() { return 1122; };
            RANDOM_RAND(std::numeric_limits<int>::max());
            Serializer ser;
            helpers::pushRng(ser, RANDOM.GetCurrentState());
            std::array<int, 10> nextValues;
            for(int& nextVal : nextValues)
                nextVal = RANDOM_RAND(std::numeric_limits<int>::max());
            RANDOM.ResetState(helpers::popRng<UsedPRNG>(ser));
            for(const int& nextVal : nextValues)
                BOOST_TEST(RANDOM_RAND(std::numeric_limits<int>::max()), nextVal);
            BOOST_TEST(helpers::serializeRng(RANDOM.GetCurrentState()) == expectedGlobal.afterUse);
        }
        {
            AI::getRandomGenerator().seed(seed);
            BOOST_TEST(helpers::serializeRng(AI::getRandomGenerator()) == expectedAI.afterSeed);
            Serializer ser;
            helpers::pushRng(ser, AI::getRandomGenerator());
            std::array<int, 10> nextValues;
            for(int& nextVal : nextValues)
                nextVal = static_cast<int>(AI::getRandomGenerator()());
            AI::getRandomGenerator() = helpers::popRng<decltype(AI::getRandomGenerator())>(ser);
            for(const int& nextVal : nextValues)
                BOOST_TEST(static_cast<int>(AI::getRandomGenerator()()), nextVal);
            BOOST_TEST(helpers::serializeRng(AI::getRandomGenerator()) == expectedAI.afterUse);
        }
    }
}
BOOST_AUTO_TEST_SUITE_END()
