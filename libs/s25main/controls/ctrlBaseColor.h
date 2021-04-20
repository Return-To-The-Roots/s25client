// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/// Base class for controls showing a color
class ctrlBaseColor
{
public:
    ctrlBaseColor() : color_(0) {}
    ctrlBaseColor(unsigned color) : color_(color) {}
    void SetColor(unsigned color) { color_ = color; }
    unsigned GetColor() const { return color_; }

protected:
    unsigned color_;
};
