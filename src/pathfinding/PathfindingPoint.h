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

#ifndef PathfindingPoint_h__
#define PathfindingPoint_h__

/// Punkte als Verweise auf die obengenannen Knoten, damit nur die beiden Koordinaten x, y im set mit rumgeschleppt werden müsen
struct PathfindingPoint
{
public:
    const unsigned id_, distance_;
    unsigned estimate_;

    PathfindingPoint(const unsigned id, const unsigned distance, const unsigned curWay): id_(id), distance_(distance), estimate_(curWay + distance_)
    {}

    /// Operator für den Vergleich
    bool operator<(const PathfindingPoint& rhs) const
    {
        // Wenn die Wegkosten gleich sind, vergleichen wir die Koordinaten, da wir für std::set eine streng monoton steigende Folge brauchen
        if(estimate_ == rhs.estimate_)
            return (id_ < rhs.id_);
        else
            return (estimate_ < rhs.estimate_);
    }
};

#endif // PathfindingPoint_h__
