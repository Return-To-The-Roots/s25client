// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

/// Klasse für den Schreiner
class nofIronfounder : public nofWorkman
{
    /// Zeichnet ihn beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override { return 69; }
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> ProduceWare() override;

public:
    nofIronfounder(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofIronfounder(SerializedGameData& sgd, unsigned obj_id);

    GO_Type GetGOT() const final { return GO_Type::NofIronfounder; }
};
