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

#ifndef helperFuncs_h__
#define helperFuncs_h__

#include "BufferedWriter.h"
#include "mygettext/mygettext.h"
#include "libutil/AvoidDuplicatesWriter.h"
#include "libutil/Log.h"
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <string>
#include <utility>

namespace boost {
namespace test_tools {
#if BOOST_VERSION >= 105900
    namespace tt_detail {
#endif
        // Allow printing of pairs
        template<typename T, typename U>
        struct print_log_value<std::pair<T, U> >
        {
            void operator()(std::ostream& os, std::pair<T, U> const& v) { os << "(" << v.first << "," << v.second << ")"; }
        };
#if BOOST_VERSION >= 105900
    }
#endif
}
} // namespace boost

struct LocaleResetter
{
    const std::string oldLoc;
    LocaleResetter(const char* newLoc) : oldLoc(mysetlocale(LC_ALL, NULL)) { mysetlocale(LC_ALL, newLoc); }
    ~LocaleResetter() { mysetlocale(LC_ALL, oldLoc.c_str()); }
};

template<typename T1, typename T2>
inline boost::test_tools::predicate_result testCmp(const char* cmp, const T1& l, const T2& r, bool equal)
{
    if((l == r) != equal)
    {
        boost::test_tools::predicate_result res(false);
        res.message() << cmp << " [" << l << (equal ? "!=" : "==") << r << "]";
        return res;
    }

    return true;
}

/// Provide the last log line via getLog
struct LogAccessor
{
    boost::shared_ptr<AvoidDuplicatesWriter> logWriter;
    boost::shared_ptr<BufferedWriter> logWriterBuff;

    LogAccessor()
    {
        logWriter = boost::dynamic_pointer_cast<AvoidDuplicatesWriter>(LOG.getStdoutWriter());
        BOOST_REQUIRE(logWriter);
        logWriterBuff = boost::dynamic_pointer_cast<BufferedWriter>(logWriter->origWriter);
        BOOST_REQUIRE(logWriterBuff);
        flush();
    }

    /// Clear the last line so it won't be written and reset duplicates avoidance as we may want the same entry again
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

#define RTTR_REQUIRE_EQUAL_MSG(L, R, MSG)                                              \
    do                                                                                 \
    {                                                                                  \
        boost::test_tools::predicate_result res = testCmp(#L "==" #R, (L), (R), true); \
        if(!res)                                                                       \
            res.message() << MSG;                                                      \
        BOOST_REQUIRE(res);                                                            \
    } while(false)

#define RTTR_REQUIRE_NE_MSG(L, R, MSG)                                                  \
    do                                                                                  \
    {                                                                                   \
        boost::test_tools::predicate_result res = testCmp(#L "!=" #R, (L), (R), false); \
        if(!res)                                                                        \
            res.message() << MSG;                                                       \
        BOOST_REQUIRE(res);                                                             \
    } while(false)

/// Require that the log contains "content" in the first line. If allowEmpty is true, then an empty log is acceptable
#define RTTR_REQUIRE_LOG_CONTAINS(content, allowEmpty)                                                                           \
    do                                                                                                                           \
    {                                                                                                                            \
        const std::string log = logAcc.getLog();                                                                                 \
        BOOST_REQUIRE_MESSAGE((allowEmpty) || !log.empty(), "Log does not contain: " << (content));                              \
        BOOST_REQUIRE_MESSAGE(log.empty() || log.find(content) < log.find('\n'), "Unexpected log: " << log << "\n"               \
                                                                                                    << "Expected: " << content); \
                                                                                                                                 \
    } while(false)

#define RTTR_REQUIRE_ASSERT(stmt)                              \
    do                                                         \
    {                                                          \
        BOOST_CHECK_THROW(stmt, RTTR_AssertError);             \
        RTTR_REQUIRE_LOG_CONTAINS("Assertion failure", false); \
    } while(false)

#endif // helperFuncs_h__
