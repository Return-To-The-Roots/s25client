// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noGranite.h"

#include "FOWObjects.h"
#include "Loader.h"
#include "SerializedGameData.h"

#include "ogl/glSmartBitmap.h"
#include <algorithm>

noGranite::noGranite(const GraniteType type, const unsigned char state)
    : noBase(NodalObjectType::Granite), type(type), state(state)
{}

unsigned char noGranite::EncodeBoostedState(const unsigned char legacyState)
{
    // legacy yield is (legacyState + 1). For 2x yield, target durability is (2 * (legacyState + 1) - 1).
    const unsigned rawBoostedState =
      std::min<unsigned>(2u * (static_cast<unsigned>(legacyState) + 1u) - 1u, RAW_STATE_MASK);
    return static_cast<unsigned char>(BOOSTED_STATE_FLAG | rawBoostedState);
}

void noGranite::Serialize(SerializedGameData& sgd) const
{
    noBase::Serialize(sgd);

    sgd.PushEnum<uint8_t>(type);
    sgd.PushUnsignedChar(state);
}

noGranite::noGranite(SerializedGameData& sgd, const unsigned obj_id)
    : noBase(sgd, obj_id), type(sgd.Pop<GraniteType>()), state(sgd.PopUnsignedChar())
{}

void noGranite::Draw(DrawPoint drawPt)
{
    LOADER.granite_cache[type][GetVisualSize()].draw(drawPt);
}

std::unique_ptr<FOWObject> noGranite::CreateFOWObject() const
{
    return std::make_unique<fowGranite>(type, state);
}

void noGranite::Hew()
{
    const unsigned char rawState = state & RAW_STATE_MASK;
    if(rawState == 0)
        return;

    const unsigned char flags = state & BOOSTED_STATE_FLAG;
    state = static_cast<unsigned char>(flags | (rawState - 1u));
}

unsigned char noGranite::GetVisualSize() const
{
    const unsigned char rawState = state & RAW_STATE_MASK;
    const bool boosted = (state & BOOSTED_STATE_FLAG) != 0u;
    const unsigned visualSize = boosted ? (rawState / 2u) : rawState;
    return static_cast<unsigned char>(std::min<unsigned>(visualSize, 5u));
}
