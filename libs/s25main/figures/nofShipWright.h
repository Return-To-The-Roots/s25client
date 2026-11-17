// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"
class SerializedGameData;
class nobUsual;

/// Schiffsbauer - erstmal nur provisorisch, da er nur Boote baut
class nofShipWright : public nofWorkman
{
    /// Punkt, an dem das Schiff steht, an dem er gerade arbeitet
    MapPoint curShipBuildPos;

private:
    /// Zeichnet ihn beim Arbeiten
    void drawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short getCarryID() const override { return 90; }
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> produceWare() override { return GoodType::Boat; }

    /// Startet das Laufen zu der Arbeitsstelle, dem Schiff
    void startWalkingToShip();

    /// Ist ein bestimmter Punkt auf der Karte für den Schiffsbau geeignet
    bool isPointGood(MapPoint pt) const;

    /// Der Schiffsbauer hat einen Bauschritt bewältigt und geht wieder zurück zum Haus
    void workFinished() override;

    void walkToWorkpoint();
    void startWalkingHome();
    void walkHome();
    void workAborted() override;
    void walkedDerived() override;

    /// Zeichnen der Figur in sonstigen Arbeitslagen
    void drawOtherStates(DrawPoint drawPt) override;

public:
    nofShipWright(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofShipWright(SerializedGameData& sgd, unsigned obj_id);
    GO_Type getGOT() const final { return GO_Type::NofShipwright; }
    void handleDerivedEvent(unsigned id) override;
    void serialize(SerializedGameData& sgd) const override;
};
