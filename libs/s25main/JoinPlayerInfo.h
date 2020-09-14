// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "PlayerInfo.h"

/// Holds information for players relevant during match-making
struct JoinPlayerInfo : public PlayerInfo
{
    std::string originName;
    bool isReady = false;

    JoinPlayerInfo();
    explicit JoinPlayerInfo(const BasePlayerInfo& baseInfo);
    explicit JoinPlayerInfo(const PlayerInfo& playerInfo);
    explicit JoinPlayerInfo(Serializer& ser);

    // Serialize complete struct
    void Serialize(Serializer& ser) const;

    void SetAIName(unsigned playerId);
    // Recovers fixed data in savegames after player slots are swapped
    void FixSwappedSaveSlot(JoinPlayerInfo& other);
};
