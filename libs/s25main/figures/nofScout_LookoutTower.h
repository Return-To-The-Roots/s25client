// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofBuildingWorker.h"
class SerializedGameData;
class nobBaseWarehouse;
class nobUsual;

/// Späher, der in einem Spähturm "arbeitet"
class nofScout_LookoutTower : public nofBuildingWorker
{
protected:
    /// Funktionen, die nur von der Basisklasse (noFigure) aufgerufen werden, wenn man gelaufen ist
    void walkedDerived() override;
    /// Malt den Arbeiter beim Arbeiten
    void drawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short getCarryID() const override { return 0; }
    /// Arbeit musste wegen Arbeitsplatzverlust abgebrochen werden
    void workAborted() override;
    /// Arbeitsplatz wurde erreicht
    void workplaceReached() override;

    bool areWaresAvailable() const override;

public:
    nofScout_LookoutTower(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofScout_LookoutTower(SerializedGameData& sgd, unsigned obj_id);

    GO_Type getGOT() const final { return GO_Type::NofScoutLookouttower; }

    void handleDerivedEvent(unsigned id) override;
};
