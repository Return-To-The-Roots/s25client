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

#include "mapGenerator/MapGenerator.h"
#include "mapGenerator/MapWriter.h"
#include "mapGenerator/Generator.h"
#include "mapGenerator/GreenlandGenerator.h"
#include "helpers/Deleter.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <stdexcept>

typedef boost::interprocess::unique_ptr<Generator, Deleter<Generator> > GeneratorPtr;

void MapGenerator::Create(const std::string& filePath, Style style, const MapSettings& settings)
{
    GeneratorPtr generator;

    // create a random map generator based on the map style
    switch (style)
    {
        case Greenland:
            generator = GeneratorPtr(new GreenlandGenerator(0.5, 0.7, 0.3, 2.5, 2.5, 2.5, 5, 20));
            break;
        case Riverland:
            break;
        case Islands:
            break;
        case Contient:
            generator = GeneratorPtr(new GreenlandGenerator(0.5, 0.7, 0.3, 0.6, 0.6, 0.9, 5, 20));
            break;
        case Migration:
            generator = GeneratorPtr(new GreenlandGenerator(0.7, 0.75, 0.2, 0.4, 0.6, 2.0, 5, 20));
            break;
        case Random:
            generator = GeneratorPtr(new GreenlandGenerator());
            break;
    }
    
    if (generator.get() == NULL)
    {
        throw new std::invalid_argument("Style not supported");
    }

    // generate the random map
    Map* map = generator->Create(settings);

    // create a map writer
    MapWriter* writer = new MapWriter();
    
    // try to write the generated map to a file
    if (!writer->Write(filePath, map))
    {
        // cleanup memory if failed
        delete map;
        delete writer;
        
        throw std::invalid_argument("Failed to write the random map to the filePath");
    }
    
    // cleanup map and writer
    delete map;
    delete writer;
}
