// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
