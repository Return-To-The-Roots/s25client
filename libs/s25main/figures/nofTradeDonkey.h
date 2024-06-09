// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/noFigure.h"
#include "variant.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/TradeDirection.h"
#include <deque>

class SerializedGameData;

/// For wares: donkey who carry the wares and follow the leader
/// Can also be the other people following the leader!
class nofTradeDonkey : public noFigure
{
    /// Successor (nullptr if this is the last one)
    nofTradeDonkey* successor;
    /// Ware this donkey carries (empty if this is a normal figure)
    helpers::OptionalEnum<GoodType> gt;
    /// Last dir this donkey used
    std::deque<TradeDirection> next_dirs;

private:
    void GoalReached() override;
    void Walked() override;
    void HandleDerivedEvent(unsigned id) override;
    void AbrogateWorkplace() override;

    /// Returns next direction
    TradeDirection GetNextDir()
    {
        TradeDirection dir = next_dirs.front();
        next_dirs.pop_front();
        return dir;
    }

public:
    nofTradeDonkey(MapPoint pos, unsigned char player, const boost_variant2<GoodType, Job>& what);
    nofTradeDonkey(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override
    {
        RTTR_Assert(!successor);
        noFigure::Destroy();
    }

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::NofTradedonkey; }

    void Draw(DrawPoint drawPt) override;
    void DrawWalking(DrawPoint drawPt) override;

    /// Wird aufgerufen, wenn die Flagge abgerissen wurde
    void LostWork();

    /// Adds the next direction, this is usually done by the predecessor
    void AddNextDir(TradeDirection dir) { next_dirs.push_back(dir); }

    /// Gets the type of ware this donkey is carrying
    const auto& GetCarriedWare() const { return gt; }

    /// Sets the successor in the caravane
    void SetSuccessor(nofTradeDonkey* const successor) { this->successor = successor; }

    // get the successor in the caravane
    nofTradeDonkey* GetSuccessor() { return successor; }

    /// Inform successor that the caravane is canceled
    void CancelTradeCaravane();
};
