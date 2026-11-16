// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

    void destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    void handleEvent(unsigned id) override;

protected:
    /// Gibt Farbe zurück, mit der das Objekt gezeichnet werden soll
    unsigned getDrawColor() const;
    /// Gibt Farbe zurück, mit der der Schatten des Objekts gezeichnet werden soll
    unsigned getDrawShadowColor() const;

private:
    /// Bin ich grad in der Sterbephase (in der das Schild immer transparenter wird, bevor es verschwindet)
    bool disappearing;
    /// Event, das bestimmt wie lange es noch lebt
    const GameEvent* dead_event;
};
