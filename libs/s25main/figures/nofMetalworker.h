// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "nofWorkman.h"
#include "notifications/Subscription.h"

class SerializedGameData;
class nobUsual;

/// Klasse für den Schreiner
class nofMetalworker : public nofWorkman
{
    helpers::OptionalEnum<GoodType> nextProducedTool;
    Subscription toolOrderSub;

protected:
    /// Zeichnet ihn beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Id in jobs.bob or carrier.bob when carrying a ware
    unsigned short GetCarryID() const override;
    /// Der Arbeiter erzeugt eine Ware
    helpers::OptionalEnum<GoodType> ProduceWare() override;
    /// Returns the next tool to be produced according to the orders
    helpers::OptionalEnum<GoodType> GetOrderedTool();
    /// Returns a random tool according to the priorities
    helpers::OptionalEnum<GoodType> GetRandomTool();

    bool HasToolOrder() const;

    bool AreWaresAvailable() const override;
    bool StartWorking() override;
    void CheckForOrders();

public:
    nofMetalworker(MapPoint pos, unsigned char player, nobUsual* workplace);
    nofMetalworker(SerializedGameData& sgd, unsigned obj_id);
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofMetalworker; }
};
