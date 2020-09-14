// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include <string>

class ILocalGameState
{
public:
    /// Get the local player id
    virtual unsigned GetPlayerId() const = 0;
    /// Return true if the local player is the host
    virtual bool IsHost() const = 0;
    /// Convert a number of GameFrames into real time (HH:MM:SS or MM:SS if hours = 0)
    virtual std::string FormatGFTime(unsigned numGFs) const = 0;
    /// Send a chat message to the local player
    virtual void SystemChat(const std::string& text) = 0;
};
