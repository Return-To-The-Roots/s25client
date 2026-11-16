// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noExtension.h"
#include "SerializedGameData.h"

void noExtension::serialize(SerializedGameData& sgd) const
{
    noBase::serialize(sgd);

    sgd.pushObject(base);
}

noExtension::noExtension(SerializedGameData& sgd, const unsigned obj_id)
    : noBase(sgd, obj_id), base(sgd.popObject<noBase>())
{}

noExtension::~noExtension() = default;
