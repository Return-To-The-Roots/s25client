
#include "defines.h"
#include "nofTradeLeader.h"
#include "nofTradeDonkey.h"
#include "GameWorldGame.h"
#include "buildings/nobBaseWarehouse.h"
#include "SerializedGameData.h"
#include "GameClientPlayer.h"

nofTradeLeader::nofTradeLeader(const MapPoint pos, const unsigned char player, const TradeRoute& tr, const MapPoint  start, const MapPoint goal)
    : noFigure(JOB_HELPER, pos, player), tr(tr), successor(NULL), start(start), goal_(goal), fails(0)
{
}

nofTradeLeader::nofTradeLeader(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id),
      tr(sgd, gwg, player),
      successor(sgd.PopObject<nofTradeDonkey>(GOT_NOF_TRADEDONKEY)),
      start(sgd.PopMapPoint()),
      goal_(sgd.PopMapPoint()),
      fails(sgd.PopUnsignedChar())
{
}


void nofTradeLeader::Serialize(SerializedGameData& sgd) const
{
    Serialize_noFigure(sgd);

    tr.Serialize(sgd);

    sgd.PushObject(successor, true);
    sgd.PushMapPoint(start);
    sgd.PushMapPoint(goal_);
    sgd.PushUnsignedChar(fails);
}

void nofTradeLeader::GoalReached()
{
}

void nofTradeLeader::Walked()
{
    // Exception handling: goal destroyed or sth. like this
    if(fails > 1)
    {
        WanderFailedTrade();
        return;
    }
    bool invalid_goal = false;

    noBase* nob = gwg->GetNO(goal_);
    if(nob->GetType() != NOP_BUILDING)
        invalid_goal = true;
    if(!static_cast<noBuilding*>(nob)->IsWarehouse())
        invalid_goal = true;

    unsigned char next_dir;
    if(invalid_goal)
    {
        TryToGoHome();
        next_dir = tr.GetNextDir();
        StartWalking(next_dir);
    }
    else
    {

        next_dir = tr.GetNextDir();
        // Are we now at the goal?
        if(next_dir == REACHED_GOAL)
        {
            gwg->GetPlayer(static_cast<nobBaseWarehouse*>(nob)->GetPlayer()).IncreaseInventoryJob(this->GetJobType(), 1);
            gwg->RemoveFigure(this, pos);
            static_cast<nobBaseWarehouse*>(nob)->AddFigure(this);
        }
        else if(next_dir != NO_PATH)
            StartWalking(next_dir);
        else
        {
            TryToGoHome();
            next_dir = tr.GetNextDir();
            if(next_dir == NO_PATH)
            {
                CancelTradeCaravane();
                next_dir = GetCurMoveDir();
                //StartWanderingFailedTrade();
            }
            StartWalking(next_dir);
            //next_dir=tr.GetNextDir();
            //next_dir = dir;
        }

    }

    //if(successor&&next_dir!=NO_PATH)
    if(successor)
    {
        successor->AddNextDir(next_dir);
    }
}

void nofTradeLeader::HandleDerivedEvent(const unsigned int id)
{
}
void nofTradeLeader::AbrogateWorkplace()
{
}

void nofTradeLeader::Draw(int x, int y)
{
    DrawWalking(x, y);
}

void nofTradeLeader::LostWork()
{
}

/// Tries to go to the home ware house, otherwise start wandering
void nofTradeLeader::TryToGoHome()
{
    fails++;
    //if(fails>1)
    //CancelTradeCaravane();

    // Find a way back home
    tr.AssignNewGoal(gwg->GetNeighbour(start, 4), this->GetPos());
    if(!tr.IsValid())
        CancelTradeCaravane();

}

/// Start wandering and informs the other successors about this
void nofTradeLeader::CancelTradeCaravane()
{
    if(successor)
    {
        successor->CancelTradeCaravane();
        successor = NULL;
    }
    //StartWanderingFailedTrade();
    //DieFailedTrade();
    //WanderFailedTrade();
}
