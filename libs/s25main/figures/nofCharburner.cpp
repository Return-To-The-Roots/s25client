// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofCharburner.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"
#include "nodeObjs/noCharburnerPile.h"
#include "gameData/TerrainDesc.h"
#include <stdexcept>

nofCharburner::nofCharburner(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::CharBurner, pos, player, workplace), harvest(false), wt(WareType::Wood)
{}

nofCharburner::nofCharburner(SerializedGameData& sgd, const unsigned obj_id)
    : nofFarmhand(sgd, obj_id), harvest(sgd.PopBool()), wt(sgd.Pop<WareType>())
{}

void nofCharburner::Serialize(SerializedGameData& sgd) const
{
    nofFarmhand::Serialize(sgd);

    sgd.PushBool(harvest);
    sgd.PushEnum<uint8_t>(wt);
}

/// Malt den Arbeiter beim Arbeiten
void nofCharburner::DrawWorking(DrawPoint drawPt)
{
    if(harvest)
    {
        unsigned short now_id = GAMECLIENT.Interpolate(39, current_ev);

        // Schaufel-Sound
        if(now_id == 6 || now_id == 18 || now_id == 30)
        {
            world->GetSoundMgr().playNOSound(76, *this, now_id / 12, 200);
            was_sounding = true;
        }

        unsigned draw_id;
        if(now_id < 36)
            draw_id = 9 + now_id % 12;
        else
            draw_id = 9 + 12 + (now_id - 36);

        LOADER.GetPlayerImage("charburner_bobs", draw_id)
          ->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);
    } else
        LOADER.GetPlayerImage("charburner_bobs", 1 + GAMECLIENT.Interpolate(18, current_ev) % 6)
          ->DrawFull(drawPt, COLOR_WHITE, world->GetPlayer(player).color);
}

unsigned short nofCharburner::GetCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofCharburner::WorkStarted() {}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofCharburner::WorkFinished()
{
    noBase* no = world->GetNO(pos);

    // Is a charburner pile is already there?
    if(no->GetGOT() == GO_Type::Charburnerpile)
    {
        // Is Pile already in the normal "coal harvest mode"?
        if(static_cast<noCharburnerPile*>(no)->GetState() == noCharburnerPile::State::Harvest)
            // Then let's bring a coal to our house
            ware = GoodType::Coal;
        // One step further
        static_cast<noCharburnerPile*>(no)->NextStep();
        return;
    }

    // Point still good?
    if(GetPointQuality(pos) != PointQuality::NotPossible)
    {
        // Delete previous elements
        // Only environt objects and signs are allowed to be removed by the worker!
        // Otherwise just do nothing
        NodalObjectType noType = no->GetType();

        if(noType == NodalObjectType::Environment || noType == NodalObjectType::Nothing)
        {
            world->DestroyNO(pos, false);
            // Plant charburner pile
            world->SetNO(pos, new noCharburnerPile(pos));

            // BQ drumrum neu berechnen
            world->RecalcBQAroundPointBig(pos);
        }
    }
}

/// Fragt abgeleitete Klasse, ob hier Platz bzw ob hier ein Baum etc steht, den z.B. der Holzfäller braucht
nofFarmhand::PointQuality nofCharburner::GetPointQuality(const MapPoint pt) const
{
    noBase* no = world->GetNO(pt);

    // Is a charburner pile already here?
    if(no->GetGOT() == GO_Type::Charburnerpile)
    {
        noCharburnerPile::State pileState = static_cast<noCharburnerPile*>(no)->GetState();
        // Can't it be harvested?
        if(pileState == noCharburnerPile::State::Smoldering)
            return PointQuality::NotPossible;

        // Wood stack which stell need resources?
        if(pileState == noCharburnerPile::State::Wood)
        {
            // Does it need resources and I don't have them hen starting new work (state = Waiting1)?
            if(!workplace->WaresAvailable() && this->state == State::Waiting1)
                return PointQuality::NotPossible;
            else
                // Only second class, harvest all piles first before continue
                // to build others
                return PointQuality::Class2;
        }

        // All ok, work on this pile
        return PointQuality::Class1;
    }

    // Try to "plant" a new pile
    // Still enough wares when starting new work (state = Waiting1)?
    if(!workplace->WaresAvailable() && state == State::Waiting1)
        return PointQuality::NotPossible;

    // Der Platz muss frei sein
    BlockingManner bm = world->GetNO(pt)->GetBM();

    if(bm != BlockingManner::None)
        return PointQuality::NotPossible;

    // Kein Grenzstein darf da stehen
    if(world->GetNode(pt).boundary_stones[BorderStonePos::OnPoint])
        return PointQuality::NotPossible;

    for(const MapPoint nb : world->GetNeighbours(pt))
    {
        // Don't set it next to buildings and other charburner piles and grain fields
        BlockingManner bm = world->GetNO(nb)->GetBM();
        if(bm != BlockingManner::None)
            return PointQuality::NotPossible;
        // darf außerdem nicht neben einer Straße liegen
        for(const auto dir2 : helpers::EnumRange<Direction>{})
        {
            if(world->GetPointRoad(nb, dir2) != PointRoad::None)
                return PointQuality::NotPossible;
        }
    }

    // Terrain untersuchen (need walkable land)
    if(world->IsOfTerrain(
         pt, [](const auto& desc) { return desc.Is(ETerrain::Walkable) && desc.kind == TerrainKind::Land; }))
        return PointQuality::Class3;
    else
        return PointQuality::NotPossible;
}

/// Inform derived class about the start of the whole working process (at the beginning when walking out of the house)
void nofCharburner::WalkingStarted()
{
    noBase* nob = world->GetNO(dest);
    if(nob->GetGOT() == GO_Type::Charburnerpile)
        harvest = !(static_cast<noCharburnerPile*>(nob)->GetState() == noCharburnerPile::State::Wood);
    else
        harvest = false;

    // Consume wares if we carry a ware
    if(!harvest)
    {
        workplace->ConsumeWares();
        // Dertermine ware which we should carry to the pile
        if(nob->GetGOT() != GO_Type::Charburnerpile)
            wt = WareType::Wood;
        else
            wt = WareType(static_cast<noCharburnerPile*>(nob)->GetNeededWareType());
    }
}

/// Draws the figure while returning home / entering the building (often carrying wares)
void nofCharburner::DrawWalkingWithWare(DrawPoint drawPt)
{
    DrawWalking(drawPt, "charburner_bobs", 200);
}

/// Draws the charburner while walking
/// (overriding standard method of nofFarmhand)
void nofCharburner::DrawOtherStates(DrawPoint drawPt)
{
    switch(state)
    {
        case State::WalkToWorkpoint:
        {
            // Carry ware?
            if(!harvest)
            {
                if(wt == WareType::Wood)
                    DrawWalking(drawPt, "charburner_bobs", 102);
                else
                    DrawWalking(drawPt, "charburner_bobs", 151);
            } else
                // Draw normal walking
                DrawWalking(drawPt);
        }
        break;
        default: return;
    }
}

bool nofCharburner::AreWaresAvailable() const
{
    // Charburner doesn't need wares for harvesting!
    // -> Wares are considered when calling GetPointQuality!
    return true;
}
