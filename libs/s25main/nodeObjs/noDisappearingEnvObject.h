// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
