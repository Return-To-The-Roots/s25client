// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "s25util/Socket.h"
#include <boost/container/static_vector.hpp>
#include <boost/filesystem/path.hpp>

class BinaryFile;

// This is for catching crashes and reporting bugs, it does not slow down anything.
class DebugInfo
{
    Socket sock;

public:
    using stacktrace_t = boost::container::static_vector<void*, 256>;

    DebugInfo();
    ~DebugInfo();

    static stacktrace_t GetStackTrace(void* ctx = nullptr) noexcept(false);

    bool Send(const void* buffer, size_t length);
    bool SendSigned(int32_t i);
    bool SendUnsigned(uint32_t i);
    bool SendString(const char* str, size_t len = 0);
    bool SendString(const std::string& str);

    bool SendStackTrace(const stacktrace_t& stacktrace);
    bool SendReplay();
    bool SendAsyncLog(const boost::filesystem::path& asyncLogFilepath);
    bool SendFile(BinaryFile& file);
};
