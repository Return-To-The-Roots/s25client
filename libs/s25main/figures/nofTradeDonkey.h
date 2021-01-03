// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
#include "gameTypes/GoodTypes.h"
#include "gameTypes/TradeDirection.h"
#include <boost/variant.hpp>
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
    nofTradeDonkey(MapPoint pos, unsigned char player, const boost::variant<GoodType, Job>& what);
    nofTradeDonkey(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override
    {
        RTTR_Assert(!successor);
        noFigure::Destroy();
    }

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const override { return GO_Type::NofTradedonkey; }

    void Draw(DrawPoint drawPt) override;

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
