// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "controls/ctrlBaseText.h"
#include "controls/ctrlDeepening.h"

class glFont;

/// Deepening with text
class ctrlTextDeepening : public ctrlDeepening, public ctrlBaseText
{
public:
    ctrlTextDeepening(Window* parent, unsigned id, DrawPoint pos, const Extent& size, TextureColor tc,
                      const std::string& text, const glFont* font, unsigned color,
                      FontStyle style = FontStyle::CENTER | FontStyle::VCENTER);

    Rect GetBoundaryRect() const override;
    /// Changes width so at most this many chars can be shown
    void ResizeForMaxChars(unsigned numChars);

protected:
    void DrawContent() const override;

private:
    DrawPoint CalcTextPos() const;
    FontStyle style_;
};
