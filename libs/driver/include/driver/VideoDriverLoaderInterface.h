// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "KeyEvent.h"
#include "MouseCoords.h"
#include <boost/config.hpp>

class BOOST_SYMBOL_VISIBLE VideoDriverLoaderInterface
{
public:
    virtual ~VideoDriverLoaderInterface() = default;

    virtual void Msg_LeftDown(MouseCoords mc) = 0;
    virtual void Msg_LeftUp(MouseCoords mc) = 0;
    virtual void Msg_RightDown(const MouseCoords& mc) = 0;
    virtual void Msg_RightUp(const MouseCoords& mc) = 0;
    virtual void Msg_WheelUp(const MouseCoords& mc) = 0;
    virtual void Msg_WheelDown(const MouseCoords& mc) = 0;
    virtual void Msg_MouseMove(const MouseCoords& mc) = 0;

    virtual void Msg_KeyDown(const KeyEvent& ke) = 0;

    virtual void WindowResized() = 0;
};
