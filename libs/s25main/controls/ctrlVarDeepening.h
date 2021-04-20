// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "controls/ctrlBaseVarText.h"
#include "controls/ctrlDeepening.h"
#include <cstdarg>
class Window;
class glFont;

class ctrlVarDeepening : public ctrlDeepening, public ctrlBaseVarText
{
public:
    /// fmtArgs contains pointers to int, unsigned or const char and must be valid for the lifetime of the var text!
    ctrlVarDeepening(Window* parent, unsigned id, const DrawPoint& pos, const Extent& size, TextureColor tc,
                     const std::string& fmtString, const glFont* font, unsigned color, unsigned count, va_list fmtArgs);

protected:
    void DrawContent() const override;
};
