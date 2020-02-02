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

#include "Identity.h"
#include "ReturnConst.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/ShipDirection.h"
#include <vector>

/// Base class for a map. A map has a size and functions for getting from one point to another in that map
class MapBase
{
    /// Size of the map in nodes
    MapExtent size_;

public:
    static unsigned CreateGUIID(MapPoint pt);

    MapBase();

    virtual void Resize(const MapExtent& newSize);

    /// Return the size of the world
    unsigned short GetWidth() const { return GetSize().x; }
    unsigned short GetHeight() const { return GetSize().y; }
    MapExtent GetSize() const { return size_; }
    /// Returns a MapPoint from a point. This ensures, the coords are actually in the map [0, mapSize)
    MapPoint MakeMapPoint(Position pt) const;
    /// Returns the linear index for a map point
    unsigned GetIdx(MapPoint pt) const;

    /// Get coordinates of neighbor in the given direction
    MapPoint GetNeighbour(MapPoint pt, Direction dir) const;
    /// Return neighboring point (2nd layer: dir 0-11)
    MapPoint GetNeighbour2(MapPoint, unsigned dir) const;
    // Convenience functions for the above function
    MapCoord GetXA(MapPoint pt, Direction dir) const;
    // Gets all neighbors (in all directions) for given position
    std::vector<MapPoint> GetNeighbours(const MapPoint& pt) const;

    /// Return all points in a radius around pt (excluding pt) that satisfy a given condition.
    /// Points can be transformed (e.g. to flags at those points) by the functor taking a map point and a radius
    /// Number of results is constrained to maxResults (if > 0)
    template<int T_maxResults = -1, class T_TransformPt = Identity<MapPoint>,
             class T_IsValidPt = ReturnConst<bool, true>>
    std::vector<typename T_TransformPt::result_type>
    GetPointsInRadius(MapPoint pt, unsigned radius, T_TransformPt&& transformPt = T_TransformPt(),
                      T_IsValidPt&& isValid = T_IsValidPt(), bool includePt = false) const;
    std::vector<MapPoint> GetPointsInRadiusWithCenter(const MapPoint pt, unsigned radius) const
    {
        return GetPointsInRadius<-1>(pt, radius, Identity<MapPoint>(), ReturnConst<bool, true>(), true);
    }
    /// Returns true, if the IsValid functor returns true for any point in the given radius
    /// If includePt is true, then the point itself is also checked
    template<class T_IsValidPt>
    bool CheckPointsInRadius(MapPoint pt, unsigned radius, T_IsValidPt&& isValid, bool includePt) const;

    /// Return the distance between 2 points on the map (includes wrapping around map borders)
    unsigned CalcDistance(const Position& p1, const Position& p2) const;
    unsigned CalcDistance(const MapPoint p1, const MapPoint p2) const
    {
        return CalcDistance(Position(p1), Position(p2));
    }
    /// Return the direction for ships for going from one point to another
    ShipDirection GetShipDir(MapPoint fromPt, MapPoint toPt) const;
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

// Convenience functions
inline MapCoord MapBase::GetXA(const MapPoint pt, Direction dir) const
{
    return GetNeighbour(pt, dir).x;
}

inline unsigned MapBase::GetIdx(const MapPoint pt) const
{
    RTTR_Assert(pt.x < size_.x && pt.y < size_.y);
    return static_cast<unsigned>(pt.y) * size_.x + pt.x;
}

template<int T_maxResults, class T_TransformPt, class T_IsValidPt>
inline std::vector<typename T_TransformPt::result_type>
MapBase::GetPointsInRadius(const MapPoint pt, unsigned radius, T_TransformPt&& transformPt, T_IsValidPt&& isValid,
                           bool includePt) const
{
    using Element = typename T_TransformPt::result_type;
    std::vector<Element> result;
    if(includePt)
    {
        Element el = transformPt(pt, 0);
        if(isValid(el))
        {
            result.push_back(el);
            if(T_maxResults == 1u)
                return result;
        }
    }
    MapPoint curStartPt = pt;
    for(unsigned r = 1; r <= radius; ++r)
    {
        // Go one level/hull to the left
        curStartPt = GetNeighbour(curStartPt, Direction::WEST);
        // Now iterate over the "circle" of radius r by going r steps in one direction, turn right and repeat
        MapPoint curPt = curStartPt;
        for(unsigned i = Direction::NORTHEAST; i < Direction::NORTHEAST + Direction::COUNT; ++i)
        {
            for(unsigned step = 0; step < r; ++step)
            {
                Element el = transformPt(curPt, r);
                if(isValid(el))
                {
                    result.push_back(el);
                    if(T_maxResults > 0 && static_cast<int>(result.size()) > T_maxResults)
                        return result;
                }
                curPt = GetNeighbour(curPt, Direction(i));
            }
        }
    }
    return result;
}

template<class T_IsValidPt>
inline bool MapBase::CheckPointsInRadius(const MapPoint pt, unsigned radius, T_IsValidPt&& isValid,
                                         bool includePt) const
{
    if(includePt && isValid(pt, 0))
        return true;
    MapPoint curStartPt = pt;
    for(unsigned r = 1; r <= radius; ++r)
    {
        // Go one level/hull to the left
        curStartPt = GetNeighbour(curStartPt, Direction::WEST);
        // Now iterate over the "circle" of radius r by going r steps in one direction, turn right and repeat
        MapPoint curPt = curStartPt;
        for(unsigned i = Direction::NORTHEAST; i < Direction::NORTHEAST + Direction::COUNT; ++i)
        {
            for(unsigned step = 0; step < r; ++step)
            {
                if(isValid(curPt, r))
                    return true;
                curPt = GetNeighbour(curPt, Direction(i));
            }
        }
    }
    return false;
}
