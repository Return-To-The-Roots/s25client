// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PlayerGameCommands.h"
#include "s25util/Serializer.h"

void PlayerGameCommands::serialize(Serializer& ser) const
{
    checksum.serialize(ser);

    ser.pushUnsignedInt(gcs.size());
    for(const gc::GameCommandPtr& gc : gcs)
        gc->serialize(ser);
}

void PlayerGameCommands::deserialize(gc::Deserializer& ser)
{
    checksum.deserialize(ser);

    gcs.resize(ser.popUnsignedInt());
    for(gc::GameCommandPtr& gc : gcs)
        gc = gc::GameCommand::deserialize(ser);
}
