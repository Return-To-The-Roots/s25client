// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "DrawPoint.h"
#include "Rect.h"

class ITexture;

/// Base class for controls with an image
class ctrlBaseImage
{
public:
    ctrlBaseImage(ITexture* img = nullptr);

    void setImage(ITexture* image) { img_ = image; }
    const ITexture* getImage() const { return img_; }
    /// Changes the color filter used for drawing
    void setModulationColor(unsigned modulationColor) { modulationColor_ = modulationColor; }
    unsigned getModulationColor() const { return modulationColor_; }

    /// Swap the images of those controls
    void SwapImage(ctrlBaseImage& other);
    Rect getImageRect() const;

    /// Draw the image on specified rectangular area. The image is centered inside dstArea and cropped to its size.
    void DrawImage(const Rect& dstArea) const;
    void DrawImage(const Rect& dstArea, unsigned color) const;

private:
    ITexture* img_;
    unsigned modulationColor_;
};
