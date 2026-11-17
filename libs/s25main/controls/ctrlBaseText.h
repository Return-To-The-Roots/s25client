// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

    void setText(const std::string& text);
    const std::string& getText() const { return text; }
    void setFont(glFont* font);
    const glFont* getFont() const { return font; }
    void setTextColor(unsigned color) { color_ = color; }
    unsigned getTextColor() const { return color_; }

protected:
    std::string text;
    unsigned color_;
    const glFont* font;
};
