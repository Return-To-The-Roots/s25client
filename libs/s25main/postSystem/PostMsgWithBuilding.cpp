// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PostMsgWithBuilding.h"
#include "buildings/noBaseBuilding.h"

PostMsgWithBuilding::PostMsgWithBuilding(unsigned sendFrame, const std::string& text, PostCategory cat,
                                         const noBaseBuilding& bld, SoundEffect soundEffect)
    : PostMsg(sendFrame, text, cat, bld.getPos(), soundEffect), bldType(bld.getBuildingType()), nation(bld.getNation())
{}

PostMsgWithBuilding::PostMsgWithBuilding(unsigned sendFrame, const std::string& text, PostCategory cat,
                                         BuildingType bld, Nation nation, const MapPoint& pos /*= MapPoint::Invalid()*/)
    : PostMsg(sendFrame, text, cat, pos), bldType(bld), nation(nation)
{}

ITexture* PostMsgWithBuilding::getImage() const
{
    return &noBaseBuilding::getBuildingImage(bldType, nation);
}
