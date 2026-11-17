// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
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
    void walked() override;      // wenn man gelaufen ist
    void goalReached() override; // wenn das Ziel erreicht wurde
    void abrogateWorkplace() override;
    void
    handleDerivedEvent(unsigned id) override; /// Für alle restlichen Events, die nicht von noFigure behandelt werden

public:
    nofPassiveWorker(Job job, MapPoint pos, unsigned char player, noRoadNode* goal);
    nofPassiveWorker(SerializedGameData& sgd, unsigned obj_id);
    nofPassiveWorker(const nofPassiveWorker&) = delete;

    /// Zeichnen
    void draw(DrawPoint drawPt) override;

    GO_Type getGOT() const final { return GO_Type::NofPassiveworker; }
};
