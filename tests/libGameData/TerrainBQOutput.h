// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef TerrainBQOutput_h__
#define TerrainBQOutput_h__

#include "gameData/TerrainDesc.h"
#include <boost/array.hpp>
#include <iostream>

inline std::ostream& operator<<(std::ostream& stream, TerrainBQ bq)
{
    static const boost::array<const char*, 6> bqNames = {{"Nothing", "Danger", "Flag", "Castle", "Mine"}};
    return stream << bqNames[boost::underlying_cast<unsigned>(bq)];
}

#endif // TerrainBQOutput_h__
