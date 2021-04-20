// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
