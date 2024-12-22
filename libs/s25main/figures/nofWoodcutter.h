// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofFarmhand.h"
class SerializedGameData;
class nobUsual;

class nofWoodcutter : public nofFarmhand
{
private:
    /// Malt den Arbeiter beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override;

    /// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
    void WorkStarted() override;
    /// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
    void WorkFinished() override;

    /// Returns the quality of this working point or determines if the worker can work here at all
    PointQuality GetPointQuality(MapPoint pt, bool isBeforeWork) const override;

    /// wird aufgerufen, wenn die Arbeit abgebrochen wird (von nofBuildingWorker aufgerufen)
    void WorkAborted() override;

public:
    nofWoodcutter(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofWoodcutter(SerializedGameData& sgd, unsigned obj_id);

    GO_Type GetGOT() const final { return GO_Type::NofWoodcutter; }
};
