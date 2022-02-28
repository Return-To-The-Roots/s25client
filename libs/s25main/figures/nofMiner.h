// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"
#include "addons/AddonMiningOverhaul.h"

class SerializedGameData;
class nobUsual;

class nofMiner : public nofWorkman
{
protected:
    /// Zeichnet ihn beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override;
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> ProduceWare() override;
    /// alter workcycle (addon)
    bool isAlteredWorkcycle;

    bool AreWaresAvailable() const override;
    bool StartWorking() override;
    ResourceType GetRequiredResType() const;
    MiningBehavior GetMiningBehavior() const;

public:
    nofMiner(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofMiner(SerializedGameData& sgd, unsigned obj_id);

    GO_Type GetGOT() const final { return GO_Type::NofMiner; }
};
