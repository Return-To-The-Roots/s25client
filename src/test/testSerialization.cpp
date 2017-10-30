// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "CreateEmptyWorld.h"
#include "GameEvent.h"
#include "GamePlayer.h"
#include "PointOutput.h"
#include "Replay.h"
#include "Savegame.h"
#include "SerializedGameData.h"
#include "WorldFixture.h"
#include "initTestHelpers.h"
#include "gameTypes/MapInfo.h"
#include "libutil/tmpFile.h"
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

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
            worldNode.resources = rand() % 20;
            worldNode.reserved = rand() % 2 == 0;
            worldNode.seaId = rand() % 20;
            worldNode.harborId = rand() % 20;
        }
        world.InitAfterLoad();
        world.GetPlayer(0).name = "Human";
        world.GetPlayer(1).ps = PS_AI;
        world.GetPlayer(1).aiInfo = AI::Info(AI::DEFAULT, AI::MEDIUM);
        world.GetPlayer(1).name = "PlAI";
        world.GetPlayer(2).ps = PS_LOCKED;
        world.GetPlayer(3).ps = PS_AI;
        world.GetPlayer(3).aiInfo = AI::Info(AI::DEFAULT, AI::EASY);
        world.GetPlayer(3).name = "PlAI2";

        ggs.speed = GS_VERYFAST;
    }
};

void AddReplayCmds(Replay& replay)
{
    replay.UpdateLastGF(1);
    replay.AddChatCommand(1, 2, 3, "Hello");
    replay.AddChatCommand(1, 3, 1, "Hello2");
    replay.AddChatCommand(2, 2, 2, "Hello3");
    std::vector<unsigned char> data(2, 42);
    replay.AddGameCommand(2, data.size(), &data.front());
    replay.AddChatCommand(2, 2, 3, "Hello4");
    replay.UpdateLastGF(5);
}

void CheckReplayCmds(Replay& loadReplay)
{
    std::vector<unsigned char> data(2, 42);

    BOOST_REQUIRE(loadReplay.IsReplaying());
    unsigned gf;
    BOOST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_REQUIRE_EQUAL(gf, 1u);
    BOOST_REQUIRE_EQUAL(loadReplay.ReadRCType(), Replay::RC_CHAT);
    uint8_t player, dst;
    std::string txt;
    loadReplay.ReadChatCommand(player, dst, txt);
    BOOST_REQUIRE_EQUAL(player, 2);
    BOOST_REQUIRE_EQUAL(dst, 3);
    BOOST_REQUIRE_EQUAL(txt, "Hello");

    BOOST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_REQUIRE_EQUAL(gf, 1u);
    BOOST_REQUIRE_EQUAL(loadReplay.ReadRCType(), Replay::RC_CHAT);
    loadReplay.ReadChatCommand(player, dst, txt);
    BOOST_REQUIRE_EQUAL(player, 3);
    BOOST_REQUIRE_EQUAL(dst, 1);
    BOOST_REQUIRE_EQUAL(txt, "Hello2");

    BOOST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_REQUIRE_EQUAL(gf, 2u);
    BOOST_REQUIRE_EQUAL(loadReplay.ReadRCType(), Replay::RC_CHAT);
    loadReplay.ReadChatCommand(player, dst, txt);
    BOOST_REQUIRE_EQUAL(player, 2);
    BOOST_REQUIRE_EQUAL(dst, 2);
    BOOST_REQUIRE_EQUAL(txt, "Hello3");

    BOOST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_REQUIRE_EQUAL(gf, 2u);
    BOOST_REQUIRE_EQUAL(loadReplay.ReadRCType(), Replay::RC_GAME);
    RTTR_REQUIRE_EQUAL_COLLECTIONS(loadReplay.ReadGameCommand(), data);

    BOOST_REQUIRE(loadReplay.ReadGF(&gf));
    BOOST_REQUIRE_EQUAL(gf, 2u);
    BOOST_REQUIRE_EQUAL(loadReplay.ReadRCType(), Replay::RC_CHAT);
    loadReplay.ReadChatCommand(player, dst, txt);
    BOOST_REQUIRE_EQUAL(player, 2);
    BOOST_REQUIRE_EQUAL(dst, 3);
    BOOST_REQUIRE_EQUAL(txt, "Hello4");

    BOOST_REQUIRE(!loadReplay.ReadGF(&gf));
    BOOST_REQUIRE_EQUAL(gf, 0xFFFFFFFF);
}
} // namespace

