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

#include "AsyncChecksum.h"
#include "GameCommand.h"
#include <utility>
#include <vector>

/// GameCommands for 1 player
struct PlayerGameCommands
{
    /// Checksum for this NWF
    AsyncChecksum checksum;
    /// The game gammands for this NWF
    std::vector<gc::GameCommandPtr> gcs;

    PlayerGameCommands() = default;
    PlayerGameCommands(const AsyncChecksum& checksum, std::vector<gc::GameCommandPtr> gcs)
        : checksum(checksum), gcs(std::move(gcs))
    {}
    void Serialize(Serializer& ser) const;
    void Deserialize(Serializer& ser);
};
