// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofWoodcutter.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorld.h"
#include "nodeObjs/noTree.h"

nofWoodcutter::nofWoodcutter(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::Woodcutter, pos, player, workplace)
{}

nofWoodcutter::nofWoodcutter(SerializedGameData& sgd, const unsigned obj_id) : nofFarmhand(sgd, obj_id) {}

/// Malt den Arbeiter beim Arbeiten
void nofWoodcutter::DrawWorking(DrawPoint drawPt)
{
    unsigned short nowId = GAMECLIENT.Interpolate(118, current_ev);

    if(nowId < 10)
    {
        // 1. Ein Stück vom Baum nach links laufen
        LOADER.getBobSprite(world->GetPlayer(player).nation, Job::Woodcutter, Direction::West, nowId % 8)
          .draw(drawPt - DrawPoint(nowId, 0), COLOR_WHITE, world->GetPlayer(player).color);
    } else if(nowId < 82)
    {
        // 2. Hacken
        LOADER.GetPlayerImage("rom_bobs", 24 + (nowId - 10) % 8)
          ->DrawFull(drawPt - DrawPoint(9, 0), COLOR_WHITE, world->GetPlayer(player).color);

        if((nowId - 10) % 8 == 3)
        {
            world->GetSoundMgr().playNOSound(53, *this, nowId);
            was_sounding = true;
        }

    } else if(nowId < 105)
    {
        // 3. Warten bis Baum umfällt
        LOADER.GetPlayerImage("rom_bobs", 24)
          ->DrawFull(drawPt - DrawPoint(9, 0), COLOR_WHITE, world->GetPlayer(player).color);

        if(nowId == 90)
        {
            world->GetSoundMgr().playNOSound(85, *this, nowId);
            was_sounding = true;
        }
    } else if(nowId < 115)
    {
        // 4. Wieder zurückgehen nach rechts
        LOADER.getBobSprite(world->GetPlayer(player).nation, Job::Woodcutter, Direction::East, (nowId - 105) % 8)
          .draw(drawPt - DrawPoint(9 - (nowId - 105), 0), COLOR_WHITE, world->GetPlayer(player).color);
    } else
    {
        // 5. kurz am Baum warten (quasi Baumstamm in die Hand nehmen)
        LOADER.getBobSprite(world->GetPlayer(player).nation, Job::Woodcutter, Direction::East, 1)
          .draw(drawPt, COLOR_WHITE, world->GetPlayer(player).color);
    }
}

unsigned short nofWoodcutter::GetCarryID() const
{
    return 61;
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofWoodcutter::WorkStarted()
{
    RTTR_Assert(world->GetSpecObj<noTree>(dest)->GetType() == NodalObjectType::Tree);

    world->GetSpecObj<noTree>(dest)->FallSoon();
}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofWoodcutter::WorkFinished()
{
    // Holz in die Hand nehmen
    ware = GoodType::Wood;
}

/// Returns the quality of this working point or determines if the worker can work here at all
nofFarmhand::PointQuality nofWoodcutter::GetPointQuality(const MapPoint pt) const
{
    // Gibt es hier an dieser Position einen Baum und ist dieser ausgewachsen?
    // außerdem keine Ananas fällen!
    const noBase* no = world->GetNO(pt);
    if(no->GetType() == NodalObjectType::Tree)
    {
        if(static_cast<const noTree*>(no)->IsFullyGrown() && static_cast<const noTree*>(no)->ProducesWood())
            return PointQuality::Class1;
    }

    return PointQuality::NotPossible;
}

void nofWoodcutter::WorkAborted()
{
    nofFarmhand::WorkAborted();
    // Dem Baum Bescheid sagen
    if(state == State::Work)
        world->GetSpecObj<noTree>(pos)->DontFall();
}
