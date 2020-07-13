// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "nofPassiveWorker.h"
#include "buildings/nobBaseWarehouse.h"
#include "world/GameWorldGame.h"
class SerializedGameData;
class noRoadNode;

nofPassiveWorker::nofPassiveWorker(const Job job, const MapPoint pos, const unsigned char player, noRoadNode* goal)
    : noFigure(job, pos, player, goal)
{}

nofPassiveWorker::nofPassiveWorker(SerializedGameData& sgd, const unsigned obj_id) : noFigure(sgd, obj_id) {}

/// von noFigure aufgerufen
// wenn man gelaufen ist
void nofPassiveWorker::Walked() {}

// wenn das Ziel erreicht wurde
void nofPassiveWorker::GoalReached()
{
    // Mich hier einquartieren
    gwg->RemoveFigure(pos, this);
    auto* wh = gwg->GetSpecObj<nobBaseWarehouse>(pos);
    RTTR_Assert(wh);
    wh->AddFigure(this);
}

void nofPassiveWorker::AbrogateWorkplace() {}

/// Zeichnen
void nofPassiveWorker::Draw(DrawPoint drawPt)
{
    DrawWalking(drawPt);
}

/// Für alle restlichen Events, die nicht von noFigure behandelt werden
void nofPassiveWorker::HandleDerivedEvent(const unsigned /*id*/) {}
