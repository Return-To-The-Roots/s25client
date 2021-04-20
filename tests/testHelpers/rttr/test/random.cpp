// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "random.hpp"
#include "s25util/warningSuppression.h"
#include <boost/lexical_cast.hpp>
#include <boost/test/framework.hpp>
#include <boost/test/tree/observer.hpp>
#include <boost/test/tree/test_unit.hpp>
#include <boost/test/unit_test_log.hpp>
#include <algorithm>
#include <chrono>
#include <iostream>

namespace rttr { namespace test {
    class randomObserver final : public boost::unit_test::test_observer
    {
        static auto getRandSeed() { return std::chrono::high_resolution_clock::now().time_since_epoch().count(); }
        void test_unit_start(boost::unit_test::test_unit const& test) override
        {
            if(test.p_type != boost::unit_test::TUT_CASE)
                return;
            lastSeed = getRandSeed();
            for(const auto& label : static_cast<const std::vector<std::string>&>(test.p_labels))
            {
                if(label.substr(0, 5) == "seed=")
                {
                    lastSeed = boost::lexical_cast<decltype(lastSeed)>(label.substr(5));
                    break;
                }
            }
            randState.seed(static_cast<decltype(randState)::result_type>(lastSeed));
            failedLastTest = false;
            testUsedRandom = false;
        }
        void test_unit_finish(boost::unit_test::test_unit const& test, unsigned long) override
        {
            if(testUsedRandom && test.p_type == boost::unit_test::TUT_CASE && failedLastTest)
            {
                std::cerr << "Random seed was " << lastSeed << std::endl;
                std::cerr << "Decorate the test with *boost::unit_test::label(\"seed=" << lastSeed
                          << "\") to reproduce this\n";
            }
        }
        RTTR_IGNORE_OVERLOADED_VIRTUAL
        void assertion_result(boost::unit_test::assertion_result ar) override
        {
            if(ar != boost::unit_test::AR_PASSED)
                failedLastTest = true;
        }
        RTTR_POP_DIAGNOSTIC

        void test_unit_aborted(boost::unit_test::test_unit const& test) override
        {
            if(test.p_type == boost::unit_test::TUT_CASE)
                failedLastTest = true;
        }

        std::mt19937 randState;
        uint64_t lastSeed = 0;
        bool testUsedRandom = false;
        bool failedLastTest = false;

    public:
        randomObserver()
        {
            boost::unit_test::framework::register_observer(*this);
            if(boost::unit_test::framework::test_in_progress())
                test_unit_start(boost::unit_test::framework::current_test_case());
        }
        ~randomObserver() { boost::unit_test::framework::deregister_observer(*this); }
        std::mt19937& getState()
        {
            testUsedRandom = true;
            return randState;
        }
    };

    std::mt19937& getRandState()
    {
        static randomObserver observer;
        return observer.getState();
    }

    struct RandCharCreator
    {
        const std::string charset;
        std::mt19937& rng;
        std::uniform_int_distribution<std::string::size_type> distr;
        RandCharCreator(const std::string& charset, std::mt19937& rng)
            : charset(charset), rng(rng), distr(0, charset.length())
        {}
        char operator()() { return charset[distr(rng)]; }
    };

    std::string randString(int len)
    {
        static const std::string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        if(len < 0)
            len = randomValue<int>(1, 30);
        std::string result;
        result.resize(len);
        std::generate_n(result.begin(), len, RandCharCreator(charset, getRandState()));
        return result;
    }

}} // namespace rttr::test
