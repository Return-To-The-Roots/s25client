// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofWinegrower.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "WineLoader.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "world/GameWorld.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noGrapefield.h"
#include <stdexcept>

using namespace wineaddon;

nofWinegrower::nofWinegrower(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofFarmhand(Job::Winegrower, pos, player, workplace), harvest(false)
{}

nofWinegrower::nofWinegrower(SerializedGameData& sgd, const unsigned obj_id)
    : nofFarmhand(sgd, obj_id), harvest(sgd.popBool())
{}

void nofWinegrower::serialize(SerializedGameData& sgd) const
{
    nofFarmhand::serialize(sgd);
    sgd.pushBool(harvest);
}

void nofWinegrower::drawWorking(DrawPoint drawPt)
{
    if(harvest)
    {
        const unsigned nowId = GAMECLIENT.interpolate(44, current_ev);
        LOADER.getPlayerImage("wine_bobs", bobIndex[BobTypes::WINEGROWER_PICKING_GRAPES_ANIMATION] + nowId % 4)
          ->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
    } else
    {
        const unsigned nowId = GAMECLIENT.interpolate(3 * 16 + 8 * 4, current_ev);
        unsigned drawId;
        if(nowId < 48)
        {
            drawId = bobIndex[BobTypes::WINEGROWER_DIGGING_ANIMATION] + nowId % 15;

            // Shovel-Sound
            if(nowId % 16 == 5)
            {
                world->getSoundMgr().playNOSound(76, *this, nowId, 200);
                was_sounding = true;
            }
        } else
            drawId = bobIndex[BobTypes::WINEGROWER_PLANTING_ANIMATION] + nowId % 4;

        LOADER.getPlayerImage("wine_bobs", drawId)->drawFull(drawPt, COLOR_WHITE, world->getPlayer(player).color);
    }
}

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4646) // function declared with [[noreturn]] has non-void return type
#endif

unsigned short nofWinegrower::getCarryID() const
{
    throw std::logic_error("Must not be called. Handled by custom DrawWalkingWithWare");
}

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

void nofWinegrower::workStarted()
{
    // Inform grapefield to not disappear during harvesting
    if(harvest)
        world->getSpecObj<noGrapefield>(pos)->beginHarvesting();
}

void nofWinegrower::workFinished()
{
    if(harvest)
    {
        // Destroy grapefield, and take before the ID of the harvested grapefield, which will be set as normal
        // ornamental object
        noBase* nob = world->getNO(pos);
        // Check if there is still a grapes field at this position
        if(nob->getGOT() != GO_Type::Grapefield)
            return;
        const unsigned wineBobs = static_cast<noGrapefield*>(nob)->getHarvestID();
        world->destroyNO(pos);
        world->setNO(pos, new noEnvObject(pos, wineBobs, 7));

        // Take harvested grapes into hands
        ware = GoodType::Grapes;
    } else
    {
        // If the point got bad (e.g. something was build), abort work
        if(getPointQuality(pos) == PointQuality::NotPossible)
            return;

        // What is currently here
        const NodalObjectType noType = world->getNO(pos)->getType();

        // Only demolish ornamental objects and signs
        if(noType == NodalObjectType::Environment || noType == NodalObjectType::Nothing)
        {
            world->destroyNO(pos, false);
            // place new grapefield
            world->setNO(pos, new noGrapefield(pos));
        }

        // We have only planted (take nothing into hand)
        ware = std::nullopt;
    }

    // recompute BQ around
    world->recalcBQAroundPoint(pos);
}

nofFarmhand::PointQuality nofWinegrower::getPointQuality(const MapPoint pt, const bool isBeforeWork) const
{
    // Either a grapefield exists we can harvest...
    if(world->getNO(pt)->getType() == NodalObjectType::Grapefield)
    {
        if(world->getSpecObj<noGrapefield>(pt)->isHarvestable())
            return PointQuality::Class1;
        else
            return PointQuality::NotPossible;
    } else // or a free space, to place a new one
    {
        // Try to "plant" a new grapefield
        // Still enough wares when starting new work
        if(isBeforeWork && !workplace->waresAvailable())
            return PointQuality::NotPossible;

        // Do not build on road
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(world->getPointRoad(pt, dir) != PointRoad::None)
                return PointQuality::NotPossible;
        }

        // Investigate terrain
        if(!world->isOfTerrain(pt, [](const auto& desc) { return desc.isVital(); }))
            return PointQuality::NotPossible;

        // Is space free?
        NodalObjectType noType = world->getNO(pt)->getType();
        if(noType != NodalObjectType::Environment && noType != NodalObjectType::Nothing)
            return PointQuality::NotPossible;

        for(const MapPoint nb : world->getNeighbours(pt))
        {
            // Do not build near to other grapefields and buildings!
            noType = world->getNO(nb)->getType();
            if(noType == NodalObjectType::Grapefield || noType == NodalObjectType::Grainfield
               || noType == NodalObjectType::Building || noType == NodalObjectType::Buildingsite)
                return PointQuality::NotPossible;
        }

        return PointQuality::Class2;
    }
}

void nofWinegrower::workAborted()
{
    nofFarmhand::workAborted();
    // inform the grapefield, so it can wither, when we harvest it
    if(harvest && state == State::Work)
        world->getSpecObj<noGrapefield>(pos)->abortHarvesting();
}

void nofWinegrower::walkingStarted()
{
    // When walking to a grapefield, harvest it or plant a new one
    harvest = (world->getNO(dest)->getType() == NodalObjectType::Grapefield);

    if(!harvest)
        workplace->consumeWares();
}

void nofWinegrower::drawWalkingWithWare(DrawPoint drawPt)
{
    drawWalking(drawPt, "wine_bobs", bobIndex[BobTypes::WINEGROWER_WALKING_WITH_FULL_BASKET]);
}

void nofWinegrower::drawOtherStates(DrawPoint drawPt)
{
    if(state == State::WalkToWorkpoint)
    {
        if(harvest) // Go to harvest grapes
            drawWalking(drawPt, "wine_bobs", bobIndex[BobTypes::WINEGROWER_WALKING_WITH_EMPTY_BASKET]);
        else // Draw normal walking
            drawWalking(drawPt);
    }
}

bool nofWinegrower::areWaresAvailable() const
{
    // wine grower doesn't need wares for harvesting!
    // -> Wares are considered when calling getPointQuality!
    return true;
}
