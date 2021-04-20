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
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override { return 90; }
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> ProduceWare() override { return GoodType::Boat; }

    /// Startet das Laufen zu der Arbeitsstelle, dem Schiff
    void StartWalkingToShip();

    /// Ist ein bestimmter Punkt auf der Karte für den Schiffsbau geeignet
    bool IsPointGood(MapPoint pt) const;

    /// Der Schiffsbauer hat einen Bauschritt bewältigt und geht wieder zurück zum Haus
    void WorkFinished() override;

    void WalkToWorkpoint();
    void StartWalkingHome();
    void WalkHome();
    void WorkAborted() override;
    void WalkedDerived() override;

    /// Zeichnen der Figur in sonstigen Arbeitslagen
    void DrawOtherStates(DrawPoint drawPt) override;

public:
    nofShipWright(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofShipWright(SerializedGameData& sgd, unsigned obj_id);
    GO_Type GetGOT() const final { return GO_Type::NofShipwright; }
    void HandleDerivedEvent(unsigned id) override;
    void Serialize(SerializedGameData& sgd) const override;
};
