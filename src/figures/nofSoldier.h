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

#ifndef NOF_SOLDIER_H_
#define NOF_SOLDIER_H_

#include "figures/noFigure.h"
#include <boost/container/flat_set.hpp>

class nobBaseMilitary;

/// Basisklasse für alle Soldatentypen
class nofSoldier : public noFigure
{
    protected:

        /// Heimatgebäude, ist bei Soldaten aus HQs das HQ!
        nobBaseMilitary* building;
        /// Hitpoints
        unsigned char hitpoints;

    protected:

        /// Zeichnet den Soldaten beim ganz normalen Laufen
        void DrawSoldierWalking(int x, int y, bool waitingsoldier = false);

    private:
        /// wenn man beim Arbeitsplatz "kündigen" soll, man das Laufen zum Ziel unterbrechen muss (warum auch immer)
        void AbrogateWorkplace();

    public:

        nofSoldier(const MapPoint pt, const unsigned char player,
                   nobBaseMilitary* const goal, nobBaseMilitary* const home, const unsigned char rank);
        nofSoldier(const MapPoint pt, const unsigned char player,
                   nobBaseMilitary* const home, const unsigned char rank);
        nofSoldier(SerializedGameData& sgd, const unsigned obj_id);

        /// Aufräummethoden
    protected:  void Destroy_nofSoldier() { Destroy_noFigure(); }
    public:     void Destroy() { Destroy_nofSoldier(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_nofSoldier(SerializedGameData& sgd) const;
    public:     void Serialize(SerializedGameData& sgd) const { Serialize_nofSoldier(sgd); }

        /// Liefert Rang des Soldaten
        unsigned char GetRank() const { return (job_ - JOB_PRIVATE); }


};

/// Comparator to sort soldiers by rank (and ID for ties)
/// Template arguments defines the sort order: True for weak ones first, false for strong ones first
template< bool T_SortAsc >
struct ComparatorSoldiersByRank
{
    bool operator()(nofSoldier* left, nofSoldier* right) const
    {
        if(left->GetRank() == right->GetRank())
            return (T_SortAsc) ? left->GetObjId() < right->GetObjId() : left->GetObjId() > right->GetObjId();
        else if(T_SortAsc)
            return left->GetRank() < right->GetRank();
        else
            return left->GetRank() > right->GetRank();
    }
};

class nofPassiveSoldier;
typedef boost::container::flat_set<nofPassiveSoldier*, ComparatorSoldiersByRank<true> > SortedTroops;

#endif
