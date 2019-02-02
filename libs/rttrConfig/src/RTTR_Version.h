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

#ifndef RTTR_Version_h__
#define RTTR_Version_h__

#include <string>

class RTTR_Version
{
public:
    static std::string GetTitle();
    static std::string GetVersionDate();
    static std::string GetRevision();
    static std::string GetShortRevision();
    static std::string GetYear();
    static std::string GetReadableVersion();
};

#endif // RTTR_Version_h__
