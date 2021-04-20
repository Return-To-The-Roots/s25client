// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/noFigure.h"

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
    nofSoldier(MapPoint pos, unsigned char player, nobBaseMilitary& home, unsigned char rank);
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

/// Comparator to sort soldiers by rank (and ID for ties), weak ones first
struct ComparatorSoldiersByRank
{
    template<typename TSoldierPtr>
    bool operator()(const TSoldierPtr& left, const TSoldierPtr& right) const
    {
        if(left->GetRank() == right->GetRank())
            return left->GetObjId() < right->GetObjId();
        else
            return left->GetRank() < right->GetRank();
    }
};
