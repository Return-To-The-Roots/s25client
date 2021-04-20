// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
