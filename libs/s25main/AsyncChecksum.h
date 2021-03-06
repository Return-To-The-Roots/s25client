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

#pragma once

#include <iosfwd>

class Game;
class Serializer;

/// Checksum of the game before the game commands of any player is executed
struct AsyncChecksum
{
    unsigned randChecksum;
    unsigned objCt, objIdCt;
    unsigned eventCt, evInstanceCt;
    AsyncChecksum();
    AsyncChecksum(unsigned randChecksum, unsigned objCt, unsigned objIdCt, unsigned eventCt, unsigned evInstanceCt);
    void Serialize(Serializer& ser) const;
    void Deserialize(Serializer& ser);
    /// Get a hash for this checksum
    unsigned getHash() const;

    static AsyncChecksum create(const Game& game);

    bool operator==(const AsyncChecksum& rhs) const;
    bool operator!=(const AsyncChecksum& rhs) const;
};

inline bool AsyncChecksum::operator==(const AsyncChecksum& rhs) const
{
    return randChecksum == rhs.randChecksum && objCt == rhs.objCt && objIdCt == rhs.objIdCt && eventCt == rhs.eventCt
           && evInstanceCt == rhs.evInstanceCt;
}

inline bool AsyncChecksum::operator!=(const AsyncChecksum& rhs) const
{
    return !(*this == rhs);
}

std::ostream& operator<<(std::ostream& os, const AsyncChecksum& checksum);