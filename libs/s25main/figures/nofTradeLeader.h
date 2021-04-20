// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
