// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/noFigure.h"

class nobBaseMilitary;
class SerializedGameData;

/// Base class for all figures which can have armor
class nofArmored : public noFigure
{
protected:
    /// Armor
    bool armor;

    explicit nofArmored(const nofArmored&) = default;

    void DrawArmor(DrawPoint drawPt);

public:
    void DrawArmorWalking(DrawPoint drawPt);
    void DrawArmorNotWalking(DrawPoint drawPt);

    nofArmored(Job job, MapPoint pos, unsigned char player, noRoadNode* goal, bool armor = false);
    nofArmored(Job job, MapPoint pos, unsigned char player, bool armor = false);
    nofArmored(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    bool HasArmor() const;
    void SetArmor(bool armor);
};
