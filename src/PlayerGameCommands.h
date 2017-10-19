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

#ifndef PlayerGameCommands_h__
#define PlayerGameCommands_h__

#include "AsyncChecksum.h"
#include <vector>
#include "GameCommand.h"

/// GameCommands for 1 player
struct PlayerGameCommands
{
    /// Checksumme, die der Spieler übermittelt
    AsyncChecksum checksum;
    /// Die einzelnen GameCommands
    std::vector<gc::GameCommandPtr> gcs;

    PlayerGameCommands() {}
    PlayerGameCommands(const AsyncChecksum& checksum, const std::vector<gc::GameCommandPtr>& gcs): checksum(checksum), gcs(gcs)
    {}
    void Serialize(Serializer& ser) const;
    void Deserialize(Serializer& ser);
};

#endif // PlayerGameCommands_h__
