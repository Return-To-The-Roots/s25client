// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noBase.h"
#include "gameTypes/MapTypes.h"
class FOWObject;
class SerializedGameData;

class noGranite final : public noBase
{
    GraniteType type;    // Which type (there are 2)
    unsigned char state; // Serialized state (legacy: 0-5, boosted: flag + durability)

    static constexpr unsigned char BOOSTED_STATE_FLAG = 0x80;
    static constexpr unsigned char RAW_STATE_MASK = 0x7F;

public:
    noGranite(GraniteType type, unsigned char state);
    noGranite(SerializedGameData& sgd, unsigned obj_id);

    /// Encodes a legacy granite state into boosted state with exactly 2x durability.
    static unsigned char EncodeBoostedState(unsigned char legacyState);

    void Destroy() override {}
    void Serialize(SerializedGameData& sgd) const override;

    GO_Type GetGOT() const final { return GO_Type::Granite; }

    void Draw(DrawPoint drawPt) override;

    BlockingManner GetBM() const override { return BlockingManner::FlagsAround; }

    /// Creates its own FOW object as a visual "memory" for the fog of war
    std::unique_ptr<FOWObject> CreateFOWObject() const override;

    /// "Works" the granite block --> chips off one stone
    void Hew();

    /// Returns true if the granite block has only 1 stone left and can be destroyed
    bool IsSmall() const { return (state & RAW_STATE_MASK) == 0; }

    /// Return the size of this pile.
    unsigned char GetSize() const { return state & RAW_STATE_MASK; }

    /// Return visual size index in range [0, 5] used for rendering.
    unsigned char GetVisualSize() const;
};
