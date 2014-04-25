// $Id: nofCharburner.h 9357 2014-04-25 15:35:25Z FloSoft $
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

#ifndef NOF_CHARBURNER_H_
#define NOF_CHARBURNER_H_

#include "nofFarmhand.h"

class nofCharburner : public nofFarmhand
{
        /// Is he harvesting a charburner pile (or planting?)
        bool harvest;
        /// If stacking wood pile: Determines which ware he carries (wood or grain?)
        enum WareType
        {
            WT_WOOD,
            WT_GRAIN
        } wt;
    private:

        /// Malt den Arbeiter beim Arbeiten
        void DrawWorking(int x, int y);
        /// Fragt die abgeleitete Klasse um die ID in JOBS.BOB, wenn der Beruf Waren rausträgt (bzw rein)
        unsigned short GetCarryID() const;

        /// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
        void WorkStarted();
        /// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
        void WorkFinished();

        /// Returns the quality of this working point or determines if the worker can work here at all
        PointQuality GetPointQuality(const MapCoord x, const MapCoord y);

        /// Inform derived class about the start of the whole working process (at the beginning when walking out of the house)
        void WalkingStarted();

        /// Draws the figure while returning home / entering the building (often carrying wares)
        void DrawReturnStates(const int x, const int y);
        /// Draws the charburner while walking
        /// (overriding standard method of nofFarmhand)
        void DrawOtherStates(const int x, const int y);

    public:

        nofCharburner(const MapCoord x, MapCoord y, const unsigned char player, nobUsual* workplace);
        nofCharburner(SerializedGameData* sgd, const unsigned obj_id);

        void Serialize(SerializedGameData* sgd) const;

        GO_Type GetGOT() const { return GOT_NOF_CHARBURNER; }

};

#endif
