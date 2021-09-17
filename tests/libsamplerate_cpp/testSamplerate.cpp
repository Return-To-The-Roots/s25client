// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#define _USE_MATH_DEFINES

#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <samplerate.hpp>
#include <vector>

namespace {
int test_data = 42;
int callback_pos = 0;
std::array<float, 512> callback_data{};
} // namespace

static long callback(void* cb_data, float** data)
{
    BOOST_TEST(cb_data == &test_data);
    *data = callback_data.data() + callback_pos;
    const auto size = std::min(static_cast<int>(callback_data.size()) - callback_pos, 128);
    callback_pos += size;
    return size;
}

BOOST_AUTO_TEST_CASE(InvalidCtorParamsThrow)
{
    BOOST_CHECK_THROW(samplerate::State(samplerate::Converter::Linear, -1), std::runtime_error);
    BOOST_CHECK_THROW(samplerate::StateCallback(samplerate::Converter::Linear, -1, callback, &test_data),
                      std::runtime_error);
    BOOST_CHECK_THROW(samplerate::StateCallback(samplerate::Converter::Linear, 1, nullptr, nullptr),
                      std::runtime_error);
    BOOST_CHECK_THROW(samplerate::StateCallback(samplerate::Converter::Linear, 1, nullptr), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(CtorAndBaseFuncsWork)
{
    samplerate::State s1(samplerate::Converter::Linear, 10);
    BOOST_TEST_REQUIRE(s1.getState());
    BOOST_TEST(s1.getChannels() == 10);
    s1.reset();                   // No exceptions thrown
    s1.setRatio(44100. / 22050.); // No exceptions thrown
}

template<typename T, typename std::enable_if<std::is_copy_constructible<T>::value>::type* = nullptr>
void cloneTests(T& s1)
{
    samplerate::State s2 = s1;
    BOOST_TEST(s2.getState());
    BOOST_TEST(s1.getState() != s2.getState());
    samplerate::State s3{s1};
    BOOST_TEST(s3.getState());
    BOOST_TEST(s1.getState() != s3.getState());
}

template<typename T, typename std::enable_if<!std::is_copy_constructible<T>::value>::type* = nullptr>
void cloneTests(const T&)
{}

BOOST_AUTO_TEST_CASE(CopyClones)
{
    samplerate::State s1(samplerate::Converter::Linear, 1);
    BOOST_TEST(s1.getState());
    cloneTests(s1);
}

BOOST_AUTO_TEST_CASE(MoveDoesNotCopy)
{
    samplerate::State s1(samplerate::Converter::Linear, 1);
    BOOST_TEST(s1.getState());
    const auto* s1State = s1.getState();
    samplerate::State s2 = std::move(s1);
    BOOST_TEST(s2.getState());
    BOOST_TEST(s1State == s2.getState());
    samplerate::State s3{std::move(s2)};
    BOOST_TEST(s3.getState());
    BOOST_TEST(s1State == s3.getState());
}

BOOST_AUTO_TEST_CASE(SimpleCallback)
{
    samplerate::StateCallback state(samplerate::Converter::Linear, 1, callback, &test_data);
    auto const ratio = 2;
    std::vector<float> out(callback_data.size() * ratio);
    callback_pos = 0;
    const auto written = state.read(ratio, out.size(), out.data());
    BOOST_TEST(written == out.size());
}

BOOST_AUTO_TEST_CASE(ConversionWorks)
{
    std::vector<float> input(2048);
    std::vector<float> output(input.size() * 2), output2(output.size());
    {
        int i = 0;
        std::generate(input.begin(), input.end(),
                      [&i]() mutable { return static_cast<float>(std::sin(i++ * 0.01 * 2 * M_PI)); });
    }
    SRC_DATA data{};
    data.data_in = input.data();
    data.data_out = output.data();
    data.input_frames = static_cast<long>(input.size());
    data.output_frames = static_cast<long>(output.size());
    data.end_of_input = true;
    data.src_ratio = 2;
    BOOST_TEST_REQUIRE(src_simple(&data, SRC_LINEAR, 1) == 0);

    const auto resultSimple =
      samplerate::simple(samplerate::Data(input.data(), input.size(), output2.data(), output2.size(), 2),
                         samplerate::Converter::Linear, 1);
    BOOST_TEST(output == output2, boost::test_tools::per_element());
    BOOST_TEST(resultSimple.input_frames_used == data.input_frames_used);
    BOOST_TEST(resultSimple.output_frames_gen == data.output_frames_gen);

    std::fill(output2.begin(), output2.end(), 0.f);
    samplerate::State state(samplerate::Converter::Linear, 1);
    const auto resultState =
      state.process(samplerate::Data(input.data(), input.size(), output2.data(), output2.size(), 2));
    BOOST_TEST(output == output2, boost::test_tools::per_element());
    BOOST_TEST(resultState.input_frames_used == data.input_frames_used);
    BOOST_TEST(resultState.output_frames_gen == data.output_frames_gen);

    std::fill(output2.begin(), output2.end(), 0.f);
    size_t curInputPos = 0;
    samplerate::StateCallback cbState(samplerate::Converter::Linear, 1, [&](float*& data) noexcept {
        data = &input[curInputPos];
        const auto size = std::min<size_t>(200u, input.size() - curInputPos);
        curInputPos += size;
        return size;
    });
    unsigned long curPos = 0;
    while(true)
    {
        const auto written = cbState.read(2, std::min<size_t>(500, output2.size() - curPos),
                                          curPos == output2.size() ? nullptr : &output2[curPos]);
        if(written == 0)
            break;
        curPos += written;
    }
    BOOST_TEST(output == output2, boost::test_tools::per_element());
    BOOST_TEST(data.input_frames_used >= 0);
    BOOST_TEST(curInputPos == static_cast<size_t>(data.input_frames_used));
    BOOST_TEST(data.output_frames_gen >= 0);
    BOOST_TEST(curPos == static_cast<unsigned long>(data.output_frames_gen));
}
