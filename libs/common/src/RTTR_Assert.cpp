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

#include "RTTR_Assert.h"
#include "RTTR_AssertError.h"
#include "s25util/Log.h"
#include "s25util/System.h"
#include <iostream>
#include <sstream>
#ifdef _WIN32
#    include <windows.h>
#endif

static bool breakOnAssertFailureEnabled = true;

bool RTTR_SetBreakOnAssertFailure(bool enabled)
{
    const bool old = breakOnAssertFailureEnabled;
    breakOnAssertFailureEnabled = enabled;
    return old;
}

bool RTTR_IsBreakOnAssertFailureEnabled()
{
    if(!breakOnAssertFailureEnabled)
        return false;
    try
    {
        std::string envVarVal = System::getEnvVar("RTTR_DISABLE_ASSERT_BREAKPOINT");
        if(envVarVal == "1" || envVarVal == "yes")
            return false;
    } catch(...)
    { //-V565
    }
    return true;
}

void RTTR_AssertFailure(const char* condition, const char* file, const int line, const char* function)
{
    static const std::string thisFilePath = __FILE__;
    static bool inAssertFailure = false;

    // Guard as e.g. LOG.write may throw an assertion
    if(!inAssertFailure)
    {
        inAssertFailure = true;
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
        try
        {
            LOG.write("%1%\n", LogTarget::Stderr) % msg;
        } catch(...)
        {
            std::cerr << msg << std::endl;
        }
#ifdef _WIN32
        OutputDebugStringA(msg.c_str());
#endif
        inAssertFailure = false;
        throw RTTR_AssertError(msg);
    } else
        throw RTTR_AssertError(std::string(condition) + " failed while handling an assertion");
}
