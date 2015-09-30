// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef NOF_PASSIVEWORKER_H_
#define NOF_PASSIVEWORKER_H_

#include "figures/noFigure.h"

class noFlag;

/// Arbeiter, der keine Arbeiten verrichtet, sondern nur entsprechend dem Beruf gezeichnet werden muss
/// und z.B. zum Auslagern benutzt wird ober beim Abbrennen eines Lagerhaus
class nofPassiveWorker : public noFigure
{
    private:

        /// von noFigure aufgerufen
        void Walked(); // wenn man gelaufen ist
        void GoalReached(); // wenn das Ziel erreicht wurde
        void AbrogateWorkplace();
        void HandleDerivedEvent(const unsigned int id); /// Für alle restlichen Events, die nicht von noFigure behandelt werden

    public:

        nofPassiveWorker(const Job job, const MapPoint pt, const unsigned char player, noRoadNode* goal);
        nofPassiveWorker(SerializedGameData& sgd, const unsigned obj_id);

        /// Zeichnen
        void Draw(int x, int y);

        GO_Type GetGOT() const
        {
            return GOT_NOF_PASSIVEWORKER;
        }
};


#endif
