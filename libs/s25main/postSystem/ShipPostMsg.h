// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "postSystem/PostMsg.h"

class noShip;

/// Post message concerning a ship. Displays ships location and picture
class ShipPostMsg : public PostMsg
{
public:
    /// Creates the message.
    /// Note: Ship object might get invalidated after this call so do not store!
    ShipPostMsg(unsigned sendFrame, const std::string& text, PostCategory cat, const noShip& ship);
    ITexture* GetImage_() const override;
};
