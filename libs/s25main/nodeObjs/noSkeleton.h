// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

    void destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::Skeleton; }

protected:
    void draw(DrawPoint drawPt) override;
    void handleEvent(unsigned id) override;

private:
    /// Type des Skeletts (0 = ganz "frisch", 1 - schon etwas verdorrt)
    unsigned char type;
    /// GameEvent*, damit der dann gelöscht werden kann, falls das Skelett von außerhalb gelöscht wird
    const GameEvent* current_event;
};
