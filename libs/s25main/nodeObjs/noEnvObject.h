// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "noStaticObject.h"
class SerializedGameData;

class noEnvObject : public noStaticObject
{
public:
    noEnvObject(MapPoint pos, unsigned short id, unsigned short file = 0xFFFF);
    noEnvObject(SerializedGameData& sgd, unsigned obj_id);

    GO_Type GetGOT() const final { return GO_Type::Envobject; }
};
