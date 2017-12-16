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

#ifndef DescriptionHelpers_h__
#define DescriptionHelpers_h__

#include "Rect.h"
#include "WorldDescription.h"
#include "gameTypes/LandscapeType.h"
#include <kaguya/kaguya.hpp>
#include <set>
#include <string>
#include <vector>

namespace descriptionHelpers {

/// Get the landscape from a string
Landscape strToLandscape(const std::string& name);

inline Landscape strToLandscape(const std::string& name)
{
    if(name == "greenland")
        return Landscape::GREENLAND;
    else if(name == "wasteland")
        return Landscape::WASTELAND;
    else if(name == "winterworld")
        return Landscape::WINTERWORLD;
    else
        throw GameDataLoadError("Invalid landscape type: " + name);
}

} // namespace descriptionHelpers

#endif // DescriptionHelpers_h__
