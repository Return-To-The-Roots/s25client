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

#include "defines.h" // IWYU pragma: keep
#include "WindowsCmdLine.h"
#include "libutil/src/ucString.h"
#include <string>

#ifdef _WIN32
#   include <windows.h>
#   include <shellapi.h>
#   include <stdexcept>

WindowsCmdLine::WindowsCmdLine(){
    int nArgs;
    wchar_t** argList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if(!argList || nArgs < 1)
        throw std::runtime_error("Could not get command line");
    init(nArgs, argList);
    LocalFree(argList);
}
#endif // _WIN32

WindowsCmdLine::WindowsCmdLine(int argc, wchar_t** argv){
    init(argc, argv);
}

void WindowsCmdLine::init(int argc, wchar_t** argv)
{
    // Required so pointers to data won't change
    arguments_.resize(argc);
    for(int i = 0; i < argc; i++){
        std::string argument = cvWideStringToUTF8(argv[i]);
        arguments_[i].assign(argument.begin(), argument.end());
        arguments_[i].push_back('\0');
        argv_.push_back(&arguments_[i][0]);
    }
}

char** WindowsCmdLine::getArgv(){
    return argv_.empty() ? NULL : &argv_[0];
}
