// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofTradeDonkey.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "buildings/nobBaseWarehouse.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "variant.h"
#include "world/GameWorld.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"

nofTradeDonkey::nofTradeDonkey(const MapPoint pos, const unsigned char player,
                               const boost::variant<GoodType, Job>& what)
    : noFigure(holds_alternative<Job>(what) ? boost::get<Job>(what) : Job::PackDonkey, pos, player), successor(nullptr)
{
    if(holds_alternative<GoodType>(what))
        gt = boost::get<GoodType>(what);
}

nofTradeDonkey::nofTradeDonkey(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), successor(sgd.PopObject<nofTradeDonkey>(GO_Type::NofTradedonkey)),
      gt(sgd.PopOptionalEnum<GoodType>())
{
    if(sgd.GetGameDataVersion() < 6)
    {
        std::vector<uint8_t> next_dirs_raw;
        sgd.PopContainer(next_dirs_raw);
        for(const auto dir : next_dirs_raw)
            next_dirs.push_back(dir == 0xDD ? TradeDirection::ReachedGoal : TradeDirection(dir));
    } else
        helpers::popContainer(sgd, next_dirs);
}

void nofTradeDonkey::Serialize(SerializedGameData& sgd) const
{
    noFigure::Serialize(sgd);

    sgd.PushObject(successor, true);
    sgd.PushOptionalEnum<uint8_t>(gt);
    helpers::pushContainer(sgd, next_dirs);
}

void nofTradeDonkey::GoalReached()
{
    RTTR_Assert(dynamic_cast<nobBaseWarehouse*>(world->GetNO(pos)));
    successor = nullptr;
    auto* wh = static_cast<nobBaseWarehouse*>(world->GetNO(pos));
    GamePlayer& whOwner = world->GetPlayer(wh->GetPlayer());

    if(gt)
    {
        Inventory goods;
        goods.goods[*gt] = 1;
        wh->AddGoods(goods, true);
    }

    whOwner.IncreaseInventoryJob(this->GetJobType(), 1);
    wh->AddFigure(world->RemoveFigure(pos, *this));
}

void nofTradeDonkey::Walked()
{
    if(next_dirs.empty())
    {
        CancelTradeCaravane();
        WanderFailedTrade();
        return;
    }

    TradeDirection nextDir = GetNextDir();
    if(successor)
        successor->AddNextDir(nextDir);
    // Are we now at the goal?
    if(nextDir == TradeDirection::ReachedGoal)
    {
        // Does target still exist?
        noBase* nob = world->GetNO(pos);
        if(nob->GetType() == NodalObjectType::Building
           && BuildingProperties::IsWareHouse(static_cast<noBuilding*>(nob)->GetBuildingType()))
            GoalReached();
        else
        {
            CancelTradeCaravane();
            WanderFailedTrade();
        }
    } else
        StartWalking(toDirection(nextDir));
}

void nofTradeDonkey::HandleDerivedEvent(const unsigned /*id*/) {}
void nofTradeDonkey::AbrogateWorkplace() {}

void nofTradeDonkey::Draw(DrawPoint drawPt)
{
    if(job_ == Job::PackDonkey)
    {
        const unsigned ani_step = CalcWalkAnimationFrame();

        drawPt += CalcFigurRelative();

        // Läuft normal mit oder ohne Ware

        // Esel
        LOADER.getDonkeySprite(GetCurMoveDir(), ani_step).draw(drawPt);

        if(gt)
        {
            // Ware im Korb zeichnen
            LOADER.GetWareDonkeyTex(*gt)->DrawFull(drawPt + WARE_POS_DONKEY[GetCurMoveDir()][ani_step]);
        }
    } else
        DrawWalking(drawPt);
}

void nofTradeDonkey::LostWork() {}

void nofTradeDonkey::CancelTradeCaravane()
{
    next_dirs.clear();
    if(successor)
    {
        successor->CancelTradeCaravane();
        successor = nullptr;
    }
}
