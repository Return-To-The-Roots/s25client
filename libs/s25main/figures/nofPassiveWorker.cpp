// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofPassiveWorker.h"
#include "buildings/nobBaseWarehouse.h"
#include "world/GameWorld.h"
class SerializedGameData;
class noRoadNode;

nofPassiveWorker::nofPassiveWorker(const Job job, const MapPoint pos, const unsigned char player, noRoadNode* goal)
    : noFigure(job, pos, player, goal)
{}

nofPassiveWorker::nofPassiveWorker(SerializedGameData& sgd, const unsigned obj_id) : noFigure(sgd, obj_id) {}

/// von noFigure aufgerufen
// wenn man gelaufen ist
void nofPassiveWorker::walked() {}

// wenn das Ziel erreicht wurde
void nofPassiveWorker::goalReached()
{
    // Mich hier einquartieren
    auto* wh = world->getSpecObj<nobBaseWarehouse>(pos);
    RTTR_Assert(wh);
    wh->addFigure(world->RemoveFigure(pos, *this));
}

void nofPassiveWorker::abrogateWorkplace() {}

/// Zeichnen
void nofPassiveWorker::draw(DrawPoint drawPt)
{
    drawWalking(drawPt);
}

/// Für alle restlichen Events, die nicht von noFigure behandelt werden
void nofPassiveWorker::handleDerivedEvent(const unsigned /*id*/) {}
