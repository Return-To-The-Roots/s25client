// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

/// Klasse für den Schreiner
class nofMiller : public nofWorkman
{
    /// Letzter Mühlensound-Zeitpunkt
    unsigned last_sound;
    /// Intervall zum nächsten Mühlensound
    unsigned next_interval;

private:
    /// Zeichnet ihn beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short getCarryID() const override { return 75; }
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> ProduceWare() override;

public:
    nofMiller(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofMiller(SerializedGameData& sgd, unsigned obj_id);

    GO_Type getGOT() const final { return GO_Type::NofMiller; }
};
