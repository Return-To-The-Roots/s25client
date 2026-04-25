// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/noFigure.h"

class nobBaseMilitary;
class SerializedGameData;

/// Base class for all solider
class nofSoldier : public noFigure
{
protected:
    /// Build to which this soldier belongs
    nobBaseMilitary* homeBld;
    uint8_t hitpoints;

    /// Draw during regular walking
    void DrawSoldierWaiting(DrawPoint drawPt);

    /// Tell workplace (i.e. home bld) that we are not coming back
    void AbrogateWorkplace() override;

    explicit nofSoldier(const nofSoldier&) = default;

public:
    nofSoldier(MapPoint pos, unsigned char player, nobBaseMilitary* goal, nobBaseMilitary* home, unsigned char rank,
               bool armor = false);
    nofSoldier(MapPoint pos, unsigned char player, nobBaseMilitary& home, unsigned char rank, bool armor = false);
    nofSoldier(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override
    {
        RTTR_Assert(!GetHomeBld());
        noFigure::Destroy();
    }
    void Serialize(SerializedGameData& sgd) const override;

    unsigned char GetRank() const;
    unsigned char GetHitpoints() const;
    const nobBaseMilitary* GetHomeBld() const { return homeBld; }
};

/// Comparator to sort soldiers by rank and armor (and ID for ties), weak ones first
struct ComparatorSoldiersByRank
{
    template<typename TSoldierPtr>
    bool operator()(const TSoldierPtr& left, const TSoldierPtr& right) const
    {
        if(left->GetRank() == right->GetRank())
        {
            if(left->HasArmor() == right->HasArmor())
                return left->GetObjId() < right->GetObjId();
            else
                return left->HasArmor() < right->HasArmor();
        } else
            return left->GetRank() < right->GetRank();
    }
};
