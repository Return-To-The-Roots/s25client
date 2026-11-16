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

    static stacktrace_t getStackTrace(void* ctx = nullptr) noexcept(false);

    bool send(const void* buffer, size_t length);
    bool sendSigned(int32_t i);
    bool sendUnsigned(uint32_t i);
    bool sendString(const char* str, size_t len = 0);
    bool sendString(const std::string& str);

    bool sendStackTrace(const stacktrace_t& stacktrace);
    bool sendReplay();
    bool sendAsyncLog(const boost::filesystem::path& asyncLogFilepath);
    bool sendFile(BinaryFile& file);
};
