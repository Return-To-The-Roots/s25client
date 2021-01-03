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

/// Menschliches Skelett (Zierobjekt, das sich automatisch umwandelt und dann verschwindet)
class noSkeleton : public noCoordBase
{
public:
    noSkeleton(MapPoint pos);
    noSkeleton(SerializedGameData& sgd, unsigned obj_id);

    ~noSkeleton() override;

    void Destroy() override { Destroy_noSkeleton(); }

    /// Serialisierungsfunktionen
protected:
    void Serialize_noSkeleton(SerializedGameData& sgd) const;

public:
    void Serialize(SerializedGameData& sgd) const override { Serialize_noSkeleton(sgd); }

    GO_Type GetGOT() const override { return GO_Type::Skeleton; }

protected:
    void Destroy_noSkeleton();

    void Draw(DrawPoint drawPt) override;
    void HandleEvent(unsigned id) override;

private:
    /// Type des Skeletts (0 = ganz "frisch", 1 - schon etwas verdorrt)
    unsigned char type;
    /// GameEvent*, damit der dann gelöscht werden kann, falls das Skelett von außerhalb gelöscht wird
    const GameEvent* current_event;
};
