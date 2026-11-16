// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noCoordBase.h"

class SerializedGameData;
class GameEvent;

/// The wood/coal piles made by the charburner
class noCharburnerPile : public noCoordBase
{
public:
    /// Status
    enum class State : uint8_t
    {
        Wood,        // Wood stack is constructed
        Smoldering,  // Smolder slightly
        RemoveCover, // Charburner removes the earth cover
        Harvest      // Coal is "harvested"
    };
    friend constexpr auto maxEnumValue(State) { return State::Harvest; }

private:
    /// Status
    State state;

    /// Current (graphical) step
    unsigned short step;
    /// Current step of the step (same graphics during the different sub steps)
    unsigned short sub_step;

    /// Event for glowing
    const GameEvent* event;

public:
    noCharburnerPile(MapPoint pos);
    noCharburnerPile(SerializedGameData& sgd, unsigned obj_id);

    ~noCharburnerPile() override;

    void destroy() override;
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::Charburnerpile; }

    void draw(DrawPoint drawPt) override;
    void handleEvent(unsigned id) override;

    BlockingManner getBM() const override { return BlockingManner::NothingAround; }

    /// Get the current state of the charburner pile
    State getState() const { return state; }

    /// Charburner has worked on it --> Goto next step
    void nextStep();

    /// Dertermines if the charburner pile needs wood or grain
    /// Only graphical "effect", dertermines which ware the charburner will be carrying
    enum class WareType
    {
        Wood,
        Grain
    };
    WareType getNeededWareType() const;
};
