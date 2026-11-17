// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

/// Klasse für den Schreiner
class nofMiner : public nofWorkman
{
protected:
    /// Zeichnet ihn beim Arbeiten
    void drawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short getCarryID() const override;
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> produceWare() override;

    bool areWaresAvailable() const override;
    bool startWorking() override;
    ResourceType getRequiredResType() const;

public:
    nofMiner(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofMiner(SerializedGameData& sgd, unsigned obj_id);

    GO_Type getGOT() const final { return GO_Type::NofMiner; }
};
