// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#include "randomMaps/waters/River.h"
#include "randomMaps/algorithm/GridUtility.h"

#include <algorithm>
#include <set>

River::River(RiverMotion motion, RiverBrush brush, int direction, Tile source)
    : motion_(motion), brush_(brush), direction_(direction), location_(source)
{
    
}

River& River::ExtendBy(int length, const MapExtent& size)
{
    if (length == 0)
    {
        return *this;
    }

    coverage_.push_back(location_);
    location_ = motion_.Next(location_, direction_, size);

    for (auto i = 0u; i < streams_.size(); i++)
    {
        streams_[i].ExtendBy(1, size);
    }
    
    return ExtendBy(length - 1, size);
}

River& River::Steer(bool clockwise, bool swap)
{
    auto direction = clockwise ? (direction_ + 1) % 8 : direction_ - 1;
    direction_ = direction < 0 ? 7 : direction;
    
    for (auto i = 0u; i < streams_.size(); i++)
    {
        streams_[i].Steer(swap ? !clockwise : clockwise, swap);
    }

    return *this;
}

River& River::Split(bool clockwise, bool recursive)
{
    if (recursive)
    {
        for (auto i = 0u; i < streams_.size(); i++)
        {
            streams_[i].Split(clockwise, recursive);
        }
    }

    auto direction = clockwise ? (direction_ + 1) % 8 : direction_ - 1;
    auto river = River(motion_.Clone(), brush_, direction < 0 ? 7 : direction, location_);
    
    streams_.push_back(river);

    return *this;
}

void River::Create(Map* map, unsigned char seaLevel)
{
    brush_.Paint(coverage_, map, seaLevel);
    
    for (auto i = 0u; i < streams_.size(); i++)
    {
        streams_[i].Create(map, seaLevel);
    }
}
