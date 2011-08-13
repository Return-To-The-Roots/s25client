#include <stdafx.h>
#include "main.h"
#include "nofTradeLeader.h"
#include "nofTradeDonkey.h"
#include "GameWorld.h"
#include "nobBaseWarehouse.h"

nofTradeLeader::nofTradeLeader(const MapCoord x, const MapCoord y,const unsigned char player,const TradeRoute& tr) 
: noFigure(JOB_HELPER,x,y,player), tr(tr), successor(NULL)
{
}

nofTradeLeader::nofTradeLeader(SerializedGameData * sgd, const unsigned obj_id)
: noFigure(sgd,obj_id), tr(NULL,Point<MapCoord>(0,0),Point<MapCoord>(0,0))
{
	 //todo
}


void nofTradeLeader::Serialize(SerializedGameData *sgd) const 
{
	Serialize_noFigure(sgd);
	 //todo
}

void nofTradeLeader::GoalReached()
{
}

void nofTradeLeader::Walked()
{
	unsigned char next_dir = tr.GetNextDir();
	// Are we now at the goal?
	if(next_dir == REACHED_GOAL)
	{
		noBase * nob = gwg->GetNO(x,y);
		if(nob->GetType() != NOP_BUILDING)
			return; //todo
		if(!static_cast<noBuilding*>(nob)->IsWarehouse())
			return; //todo

		gwg->GetPlayer(static_cast<nobBaseWarehouse*>(nob)->GetPlayer())
			->IncreaseInventoryJob(this->GetJobType(),1);
		gwg->RemoveFigure(this,x,y);
		static_cast<nobBaseWarehouse*>(nob)->AddFigure(this);
	}
	else
		StartWalking(next_dir);

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
