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

#include "s25util/Socket.h"
#include <boost/filesystem/path.hpp>
#include <vector>

class BinaryFile;

// This is for catching crashes and reporting bugs, it does not slow down anything.
class DebugInfo
{
    Socket sock;

public:
    DebugInfo();
    ~DebugInfo();

    static std::vector<void*> GetStackTrace(void* ctx = nullptr);

    bool Send(const void* buffer, size_t length);
    bool SendSigned(int32_t i);
    bool SendUnsigned(uint32_t i);
    bool SendString(const char* str, size_t len = 0);
    bool SendString(const std::string& str);

    bool SendStackTrace(const std::vector<void*>& stacktrace);
    bool SendReplay();
    bool SendAsyncLog(const boost::filesystem::path& asyncLogFilepath);
    bool SendFile(BinaryFile& file);
};
