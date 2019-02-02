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
#ifndef NOF_SHIPWRIGHT_H_
#define NOF_SHIPWRIGHT_H_

#include "nofWorkman.h"
class SerializedGameData;
class nobUsual;

/// Schiffsbauer - erstmal nur provisorisch, da er nur Boote baut
class nofShipWright : public nofWorkman
{
    /// Punkt, an dem das Schiff steht, an dem er gerade arbeitet
    MapPoint dest;

private:
    /// Zeichnet ihn beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Gibt die ID in JOBS.BOB zurück, wenn der Beruf Waren rausträgt (bzw rein)
    unsigned short GetCarryID() const override { return 90; }
    /// Der Arbeiter erzeugt eine Ware
    GoodType ProduceWare() override { return GD_BOAT; }

    /// Startet das Laufen zu der Arbeitsstelle, dem Schiff
    void StartWalkingToShip(unsigned char first_dir);

    /// Ist ein bestimmter Punkt auf der Karte für den Schiffsbau geeignet
    bool IsPointGood(const MapPoint pt) const;

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
    nofShipWright(const MapPoint pt, unsigned char player, nobUsual* workplace);
    nofShipWright(SerializedGameData& sgd, unsigned obj_id);
    GO_Type GetGOT() const override { return GOT_NOF_SHIPWRIGHT; }
    void HandleDerivedEvent(unsigned id) override;
    void Serialize(SerializedGameData& sgd) const override;
};

#endif
