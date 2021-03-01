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

#include "noBase.h"
#include "gameTypes/MapTypes.h"
class FOWObject;
class SerializedGameData;

class noGranite final : public noBase
{
    GraniteType type;    // Welcher Typ ( gibt 2 )
    unsigned char state; // Status, 0 - 5, von sehr wenig bis sehr viel

public:
    noGranite(GraniteType type, unsigned char state);
    noGranite(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override {}
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Granite; }

    void Draw(DrawPoint drawPt) override;

    BlockingManner GetBM() const override { return BlockingManner::FlagsAround; }

    /// Erzeugt von ihnen selbst ein FOW Objekt als visuelle "Erinnerung" für den Fog of War
    std::unique_ptr<FOWObject> CreateFOWObject() const override;

    /// "Bearbeitet" den Granitglotz --> haut ein Stein ab
    void Hew();

    /// Gibt true zurück, falls der Granitblock nur noch 1 Stein groß ist und damit dann vernichtet werden kann
    bool IsSmall() const { return (state == 0); }

    /// Return the size of this pile.
    unsigned char GetSize() const { return state; }
};
