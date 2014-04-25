// $Id: nofScout_Free.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef NOF_SCOUT_FREE_H_
#define NOF_SCOUT_FREE_H_

#include "nofFlagWorker.h"
#include "MapConsts.h"

/// Frei herumlaufender Erkunder
class nofScout_Free : public nofFlagWorker
{
        /// Nächster Punkt, wo der Späher hingehen soll
        MapCoord next_x, next_y;
        /// Weg, weit weit er noch laufen soll
        unsigned rest_way;

    private:

        void GoalReached();
        void Walked();
        void HandleDerivedEvent(const unsigned int id);

        /// Erkundet (quasi ein Umherirren)
        void Scout();

        /// Sucht einen neuen Zielpunkt und geht zu diesen
        void GoToNewNode();

        /// Gibt den Sichtradius dieser Figur zurück (0, falls nicht-spähend)
        virtual unsigned GetVisualRange() const;

    public:

        nofScout_Free(const MapCoord x, const MapCoord y, const unsigned char player, noRoadNode* goal);
        nofScout_Free(SerializedGameData* sgd, const unsigned obj_id);

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofScout_Free(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofScout_Free(sgd); }

        GO_Type GetGOT() const { return GOT_NOF_SCOUT_FREE; }

        void Draw(int x, int y);

        /// Wird aufgerufen, wenn die Flagge abgerissen wurde
        void LostWork();

        ///// Ist der Erkunder am erkunden (Sichtbereich um ihn herum)?
        //bool IsScouting() const { return (state == STATE_SCOUT_SCOUTING || state == STATE_GOTOFLAG); }
};


#endif //!NOF_SCOUT_FREE_H_
