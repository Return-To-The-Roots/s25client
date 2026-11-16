// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

/// Klasse für den Schreiner
class nofPigbreeder : public nofWorkman
{
    /// Zeichnet ihn beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short getCarryID() const override { return 73; }
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> ProduceWare() override;

public:
    nofPigbreeder(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofPigbreeder(SerializedGameData& sgd, unsigned obj_id);

    GO_Type getGOT() const final { return GO_Type::NofPigbreeder; }

    /// Ggf. Schweinesounds abspielen
    void MakePigSounds();
};
