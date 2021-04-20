// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/MilitarySquares.h"
#include "buildings/nobBaseMilitary.h"
#include "helpers/containerUtils.h"
#include "gameData/MilitaryConsts.h"

MilitarySquares::MilitarySquares() : size_(MapExtent::all(0)) {}

void MilitarySquares::Init(const MapExtent& mapSize)
{
    RTTR_Assert(size_ == MapExtent::all(0));     // Already initialized
    RTTR_Assert(mapSize.x > 0 && mapSize.y > 0); // No empty map
    // Calculate size (rounding up)
    size_ = (mapSize + MapExtent::all(MILITARY_SQUARE_SIZE - 1)) / MILITARY_SQUARE_SIZE;
    squares.resize(size_.x * size_.y);
}

void MilitarySquares::Clear()
{
    squares.clear();
    size_ = MapExtent::all(0);
}

std::list<nobBaseMilitary*>& MilitarySquares::GetSquare(const MapPoint pt)
{
    MapPoint milPt = pt / MILITARY_SQUARE_SIZE;
    return squares[milPt.y * size_.x + milPt.x];
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
    const Position offsets = elMin((size_ + Position::all(1)) / 2, Position::all(radius));

    // Convert to military coords
    const Position milPos(pt / MILITARY_SQUARE_SIZE);

    const Position firstPt = milPos - offsets;
    const Position lastPt = milPos + offsets;

    // List with unique(!) military buildings
    sortedMilitaryBlds buildings;

    // Create list
    for(int cy = firstPt.y; cy <= lastPt.y; ++cy)
    {
        // Handle wrap-around
        int realY = cy;
        if(realY < 0)
            realY += size_.y;
        else if(realY >= static_cast<int>(size_.y))
            realY -= size_.y;
        RTTR_Assert(realY >= 0 && realY < static_cast<int>(size_.y));
        for(int cx = firstPt.x; cx <= lastPt.x; ++cx)
        {
            int realX = cx;
            if(realX < 0)
                realX += size_.x;
            else if(realX >= static_cast<int>(size_.x))
                realX -= size_.x;
            RTTR_Assert(realX >= 0 && realX < static_cast<int>(size_.x));
            const std::list<nobBaseMilitary*>& milBuildings = squares[realY * size_.x + realX];
            for(auto* milBuilding : milBuildings)
                buildings.insert(milBuilding);
        }
    }

    return buildings;
}
