// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Window.h"
#include "controls/ctrlBaseVarText.h"
#include "ogl/FontStyle.h"
#include <cstdarg>
#include <string>

class glFont;

class ctrlVarText : public Window, public ctrlBaseVarText
{
public:
    /// fmtArgs contains pointers to int, unsigned or const char and must be valid for the lifetime of the var text!
    ctrlVarText(Window* parent, unsigned id, const DrawPoint& pos, const std::string& formatstr, unsigned color,
                FontStyle format, const glFont* font, unsigned count, va_list fmtArgs);
    ~ctrlVarText() override;

    Rect GetBoundaryRect() const override;

protected:
    void Draw_() override;

    FontStyle format_;
};
