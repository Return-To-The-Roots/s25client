// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "enum_cast.hpp"
#include "nofWorkman.h"

class nobBaseWarehouse;
class SerializedGameData;
class nobUsual;

/// Klasse für den Schreiner
class nofWellguy : public nofWorkman
{
protected:
    /// Zeichnet ihn beim Arbeiten
    void drawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short getCarryID() const override { return CARRY_ID_CARRIER_OFFSET + rttr::enumCast(GoodType::Water); }
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> produceWare() override;

    bool areWaresAvailable() const override;
    bool startWorking() override;

public:
    nofWellguy(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofWellguy(SerializedGameData& sgd, unsigned obj_id);

    GO_Type getGOT() const final { return GO_Type::NofWellguy; }
};