BOOST_AUTO_TEST_SUITE(Serialization)

BOOST_FIXTURE_TEST_CASE(BaseSaveLoad, RandWorldFixture)
{
    for(unsigned i = 0; i < 10; i++)
        em.ExecuteNextGF();

    Savegame save;

    for(unsigned i = 0; i < world.GetPlayerCount(); i++)
        save.AddPlayer(world.GetPlayer(i));

    save.ggs = ggs;
    save.start_gf = em.GetCurrentGF();
    save.sgd.MakeSnapshot(world);

    TmpFile tmpFile;
    BOOST_REQUIRE(tmpFile.isValid());
    tmpFile.close();

    libutil::time64_t saveTime = libutil::Time::CurrentTime();
    BOOST_REQUIRE(save.Save(tmpFile.filePath, "MapTitle"));
    BOOST_REQUIRE_LE(save.GetSaveTime() - saveTime, 20); // 20s difference max
    const unsigned origObjNum = GameObject::GetObjCount();
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
            BOOST_REQUIRE_EQUAL(loadSave.GetPlayerCount(), 0u);
        } else
        {
            BOOST_REQUIRE_EQUAL(loadSave.GetPlayerCount(), 4u);
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
            GlobalGameSettings newGGS = save.ggs;
            TestEventManager newEm(loadSave.start_gf);
            GameWorld newWorld(players, newGGS, newEm);
            save.sgd.ReadSnapshot(newWorld);

            BOOST_REQUIRE_EQUAL(newWorld.GetSize(), world.GetSize());
            BOOST_REQUIRE_EQUAL(newEm.GetCurrentGF(), em.GetCurrentGF());
            BOOST_REQUIRE_EQUAL(GameObject::GetObjCount(), origObjNum);
            BOOST_REQUIRE_EQUAL(GameObject::GetObjIDCounter(), origObjIdNum);
            std::vector<const GameEvent*> worldEvs = em.GetEvents();
            std::vector<const GameEvent*> loadEvs = newEm.GetEvents();
            BOOST_REQUIRE_EQUAL(worldEvs.size(), loadEvs.size());
            for(unsigned j = 0; j < worldEvs.size(); ++j)
            {
                BOOST_REQUIRE_EQUAL(worldEvs[j]->GetObjId(), loadEvs[j]->GetObjId());
                BOOST_REQUIRE_EQUAL(worldEvs[j]->startGF, loadEvs[j]->startGF);
                BOOST_REQUIRE_EQUAL(worldEvs[j]->length, loadEvs[j]->length);
                BOOST_REQUIRE_EQUAL(worldEvs[j]->id, loadEvs[j]->id);
            }
            RTTR_FOREACH_PT(MapPoint, world.GetSize())
            {
                const MapNode& worldNode = world.GetNode(pt);
                const MapNode& loadNode = newWorld.GetNode(pt);
                BOOST_REQUIRE_EQUAL(loadNode.altitude, worldNode.altitude);
                BOOST_REQUIRE_EQUAL(loadNode.shadow, worldNode.shadow);
                BOOST_REQUIRE_EQUAL(loadNode.t1, worldNode.t1);
                BOOST_REQUIRE_EQUAL(loadNode.t2, worldNode.t2);
                BOOST_REQUIRE_EQUAL(loadNode.resources, worldNode.resources);
                BOOST_REQUIRE_EQUAL(loadNode.reserved, worldNode.reserved);
                BOOST_REQUIRE_EQUAL(loadNode.owner, worldNode.owner);
                BOOST_REQUIRE_EQUAL(loadNode.bq, worldNode.bq);
                BOOST_REQUIRE_EQUAL(loadNode.seaId, worldNode.seaId);
                BOOST_REQUIRE_EQUAL(loadNode.harborId, worldNode.harborId);
                BOOST_REQUIRE_EQUAL(loadNode.obj != NULL, worldNode.obj != NULL);
            }
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
    map.luaData.data = std::vector<char>(21, 0x21);
    std::vector<BasePlayerInfo> players(4);
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
    BOOST_FOREACH(const BasePlayerInfo& player, players)
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
    libutil::time64_t saveTime = libutil::Time::CurrentTime();
    BOOST_REQUIRE(replay.StartRecording(tmpFile.filePath, map));
    BOOST_REQUIRE_LE(replay.GetSaveTime() - saveTime, 20); // 20s difference max
    BOOST_REQUIRE(replay.IsValid());
    BOOST_REQUIRE(replay.IsRecording());
    BOOST_REQUIRE(!replay.IsReplaying());

    AddReplayCmds(replay);
    replay.StopRecording();
    BOOST_REQUIRE(!replay.IsValid());
    BOOST_REQUIRE(!replay.IsRecording());

    for(int i = 0; i < 1; i++)
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
            BOOST_REQUIRE_EQUAL(loadReplay.GetPlayerCount(), 0u);
            continue;
        }
        BOOST_REQUIRE_EQUAL(loadReplay.random_init, replay.random_init);
        BOOST_REQUIRE_EQUAL(loadReplay.GetPlayerCount(), 4u);
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
        BOOST_REQUIRE(replay.LoadGameData(newMap));
        BOOST_REQUIRE_EQUAL(newMap.type, map.type);
        BOOST_REQUIRE_EQUAL(newMap.title, map.title);
        BOOST_REQUIRE_EQUAL(newMap.filepath, map.filepath);
        RTTR_REQUIRE_EQUAL_COLLECTIONS(newMap.mapData.data, map.mapData.data);
        RTTR_REQUIRE_EQUAL_COLLECTIONS(newMap.luaData.data, map.luaData.data);
        BOOST_REQUIRE(loadReplay.IsReplaying());

        CheckReplayCmds(loadReplay);
    }
}

