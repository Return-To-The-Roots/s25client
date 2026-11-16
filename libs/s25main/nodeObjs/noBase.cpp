// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noBase.h"
#include "SerializedGameData.h"

noBase::noBase(SerializedGameData& sgd, const unsigned obj_id) : GameObject(sgd, obj_id)
{
    nop = sgd.pop<NodalObjectType>();
}

void noBase::Serialize(SerializedGameData& sgd) const
{
    sgd.pushEnum<uint8_t>(nop);
}

std::unique_ptr<FOWObject> noBase::createFOWObject() const
{
    return nullptr;
}

BlockingManner noBase::getBM() const
{
    return BlockingManner::None;
}

/// Gibt zurück, ob sich das angegebene Objekt zwischen zwei Punkten bewegt
bool noBase::isMoving() const
{
    return false;
}
