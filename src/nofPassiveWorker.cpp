// $Id: nofPassiveWorker.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "nofPassiveWorker.h"
#include "nobBaseWarehouse.h"
#include "GameWorld.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofPassiveWorker::nofPassiveWorker(const Job job, const unsigned short x, const unsigned short y, const unsigned char player, noRoadNode* goal)
    : noFigure(job, x, y, player, goal)
{
}

nofPassiveWorker::nofPassiveWorker(SerializedGameData* sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id)
{
}

/// von noFigure aufgerufen
// wenn man gelaufen ist
void nofPassiveWorker::Walked()
{
}

// wenn das Ziel erreicht wurde
void nofPassiveWorker::GoalReached()
{
    // Mich hier einquartieren
    gwg->RemoveFigure(this, x, y);
    nobBaseWarehouse* wh = gwg->GetSpecObj<nobBaseWarehouse>(x, y);
    assert(wh);
    wh->AddFigure(this);

}

void nofPassiveWorker::AbrogateWorkplace()
{
}


/// Zeichnen
void nofPassiveWorker::Draw(int x, int y)
{
    DrawWalking(x, y);
}

/// Für alle restlichen Events, die nicht von noFigure behandelt werden
void nofPassiveWorker::HandleDerivedEvent(const unsigned int id)
{

}
