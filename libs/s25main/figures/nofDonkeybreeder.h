// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

/// Klasse für den Eselzüchter
class nofDonkeybreeder : public nofWorkman
{
public:
    nofDonkeybreeder(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofDonkeybreeder(SerializedGameData& sgd, unsigned obj_id);

    GO_Type GetGOT() const final { return GO_Type::NofDonkeybreeder; }

private:
    /// Zeichnet ihn beim Arbeiten.
    void DrawWorking(DrawPoint drawPt) override;
    /// Der Arbeiter erzeugt eine Ware.
    helpers::OptionalEnum<GoodType> ProduceWare() override;
    /// Wird aufgerufen, wenn er fertig mit arbeiten ist
    void WorkFinished() override;

    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override { return 0; }
};
