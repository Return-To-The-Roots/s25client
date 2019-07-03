// Copyright (c) 2005 - 2019 Settlers Freaks (sf-team at siedler25.org)
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
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once
#ifndef libs_driver_include_driver_VideoDriverLoaderInterface_h
#define libs_driver_include_driver_VideoDriverLoaderInterface_h

#include "KeyEvent.h"
#include "MouseCoords.h"

class VideoDriverLoaderInterface
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

#endif // !libs_driver_include_driver_VideoDriverLoaderInterface_h
