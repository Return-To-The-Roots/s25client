// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PlayerGameCommands.h"
#include "s25util/Serializer.h"

void PlayerGameCommands::Serialize(Serializer& ser) const
{
    checksum.Serialize(ser);

    ser.PushUnsignedInt(gcs.size());
    for(const gc::GameCommandPtr& gc : gcs)
        gc->Serialize(ser);
}

void PlayerGameCommands::Deserialize(Serializer& ser)
{
    checksum.Deserialize(ser);

    gcs.resize(ser.PopUnsignedInt());
    for(gc::GameCommandPtr& gc : gcs)
        gc = gc::GameCommand::Deserialize(ser);
}
