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

/// Menschliches Skelett (Zierobjekt, das sich automatisch umwandelt und dann verschwindet)
class noShipBuildingSite : public noCoordBase
{
public:
    noShipBuildingSite(MapPoint pos, unsigned char player);
    noShipBuildingSite(SerializedGameData& sgd, unsigned obj_id);
    ~noShipBuildingSite() override;
    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;
    GO_Type GetGOT() const final { return GO_Type::Shipbuildingsite; }

    /// Gibt den Eigentümer zurück
    unsigned char GetPlayer() const { return player; }

    /// Das Schiff wird um eine Stufe weitergebaut
    void MakeBuildStep();

    BlockingManner GetBM() const override { return BlockingManner::Building; }

protected:
    void Draw(DrawPoint drawPt) override;

private:
    /// Spieler, dem dieses Schiff gehört
    unsigned char player;
    /// Baufortschritt des Schiffes
    unsigned char progress;
};
