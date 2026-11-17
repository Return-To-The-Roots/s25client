// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noCoordBase.h"

class SerializedGameData;
class GameEvent;

class noGrapefield : public noCoordBase
{
private:
    /// Type of the grape field (for drawing only, currently 0-1)
    unsigned char type;

    /// State
    enum class State : uint8_t
    {
        GrowingWaiting, /// Growing phase, waiting of the next growing boost
        Growing,        /// Growing
        Normal,         /// Is fully grown and withers after a time
        Withering       /// Disappearing
    } state;
    friend constexpr auto maxEnumValue(State) { return State::Withering; }

    /// Size of the field (0-3), 3 fully grown
    unsigned char size;

    /// Grow-Event
    const GameEvent* event;

public:
    noGrapefield(MapPoint pos);
    noGrapefield(SerializedGameData& sgd, unsigned obj_id);

    ~noGrapefield() override;

    void destroy() override;
    void serialize(SerializedGameData& sgd) const override;

    GO_Type getGOT() const final { return GO_Type::Grapefield; }

    void draw(DrawPoint drawPt) override;
    void handleEvent(unsigned id) override;

    BlockingManner getBM() const override { return BlockingManner::FlagsAround; }

    /// Can we harvest it?
    bool isHarvestable() const { return size == 3 && state == State::Normal; }

    /// Return the ID of the withered grape field in the wine_bobs
    unsigned getHarvestID() const;

    /// Winegrower starts harvesting the field
    void beginHarvesting();
    /// Winegrower is interrupted during harvesting
    void abortHarvesting();
};
