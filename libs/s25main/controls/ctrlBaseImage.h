// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "DrawPoint.h"
#include "Rect.h"

class ITexture;

/// Base class for controls with an image
class ctrlBaseImage
{
public:
    ctrlBaseImage(ITexture* img = nullptr);

    void SetImage(ITexture* image) { img_ = image; }
    const ITexture* GetImage() const { return img_; }
    /// Changes the color filter used for drawing
    void SetModulationColor(unsigned modulationColor) { modulationColor_ = modulationColor; }
    unsigned GetModulationColor() const { return modulationColor_; }

    /// Swap the images of those controls
    void SwapImage(ctrlBaseImage& other);
    Rect GetImageRect() const;
    void DrawImage(const DrawPoint& pos) const;
    void DrawImage(const DrawPoint& pos, unsigned color) const;

private:
    ITexture* img_;
    unsigned modulationColor_;
};
