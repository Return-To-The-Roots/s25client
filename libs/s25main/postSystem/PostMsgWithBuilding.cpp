// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PostMsgWithBuilding.h"
#include "buildings/noBaseBuilding.h"

PostMsgWithBuilding::PostMsgWithBuilding(unsigned sendFrame, const std::string& text, PostCategory cat,
                                         const noBaseBuilding& bld, SoundEffect soundEffect)
    : PostMsg(sendFrame, text, cat, bld.GetPos(), soundEffect), bldType(bld.GetBuildingType()), nation(bld.GetNation())
{}

PostMsgWithBuilding::PostMsgWithBuilding(unsigned sendFrame, const std::string& text, PostCategory cat,
                                         BuildingType bld, Nation nation, const MapPoint& pos /*= MapPoint::Invalid()*/)
    : PostMsg(sendFrame, text, cat, pos), bldType(bld), nation(nation)
{}

ITexture* PostMsgWithBuilding::GetImage_() const
{
    return &noBaseBuilding::GetBuildingImage(bldType, nation);
}
