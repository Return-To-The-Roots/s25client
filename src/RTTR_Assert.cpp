// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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
#include "RTTR_Assert.h"
#include "RTTR_AssertError.h"
#include "libutil/src/System.h"
#include <iostream>
#include <sstream>
#ifdef _WIN32
#   include <windows.h>
#endif

bool RTTR_AssertEnableBreak = true;

bool RTTR_IsBreakOnAssertFailureEnabled()
{
    bool assertBreakDisabled = false;
    try{
        std::string envVarVal = System::getEnvVar("RTTR_DISABLE_ASSERT_BREAKPOINT");
        if(envVarVal == "1" || envVarVal == "yes")
            assertBreakDisabled = true;
    }catch(...){}
    return !assertBreakDisabled && RTTR_AssertEnableBreak;
}

void RTTR_AssertFailure(const char* condition, const char* file, const int line, const char* function)
{
    static const std::string thisFilePath = __FILE__;
    std::string filePath = file;
    std::string::size_type pos = thisFilePath.find_last_of("/\\");
    if(pos != std::string::npos && filePath.size() > pos && filePath.substr(0, pos) == thisFilePath.substr(0, pos))
        filePath = filePath.substr(pos + 1);

    std::stringstream sMsg;
    sMsg << "Assertion failure";
    if(function)
        sMsg << " in \"" << function << "\"";
    sMsg << " at " << filePath << "#" << line << ": " << condition;
    std::string msg = sMsg.str();
    std::cerr << msg << std::endl;
#ifdef _WIN32
    OutputDebugStringA(msg.c_str());
#endif
    throw RTTR_AssertError(msg);
}
