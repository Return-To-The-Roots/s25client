// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef WindowsCmdLine_h__
#define WindowsCmdLine_h__

#include <vector>

/// Provides UTF8 encoded arguments to use instead of the original argv
/// Inspired by the class of the same name in  MongoDB
/// The data is valid for the lifetime of this class
class WindowsCmdLine
{
public:
    /// Initialize the class from the given parameters (e.g. from main function)
    WindowsCmdLine(int argc, wchar_t** argv);
#ifdef _WIN32
    /// Initialize by finding the parameters using the WinAPI
    WindowsCmdLine();
#endif // _WIN32

    char** getArgv();

private:
    void init(int argc, wchar_t** argv);
    std::vector<std::vector<char> > arguments_;
    std::vector<char*> argv_;
};

#endif // WindowsCmdLine_h__
