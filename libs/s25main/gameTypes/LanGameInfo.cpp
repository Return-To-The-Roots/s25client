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

#include "LanGameInfo.h"
#include "s25util/Serializer.h"

bool LanGameInfo::Serialize(Serializer& serializer)
{
    if(name.size() > 64)
        name.resize(64);
    if(map.size() > 64)
        map.resize(64);
    if(version.size() > 16)
        version.resize(16);
    serializer.PushString(name);
    serializer.PushBool(hasPwd);
    serializer.PushString(map);
    serializer.PushUnsignedChar(curNumPlayers);
    serializer.PushUnsignedChar(maxNumPlayers);
    serializer.PushUnsignedShort(port);
    serializer.PushBool(isIPv6);
    serializer.PushString(version);
    serializer.PushString(revision);
    return true;
}

bool LanGameInfo::Deserialize(Serializer& serializer)
{
    name = serializer.PopString();
    hasPwd = serializer.PopBool();
    map = serializer.PopString();
    curNumPlayers = serializer.PopUnsignedChar();
    maxNumPlayers = serializer.PopUnsignedChar();
    port = serializer.PopUnsignedShort();
    isIPv6 = serializer.PopBool();
    version = serializer.PopString();
    revision = serializer.PopString();
    return true;
}
