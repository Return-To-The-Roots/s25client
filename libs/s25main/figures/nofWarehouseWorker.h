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

#include "figures/noFigure.h"

class Ware;
class SerializedGameData;

/// Der "Warehouse-Worker" ist ein einfacher(er) Träger, der die Waren aus dem Lagerhaus holt
class nofWarehouseWorker : public noFigure
{
    // Mein Lagerhaus, in dem ich arbeite, darf auch mal ein bisschen was an mir ändern
    friend class nobBaseWarehouse;

private:
    /// Ware currently being carried or nullptr
    Ware* carried_ware;

    // Aufgabe, die der Warenhaustyp hat (Ware raustragen (0) oder reinholen)
    const bool shouldBringWareIn;

    // Bin ich fett? (werde immer mal dünn oder fett, damits nicht immer gleich aussieht, wenn jemand rauskommt)
    bool fat;

    void GoalReached() override;
    void Walked() override;
    /// wenn man beim Arbeitsplatz "kündigen" soll, man das Laufen zum Ziel unterbrechen muss (warum auch immer)
    void AbrogateWorkplace() override;

    void LooseWare();

    void HandleDerivedEvent(unsigned id) override;

public:
    nofWarehouseWorker(MapPoint pos, unsigned char player, Ware* ware, bool task);
    nofWarehouseWorker(SerializedGameData& sgd, unsigned obj_id);

    ~nofWarehouseWorker() override;

    void Destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofWarehouseworker; }

    void Draw(DrawPoint drawPt) override;

    // Ware nach draußen bringen (von Lagerhaus aus aufgerufen)
    void CarryWare(Ware* ware);

    /// Mitglied von nem Lagerhaus(Lagerhausarbeiter, die die Träger-Bestände nicht beeinflussen?)
    bool MemberOfWarehouse() const override { return true; }
};
