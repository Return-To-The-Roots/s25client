// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noGranite.h"

#include "FOWObjects.h"
#include "Loader.h"
#include "SerializedGameData.h"

#include "ogl/glSmartBitmap.h"

noGranite::noGranite(const GraniteType type, const unsigned char state)
    : noBase(NodalObjectType::Granite), type(type), state(state)
{}

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
    LOADER.granite_cache[type][state].draw(drawPt);
}

std::unique_ptr<FOWObject> noGranite::CreateFOWObject() const
{
    return std::make_unique<fowGranite>(type, state);
}

void noGranite::Hew()
{
    if(state)
        --state;
}
