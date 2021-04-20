// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ShipPostMsg.h"
#include "Loader.h"
#include "nodeObjs/noShip.h"

ShipPostMsg::ShipPostMsg(unsigned sendFrame, const std::string& text, PostCategory cat, const noShip& ship)
    : PostMsg(sendFrame, text, cat, ship.GetPos())
{}

ITexture* ShipPostMsg::GetImage_() const
{
    return LOADER.GetTextureN("boot_z", 12);
}
