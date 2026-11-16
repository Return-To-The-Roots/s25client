// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noCoordBase.h"

class SerializedGameData;
class GameEvent;

// Klasse für ein brennendes Gebäude
class noFire : public noCoordBase
{
    /// Größe des Feuers: klein (0) oder groß (1)
    bool isBig;
    /// "Todesevent" (also bis es abgebrannt ist) speichern, damit dann interpoliert wird
    const GameEvent* dead_event;
    /// Wurden Feuersounds abgespielt
    bool was_sounding;
    /// Letzter Feuersound-Zeitpunkt
    unsigned last_sound;
    /// Intervall zum nächsten Feuersound
    unsigned next_interval;

public:
    noFire(MapPoint pos, bool isBig);
    noFire(SerializedGameData& sgd, unsigned obj_id);

    ~noFire() override;
    void destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::Fire; }

    BlockingManner getBM() const override { return BlockingManner::FlagsAround; }

    /// Zeichnen
    void draw(DrawPoint drawPt) override;
    /// Benachrichtigen, wenn neuer gf erreicht wurde
    void handleEvent(unsigned id) override;
};
