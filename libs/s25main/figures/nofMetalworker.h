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

    GO_Type GetGOT() const override { return GO_Type::NofMetalworker; }
};
