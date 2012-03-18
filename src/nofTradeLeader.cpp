
#include "main.h"
#include "nofTradeLeader.h"
#include "nofTradeDonkey.h"
#include "GameWorld.h"
#include "nobBaseWarehouse.h"
#include "SerializedGameData.h"

nofTradeLeader::nofTradeLeader(const MapCoord x, const MapCoord y,const unsigned char player,const TradeRoute& tr, const Point<MapCoord>  start,const Point<MapCoord> goal) 
: noFigure(JOB_HELPER,x,y,player), tr(tr), successor(NULL), start(start), goal(goal)
{
}

nofTradeLeader::nofTradeLeader(SerializedGameData * sgd, const unsigned obj_id)
: noFigure(sgd,obj_id),
tr(sgd,gwg,player), 
successor(sgd->PopObject<nofTradeDonkey>(GOT_NOF_TRADEDONKEY)),
start(sgd->PopMapPoint()),
goal(sgd->PopMapPoint())

{
}


void nofTradeLeader::Serialize(SerializedGameData *sgd) const 
{
	Serialize_noFigure(sgd);
	
	tr.Serialize(sgd);
	
	sgd->PushObject(successor,true);
	sgd->PushMapPoint(start);
	sgd->PushMapPoint(goal);
}

void nofTradeLeader::GoalReached()
{
}

void nofTradeLeader::Walked()
{
	// Exception handling: goal destroyed or sth. like this
	bool invalid_goal = false;

	noBase * nob = gwg->GetNO(goal.x,goal.y);
	if(nob->GetType() != NOP_BUILDING)
		invalid_goal = true;
	if(!static_cast<noBuilding*>(nob)->IsWarehouse())
		invalid_goal = true;

	unsigned char next_dir;
	if(invalid_goal)
	{
		TryToGoHome();
		next_dir = dir;
	}
	else
	{

		next_dir = tr.GetNextDir();
		// Are we now at the goal?
		if(next_dir == REACHED_GOAL)
		{
			gwg->GetPlayer(static_cast<nobBaseWarehouse*>(nob)->GetPlayer())
				->IncreaseInventoryJob(this->GetJobType(),1);
			gwg->RemoveFigure(this,x,y);
			static_cast<nobBaseWarehouse*>(nob)->AddFigure(this);
		}
		else
			if(next_dir!= NO_PATH)
				StartWalking(next_dir);
			else
			{
				TryToGoHome();
				next_dir = dir;
			}

	}

	if(successor)
		successor->AddNextDir(next_dir);
}

void nofTradeLeader::HandleDerivedEvent(const unsigned int id)
{
}
void nofTradeLeader::AbrogateWorkplace()
{
}

void nofTradeLeader::Draw(int x, int y)
{
	DrawWalking(x,y);
}

void nofTradeLeader::LostWork()
{
}

/// Tries to go to the home ware house, otherwise start wandering
void nofTradeLeader::TryToGoHome()
{
	goal = start;
	// Find a way back home
	tr.AssignNewGoal(gwg->GetPointA(start,4));
	if(!tr.IsValid())
		CancelTradeCaravane();
	else
		StartWalking(tr.GetNextDir());
}

/// Start wandering and informs the other successors about this
void nofTradeLeader::CancelTradeCaravane()
{
	if(successor) 
	{
		successor->CancelTradeCaravane();
		successor = NULL;
	}
	StartWandering();
	Wander();
}