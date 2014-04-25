// $Id: nofScout_LookoutTower.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef NOF_SCOUT_LOOKOUTTOWER
#define NOF_SCOUT_LOOKOUTTOWER

#include "nofBuildingWorker.h"

/// Späher, der in einem Spähturm "arbeitet"
class nofScout_LookoutTower : public nofBuildingWorker
{
    private:

        /// Funktionen, die nur von der Basisklasse (noFigure) aufgerufen werden, wenn man gelaufen ist
        void WalkedDerived();
        /// Malt den Arbeiter beim Arbeiten
        void DrawWorking(int x, int y);
        /// Fragt die abgeleitete Klasse um die ID in JOBS.BOB, wenn der Beruf Waren rausträgt (bzw rein)
        unsigned short GetCarryID() const { return 0; }
        /// Arbeit musste wegen Arbeitsplatzverlust abgebrochen werden
        void WorkAborted();
        /// Arbeitsplatz wurde erreicht
        void WorkplaceReached();


    public:

        nofScout_LookoutTower(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace);
        nofScout_LookoutTower(SerializedGameData* sgd, const unsigned obj_id);

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofScout_LookoutTower(SerializedGameData* sgd) const;
    public:     void Serialize(SerializedGameData* sgd) const { Serialize_nofScout_LookoutTower(sgd); }

        GO_Type GetGOT() const { return GOT_NOF_SCOUT_LOOKOUTTOWER; }

        void HandleDerivedEvent(const unsigned int id);
};


#endif //!NOF_SCOUT_LOOKOUTTOWER
