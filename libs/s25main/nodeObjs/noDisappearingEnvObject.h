// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noCoordBase.h"

class SerializedGameData;
class GameEvent;

class noDisappearingEnvObject : public noCoordBase
{
public:
    noDisappearingEnvObject(MapPoint pos, unsigned living_time, unsigned add_var_living_time);
    noDisappearingEnvObject(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    /// Benachrichtigen, wenn neuer GF erreicht wurde.
    void HandleEvent(unsigned id) override;

protected:
    /// Gibt Farbe zurück, mit der das Objekt gezeichnet werden soll
    unsigned GetDrawColor() const;
    /// Gibt Farbe zurück, mit der der Schatten des Objekts gezeichnet werden soll
    unsigned GetDrawShadowColor() const;

private:
    /// Bin ich grad in der Sterbephase (in der das Schild immer transparenter wird, bevor es verschwindet)
    bool disappearing;
    /// Event, das bestimmt wie lange es noch lebt
    const GameEvent* dead_event;
};
