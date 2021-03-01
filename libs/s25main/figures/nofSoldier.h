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
#include <boost/container/flat_set.hpp>

class nobBaseMilitary;
class SerializedGameData;

/// Basisklasse für alle Soldatentypen
class nofSoldier : public noFigure
{
protected:
    /// Heimatgebäude, ist bei Soldaten aus HQs das HQ!
    nobBaseMilitary* building;
    /// Hitpoints
    unsigned char hitpoints;

    /// Zeichnet den Soldaten beim ganz normalen Laufen
    void DrawSoldierWaiting(DrawPoint drawPt);

    /// wenn man beim Arbeitsplatz "kündigen" soll, man das Laufen zum Ziel unterbrechen muss (warum auch immer)
    void AbrogateWorkplace() override;

public:
    nofSoldier(MapPoint pos, unsigned char player, nobBaseMilitary* goal, nobBaseMilitary* home, unsigned char rank);
    nofSoldier(MapPoint pos, unsigned char player, nobBaseMilitary* home, unsigned char rank);
    nofSoldier(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override
    {
        RTTR_Assert(HasNoHome());
        noFigure::Destroy();
    }
    void Serialize(SerializedGameData& sgd) const override;

    /// Liefert Rang des Soldaten
    unsigned char GetRank() const;
    unsigned char GetHitpoints() const;
    bool HasNoHome() const { return building == nullptr; }
};

/// Comparator to sort soldiers by rank (and ID for ties)
/// Template arguments defines the sort order: True for weak ones first, false for strong ones first
template<bool T_SortAsc>
struct ComparatorSoldiersByRank
{
    bool operator()(const nofSoldier* left, const nofSoldier* right) const
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
using SortedTroops = boost::container::flat_set<nofPassiveSoldier*, ComparatorSoldiersByRank<true>>;