BOOST_FIXTURE_TEST_CASE(ReplayWithSavegame, RandWorldFixture)
{
    MapInfo map;
    map.type = MAPTYPE_SAVEGAME;
    map.title = "MapTitle";
    map.filepath = "Map.swd";
    map.luaFilepath = "Map.lua";
    map.savegame.reset(new Savegame);
    for(unsigned i = 0; i < world.GetPlayerCount(); i++)
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
    map.savegame->sgd.MakeSnapshot(world);

    Replay replay;
    BOOST_REQUIRE(!replay.IsValid());
    BOOST_REQUIRE(!replay.IsRecording());
    BOOST_REQUIRE(!replay.IsReplaying());
    BOOST_FOREACH(const BasePlayerInfo& player, players)
        replay.AddPlayer(player);
    replay.ggs.speed = GS_VERYFAST;
    replay.random_init = 815;

    TmpFile tmpFile;
    BOOST_REQUIRE(tmpFile.isValid());
    tmpFile.close();
    bfs::remove(tmpFile.filePath);
    libutil::time64_t saveTime = libutil::Time::CurrentTime();
    BOOST_REQUIRE(replay.StartRecording(tmpFile.filePath, map));
    BOOST_REQUIRE_LE(replay.GetSaveTime() - saveTime, 20); // 20s difference max
    BOOST_REQUIRE(replay.IsValid());
    BOOST_REQUIRE(replay.IsRecording());
    BOOST_REQUIRE(!replay.IsReplaying());

    AddReplayCmds(replay);
    replay.StopRecording();
    BOOST_REQUIRE(!replay.IsValid());
    BOOST_REQUIRE(!replay.IsRecording());

    for(int i = 0; i < 1; i++)
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
            BOOST_REQUIRE_EQUAL(loadReplay.GetPlayerCount(), 0u);
            continue;
        }
        BOOST_REQUIRE_EQUAL(loadReplay.random_init, replay.random_init);
        BOOST_REQUIRE_EQUAL(loadReplay.GetPlayerCount(), 4u);
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
        BOOST_REQUIRE(replay.LoadGameData(newMap));
        BOOST_REQUIRE_EQUAL(newMap.type, map.type);
        BOOST_REQUIRE_EQUAL(newMap.title, map.title);
        BOOST_REQUIRE_EQUAL(newMap.filepath, map.filepath);

        BOOST_REQUIRE_EQUAL_COLLECTIONS(newMap.savegame->sgd.GetData(), newMap.savegame->sgd.GetData() + newMap.savegame->sgd.GetLength(),
                                        map.savegame->sgd.GetData(), map.savegame->sgd.GetData() + map.savegame->sgd.GetLength());

        CheckReplayCmds(loadReplay);
    }
}

BOOST_AUTO_TEST_SUITE_END()
