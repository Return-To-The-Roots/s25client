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

#include "defines.h" // IWYU pragma: keep
#include "PostMsgWithBuilding.h"
#include "buildings/noBaseBuilding.h"
#include "Loader.h"
#include <stdexcept>

PostMsgWithBuilding::PostMsgWithBuilding(unsigned sendFrame, const std::string& text, PostMessageCategory cat, const noBaseBuilding& bld):
    PostMsg(sendFrame, text, cat, bld.GetPos()), bldType(bld.GetBuildingType()), nation(bld.GetNation())
{}

PostMsgWithBuilding::PostMsgWithBuilding(unsigned sendFrame, const std::string& text, PostMessageCategory cat, BuildingType bld, Nation nation, const MapPoint& pos /*= MapPoint::Invalid()*/):
    PostMsg(sendFrame, text, cat, pos), bldType(bld), nation(nation)
{}

glArchivItem_Bitmap* PostMsgWithBuilding::GetImage_() const
{
    return noBaseBuilding::GetBuildingImage(bldType, nation);
}
