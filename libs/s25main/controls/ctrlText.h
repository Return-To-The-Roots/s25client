// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "ctrlBaseText.h"
#include "ogl/FontStyle.h"

class glFont;

class ctrlText : public Window, public ctrlBaseText
{
public:
    ctrlText(Window* parent, unsigned id, const DrawPoint& pos, const std::string& text, unsigned color,
             FontStyle format, const glFont* font);

    Rect GetBoundaryRect() const override;

protected:
    void Draw_() override;

    FontStyle format;
};
