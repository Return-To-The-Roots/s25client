// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofTradeDonkey.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "buildings/nobBaseWarehouse.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "world/GameWorld.h"
#include "gameData/BuildingProperties.h"
#include "gameData/JobConsts.h"

nofTradeDonkey::nofTradeDonkey(const MapPoint pos, const unsigned char player,
                               const boost_variant2<GoodType, Job>& what)
    : noFigure(holds_alternative<Job>(what) ? get<Job>(what) : Job::PackDonkey, pos, player), successor(nullptr)
{
    if(holds_alternative<GoodType>(what))
        gt = get<GoodType>(what);
}

nofTradeDonkey::nofTradeDonkey(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), successor(sgd.popObject<nofTradeDonkey>(GO_Type::NofTradedonkey)),
      gt(sgd.popOptionalEnum<GoodType>())
{
    if(sgd.getGameDataVersion() < 6)
    {
        std::vector<uint8_t> nextDirsRaw;
        sgd.popContainer(nextDirsRaw);
        for(const auto dir : nextDirsRaw)
            next_dirs.push_back(dir == 0xDD ? TradeDirection::ReachedGoal : TradeDirection(dir));
    } else
        helpers::popContainer(sgd, next_dirs);
}

void nofTradeDonkey::serialize(SerializedGameData& sgd) const
{
    noFigure::serialize(sgd);

    sgd.pushObject(successor, true);
    sgd.pushOptionalEnum<uint8_t>(gt);
    helpers::pushContainer(sgd, next_dirs);
}

void nofTradeDonkey::goalReached()
{
    RTTR_Assert(dynamic_cast<nobBaseWarehouse*>(world->getNO(pos)));
    successor = nullptr;
    auto* wh = static_cast<nobBaseWarehouse*>(world->getNO(pos));
    GamePlayer& whOwner = world->getPlayer(wh->getPlayer());

    if(gt)
        wh->addToInventory(GoodCounts::make(*gt, 1), true);

    whOwner.increaseInventoryJob(this->getJobType(), 1);
    if(hasArmor_)
        whOwner.increaseInventoryJob(jobEnumToAmoredSoldierEnum(this->getJobType()), 1);
    wh->addFigure(world->removeFigure(pos, *this));
}

void nofTradeDonkey::walked()
{
    if(next_dirs.empty())
    {
        cancelTradeCaravane();
        wanderFailedTrade();
        return;
    }

    TradeDirection nextDir = getNextDir();
    if(successor)
        successor->addNextDir(nextDir);
    // Are we now at the goal?
    if(nextDir == TradeDirection::ReachedGoal)
    {
        // Does target still exist?
        noBase* nob = world->getNO(pos);
        if(nob->getType() == NodalObjectType::Building
           && BuildingProperties::isWareHouse(static_cast<noBuilding*>(nob)->getBuildingType()))
            goalReached();
        else
        {
            cancelTradeCaravane();
            wanderFailedTrade();
        }
    } else
        startWalking(toDirection(nextDir));
}

void nofTradeDonkey::handleDerivedEvent(const unsigned /*id*/) {}
void nofTradeDonkey::abrogateWorkplace() {}

void nofTradeDonkey::abrogate()
{
    cancelTradeCaravane();
    // The cargo was already removed all inventories when the caravan started so just drop it.
    gt.reset();
    noFigure::abrogate();
    RTTR_Assert(!successor);
}

void nofTradeDonkey::draw(DrawPoint drawPt)
{
    if(job_ == Job::PackDonkey)
    {
        const unsigned aniStep = calcWalkAnimationFrame();

        drawPt += calcFigurRelative();

        // Läuft normal mit oder ohne Ware

        // Esel
        LOADER.getDonkeySprite(getCurMoveDir(), aniStep).draw(drawPt);

        if(gt)
        {
            // Ware im Korb zeichnen
            LOADER.getWareDonkeyTex(*gt)->drawFull(drawPt + WARE_POS_DONKEY[getCurMoveDir()][aniStep]);
        }
    } else
        drawWalking(drawPt);
}

void nofTradeDonkey::lostWork() {}

void nofTradeDonkey::cancelTradeCaravane()
{
    next_dirs.clear();
    if(successor)
    {
        successor->cancelTradeCaravane();
        successor = nullptr;
    }
}
