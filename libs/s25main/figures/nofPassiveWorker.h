// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/noFigure.h"

class SerializedGameData;
class noRoadNode;

/// Arbeiter, der keine Arbeiten verrichtet, sondern nur entsprechend dem Beruf gezeichnet werden muss
/// und z.B. zum Auslagern benutzt wird ober beim Abbrennen eines Lagerhaus
class nofPassiveWorker : public noFigure
{
private:
    /// von noFigure aufgerufen
    void Walked() override;      // wenn man gelaufen ist
    void GoalReached() override; // wenn das Ziel erreicht wurde
    void AbrogateWorkplace() override;
    void
    HandleDerivedEvent(unsigned id) override; /// Für alle restlichen Events, die nicht von noFigure behandelt werden

public:
    nofPassiveWorker(Job job, MapPoint pos, unsigned char player, noRoadNode* goal);
    nofPassiveWorker(SerializedGameData& sgd, unsigned obj_id);

    /// Zeichnen
    void Draw(DrawPoint drawPt) override;

    GO_Type GetGOT() const final { return GO_Type::NofPassiveworker; }
};
