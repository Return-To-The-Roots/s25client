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

#ifndef PostMsgWithBuilding_h__
#define PostMsgWithBuilding_h__

#include "postSystem/PostMsg.h"
#include "gameTypes/BuildingTypes.h"
#include "gameData/NationConsts.h"

class glArchivItem_Bitmap;
class noBaseBuilding;

/// Message that is related to a building, showing the buildings image and a GoTo button
class PostMsgWithBuilding: public PostMsg
{
public:
    /// Creates the message.
    /// NOTE: Building is only valid during the call (do not store)
    PostMsgWithBuilding(unsigned sendFrame, const std::string& text, PostCategory cat, const noBaseBuilding& bld);
    PostMsgWithBuilding(unsigned sendFrame, const std::string& text, PostCategory cat, BuildingType bld, Nation nation, const MapPoint& pos = MapPoint::Invalid());

    glArchivItem_Bitmap* GetImage_() const override;

private:
    const BuildingType bldType;
    const Nation nation;
};

#endif // PostMsgWithBuilding_h__
