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

#ifndef NOF_METALWORKER_H_
#define NOF_METALWORKER_H_

#include "nofWorkman.h"
#include "notifications/Subscribtion.h"

class SerializedGameData;
class nobUsual;

/// Klasse für den Schreiner
class nofMetalworker : public nofWorkman
{
    GoodType nextProducedTool;
    Subscribtion toolOrderSub;

protected:
    /// Zeichnet ihn beim Arbeiten
    void DrawWorking(DrawPoint drawPt) override;
    /// Gibt die ID in JOBS.BOB zurück, wenn der Beruf Waren rausträgt (bzw rein)
    unsigned short GetCarryID() const override;
    /// Der Arbeiter erzeugt eine Ware
    GoodType ProduceWare() override;
    /// Returns the next tool to be produced according to the orders
    GoodType GetOrderedTool();
    /// Returns a random tool according to the priorities
    GoodType GetRandomTool();

    bool HasToolOrder() const;

    bool AreWaresAvailable() const override;
    bool StartWorking() override;
    void CheckForOrders();

public:
    nofMetalworker(const MapPoint pt, const unsigned char player, nobUsual* workplace);
    nofMetalworker(SerializedGameData& sgd, const unsigned obj_id);
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const override { return GOT_NOF_METALWORKER; }
};

#endif
