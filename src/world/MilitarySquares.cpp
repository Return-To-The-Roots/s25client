// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h"
#include "world/MilitarySquares.h"
#include "buildings/nobBaseMilitary.h"
#include "gameData/MilitaryConsts.h"
#include "helpers/containerUtils.h"

MilitarySquares::MilitarySquares(): width(0), height(0)
{}

void MilitarySquares::Init(const unsigned short mapWidth, const unsigned short mapHeight)
{
    // Calculate size (rounding up)
    width  = (mapWidth + MILITARY_SQUARE_SIZE - 1) / MILITARY_SQUARE_SIZE;
    height = (mapHeight + MILITARY_SQUARE_SIZE - 1) / MILITARY_SQUARE_SIZE;
    squares.clear();
    squares.resize(width * height);
}

void MilitarySquares::Clear()
{
    squares.clear();
    width = height = 0;
}

std::list<nobBaseMilitary*>& MilitarySquares::GetSquare(const MapPoint pt)
{
    return squares[(pt.y / MILITARY_SQUARE_SIZE) * width + pt.x / MILITARY_SQUARE_SIZE];
}

void MilitarySquares::Add(nobBaseMilitary* const bld)
{
    GetSquare(bld->GetPos()).push_back(bld);
}

void MilitarySquares::Remove(nobBaseMilitary* const bld)
{
    RTTR_Assert(helpers::contains(GetSquare(bld->GetPos()), bld));
    GetSquare(bld->GetPos()).remove(bld);
}

sortedMilitaryBlds MilitarySquares::GetBuildingsInRange(const MapPoint pt, unsigned short radius) const
{
    // maximum radius is half the size (rounded up) to avoid overlapping
    const Point<int> offsets = Point<int>(std::min<unsigned>((width  + 1 / 2), radius),
                                          std::min<unsigned>((height + 1 / 2), radius));

    // Convert to military coords
    const Point<int> milPos = Point<int>(pt) / MILITARY_SQUARE_SIZE;

    const Point<int> firstPt = milPos - offsets;
    const Point<int> lastPt  = milPos + offsets;

    // List with unique(!) military buildings
    sortedMilitaryBlds buildings;

    // Create list
    for(int cy = firstPt.y; cy <= lastPt.y; ++cy)
    {
        // Handle wrap-around
        int realY = cy;
        if(realY < 0)
            realY += height;
        else if(realY >= static_cast<int>(height))
            realY -= height;
        RTTR_Assert(realY >= 0 && realY < static_cast<int>(height));
        for(int cx = firstPt.x; cx <= lastPt.x; ++cx)
        {
            int realX = cx;
            if(realX < 0)
                realX += width;
            else if(realX >= static_cast<int>(width))
                realX -= width;
            RTTR_Assert(realX >= 0 && realX < static_cast<int>(width));
            const std::list<nobBaseMilitary*>& milBuildings = squares[realY * width + realX];
            for(std::list<nobBaseMilitary*>::const_iterator it = milBuildings.begin(); it != milBuildings.end(); ++it)
                buildings.insert(*it);
        }
    }

    return buildings;
}
