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

#ifndef MapWriter_h__
#define MapWriter_h__

#include <string>
#include "mapGenerator/Map.h"

// The MapWriter is used to write mgMap instances to a file.
class MapWriter
{
    public:

    /**
     * Writes the specified map to a file.
     * @param filePath output file to save the map to
     * @param map map instance to save to a file
     * @return true if map is written to the file path successfully, false otherwise
     */
    bool Write(const std::string& filePath, Map* map);
};

#endif // MapWriter_h__
