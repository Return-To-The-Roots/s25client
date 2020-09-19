// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "nofTradeDonkey.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "buildings/nobBaseWarehouse.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "variant.h"
#include "world/GameWorldGame.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"

nofTradeDonkey::nofTradeDonkey(const MapPoint pos, const unsigned char player,
                               const boost::variant<GoodType, Job>& what)
    : noFigure(holds_alternative<Job>(what) ? boost::get<Job>(what) : JOB_PACKDONKEY, pos, player), successor(nullptr)
{
    if(holds_alternative<GoodType>(what))
        gt = boost::get<GoodType>(what);
}

nofTradeDonkey::nofTradeDonkey(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), successor(sgd.PopObject<nofTradeDonkey>(GOT_NOF_TRADEDONKEY)),
      gt(sgd.PopOptionalEnum<GoodType>())
{
    if(sgd.GetGameDataVersion() < 6)
    {
        std::vector<uint8_t> next_dirs_raw;
        sgd.PopContainer(next_dirs_raw);
        for(const auto dir : next_dirs_raw)
            next_dirs.push_back(dir == 0xDD ? TradeDirection::ReachedGoal : TradeDirection(dir));
    } else
        sgd.PopContainer(next_dirs);
}

void nofTradeDonkey::Serialize(SerializedGameData& sgd) const
{
    Serialize_noFigure(sgd);

    sgd.PushObject(successor, true);
    sgd.PushOptionalEnum<uint8_t>(gt);
    sgd.PushContainer(next_dirs);
}

void nofTradeDonkey::GoalReached()
{
    RTTR_Assert(dynamic_cast<nobBaseWarehouse*>(gwg->GetNO(pos)));
    successor = nullptr;
    auto* wh = static_cast<nobBaseWarehouse*>(gwg->GetNO(pos));
    GamePlayer& whOwner = gwg->GetPlayer(wh->GetPlayer());

    if(gt)
    {
        Inventory goods;
        goods.goods[*gt] = 1;
        wh->AddGoods(goods, true);
    }

    whOwner.IncreaseInventoryJob(this->GetJobType(), 1);
    gwg->RemoveFigure(pos, this);
    wh->AddFigure(this);
}

void nofTradeDonkey::Walked()
{
    if(next_dirs.empty())
    {
        CancelTradeCaravane();
        WanderFailedTrade();
    }

    TradeDirection nextDir = GetNextDir();
    if(successor)
        successor->AddNextDir(nextDir);
    // Are we now at the goal?
    if(nextDir == TradeDirection::ReachedGoal)
    {
        // Does target still exist?
        noBase* nob = gwg->GetNO(pos);
        if(nob->GetType() == NOP_BUILDING
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
    if(job_ == JOB_PACKDONKEY)
    {
        const unsigned ani_step = CalcWalkAnimationFrame();

        drawPt += CalcFigurRelative();

        // Läuft normal mit oder ohne Ware

        // Esel
        LOADER.getDonkeySprite(GetCurMoveDir(), ani_step).draw(drawPt);

        if(gt)
        {
            // Ware im Korb zeichnen
            LOADER.GetMapImageN(2350 + *gt)->DrawFull(drawPt + WARE_POS_DONKEY[GetCurMoveDir()][ani_step]);
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
