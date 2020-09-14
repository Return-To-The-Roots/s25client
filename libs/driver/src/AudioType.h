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

#include <cassert>

struct AudioType
{
    enum Type
    {
        AD_UNKNOWN = 0,
        AD_WAVE,
        AD_MIDI,
        AD_OTHER
        // AD_MP3, usw
    };
    static const int COUNT = AD_OTHER + 1;

    Type t_;
    AudioType(Type t) : t_(t) { assert(static_cast<int>(t_) >= AD_UNKNOWN && static_cast<int>(t_) < COUNT); }
    /// Converts an UInt safely to a Direction
    explicit AudioType(unsigned t) : t_(Type(t % COUNT)) {}
    /// Converts an UInt to a Direction without checking its value. Use only when this is actually a Direction
    static AudioType fromInt(unsigned t) { return Type(t); }
    static AudioType fromInt(int t) { return Type(t); }
    operator Type() const { return t_; }
    /// Returns the Direction as an UInt (for legacy code)
    unsigned toUInt() { return t_; }

private:
    // prevent automatic conversion for any other built-in types such as bool, int, etc
    template<typename T>
    operator T() const;
};
