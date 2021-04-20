// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofFarmhand.h"
class SerializedGameData;
class nobUsual;

class nofFarmer : public nofFarmhand
{
private:
    /// Was soll gemacht werden: Ernten oder Sähen?
    bool harvest;

    /// Malt den Arbeiter beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override;

    /// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
    void WorkStarted() override;
    /// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
    void WorkFinished() override;
    /// Abgeleitete Klasse informieren, wenn Arbeiten abgebrochen werden müssen
    void WorkAborted() override;

    /// Returns the quality of this working point or determines if the worker can work here at all
    PointQuality GetPointQuality(MapPoint pt) const override;

public:
    nofFarmer(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofFarmer(SerializedGameData& sgd, unsigned obj_id);

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofFarmer; }
};
