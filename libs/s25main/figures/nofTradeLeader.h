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
#include "world/TradeRoute.h"

class nofTradeDonkey;
class SerializedGameData;

/// Leader of a trade caravane
class nofTradeLeader : public noFigure
{
    /// Route of this caravane
    TradeRoute tr;
    /// Successor (nullptr if there is none)
    nofTradeDonkey* successor;
    /// The start and home warehosue
    MapPoint homePos, goalPos;

private:
    void GoalReached() override;
    void Walked() override;
    void HandleDerivedEvent(unsigned id) override;
    void AbrogateWorkplace() override;

    /// Tries to go to the home ware house and returns whether this is possible
    bool TryToGoHome();
    /// Inform successor that the caravane is canceled
    void CancelTradeCaravane();

public:
    nofTradeLeader(MapPoint pos, unsigned char player, TradeRoute tr, MapPoint homePos, MapPoint goalPos);
    nofTradeLeader(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override
    {
        RTTR_Assert(!successor);
        noFigure::Destroy();
    }

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofTradeleader; }

    void Draw(DrawPoint drawPt) override;

    /// Wird aufgerufen, wenn die Flagge abgerissen wurde
    void LostWork();

    /// Sets the sucessor in the caravane
    void SetSuccessor(nofTradeDonkey* const successor) { this->successor = successor; }
};
