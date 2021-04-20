// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "controls/ctrlButton.h"
#include "ctrlBaseImage.h"

/// Button mit einem Bild
class ctrlImageButton : public ctrlButton, public ctrlBaseImage
{
public:
    ctrlImageButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                    ITexture* image, const std::string& tooltip);

protected:
    void DrawContent() const override;
};
