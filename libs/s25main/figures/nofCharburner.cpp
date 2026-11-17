// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    : nofFarmhand(sgd, obj_id), harvest(sgd.popBool()), wt(sgd.pop<WareType>())
{}

void nofCharburner::serialize(SerializedGameData& sgd) const
{
    nofFarmhand::serialize(sgd);

    sgd.pushBool(harvest);
    sgd.pushEnum<uint8_t>(wt);
}

/// Malt den Arbeiter beim Arbeiten
void nofCharburner::drawWorking(DrawPoint drawPt)
{
    if(harvest)
    {
        unsigned short now_id = GAMECLIENT.interpolate(39, current_ev);

        // Schaufel-Sound
        if(now_id == 6 || now_id == 18 || now_id == 30)
        {
            world->getSoundMgr().playNOSound(76, *this, now_id / 12, 200);
            was_sounding = true;
        }

        unsigned draw_id;
        if(now_id < 36)
            draw_id = 9 + now_id % 12;
        else
            draw_id = 9 + 12 + (now_id - 36);

        LOADER.getPlayerImage("charburner_bobs", draw_id)
          ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
    } else
        LOADER.getPlayerImage("charburner_bobs", 1 + GAMECLIENT.interpolate(18, current_ev) % 6)
          ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
}

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4646) // function declared with [[noreturn]] has non-void return type
#endif

unsigned short nofCharburner::getCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

/// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
void nofCharburner::workStarted() {}

/// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
void nofCharburner::workFinished()
{
    noBase* no = world->getNO(pos);

    // Is a charburner pile is already there?
    if(no->getGOT() == GO_Type::Charburnerpile)
    {
        // Is Pile already in the normal "coal harvest mode"?
        if(static_cast<noCharburnerPile*>(no)->getState() == noCharburnerPile::State::Harvest)
            // Then let's bring a coal to our house
            ware = GoodType::Coal;
        // One step further
        static_cast<noCharburnerPile*>(no)->nextStep();
        return;
    }

    // Point still good?
    if(getPointQuality(pos) != PointQuality::NotPossible)
    {
        // Delete previous elements
        // Only environ objects and signs are allowed to be removed by the worker!
        // Otherwise just do nothing
        NodalObjectType noType = no->getType();

        if(noType == NodalObjectType::Environment || noType == NodalObjectType::Nothing)
        {
            world->destroyNO(pos, false);
            // Plant charburner pile
            world->setNO(pos, new noCharburnerPile(pos));

            // BQ drumrum neu berechnen
            world->recalcBQAroundPointBig(pos);
        }
    }
}

/// Fragt abgeleitete Klasse, ob hier Platz bzw ob hier ein Baum etc steht, den z.B. der Holzfäller braucht
nofFarmhand::PointQuality nofCharburner::getPointQuality(const MapPoint pt, const bool isBeforeWork) const
{
    noBase* no = world->getNO(pt);

    // Is a charburner pile already here?
    if(no->getGOT() == GO_Type::Charburnerpile)
    {
        noCharburnerPile::State pileState = static_cast<noCharburnerPile*>(no)->getState();
        // Can't it be harvested?
        if(pileState == noCharburnerPile::State::Smoldering)
            return PointQuality::NotPossible;

        // Wood stack which still need resources?
        if(pileState == noCharburnerPile::State::Wood)
        {
            // Only check for resources before going out
            if(isBeforeWork && !workplace->waresAvailable())
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
    // Enough wares when starting new work?
    if(isBeforeWork && !workplace->waresAvailable())
        return PointQuality::NotPossible;

    // Der Platz muss frei sein
    BlockingManner bm = world->getNO(pt)->getBM();

    if(bm != BlockingManner::None)
        return PointQuality::NotPossible;

    // Kein Grenzstein darf da stehen
    if(world->getNode(pt).boundary_stones[BorderStonePos::OnPoint])
        return PointQuality::NotPossible;

    for(const MapPoint nb : world->getNeighbours(pt))
    {
        // Don't set it next to buildings and other charburner piles and grain fields
        BlockingManner bm = world->getNO(nb)->getBM();
        if(bm != BlockingManner::None)
            return PointQuality::NotPossible;
        // darf außerdem nicht neben einer Straße liegen
        for(const auto dir2 : helpers::EnumRange<Direction>{})
        {
            if(world->getPointRoad(nb, dir2) != PointRoad::None)
                return PointQuality::NotPossible;
        }
    }

    // Terrain untersuchen (need walkable land)
    if(world->isOfTerrain(
         pt, [](const auto& desc) { return desc.is(ETerrain::Walkable) && desc.kind == TerrainKind::Land; }))
        return PointQuality::Class3;
    else
        return PointQuality::NotPossible;
}

/// Inform derived class about the start of the whole working process (at the beginning when walking out of the house)
void nofCharburner::walkingStarted()
{
    noBase* nob = world->getNO(dest);
    if(nob->getGOT() == GO_Type::Charburnerpile)
        harvest = !(static_cast<noCharburnerPile*>(nob)->getState() == noCharburnerPile::State::Wood);
    else
        harvest = false;

    // Consume wares if we carry a ware
    if(!harvest)
    {
        workplace->consumeWares();
        // Dertermine ware which we should carry to the pile
        if(nob->getGOT() != GO_Type::Charburnerpile)
            wt = WareType::Wood;
        else
            wt = WareType(static_cast<noCharburnerPile*>(nob)->getNeededWareType());
    }
}

/// Draws the figure while returning home / entering the building (often carrying wares)
void nofCharburner::drawWalkingWithWare(DrawPoint drawPt)
{
    drawWalking(drawPt, "charburner_bobs", 200);
}

/// Draws the charburner while walking
/// (overriding standard method of nofFarmhand)
void nofCharburner::drawOtherStates(DrawPoint drawPt)
{
    switch(state)
    {
        case State::WalkToWorkpoint:
        {
            // Carry ware?
            if(!harvest)
            {
                if(wt == WareType::Wood)
                    drawWalking(drawPt, "charburner_bobs", 102);
                else
                    drawWalking(drawPt, "charburner_bobs", 151);
            } else
                // Draw normal walking
                drawWalking(drawPt);
        }
        break;
        default: return;
    }
}

bool nofCharburner::areWaresAvailable() const
{
    // Charburner doesn't need wares for harvesting!
    // -> Wares are considered when calling GetPointQuality!
    return true;
}
