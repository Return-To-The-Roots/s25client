// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofPassiveWorker.h"
#include "buildings/nobBaseWarehouse.h"
#include "world/GameWorld.h"
class SerializedGameData;
class noRoadNode;

nofPassiveWorker::nofPassiveWorker(const Job job, const MapPoint pos, const unsigned char player, noRoadNode* goal)
    : nofArmored(job, pos, player, goal)
{}

nofPassiveWorker::nofPassiveWorker(SerializedGameData& sgd, const unsigned obj_id) : nofArmored(sgd, obj_id) {}

/// von noFigure aufgerufen
// wenn man gelaufen ist
void nofPassiveWorker::Walked() {}

// wenn das Ziel erreicht wurde
void nofPassiveWorker::GoalReached()
{
    // Mich hier einquartieren
    auto* wh = world->GetSpecObj<nobBaseWarehouse>(pos);
    RTTR_Assert(wh);
    wh->AddFigure(world->RemoveFigure(pos, *this));
}

void nofPassiveWorker::AbrogateWorkplace() {}

/// Zeichnen
void nofPassiveWorker::Draw(DrawPoint drawPt)
{
    DrawWalking(drawPt);
    DrawArmorWalking(drawPt);
}

/// Für alle restlichen Events, die nicht von noFigure behandelt werden
void nofPassiveWorker::HandleDerivedEvent(const unsigned /*id*/) {}
