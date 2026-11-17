// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofFarmhand.h"
class GameWorld;
class SerializedGameData;
class nobUsual;

class nofFarmer : public nofFarmhand
{
private:
    /// Was soll gemacht werden: Ernten oder Sähen?
    bool harvest;

    /// Malt den Arbeiter beim Arbeiten
    void drawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short getCarryID() const override;

    /// Abgeleitete Klasse informieren, wenn sie anfängt zu arbeiten (Vorbereitungen)
    void workStarted() override;
    /// Abgeleitete Klasse informieren, wenn fertig ist mit Arbeiten
    void workFinished() override;
    /// Abgeleitete Klasse informieren, wenn Arbeiten abgebrochen werden müssen
    void workAborted() override;

    /// Returns the quality of this working point or determines if the worker can work here at all
    PointQuality getPointQuality(MapPoint pt, bool isBeforeWork) const override;
    using nofFarmhand::getPointQuality;

public:
    nofFarmer(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofFarmer(SerializedGameData& sgd, unsigned obj_id);

    static nofFarmhand::PointQuality getNewFieldPointQuality(const GameWorld& world, MapPoint pt);

    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NofFarmer; }
};
