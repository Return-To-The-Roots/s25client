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

#ifndef samplerate_hpp__
#define samplerate_hpp__

#include <functional>
#include <samplerate.h>
#include <stdexcept>
#include <utility>

namespace samplerate {
enum class Converter
{
    SincBestQuality = SRC_SINC_BEST_QUALITY,
    SincMediumQuality = SRC_SINC_MEDIUM_QUALITY,
    SincFastest = SRC_SINC_FASTEST,
    ZeroOrder = SRC_ZERO_ORDER_HOLD,
    Linear = SRC_LINEAR
};

inline const char* getName(Converter type)
{
    return src_get_name(static_cast<int>(type));
}
inline const char* getDescription(Converter type)
{
    return src_get_description(static_cast<int>(type));
}

namespace detail {
    [[noreturn]] inline void throwError(int errorCode) { throw std::runtime_error(src_strerror(errorCode)); }
    inline void throwOnError(int errorCode)
    {
        if(errorCode)
            throw std::runtime_error(src_strerror(errorCode));
    }

    template<class... Ts>
    using void_t = void;

    template<class T, class = void>
    struct has_src_clone : std::false_type
    {};

    template<class T>
    struct has_src_clone<T, void_t<decltype(src_clone(std::declval<T>(), nullptr))>> : std::true_type
    {};

    template<typename T, bool = has_src_clone<T*>::value>
    class State
    {
        T* state_;

    public:
        explicit State(SRC_STATE* state) : state_(state) {}
        ~State() { src_delete(state_); }
        State(State&& other) noexcept : state_(other.state_) { other.state_ = nullptr; }
        State(const State& other) : state_(nullptr)
        {
            if(other.state_)
            {
                int error;
                state_ = src_clone(other.state_, &error);
                if(!state_)
                    throwOnError(error);
            }
        }
        State& operator=(State other) noexcept
        {
            std::swap(state_, other.state_);
            return *this;
        }
        operator SRC_STATE*() { return state_; }
    };

    template<typename T>
    class State<T, false>
    {
        T* state_;

    public:
        explicit State(SRC_STATE* state) : state_(state) {}
        ~State() { src_delete(state_); }
        State(State&& other) noexcept : state_(other.state_) { other.state_ = nullptr; }
        State& operator=(State&& other) noexcept
        {
            state_ = other.state_;
            other.state_ = nullptr;
            return *this;
        }
        operator SRC_STATE*() { return state_; }
    };

    class StateBase
    {
    protected:
        State<SRC_STATE> state_;
        explicit StateBase(SRC_STATE* state) : state_(state) {}

    public:
        void reset() { throwOnError(src_reset(state_)); }
        void setRatio(double newRatio) { throwOnError(src_set_ratio(state_, newRatio)); }
        int getChannels()
        {
            int result = src_get_channels(state_);
            if(result < 0)
                throwError(-result);
            return result;
        }
        /// Get internal state
        SRC_STATE* getState() { return state_; }
    };
} // namespace detail

struct Data
{
    SRC_DATA data;
    Data(const float* in, size_t input_frames, float* out, size_t output_frames, double src_ratio, bool end_of_input = true) : data{}
    {
        data.data_in = in;
        data.data_out = out;
        data.input_frames = static_cast<long>(input_frames);
        data.output_frames = static_cast<long>(output_frames);
        data.src_ratio = src_ratio;
        data.end_of_input = end_of_input;
    }
    operator SRC_DATA*() { return &data; }
    long input_frames_used() const { return data.input_frames_used; }
    long output_frames_gen() const { return data.output_frames_gen; }
};

struct Result
{
    long input_frames_used, output_frames_gen;
};

inline Result simple(Data& data, Converter converter, int channels)
{
    detail::throwOnError(src_simple(&data.data, static_cast<int>(converter), channels));
    return {data.input_frames_used(), data.output_frames_gen()};
}

inline Result simple(Data&& data, Converter converter, int channels)
{
    return simple(static_cast<Data&>(data), converter, channels);
}

class State : public detail::StateBase
{
    static SRC_STATE* createOrThrow(Converter converter, int channels)
    {
        int error;
        auto* state = src_new(static_cast<int>(converter), channels, &error);
        if(!state)
            detail::throwError(error);
        return state;
    }

public:
    State(Converter converter, int channels) : StateBase(createOrThrow(converter, channels)) {}
    void process(SRC_DATA& data) { detail::throwOnError(src_process(state_, &data)); }
    Result process(Data& data)
    {
        process(data.data);
        return {data.input_frames_used(), data.output_frames_gen()};
    }
    Result process(Data&& data) { return process(static_cast<Data&>(data)); }
};

class StateCallback : public detail::StateBase
{
    static SRC_STATE* createOrThrow(Converter converter, int channels, src_callback_t func, void* data)
    {
        int error;
        auto* state = src_callback_new(func, static_cast<int>(converter), channels, &error, data);
        if(!state)
            detail::throwError(error);
        return state;
    }
    static long callback(void* cb_data, float** data) { return static_cast<long>(static_cast<StateCallback*>(cb_data)->callback_(*data)); }
    std::function<size_t(float*&)> callback_;

public:
    StateCallback(Converter converter, int channels, src_callback_t func, void* data)
        : StateBase(createOrThrow(converter, channels, func, data))
    {}
    StateCallback(Converter converter, int channels, std::function<size_t(float*&)> func)
        : StateBase(createOrThrow(converter, channels, func ? StateCallback::callback : nullptr, this)), callback_(std::move(func))
    {}
    long read(double src_ratio, size_t frames, float* data)
    {
        const long framesGenerated = src_callback_read(state_, src_ratio, static_cast<long>(frames), data);
        if(framesGenerated == 0)
            detail::throwOnError(src_error(state_));
        return framesGenerated;
    }
};
} // namespace samplerate

#endif // samplerate_hpp__
