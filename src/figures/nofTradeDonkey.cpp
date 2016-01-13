// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h"
#include "nofTradeDonkey.h"
#include "GameClient.h"
#include "gameData/JobConsts.h"
#include "buildings/nobBaseWarehouse.h"
#include "SerializedGameData.h"
#include "gameData/GameConsts.h"

nofTradeDonkey::nofTradeDonkey(const MapPoint pos, const unsigned char player, const GoodType gt, const Job job)
    : noFigure((job != JOB_NOTHING) ? job : JOB_PACKDONKEY, pos, player), successor(NULL), gt(gt)
{
}

nofTradeDonkey::nofTradeDonkey(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id),
      successor(sgd.PopObject<nofTradeDonkey>(GOT_NOF_TRADEDONKEY)),
      gt(GoodType(sgd.PopUnsignedChar())),
      next_dirs(sgd.PopContainer(next_dirs))
{}


void nofTradeDonkey::Serialize(SerializedGameData& sgd) const
{
    Serialize_noFigure(sgd);

    sgd.PushObject(successor, true);
    sgd.PushUnsignedChar(static_cast<unsigned char>(gt));
    sgd.PushContainer(next_dirs);
}

void nofTradeDonkey::GoalReached()
{
    assert(dynamic_cast<nobBaseWarehouse*>(gwg->GetNO(pos)));
    successor = NULL;
    nobBaseWarehouse* wh = static_cast<nobBaseWarehouse*>(gwg->GetNO(pos));
    GameClientPlayer& player = gwg->GetPlayer(wh->GetPlayer());

    if(gt != GD_NOTHING)
    {
        Goods goods;
        goods.goods[gt] = 1;
        wh->AddGoods(goods);
        player.IncreaseInventoryWare(gt, 1);
    }

    player.IncreaseInventoryJob(this->GetJobType(), 1);
    gwg->RemoveFigure(this, pos);
    wh->AddFigure(this);
}

void nofTradeDonkey::Walked()
{
    if(next_dirs.empty())
        return;

    unsigned char nextDir = GetNextDir();
    if(successor)
        successor->AddNextDir(nextDir);
    // Are we now at the goal?
    if(nextDir == REACHED_GOAL)
    {
        // Does target still exist?
        noBase* nob = gwg->GetNO(pos);
        if(nob->GetType() == NOP_BUILDING && static_cast<noBuilding*>(nob)->IsWarehouse())
            GoalReached();
        else
        {
            CancelTradeCaravane();
            WanderFailedTrade();
        }
    }
    else if(nextDir != INVALID_DIR)
        StartWalking(nextDir);
    else
    {
        CancelTradeCaravane();
        WanderFailedTrade();
    }
}

void nofTradeDonkey::HandleDerivedEvent(const unsigned int id)
{
}
void nofTradeDonkey::AbrogateWorkplace()
{
}

void nofTradeDonkey::Draw(int x, int y)
{
    if(job_ == JOB_PACKDONKEY)
    {
        // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
        // Wenn event = 0, dann sind wir mittem auf dem Weg angehalten!
        unsigned ani_step = GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

        Point<int> realPos = Point<int>(x, y) + CalcFigurRelative();

        // Läuft normal mit oder ohne Ware

        // Esel
        LOADER.GetMapImageN(2000 + ((GetCurMoveDir() + 3) % 6) * 8 + ani_step)->Draw(realPos.x, realPos.y);
        // Schatten des Esels
        LOADER.GetMapImageN(2048 + GetCurMoveDir() % 3)->Draw(realPos.x, realPos.y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);

        if(gt != GD_NOTHING)
        {
            // Ware im Korb zeichnen
            LOADER.GetMapImageN(2350 + gt)->Draw(realPos.x + WARE_POS_DONKEY[GetCurMoveDir() * 16 + ani_step * 2], realPos.y + WARE_POS_DONKEY[GetCurMoveDir() * 16 + ani_step * 2 + 1]);
        }
    }
    else
        DrawWalking(x, y);
}

void nofTradeDonkey::LostWork()
{
}


void nofTradeDonkey::CancelTradeCaravane()
{
    next_dirs.clear();
    next_dirs.push_back(INVALID_DIR);
    if(successor)
    {
        successor->CancelTradeCaravane();
        successor = NULL;
    }
}
