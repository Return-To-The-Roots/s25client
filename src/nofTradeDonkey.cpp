
#include "main.h"


#include "main.h"
#include "nofTradeDonkey.h"
#include "nofTradeLeader.h"
#include "GameClient.h"
#include "JobConsts.h"
#include "GameWorld.h"
#include "nobBaseWarehouse.h"
#include "SerializedGameData.h"

nofTradeDonkey::nofTradeDonkey(const MapCoord x, const MapCoord y, const unsigned char player,
                               nofTradeLeader* const leader, const GoodType gt, const Job job)
    : noFigure((job != JOB_NOTHING) ? job : JOB_PACKDONKEY, x, y, player), leader(leader), successor(NULL), gt(gt)
{
}

nofTradeDonkey::nofTradeDonkey(SerializedGameData* sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id),
      leader(sgd->PopObject<nofTradeLeader>(GOT_NOF_TRADELEADER)),
      successor(sgd->PopObject<nofTradeDonkey>(GOT_NOF_TRADEDONKEY)),
      gt(GoodType(sgd->PopUnsignedChar())),
      next_dirs(sgd->PopUnsignedInt())
{
    for(unsigned i = 0; i < next_dirs.size(); ++i)
        next_dirs[i] = sgd->PopUnsignedChar();
}


void nofTradeDonkey::Serialize(SerializedGameData* sgd) const
{
    Serialize_noFigure(sgd);

    sgd->PushObject(leader, true);
    sgd->PushObject(successor, true);
    sgd->PushUnsignedChar(static_cast<unsigned char>(gt));
    sgd->PushUnsignedInt(next_dirs.size());

    for(unsigned i = 0; i < next_dirs.size(); ++i)
        sgd->PushUnsignedChar(next_dirs[i]);
}

void nofTradeDonkey::GoalReached()
{
}

void nofTradeDonkey::Walked()
{
    if(next_dirs.size() < 1)
    {
        //WanderFailedTrade();
        gwg->RemoveFigure(this, x, y);
        return;
    }
    unsigned char next_dir = GetNextDir();
    // Are we now at the goal?
    if(next_dir == REACHED_GOAL)
    {
        noBase* nob = gwg->GetNO(x, y);
        bool invalid_goal = false;
        if(nob->GetType() != NOP_BUILDING)
            invalid_goal = true;
        else if(!static_cast<noBuilding*>(nob)->IsWarehouse())
            invalid_goal = true;

        if(invalid_goal)
        {
            CancelTradeCaravane();
            WanderFailedTrade();
            return;
        }

        gwg->GetPlayer(static_cast<nobBaseWarehouse*>(nob)->GetPlayer())
        ->IncreaseInventoryJob(this->GetJobType(), 1);
        gwg->RemoveFigure(this, x, y);
        static_cast<nobBaseWarehouse*>(nob)->AddFigure(this);

        // Add also our ware if we carry one
        if(gt != GD_NOTHING)
        {
            Goods goods;
            goods.goods[gt] = 1;
            static_cast<nobBaseWarehouse*>(nob)->AddGoods(goods);
            gwg->GetPlayer(static_cast<nobBaseWarehouse*>(nob)->GetPlayer())
            ->IncreaseInventoryWare(gt, 1);

        }
    }
    else
    {
        if(next_dir != NO_PATH)
            StartWalking(next_dir);
        else
        {
            CancelTradeCaravane();
            WanderFailedTrade();
        }
    }
    if(successor)
        successor->AddNextDir(next_dir);
}

void nofTradeDonkey::HandleDerivedEvent(const unsigned int id)
{
}
void nofTradeDonkey::AbrogateWorkplace()
{
}

void nofTradeDonkey::Draw(int x, int y)
{

    if(job == JOB_PACKDONKEY)
    {
        // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
        // Wenn event = 0, dann sind wir mittem auf dem Weg angehalten!
        unsigned ani_step = GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

        CalcFigurRelative(x, y);

        // Läuft normal mit oder ohne Ware

        // Esel
        LOADER.GetMapImageN(2000 + ((dir + 3) % 6) * 8 + ani_step)->Draw(x, y);
        // Schatten des Esels
        LOADER.GetMapImageN(2048 + dir % 3)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);

        if(gt != GD_NOTHING)
        {
            // Ware im Korb zeichnen
            LOADER.GetMapImageN(2350 + gt)
            ->Draw(x + WARE_POS_DONKEY[dir * 16 + ani_step * 2], y + WARE_POS_DONKEY[dir * 16 + ani_step * 2 + 1]);
        }
    }
    else
        DrawWalking(x, y);
}

void nofTradeDonkey::LostWork()
{
}


/// Start wandering and informs the other successors about this
void nofTradeDonkey::CancelTradeCaravane()
{
    next_dirs.clear();
    next_dirs.push_front(REACHED_GOAL);
    if(successor)
    {
        successor->CancelTradeCaravane();
        successor = NULL;
    }
    //DieFailedTrade();
    //StartWanderingFailedTrade();
}