// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "controls/ctrlBaseText.h"
#include <cstdarg>
#include <string>
#include <vector>

class glFont;

/// Base class for controls containing a text
class ctrlBaseVarText : public ctrlBaseText
{
public:
    /// fmtArgs contains pointers to int, unsigned or const char and must be valid for the lifetime of the var text!
    ctrlBaseVarText(const std::string& fmtString, unsigned color, const glFont* font, unsigned count, va_list fmtArgs);

protected:
    /// Returns the text with placeholders replaced by the actual vars
    std::string GetFormatedText() const;

private:
    std::vector<void*> vars;
};
