// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef BQOutput_h__
#define BQOutput_h__

#include "gameTypes/BuildingQuality.h"
#include <boost/array.hpp>
#include <iosfwd>

const boost::array<const char*, 6> bqNames = {{"Nothing", "Flag", "Hut", "House", "Castle", "Mine"}};
inline std::ostream& operator<<(std::ostream& stream, BuildingQuality bq)
{
    return stream << bqNames[bq];
}

#endif // BQOutput_h__
