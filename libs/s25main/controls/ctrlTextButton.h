// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "controls/ctrlButton.h"
#include "ctrlBaseText.h"

/// Button mit Text
class ctrlTextButton : public ctrlButton, public ctrlBaseText
{
public:
    ctrlTextButton(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                   const std::string& text, const glFont* font, const std::string& tooltip);

    /// Changes width so at most this many chars can be shown
    void ResizeForMaxChars(unsigned numChars);

protected:
    /// Draw actual content (text here)
    void DrawContent() const override;
};
