// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/// Punkte als Verweise auf die obengenannen Knoten, damit nur die beiden Koordinaten x, y im set mit rumgeschleppt
/// werden müsen
struct PathfindingPoint
{
public:
    const unsigned id_, distance_;
    unsigned estimate_;
    bool even_;

    PathfindingPoint(unsigned id, unsigned distance, unsigned curWay, bool even)
        : id_(id), distance_(distance), estimate_(curWay + distance_), even_(even)
    {}

    /// Operator für den Vergleich
    bool operator<(const PathfindingPoint& rhs) const
    {
        // Wenn die Wegkosten gleich sind, vergleichen wir die Koordinaten, da wir für std::set eine streng monoton
        // steigende Folge brauchen
        if(estimate_ == rhs.estimate_)
            return (id_ < rhs.id_);
        else
            return (estimate_ < rhs.estimate_);
    }
};
