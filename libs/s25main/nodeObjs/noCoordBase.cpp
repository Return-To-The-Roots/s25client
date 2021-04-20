// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noCoordBase.h"
#include "SerializedGameData.h"
#include "world/GameWorld.h"
#include "gameData/MapConsts.h"

void noCoordBase::Serialize(SerializedGameData& sgd) const
{
    noBase::Serialize(sgd);

    helpers::pushPoint(sgd, pos);
}

noCoordBase::noCoordBase(SerializedGameData& sgd, const unsigned obj_id) : noBase(sgd, obj_id), pos(sgd.PopMapPoint())
{}
