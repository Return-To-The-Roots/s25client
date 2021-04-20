// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIInfo.h"
#include "helpers/serializeEnums.h"
#include "s25util/Serializer.h"

namespace AI {
Info::Info(Serializer& ser) : type(helpers::popEnum<Type>(ser)), level(helpers::popEnum<Level>(ser)) {}

void Info::serialize(Serializer& ser) const
{
    helpers::pushEnum<uint8_t>(ser, type);
    helpers::pushEnum<uint8_t>(ser, level);
}
} // namespace AI
