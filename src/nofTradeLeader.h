// $Id: nofTradeLeader.h 6582 2010-07-16 11:23:35Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef NOF_TRADELEADER_H_
#define NOF_TRADELEADER_H_

#include "noFigure.h"
#include "TradeGraph.h"

class nofTradeDonkey;
class nobBaseWarehouse;

/// Leader of a trade caravane
class nofTradeLeader : public noFigure
{
        /// Route of this caravane
        TradeRoute tr;
        /// Successor (NULL if this is the one behind the leader)
        nofTradeDonkey* successor;
        /// The start and home warehosue
        Point<MapCoord> start, goal;

    private:

        unsigned char fails;
        void GoalReached();
        void Walked();
        void HandleDerivedEvent(const unsigned int id);
        void AbrogateWorkplace();

        /// Tries to go to the home ware house, otherwise start wandering
        void TryToGoHome();
        /// Start wandering and informs the other successors about this
        void CancelTradeCaravane();

    public:

        nofTradeLeader(const MapCoord x, const MapCoord y, const unsigned char player, const TradeRoute& tr, const Point<MapCoord>  start, const Point<MapCoord> goal);
        nofTradeLeader(SerializedGameData* sgd, const unsigned obj_id);

        void Serialize(SerializedGameData* sgd) const;

        GO_Type GetGOT() const { return GOT_NOF_TRADELEADER; }

        void Draw(int x, int y);

        /// Wird aufgerufen, wenn die Flagge abgerissen wurde
        void LostWork();

        /// Sets the sucessor in the caravane
        void SetSuccessor(nofTradeDonkey* const successor)
        { this->successor = successor; }
};


#endif //!NOF_SCOUT_FREE_H_
