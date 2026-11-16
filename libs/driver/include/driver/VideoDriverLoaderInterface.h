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

    virtual void msgLeftDown(MouseCoords mc) = 0;
    virtual void msgLeftUp(MouseCoords mc) = 0;
    virtual void msgRightDown(const MouseCoords& mc) = 0;
    virtual void msgRightUp(const MouseCoords& mc) = 0;
    virtual void msgMiddleDown(const MouseCoords& mc) = 0;
    virtual void msgMiddleUp(const MouseCoords& mc) = 0;
    virtual void msgWheelUp(const MouseCoords& mc) = 0;
    virtual void msgWheelDown(const MouseCoords& mc) = 0;
    virtual void msgMouseMove(const MouseCoords& mc) = 0;

    virtual void msgKeyDown(const KeyEvent& ke) = 0;

    virtual void WindowResized() = 0;
};
