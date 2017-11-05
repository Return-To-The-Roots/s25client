// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "rttrDefines.h" // IWYU pragma: keep
#include "PlayerGameCommands.h"
#include "libutil/Serializer.h"
#include <boost/foreach.hpp>

void PlayerGameCommands::Serialize(Serializer& ser) const
{
    ser.PushUnsignedInt(checksum.randChecksum);
    ser.PushUnsignedInt(checksum.objCt);
    ser.PushUnsignedInt(checksum.objIdCt);
    ser.PushUnsignedInt(gcs.size());

    BOOST_FOREACH(const gc::GameCommandPtr& gc, gcs)
    {
        ser.PushUnsignedChar(gc->GetType());
        gc->Serialize(ser);
    }
}

void PlayerGameCommands::Deserialize(Serializer& ser)
{
    checksum.randChecksum = ser.PopUnsignedInt();
    checksum.objCt = ser.PopUnsignedInt();
    checksum.objIdCt = ser.PopUnsignedInt();
    gcs.resize(ser.PopUnsignedInt());
    BOOST_FOREACH(gc::GameCommandPtr& gc, gcs)
    {
        gc::Type type = gc::Type(ser.PopUnsignedChar());
        gc = gc::GameCommand::Deserialize(type, ser);
    }
}
