#include <stdafx.h>
#include "main.h"
#include "nofTradeLeader.h"

nofTradeLeader::nofTradeLeader(const MapCoord x, const MapCoord y,const unsigned char player,const TradeRoute& tr) 
: noFigure(JOB_HELPER,x,y,player), tr(tr)
{
}

nofTradeLeader::nofTradeLeader(SerializedGameData * sgd, const unsigned obj_id)
: noFigure(sgd,obj_id), tr(NULL,Point<MapCoord>(0,0),Point<MapCoord>(0,0))
{
}


void nofTradeLeader::Serialize(SerializedGameData *sgd) const 
{
	Serialize_noFigure(sgd);
}

void nofTradeLeader::GoalReached()
{
}

void nofTradeLeader::Walked()
{
	// Continue walking in the next direction
	StartWalking(tr.GetNextDir());
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
