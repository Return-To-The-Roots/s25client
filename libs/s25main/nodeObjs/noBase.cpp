// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noBase.h"
#include "SerializedGameData.h"

noBase::noBase(SerializedGameData& sgd, const unsigned obj_id) : GameObject(sgd, obj_id)
{
    nop = sgd.Pop<NodalObjectType>();
}

void noBase::Serialize(SerializedGameData& sgd) const
{
    sgd.PushEnum<uint8_t>(nop);
}

std::unique_ptr<FOWObject> noBase::CreateFOWObject() const
{
    return nullptr;
}

BlockingManner noBase::GetBM() const
{
    return BlockingManner::None;
}

/// Gibt zurück, ob sich das angegebene Objekt zwischen zwei Punkten bewegt
bool noBase::IsMoving() const
{
    return false;
}
