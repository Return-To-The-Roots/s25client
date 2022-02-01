// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
class glFont;

class ctrlPercent : public Window
{
public:
    ctrlPercent(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                unsigned text_color, const glFont* font, const unsigned short* percentage);

    unsigned short getPercentage() const { return percentage_ ? *percentage_ : 0u; }

protected:
    /// Zeichenmethode.
    void Draw_() override;

private:
    TextureColor tc;
    unsigned text_color;
    const glFont* font;
    const unsigned short* percentage_;
};
