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
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override { return CARRY_ID_CARRIER_OFFSET + rttr::enum_cast(GoodType::Water); }
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> ProduceWare() override;

    bool AreWaresAvailable() const override;
    bool StartWorking() override;

public:
    nofWellguy(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofWellguy(SerializedGameData& sgd, unsigned obj_id);

    GO_Type GetGOT() const final { return GO_Type::NofWellguy; }
};
