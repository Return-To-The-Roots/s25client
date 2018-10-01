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
#ifndef NOF_TRADEDONKEY_H_
#define NOF_TRADEDONKEY_H_

#include "figures/noFigure.h"
#include "gameTypes/GoodTypes.h"
#include <deque>
class SerializedGameData;

/// For wares: donkey who carry the wares and follow the leader
/// Can also be the other people following the leader!
class nofTradeDonkey : public noFigure
{
    /// Successor (NULL if this is the last one)
    nofTradeDonkey* successor;
    /// Ware this donkey carries (GD_NOTHING if this is a normal figure)
    GoodType gt;
    /// Last dir this donkey used
    std::deque<unsigned char> next_dirs;

private:
    void GoalReached() override;
    void Walked() override;
    void HandleDerivedEvent(const unsigned id) override;
    void AbrogateWorkplace() override;

    /// Returns next direction
    unsigned char GetNextDir()
    {
        unsigned char dir = next_dirs.front();
        next_dirs.pop_front();
        return dir;
    }

public:
    nofTradeDonkey(const MapPoint pt, const unsigned char player, const GoodType gt, const Job job);
    nofTradeDonkey(SerializedGameData& sgd, const unsigned obj_id);

    void Destroy() override
    {
        RTTR_Assert(!successor);
        noFigure::Destroy();
    }

    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const override { return GOT_NOF_TRADEDONKEY; }

    void Draw(DrawPoint drawPt) override;

    /// Wird aufgerufen, wenn die Flagge abgerissen wurde
    void LostWork();

    /// Adds the next direction, this is usually done by the predecessor
    void AddNextDir(const unsigned char dir) { next_dirs.push_back(dir); }

    /// Gets the type of ware this donkey is carrying
    GoodType GetCarriedWare() const { return gt; }

    /// Sets the successor in the caravane
    void SetSuccessor(nofTradeDonkey* const successor) { this->successor = successor; }

    // get the successor in the caravane
    nofTradeDonkey* GetSuccessor() { return successor; }

    /// Inform successor that the caravane is canceled
    void CancelTradeCaravane();
};

#endif //! NOF_SCOUT_FREE_H_
