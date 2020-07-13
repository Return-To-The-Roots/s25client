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

#include "AIInfo.h"
#include "s25util/Serializer.h"

namespace AI {
Info::Info(Serializer& ser) : type(static_cast<Type>(ser.PopUnsignedChar())), level(static_cast<Level>(ser.PopUnsignedChar())) {}

void Info::serialize(Serializer& ser) const
{
    ser.PushUnsignedChar(static_cast<unsigned char>(type));
    ser.PushUnsignedChar(static_cast<unsigned char>(level));
}
} // namespace AI
