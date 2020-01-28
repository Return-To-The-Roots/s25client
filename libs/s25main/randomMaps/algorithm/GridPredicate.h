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

#ifndef GridPredicate_h__
#define GridPredicate_h__

#include "randomMaps/algorithm/GridUtility.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

class GridPredicate
{
public:
    GridPredicate() {}
    
    virtual ~GridPredicate() {}
    virtual bool Check(const Position& point, const MapExtent& size) const = 0;
};

class DistancePredicate : public GridPredicate
{
private:
    Position reference_;
    double distance_;
    
public:
    DistancePredicate(const Position& reference, double distance)
        : reference_(reference), distance_(distance) {}
    
    ~DistancePredicate() {}
    
    bool Check(const Position& p, const MapExtent& size) const
    {
        return GridUtility::Distance(reference_, p, size) < distance_;
    }
};

class ThresholdPredicate : public GridPredicate
{
private:
    std::vector<double>& values_;
    double threshold_;
    
public:
    ThresholdPredicate(std::vector<double>& values, double threshold)
        : values_(values), threshold_(threshold) {}
    
    ~ThresholdPredicate() {}
    
    bool Check(const Position& p, const MapExtent& size) const
    {
        return values_[p.x + p.y * size.x] < threshold_;
    }
};

class CharThresholdPredicate : public GridPredicate
{
private:
    std::vector<unsigned char>& values_;
    unsigned char threshold_;
    
public:
    CharThresholdPredicate(std::vector<unsigned char>& values, unsigned char threshold)
    : values_(values), threshold_(threshold) {}
    
    ~CharThresholdPredicate() {}
    
    bool Check(const Position& p, const MapExtent& size) const
    {
        return values_[p.x + p.y * size.x] < threshold_;
    }
};

class ExclusionPredicate : public GridPredicate
{
private:
    std::vector<bool> exclude_;
    
public:
    ExclusionPredicate(const std::vector<bool>& exclude) : exclude_(exclude) {}
    ~ExclusionPredicate() {}
    
    bool Check(const Position& p, const MapExtent& size) const
    {
        return !exclude_[p.x + p.y * size.x];
    }
};

class IncludePredicate : public GridPredicate
{
private:
    std::vector<bool> include_;
    
public:
    IncludePredicate(const std::vector<bool>& include) : include_(include) {}
    ~IncludePredicate() {}
    
    bool Check(const Position& p, const MapExtent& size) const
    {
        return include_[p.x + p.y * size.x];
    }
};

#endif // GridPredicate_h__
