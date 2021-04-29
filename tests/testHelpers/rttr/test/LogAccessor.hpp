// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "BufferedWriter.hpp"
#include "s25util/AvoidDuplicatesWriter.h"
#include "s25util/Log.h"
#include <boost/test/unit_test.hpp>

namespace rttr { namespace test {
    /// Provide the last log line via getLog
    struct LogAccessor
    {
        std::shared_ptr<AvoidDuplicatesWriter> logWriter;
        std::shared_ptr<BufferedWriter> logWriterBuff;

        LogAccessor()
        {
            logWriter = std::dynamic_pointer_cast<AvoidDuplicatesWriter>(LOG.getStdoutWriter());
            BOOST_TEST_REQUIRE(logWriter);
            logWriterBuff = std::dynamic_pointer_cast<BufferedWriter>(logWriter->origWriter);
            BOOST_TEST_REQUIRE(logWriterBuff);
            flush();
        }

        /// Clear the last line so it won't be written and reset duplicates avoidance as we may want the same entry
        /// again
        void clearLog()
        {
            logWriter->reset();
            logWriterBuff->curText.clear();
        }
        void flush()
        {
            logWriter->reset();
            logWriterBuff->flush();
            logWriter->reset();
        }
        std::string getLog(bool clear = true)
        {
            std::string result = logWriterBuff->curText;
            if(clear)
                clearLog();
            return result;
        }
    };
}} // namespace rttr::test

/// Require that the log contains "content" in the first line. If allowEmpty is true, then an empty log is acceptable
#define RTTR_REQUIRE_LOG_CONTAINS(content, allowEmpty)                                              \
    do                                                                                              \
    {                                                                                               \
        const std::string log = logAcc.getLog();                                                    \
        BOOST_REQUIRE_MESSAGE((allowEmpty) || !log.empty(), "Log does not contain: " << (content)); \
        BOOST_REQUIRE_MESSAGE(log.empty() || log.find(content) < log.find('\n'),                    \
                              "Unexpected log: " << log << "\n"                                     \
                                                 << "Expected: " << (content));                     \
                                                                                                    \
    } while(false)

#define RTTR_REQUIRE_ASSERT(stmt)                              \
    do                                                         \
    {                                                          \
        const bool old = RTTR_SetBreakOnAssertFailure(false);  \
        BOOST_CHECK_THROW(stmt, RTTR_AssertError);             \
        RTTR_SetBreakOnAssertFailure(old);                     \
        RTTR_REQUIRE_LOG_CONTAINS("Assertion failure", false); \
    } while(false)
