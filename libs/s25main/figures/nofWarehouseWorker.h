// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/noFigure.h"
#include <memory>

class Ware;
class SerializedGameData;

/// Der "Warehouse-Worker" ist ein einfacher(er) Träger, der die Waren aus dem Lagerhaus holt
class nofWarehouseWorker : public noFigure
{
    // Mein Lagerhaus, in dem ich arbeite, darf auch mal ein bisschen was an mir ändern
    friend class nobBaseWarehouse;

private:
    /// Ware currently being carried or nullptr
    std::unique_ptr<Ware> carried_ware;

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
    nofWarehouseWorker(MapPoint pos, unsigned char player, std::unique_ptr<Ware> ware, bool task);
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
