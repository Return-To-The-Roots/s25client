// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
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

    void goalReached() override;
    void walked() override;
    /// wenn man beim Arbeitsplatz "kündigen" soll, man das Laufen zum Ziel unterbrechen muss (warum auch immer)
    void abrogateWorkplace() override;
    void abrogate() override;

    void looseWare();

    void handleDerivedEvent(unsigned id) override;

public:
    nofWarehouseWorker(MapPoint pos, unsigned char player, std::unique_ptr<Ware> ware, bool task);
    nofWarehouseWorker(SerializedGameData& sgd, unsigned obj_id);
    nofWarehouseWorker(const nofWarehouseWorker&) = delete;

    ~nofWarehouseWorker() override;

    void destroy() override;
    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NofWarehouseworker; }

    void draw(DrawPoint drawPt) override;

    // Ware nach draußen bringen (von Lagerhaus aus aufgerufen)
    void carryWare(Ware* ware);

    /// Mitglied von nem Lagerhaus(Lagerhausarbeiter, die die Träger-Bestände nicht beeinflussen?)
    bool memberOfWarehouse() const override { return true; }
};
