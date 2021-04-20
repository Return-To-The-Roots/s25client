// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"

class SerializedGameData;
class nobUsual;

/// Klasse für den Schmied
class nofArmorer : public nofWorkman
{
private:
    /// Bestimmt, was der Schmied als nächstes schmieden soll (immer Schwert-Schild im Wechsel)
    bool sword_shield;

protected:
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override;
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> ProduceWare() override;
    void HandleDerivedEvent(unsigned id) override;

    bool AreWaresAvailable() const override;

public:
    nofArmorer(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofArmorer(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofArmorer; }
};
