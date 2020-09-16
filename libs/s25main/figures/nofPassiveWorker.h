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

    GO_Type GetGOT() const override { return GOT_NOF_PASSIVEWORKER; }
};
