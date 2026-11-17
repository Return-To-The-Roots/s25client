// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameCommands.h"
#include "GameEvent.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
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
#include "helpers/OptionalIO.h"
#include "helpers/format.hpp"
#include "helpers/serializeEnums.h"
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
#include "gameTypes/MineResourceBehavior.h"
#include "s25util/tmpFile.h"
#include <rttr/test/random.hpp>
#include <rttr/test/testHelpers.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(Resource)
BOOST_TEST_DONT_PRINT_LOG_VALUE(AddonId)
BOOST_TEST_DONT_PRINT_LOG_VALUE(MineResourceBehavior)
BOOST_TEST_DONT_PRINT_LOG_VALUE(nofBuildingWorker::State)
// LCOV_EXCL_STOP

namespace {
using EmptyWorldFixture1P = WorldFixture<CreateEmptyWorld, 1>;
struct RandWorldFixture : public WorldFixture<CreateEmptyWorld, 4>
{
    RandWorldFixture()
    {
        RTTR_FOREACH_PT(MapPoint, world.getSize())
        {
            MapNode& worldNode = world.getNodeWriteable(pt);
            worldNode.altitude = rttr::test::randomValue(10, 20);
            worldNode.shadow = rttr::test::randomValue(0, 20);
            worldNode.resources = Resource(rttr::test::randomValue<uint8_t>());
            worldNode.reserved = rttr::test::randomValue(0, 1) == 0;
            worldNode.seaId = SeaId(rttr::test::randomValue(0, 20));
            worldNode.harborId = HarborId(rttr::test::randomValue(0, 20));
        }
        world.initAfterLoad();
        world.getPlayer(0).name = "Human";
        world.getPlayer(1).ps = PlayerState::AI; //-V807
        world.getPlayer(1).aiInfo = AI::Info(AI::Type::Default, AI::Level::Medium);
        world.getPlayer(1).name = "PlAI";
        world.getPlayer(2).ps = PlayerState::Locked;
        world.getPlayer(3).ps = PlayerState::AI; //-V807
        world.getPlayer(3).aiInfo = AI::Info(AI::Type::Default, AI::Level::Easy);
        world.getPlayer(3).name = "PlAI2";

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
    bool addGC(gc::GameCommandPtr gc) override
    {
        result.gcs.push_back(gc);
        return true;
    }
};

void addReplayCmds(Replay& replay, const PlayerGameCommands& cmds)
{
    replay.updateLastGF(1);
    replay.addChatCommand(1, 2, ChatDestination::Enemies, "Hello");
    replay.addChatCommand(1, 3, ChatDestination::All, "Hello2");
    replay.addChatCommand(2, 2, ChatDestination::Allies, "Hello3");

    replay.addGameCommand(2, 0, cmds);
    replay.addChatCommand(2, 2, ChatDestination::Enemies, "Hello4");
    replay.updateLastGF(5);
}

void checkReplayCmds(Replay& loadReplay, const PlayerGameCommands& recordedCmds)
{
    BOOST_TEST_REQUIRE(loadReplay.isReplaying());

    auto gf = loadReplay.readGF();
    BOOST_TEST(gf == 1u);
    {
        const auto cmd = get<Replay::ChatCommand>(loadReplay.readCommand());
        BOOST_TEST(cmd.player == 2);
        BOOST_TEST(cmd.dest == ChatDestination::Enemies);
        BOOST_TEST(cmd.msg == "Hello");
    }

    gf = loadReplay.readGF();
    BOOST_TEST(gf == 1u);
    {
        const auto cmd = get<Replay::ChatCommand>(loadReplay.readCommand());
        BOOST_TEST(cmd.player == 3);
        BOOST_TEST(cmd.dest == ChatDestination::All);
        BOOST_TEST(cmd.msg == "Hello2");
    }

    gf = loadReplay.readGF();
    BOOST_TEST(gf == 2u);
    {
        const auto cmd = get<Replay::ChatCommand>(loadReplay.readCommand());
        BOOST_TEST(cmd.player == 2);
        BOOST_TEST(cmd.dest == ChatDestination::Allies);
        BOOST_TEST(cmd.msg == "Hello3");
    }

    gf = loadReplay.readGF();
    BOOST_TEST(gf == 2u);
    {
        const auto cmd = get<Replay::GameCommand>(loadReplay.readCommand());
        BOOST_TEST(cmd.player == 0);
        BOOST_TEST(cmd.cmds.checksum == recordedCmds.checksum);
        BOOST_TEST(cmd.cmds.gcs.size() == recordedCmds.gcs.size());
        BOOST_TEST(dynamic_cast<gc::SetFlag*>(cmd.cmds.gcs[0].get()));
        BOOST_TEST(dynamic_cast<gc::SetCoinsAllowed*>(cmd.cmds.gcs[1].get()));
    }

    gf = loadReplay.readGF();
    BOOST_TEST(gf == 2u);
    {
        const auto cmd = get<Replay::ChatCommand>(loadReplay.readCommand());
        BOOST_TEST(cmd.player == 2);
        BOOST_TEST(cmd.dest == ChatDestination::Enemies);
        BOOST_TEST(cmd.msg == "Hello4");
    }

    gf = loadReplay.readGF();
    BOOST_TEST(!gf);
}

void pushSerializedGGSHeader(Serializer& ser)
{
    helpers::pushEnum<uint8_t>(ser, GameSpeed::Normal);
    helpers::pushEnum<uint8_t>(ser, GameObjective::None);
    helpers::pushEnum<uint8_t>(ser, StartWares::Normal);
    ser.PushBool(false);
    helpers::pushEnum<uint8_t>(ser, Exploration::FogOfWar);
    ser.PushBool(true);
    ser.PushBool(false);
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
    ggs.serialize(ser);
    ::Serializer loader(ser.GetData(), ser.GetLength());
    GlobalGameSettings ggsLoaded;
    ggsLoaded.deserialize(loader);
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

BOOST_AUTO_TEST_CASE(LegacyInexhaustibleMinesDeserializeMigratesToPerMineBehaviors)
{
    Serializer ser;
    pushSerializedGGSHeader(ser);
    ser.PushUnsignedInt(1);
    ser.PushUnsignedInt(static_cast<unsigned>(AddonId::INEXHAUSTIBLE_MINES));
    ser.PushUnsignedInt(1);

    Serializer loader(ser.GetData(), ser.GetLength());
    GlobalGameSettings ggsLoaded;
    ggsLoaded.deserialize(loader);

    for(const BuildingType mineType :
        {BuildingType::GraniteMine, BuildingType::CoalMine, BuildingType::IronMine, BuildingType::GoldMine})
        BOOST_TEST(getMineResourceBehavior(ggsLoaded, mineType) == MineResourceBehavior::Inexhaustible);
    BOOST_TEST(ggsLoaded.getSelection(AddonId::INEXHAUSTIBLE_MINES) == 0u);
}

BOOST_AUTO_TEST_CASE(LegacyInexhaustibleMinesDeserializeAlsoMigratesLegacyGraniteAddonValue)
{
    // Old data always contains an explicit value for GRANITEMINE_RESOURCE_BEHAVIOR, because that
    // id reuses the slot of the former INEXHAUSTIBLE_GRANITEMINES addon which was always serialized. Before this
    // refactor the separate INEXHAUSTIBLE_MINES setting turned granite mines inexhaustible on its own, so migration
    // must not let a disabled granite value suppress that: granite still becomes Inexhaustible here.
    Serializer ser;
    pushSerializedGGSHeader(ser);
    ser.PushUnsignedInt(2);
    ser.PushUnsignedInt(static_cast<unsigned>(AddonId::INEXHAUSTIBLE_MINES));
    ser.PushUnsignedInt(1);
    ser.PushUnsignedInt(static_cast<unsigned>(AddonId::GRANITEMINE_RESOURCE_BEHAVIOR));
    ser.PushUnsignedInt(0);

    Serializer loader(ser.GetData(), ser.GetLength());
    GlobalGameSettings ggsLoaded;
    ggsLoaded.deserialize(loader);

    for(const BuildingType mineType :
        {BuildingType::GraniteMine, BuildingType::CoalMine, BuildingType::IronMine, BuildingType::GoldMine})
        BOOST_TEST(getMineResourceBehavior(ggsLoaded, mineType) == MineResourceBehavior::Inexhaustible);
}

BOOST_AUTO_TEST_CASE(LegacyInexhaustibleMinesDeserializeDoesNotOverridePerMineBehavior)
{
    Serializer ser;
    pushSerializedGGSHeader(ser);
    ser.PushUnsignedInt(2);
    ser.PushUnsignedInt(static_cast<unsigned>(AddonId::INEXHAUSTIBLE_MINES));
    ser.PushUnsignedInt(1);
    ser.PushUnsignedInt(static_cast<unsigned>(AddonId::COALMINE_RESOURCE_BEHAVIOR));
    ser.PushUnsignedInt(static_cast<unsigned>(MineResourceBehavior::S4LikeExhaustion));

    Serializer loader(ser.GetData(), ser.GetLength());
    GlobalGameSettings ggsLoaded;
    ggsLoaded.deserialize(loader);

    BOOST_TEST(getMineResourceBehavior(ggsLoaded, BuildingType::CoalMine) == MineResourceBehavior::S4LikeExhaustion);
    BOOST_TEST(getMineResourceBehavior(ggsLoaded, BuildingType::GraniteMine) == MineResourceBehavior::Inexhaustible);
    BOOST_TEST(getMineResourceBehavior(ggsLoaded, BuildingType::IronMine) == MineResourceBehavior::Inexhaustible);
    BOOST_TEST(getMineResourceBehavior(ggsLoaded, BuildingType::GoldMine) == MineResourceBehavior::Inexhaustible);
}

BOOST_FIXTURE_TEST_CASE(BaseSaveLoad, RandWorldFixture)
{
    MockLocalGameState lgsGame;
    const std::string luaScript = helpers::format(
      "-- Hello World\n function getRequiredLuaVersion()\n return %1%\n end", LuaInterfaceGameBase::getVersion());
    {
        MapLoader loader(world);
        TmpFile validLuaFile(".lua");
        validLuaFile.getStream() << luaScript;
        validLuaFile.close();

        BOOST_TEST_REQUIRE(loader.loadLuaScript(*game, lgsGame, validLuaFile.filePath));
        BOOST_TEST(world.hasLua());
    }

    const MapPoint hqPos = world.getPlayer(0).getHQPos();
    auto& hq = ensureNonNull(world.getSpecObj<nobBaseWarehouse>(hqPos));
    auto& hqFlag = ensureNonNull(hq.getFlag());
    const MapPoint usualBldPos = world.makeMapPoint(hqPos + Position(3, 0));
    auto* usualBld = static_cast<nobUsual*>(
      BuildingFactory::createBuilding(world, BuildingType::Bakery, usualBldPos, 0, Nation::Vikings));
    world.recalcBQAroundPointBig(usualBldPos);
    world.BuildRoad(0, false, hqFlag.getPos(), std::vector<Direction>(3, Direction::East));
    usualBld->is_working = true;

    // Add 3 fires with first between the others to have a mixed event order in the same GF
    for(const auto& offset : {Position(8, 0), Position(7, 0), Position(9, 0)})
    {
        const auto pt = world.makeMapPoint(hqPos + offset);
        BOOST_TEST_REQUIRE(!world.getNode(pt).obj);
        world.setNO(pt, new noFire(pt, false));
        world.recalcBQAroundPoint(pt);
    }

    for(unsigned i = 0; i < 100; i++)
        em.executeNextGF();

    // Do this after running GFs to keep the state
    // Add ware to flag
    auto ware = std::make_unique<Ware>(GoodType::Flour, usualBld, &hqFlag);
    ware->waitAtFlag(hqFlag);
    ware->recalcRoute();
    hqFlag.addWare(std::move(ware));
    // Add a ware waiting in a warehouse. See https://github.com/Return-To-The-Roots/s25client/issues/1293
    ware = std::make_unique<Ware>(GoodType::Flour, usualBld, &hq);
    hq.addWaitingWare(std::move(ware));

    Savegame save;

    for(const auto& player : world.getPlayers())
        save.addPlayer(player);

    save.ggs = ggs;
    save.start_gf = em.getCurrentGF();
    save.sgd.makeSnapshot(*game);

    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(tmpFile.isValid());
    tmpFile.close();

    s25util::time64_t saveTime = s25util::Time::CurrentTime();
    BOOST_TEST_REQUIRE(save.save(tmpFile.filePath, "MapTitle"));
    BOOST_TEST(save.getSaveTime() - saveTime <= 20); // 20s difference max
    const unsigned origObjNum = GameObject::getNumObjs();
    const unsigned origObjIdNum = GameObject::getObjIDCounter();

    for(const auto what : {SaveGameDataToLoad::Header, SaveGameDataToLoad::HeaderAndSettings, SaveGameDataToLoad::All})
    {
        Savegame loadSave;
        BOOST_TEST_REQUIRE(loadSave.load(tmpFile.filePath, what));
        BOOST_TEST(loadSave.getSaveTime() == save.getSaveTime());
        BOOST_TEST(loadSave.getMapName() == "MapTitle");
        BOOST_TEST(loadSave.getPlayerNames().size() == 3u);
        BOOST_TEST(loadSave.getPlayerNames()[0] == "Human");
        BOOST_TEST(loadSave.getPlayerNames()[1] == "PlAI");
        BOOST_TEST(loadSave.getPlayerNames()[2] == "PlAI2");
        BOOST_TEST(loadSave.start_gf == em.getCurrentGF());
        // Players are loaded with the settings
        if(what == SaveGameDataToLoad::Header)
        {
            BOOST_TEST(loadSave.getNumPlayers() == 0u);
        } else
        {
            BOOST_TEST_REQUIRE(loadSave.getNumPlayers() == 4u);
            for(unsigned j = 0; j < 4; j++)
            {
                const BasePlayerInfo& loadPlayer = loadSave.getPlayer(j);
                const BasePlayerInfo& worldPlayer = world.getPlayer(j);
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
                players.push_back(PlayerInfo(loadSave.getPlayer(j)));
            Game game(save.ggs, loadSave.start_gf, players);
            MockLocalGameState localGameState;
            save.sgd.readSnapshot(game, localGameState);
            game.world_.initAfterLoad();
            const World& newWorld = game.world_;
            auto& newEm = static_cast<TestEventManager&>(game.world_.getEvMgr());

            BOOST_TEST(newWorld.getSize() == world.getSize());
            BOOST_TEST(newEm.getCurrentGF() == em.getCurrentGF());
            BOOST_TEST(GameObject::getNumObjs() == origObjNum);
            BOOST_TEST(GameObject::getObjIDCounter() == origObjIdNum);
            std::vector<const GameEvent*> worldEvs = em.getEvents();
            std::vector<const GameEvent*> loadEvs = newEm.getEvents();
            BOOST_TEST(worldEvs.size() == loadEvs.size());
            for(unsigned j = 0; j < worldEvs.size(); ++j)
            {
                BOOST_TEST(worldEvs[j]->getInstanceId() == loadEvs[j]->getInstanceId());
                BOOST_TEST(worldEvs[j]->startGF == loadEvs[j]->startGF);
                BOOST_TEST(worldEvs[j]->length == loadEvs[j]->length);
                BOOST_TEST(worldEvs[j]->id == loadEvs[j]->id);
            }
            RTTR_FOREACH_PT(MapPoint, world.getSize())
                BOOST_TEST_CONTEXT("Point " << pt)
                {
                    const MapNode& worldNode = world.getNode(pt);
                    const MapNode& loadNode = newWorld.getNode(pt);
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
            const auto* newUsual = newWorld.getSpecObj<nobUsual>(usualBldPos);
            BOOST_TEST_REQUIRE(newUsual);
            BOOST_TEST(newUsual->is_working == usualBld->is_working);
            BOOST_TEST(newUsual->hasWorker() == usualBld->hasWorker());
            BOOST_TEST(newUsual->getProductivity() == usualBld->getProductivity());

            auto* hqLoaded = world.getSpecObj<nobBaseWarehouse>(hqPos);
            BOOST_TEST_REQUIRE(hqLoaded);
            auto* hqFlagLoaded = hqLoaded->getFlag();
            BOOST_TEST_REQUIRE(hqFlagLoaded);
            BOOST_TEST(hqFlagLoaded->getNumWares() == 1u);

            BOOST_TEST_REQUIRE(world.hasLua());
            BOOST_TEST(world.getLua().getScript() == luaScript);

            SerializedGameData loadedSgd;
            loadedSgd.makeSnapshot(game);
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
        replay.addPlayer(player);
    replay.ggs.speed = GameSpeed::VeryFast;

    TmpFile tmpFile(".rpl");
    BOOST_TEST_REQUIRE(tmpFile.isValid());
    tmpFile.close();
    bfs::remove(tmpFile.filePath);

    BOOST_TEST_REQUIRE(replay.startRecording(tmpFile.filePath, map, 42));
    BOOST_TEST_REQUIRE(replay.isRecording());
    BOOST_TEST(replay.getLastGF() == 0u);

    BOOST_TEST_REQUIRE(replay.stopRecording());
    BOOST_TEST_REQUIRE(!replay.isRecording());
    BOOST_TEST(!bfs::exists(tmpFile.filePath));
}
BOOST_FIXTURE_TEST_CASE(ReplayWithMap, ReplayMapFixture)
{
    Replay replay;
    BOOST_TEST_REQUIRE(!replay.isRecording());
    BOOST_TEST_REQUIRE(!replay.isReplaying());
    for(const BasePlayerInfo& player : players)
        replay.addPlayer(player);
    replay.ggs.speed = GameSpeed::VeryFast;

    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(tmpFile.isValid());
    tmpFile.close();
    // No overwrite
    BOOST_TEST_REQUIRE(!replay.startRecording(tmpFile.filePath, map, 815));
    BOOST_TEST_REQUIRE(!replay.isRecording());
    BOOST_TEST(!replay.isReplaying());

    bfs::remove(tmpFile.filePath);
    s25util::time64_t saveTime = s25util::Time::CurrentTime();
    BOOST_TEST_REQUIRE(replay.startRecording(tmpFile.filePath, map, 42));
    BOOST_TEST(replay.getSaveTime() - saveTime <= 20); // 20s difference max
    BOOST_TEST_REQUIRE(replay.isRecording());
    BOOST_TEST(!replay.isReplaying());

    GlobalGameSettings ggs = replay.ggs;
    Game game(ggs, 0u, players);
    PlayerGameCommands cmds = GetTestCommands().create(game).result;
    addReplayCmds(replay, cmds);
    BOOST_TEST(replay.getLastGF() == 5u);
    BOOST_TEST_REQUIRE(replay.stopRecording());
    BOOST_TEST_REQUIRE(!replay.isRecording());
    BOOST_TEST(!replay.isReplaying());

    for(const bool loadSettings : {false, true})
    {
        Replay loadReplay;
        BOOST_TEST_REQUIRE(loadReplay.loadHeader(tmpFile.filePath));
        BOOST_TEST(loadReplay.getSaveTime() == replay.getSaveTime());
        BOOST_TEST(loadReplay.getMapName() == "MapTitle");
        BOOST_TEST_REQUIRE(loadReplay.getPlayerNames().size() == 3u);
        BOOST_TEST(loadReplay.getPlayerNames()[0] == "Human");
        BOOST_TEST(loadReplay.getPlayerNames()[1] == "PlAI");
        BOOST_TEST(loadReplay.getPlayerNames()[2] == "PlAI2");
        BOOST_TEST(loadReplay.getLastGF() == 5u);
        if(!loadSettings)
        {
            // Not loaded
            BOOST_TEST(loadReplay.getNumPlayers() == 0u);
            continue;
        }
        MapInfo newMap;
        BOOST_TEST_REQUIRE(loadReplay.loadGameData(newMap));
        BOOST_TEST_REQUIRE(loadReplay.getNumPlayers() == 4u);
        for(unsigned j = 0; j < 4; j++)
        {
            const BasePlayerInfo& loadPlayer = loadReplay.getPlayer(j);
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

        checkReplayCmds(loadReplay, cmds);
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
            replay.addPlayer(player);
        replay.ggs.speed = GameSpeed::VeryFast;

        BOOST_TEST_REQUIRE(replay.startRecording(tmpFile.filePath, map, randomInit));
        saveTime = replay.getSaveTime();
        BOOST_TEST_REQUIRE(replay.isRecording());
        addReplayCmds(replay, cmds);
        BOOST_TEST(replay.getLastGF() == 5u);
        // Assume an exception/crash here so Replay is simply destroyed without Close or StopRecording
    }

    Replay loadReplay;
    BOOST_TEST_REQUIRE(loadReplay.loadHeader(tmpFile.filePath));
    BOOST_TEST(loadReplay.getSaveTime() == saveTime);
    BOOST_TEST(loadReplay.getMapName() == map.title);
    BOOST_TEST(loadReplay.getPlayerNames().size() == 3u);
    BOOST_TEST(loadReplay.getPlayerNames()[0] == "Human");
    BOOST_TEST(loadReplay.getPlayerNames()[1] == "PlAI");
    BOOST_TEST(loadReplay.getPlayerNames()[2] == "PlAI2");
    BOOST_TEST(loadReplay.getLastGF() == 5u);
    MapInfo newMap;
    BOOST_TEST_REQUIRE(loadReplay.loadGameData(newMap));
    BOOST_TEST_REQUIRE(loadReplay.getNumPlayers() == 4u);
    for(unsigned j = 0; j < 4; j++)
    {
        const BasePlayerInfo& loadPlayer = loadReplay.getPlayer(j);
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

    checkReplayCmds(loadReplay, cmds);
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
        map.savegame->addPlayer(player);
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
    map.savegame->start_gf = em.getCurrentGF();
    map.savegame->sgd.makeSnapshot(*game);

    Replay replay;
    BOOST_TEST_REQUIRE(!replay.isRecording());
    BOOST_TEST_REQUIRE(!replay.isReplaying());
    for(const BasePlayerInfo& player : players)
        replay.addPlayer(player);
    replay.ggs.speed = GameSpeed::VeryFast;

    TmpFile tmpFile;
    BOOST_TEST_REQUIRE(tmpFile.isValid());
    tmpFile.close();
    bfs::remove(tmpFile.filePath);
    s25util::time64_t saveTime = s25util::Time::CurrentTime();
    BOOST_TEST_REQUIRE(replay.startRecording(tmpFile.filePath, map, 815));
    BOOST_TEST_REQUIRE(replay.getSaveTime() - saveTime <= 20); // 20s difference max
    BOOST_TEST_REQUIRE(replay.isRecording());
    BOOST_TEST(!replay.isReplaying());

    PlayerGameCommands cmds = GetTestCommands().create(*game).result;
    addReplayCmds(replay, cmds);
    BOOST_TEST(replay.getLastGF() == 5u);
    BOOST_TEST_REQUIRE(replay.stopRecording());
    BOOST_TEST(!replay.isRecording());
    BOOST_TEST_REQUIRE(!replay.isReplaying());

    for(const bool loadSettings : {false, true})
    {
        Replay loadReplay;
        BOOST_TEST_REQUIRE(loadReplay.loadHeader(tmpFile.filePath));
        BOOST_TEST(loadReplay.getSaveTime() == replay.getSaveTime());
        BOOST_TEST(loadReplay.getMapName() == "MapTitle");
        BOOST_TEST_REQUIRE(loadReplay.getPlayerNames().size() == 3u);
        BOOST_TEST(loadReplay.getPlayerNames()[0] == "PlAI");
        BOOST_TEST(loadReplay.getPlayerNames()[1] == "Human");
        BOOST_TEST(loadReplay.getPlayerNames()[2] == "PlAI2");
        BOOST_TEST(loadReplay.getLastGF() == 5u);
        if(!loadSettings)
        {
            // Not loaded
            BOOST_TEST(loadReplay.getNumPlayers() == 0u);
            continue;
        }
        MapInfo newMap;
        BOOST_TEST_REQUIRE(loadReplay.loadGameData(newMap));
        BOOST_TEST_REQUIRE(loadReplay.getNumPlayers() == 4u);
        for(unsigned j = 0; j < 4; j++)
        {
            const BasePlayerInfo& loadPlayer = loadReplay.getPlayer(j);
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

        checkReplayCmds(loadReplay, cmds);
    }
}

BOOST_FIXTURE_TEST_CASE(SerializeHunter, EmptyWorldFixture1P)
{
    SerializedGameData sgd;
    const auto hunterPos1 = world.makeMapPoint(world.getPlayer(0).getHQPos() + rttr::test::randomPoint<Position>(2, 4));
    const auto hunterPos2 = world.makeMapPoint(hunterPos1 + Position(2, 0));
    {
        auto* hunterBld1 = static_cast<nobUsual*>(BuildingFactory::createBuilding(
          world, BuildingType::Hunter, hunterPos1, 0, rttr::test::randomEnum<Nation>()));
        auto* hunterBld2 = static_cast<nobUsual*>(BuildingFactory::createBuilding(
          world, BuildingType::Hunter, hunterPos2, 0, rttr::test::randomEnum<Nation>()));
        world.addFigure(hunterPos1,
                        std::make_unique<nofHunter>(hunterPos1, rttr::test::randomValue(0u, 10u), hunterBld1));
        auto& hunter2 = world.addFigure(
          hunterPos2, std::make_unique<nofHunter>(hunterPos2, rttr::test::randomValue(0u, 10u), hunterBld2));
        world.addFigure(hunterPos2, std::make_unique<noAnimal>(Species::Deer, hunterPos2));
        hunter2.tryStartHunting();
        sgd.makeSnapshot(*game);
    }
    MockLocalGameState lgs;
    em.clear();
    world.unload();
    sgd.readSnapshot(*game, lgs);
    BOOST_TEST_REQUIRE(world.getFigures(hunterPos1).size() == 1u);
    BOOST_TEST_REQUIRE(world.getFigures(hunterPos2).size() == 2u);
    const auto* deserializedHunter1 = dynamic_cast<const nofHunter*>(&world.getFigures(hunterPos1).front());
    const auto* deserializedHunter2 = dynamic_cast<const nofHunter*>(&world.getFigures(hunterPos2).front());
    BOOST_TEST_REQUIRE(deserializedHunter1);
    BOOST_TEST_REQUIRE(deserializedHunter2);
    BOOST_TEST(deserializedHunter1->getState() != deserializedHunter2->getState());

    // serialize again and compare data
    SerializedGameData sgd2;
    sgd2.makeSnapshot(*game);
    BOOST_CHECK_EQUAL_COLLECTIONS(sgd.GetData(), sgd.GetData() + sgd.GetLength(), sgd2.GetData(),
                                  sgd2.GetData() + sgd2.GetLength());
}

BOOST_AUTO_TEST_CASE(SerializeGameMessageChat)
{
    const GameMessage_Chat msg(rttr::test::randomValue(0u, 10u), rttr::test::randomEnum<ChatDestination>(), "Hello");
    Serializer ser;
    msg.serialize(ser);
    std::unique_ptr<Message> newMsg(GameMessage::createGame(msg.getId()));
    BOOST_TEST_REQUIRE(!!newMsg);
    newMsg->deserialize(ser);
    const auto* newMsgChat = dynamic_cast<GameMessage_Chat*>(newMsg.get());
    BOOST_TEST_REQUIRE(newMsgChat);
    BOOST_TEST(newMsgChat->player == msg.player);
    BOOST_TEST(newMsgChat->destination == msg.destination);
    BOOST_TEST(newMsgChat->text == msg.text);
}

BOOST_AUTO_TEST_SUITE_END()
