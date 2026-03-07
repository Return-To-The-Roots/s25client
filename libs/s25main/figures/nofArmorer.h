// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

/// Class for the armorer
class nofArmorer : public nofWorkman
{
private:
    /// Determines what the armorer should forge next (always alternating sword and shield)
    bool sword_shield;

protected:
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override;
    /// The worker produces one ware
    helpers::OptionalEnum<GoodType> ProduceWare() override;
    void HandleDerivedEvent(unsigned id) override;

    bool AreWaresAvailable() const override;

public:
    nofArmorer(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofArmorer(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofArmorer; }
};
