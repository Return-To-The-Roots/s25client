// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

class glFont;

/// Base class for controls containing a text
class ctrlBaseText
{
public:
    ctrlBaseText(std::string text, unsigned color, const glFont* font);

    void SetText(const std::string& text);
    const std::string& GetText() const { return text; }
    void SetFont(glFont* font);
    const glFont* GetFont() const { return font; }
    void SetTextColor(unsigned color) { color_ = color; }
    unsigned GetTextColor() const { return color_; }

protected:
    std::string text;
    unsigned color_;
    const glFont* font;
};
