// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
