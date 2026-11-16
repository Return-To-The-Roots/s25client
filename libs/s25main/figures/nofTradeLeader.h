// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
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
    void goalReached() override;
    void walked() override;
    void handleDerivedEvent(unsigned id) override;
    void abrogateWorkplace() override;

    /// Tries to go to the home ware house and returns whether this is possible
    bool tryToGoHome();
    /// Inform successor that the caravane is canceled
    void cancelTradeCaravane();

public:
    nofTradeLeader(MapPoint pos, unsigned char player, TradeRoute tr, MapPoint homePos, MapPoint goalPos);
    nofTradeLeader(SerializedGameData& sgd, unsigned obj_id);
    nofTradeLeader(const nofTradeLeader&) = delete;

    void destroy() override
    {
        RTTR_Assert(!successor);
        noFigure::destroy();
    }

    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::NofTradeleader; }

    void draw(DrawPoint drawPt) override;

    /// Wird aufgerufen, wenn die Flagge abgerissen wurde
    void lostWork();

    /// Sets the sucessor in the caravane
    void setSuccessor(nofTradeDonkey* const successor) { this->successor = successor; }
};
