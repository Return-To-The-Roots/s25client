// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noCoordBase.h"

class SerializedGameData;
class GameEvent;

/// Menschliches Skelett (Zierobjekt, das sich automatisch umwandelt und dann verschwindet)
class noSkeleton : public noCoordBase
{
public:
    noSkeleton(MapPoint pos);
    noSkeleton(SerializedGameData& sgd, unsigned obj_id);
    ~noSkeleton() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Skeleton; }

protected:
    void Draw(DrawPoint drawPt) override;
    void HandleEvent(unsigned id) override;

private:
    /// Type des Skeletts (0 = ganz "frisch", 1 - schon etwas verdorrt)
    unsigned char type;
    /// GameEvent*, damit der dann gelöscht werden kann, falls das Skelett von außerhalb gelöscht wird
    const GameEvent* current_event;
};
