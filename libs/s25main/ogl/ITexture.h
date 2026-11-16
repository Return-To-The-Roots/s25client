// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"
#include "Rect.h"

class ITexture
{
protected:
    virtual ~ITexture() = default;

public:
    virtual Position getOrigin() const = 0;
    virtual Extent getSize() const = 0;
    virtual void drawFull(const Position& dstPos, unsigned color = 0xFFFFFFFFu) = 0;

    /// Draws portion of image specified by srcArea on area defined by dstArea.
    /// In case of srcArea and dstArea size mismatch, scaling will occur.
    virtual void draw(Rect dstArea, Rect srcArea, unsigned color = 0xFFFFFFFFu) = 0;
};
