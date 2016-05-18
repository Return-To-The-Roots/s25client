// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef NO_CHARBURNERPILE_H_
#define NO_CHARBURNERPILE_H_

#include "noCoordBase.h"

class SerializedGameData;
class GameEvent;

/// The wood/coal piles made by the charburner
class noCharburnerPile : public noCoordBase
{
    public:
        /// Status
        enum State
        {
            STATE_WOOD, // Wood stack is constructed
            STATE_SMOLDERING, // Smolder slightly
            STATE_REMOVECOVER, // Charburner removes the earth cover
            STATE_HARVEST // Coal is "harvested"
        };

    private:

        /// Status
        State state;

        /// Current (graphical) step
        unsigned short step;
        /// Current step of the step (same graphics during the different sub steps)
        unsigned short sub_step;

        /// Event for glowing
        GameEvent* event;

    public:

        noCharburnerPile(const MapPoint pt);
        noCharburnerPile(SerializedGameData& sgd, const unsigned obj_id);

        ~noCharburnerPile() override;

        /// Aufräummethoden
    protected:  void Destroy_noCharburnerPile();
    public:     void Destroy() override { Destroy_noCharburnerPile(); }

        /// Serialisierungsfunktionen
    protected:  void Serialize_noCharburnerPile(SerializedGameData& sgd) const;
    public:     void Serialize(SerializedGameData& sgd) const override { Serialize_noCharburnerPile(sgd); }

        GO_Type GetGOT() const override { return GOT_CHARBURNERPILE; }

        void Draw(DrawPoint drawPt) override;
        void HandleEvent(const unsigned int id) override;

        BlockingManner GetBM() const override { return BM_CHARBURNERPILE; }

        /// Get the current state of the charburner pile
        State GetState() const { return state; }

        /// Charburner has worked on it --> Goto next step
        void NextStep();

        /// Dertermines if the charburner pile needs wood or grain
        /// Only graphical "effect", dertermines which ware the charburner will be carrying
        enum WareType
        {
            WT_WOOD,
            WT_GRAIN
        };
        WareType GetNeededWareType() const;

};


#endif
